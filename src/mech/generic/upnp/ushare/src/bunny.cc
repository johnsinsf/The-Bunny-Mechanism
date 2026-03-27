/*
 * bunny.c : bunny-uShare Web Server handler.
 * Copyright (C) 2025 John Sullivan <john@dwanta.com>
 *
 * original header:
 *
 * Originally developped for the GeeXboX project.
 * Parts of the code are originated from GMediaServer from Oskar Liljeblad.
 * Copyright (C) 2005-2007 Benjamin Zores <ben@geexbox.org>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "services.h"
#include "cds.h"
#include "cms.h"
#include "msr.h"
#include "metadata.h"
#include "bunny_meta.h"
#include "bunny.h"
#include "minmax.h"
#include "trace.h"
#include "presentation.h"
#include "osdep.h"
#include "mime.h"
#include "SocketIO.h"

#define PROTOCOL_TYPE_PRE_SZ  11   /* for the str length of "http-get:*:" */
#define PROTOCOL_TYPE_SUFF_SZ 2    /* for the str length of ":*" */
#define MAX_HEADER 32000
#define MAX_HEADERS 32
#define MAX_CONTENT 100000000

enum {
  FILE_LOCAL,
  FILE_MEMORY,
  FILE_DSS
} filetype;


struct web_file_t {
  char *fullpath;
  off_t pos;
  enum {
    FILE_LOCAL,
    FILE_MEMORY,
    FILE_DSS
  } type;
  union {
    struct {
      int fd;
      struct upnp_entry_t *entry;
      SocketIO* bunny_sock;
      DssObject* dssObj;
    } local;
    struct {
      char *contents;
      off_t len;
    } memory;
  } detail;
};

static inline void
set_info_file (UpnpFileInfo *info, const size_t length,
               const char *content_type) {
  UpnpFileInfo_set_FileLength(info, length);
  UpnpFileInfo_set_LastModified(info, 0);
  UpnpFileInfo_set_IsDirectory(info, 0);
  UpnpFileInfo_set_IsReadable(info, 1);
  UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString (content_type));
}

/*
  Flying Bunny Cache

  After testing out the code I decided to add the cache.
  You'll know if you need it if the music stops for no reason.
  It turns out the turnaround time can be excessive with high bit rate music. (96k)
  My test was receiver -> bunnymech -> internet -> bunnyfarm -> internet -> bunnymech ->internet -> bunnyfarm -> internet -> bunnymech ->receiver
  With flac files the receiver couldn't get enough data to play the files, but cds were ok. I think the cut off is about 500ms.
  It's best to have the cache off and just use the native cache in the receiver, but my Yamaha has a 1MB buffer
  that can't be changed, so it's impossible for it to get data fast enough in all conditions, so the cache can be needed.
  The problem of using the cache is if you change the stream you're playing a lot that flushs the cache, so it can slow things down. 
*/

bool bunny_cache_enabled = false;
unsigned int bunny_cache_size = 40000000;
unsigned int bunny_cache_fd = 0;
unsigned int bunny_cache_sem = 0;
unsigned int bunny_cache_seminit = 0;
unsigned int bunny_cache_semdata = 0;

unsigned long bunny_cache_starting_filepos = 0;
unsigned long bunny_cache_ending_filepos = 0;
unsigned long bunny_cache_filepos = 0;
unsigned long bunny_cache_filesize = 0;

unsigned char* bunny_cache = NULL;
unsigned char* bunny_cache_bufptr = NULL;
unsigned char* bunny_cache_ending_bufptr = NULL;
unsigned char* bunny_cache_starting_filepos_bufptr = 0;
unsigned char* bunny_cache_ending_filepos_bufptr = 0;

SocketIO *bunny_cache_sock = NULL;

string bunny_cache_servername;
string bunny_cache_request;
pthread_mutex_t bunny_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

void*
bunny_cache_thread( void* a ) {

  if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
    g_quit = true;
  }
  if( bunny_cache_seminit < 0 ) {
    log_verbose("error getting init semaphore %d\n", errno);
    g_quit = true;
  }
  if( bunny_cache != NULL )
    free(bunny_cache);
  if( bunny_cache_size < 5000000 )
    bunny_cache_size = 5000000;

  bunny_cache = (unsigned char*)malloc(bunny_cache_size);

  if( ! bunny_cache ) {
    log_verbose("failed to allocate cache\n");
    g_quit = true;
  }
  
  bunny_cache_bufptr = bunny_cache;
  bunny_cache_ending_bufptr = bunny_cache + bunny_cache_size;
  bunny_cache_ending_filepos_bufptr = bunny_cache;
  bunny_cache_starting_filepos_bufptr = bunny_cache;

  if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
    g_quit = true;
  }

  struct sembuf semOp;
  semOp.sem_num = 0;
  semOp.sem_op  = 1;
  semOp.sem_flg = 0;

  if( semop( bunny_cache_seminit, &semOp, 1 ) < 0 ) {
    log_verbose("error signaling seminit %d\n", errno);
    g_quit = true;
  }

  if( bunny_cache_sem < 0 ) {
    log_verbose("error getting cache semaphore %d\n", errno);
    g_quit = true;
  }

  while( ! g_quit ) {
    struct sembuf semOp;
    semOp.sem_num = 0;
    semOp.sem_op  = -1;
    semOp.sem_flg = 0;
 
    // wait for cache signal
    log_verbose("cache thread waiting\n");

    int rc = semop( bunny_cache_sem, &semOp, 1 );

    //log_verbose("cache thread rc %d\n", rc);

    if( rc == -1 )
      g_quit = true;

    if( rc == 0 ) {
      //log_verbose("locking mutex\n");
      if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
        g_quit = true;
      }
      //log_verbose("locked mutex\n");
      unsigned long t_size = bunny_cache_ending_filepos - bunny_cache_starting_filepos;
      unsigned long t_pos = bunny_cache_ending_filepos;
      unsigned long t_filesize = bunny_cache_filesize;

      string t_request = bunny_cache_request;
      string t_servername = bunny_cache_servername;

      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "ERROR: failed to unlock bunny_cache %d\n", errno );
        g_quit = true;
      }

      log_verbose("cache thread pos %d, size %d, starting %d, ending %d, filesize %d\n", 
       t_pos, t_size, bunny_cache_starting_filepos, bunny_cache_ending_filepos, bunny_cache_filesize);

      if( t_pos >= t_filesize ) {
        log_verbose("bad pos %d %d\n", t_pos, t_filesize);
        //g_quit = true;
      }
      else
      if( t_size <= 15000000 && t_pos < t_filesize && ! g_quit ) {
        //log_verbose("bunny_cache_thread getting some data %d\n", bunny_cache_size - t_size );

        bool done = false;
        bool finished = false;
        LObj obj;
        while( t_size <= 15000000 && ! finished && ! done && ! g_quit && t_pos < t_filesize ) {

          if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
            log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
            g_quit = true;
          }
          if( t_pos >= t_filesize ) {
            finished = true;
          }
          string test;
          int buflen = 5000000;
          unsigned long remaining = bunny_cache_filesize - bunny_cache_ending_filepos;
          if( remaining < buflen ) {
            buflen = remaining;
            finished = true;
          }
          if( t_servername != bunny_cache_servername || t_request != bunny_cache_request ) {
            done = true;
            log_verbose("changed cache request, exiting\n");
          }
          if( ! done ) {
            test = "GET " + bunny_cache_request + " HTTP/1.0\n";
            test += "host: " + bunny_cache_servername + "\n";
            test += "range: bytes=" + to_string(bunny_cache_ending_filepos) + "-" + to_string(buflen) + "\n\n";

            if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
              log_verbose( "ERROR: failed to unlock bunny_cache %d\n", errno );
              g_quit = true;
            }

            SocketIO bunny_sock;
            bunny_sock.useSSL = true;
            string fullserver = bunny_cache_servername;
            string bunny_server, server_dir;
            int x = fullserver.find_first_of("/");
            if( x > 0 ) {
              bunny_server = fullserver.substr(0, x - 1);
              server_dir = fullserver.substr(x + 1, fullserver.size() - x - 1 );
            } else {
              bunny_server = fullserver;
              server_dir = "/";
            }
            int bunny_server_port = 443;
   
            //log_verbose("opening server %s\n", bunny_server);

            int fd = bunny_sock.openClient( bunny_server, bunny_server_port );
   
            log_verbose("sending %s %d\n", test.c_str(), fd );

            if( fd < 0 ) {
              log_verbose("bad socket\n");
              g_quit = true;
            } else {
              bunny_sock.write(test.c_str(), test.size());

              int rc2 = readHeader( &bunny_sock, obj );
              if( rc2 != 1 ) {
                done = true;
                log_verbose("bad readHeader rc %d\n", rc2);
              }
              bunny_sock.doClose();
            }
            log_verbose("cache locking mutex\n");
            if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
              log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
              g_quit = true;
              done = true;
            }
          }
          if( ! done && ! g_quit ) {
            if( bunny_cache_bufptr + obj.packet.size() <= bunny_cache_ending_bufptr ) {
              memcpy( bunny_cache_bufptr, obj.packet.c_str(), obj.packet.size() );
              bunny_cache_bufptr += obj.packet.size();
            } else {
              unsigned long len1 = bunny_cache_ending_bufptr - bunny_cache_bufptr;
              unsigned long len2 = obj.packet.size() - len1;
              if( len1 > 0 )
                memcpy( bunny_cache_bufptr, obj.packet.substr(0,len1).c_str(), len1 );
              memcpy( bunny_cache, obj.packet.substr(len1, len2).c_str(), len2 );
              bunny_cache_bufptr = bunny_cache + len2;
            }

            bunny_cache_ending_filepos = bunny_cache_ending_filepos + obj.packet.size();
            if( bunny_cache_ending_filepos > bunny_cache_filesize )
              bunny_cache_ending_filepos = bunny_cache_filesize;
            
            bunny_cache_ending_filepos_bufptr = bunny_cache_bufptr;

            //log_verbose("have starting/ending filepos %d/%d\n", bunny_cache_starting_filepos, bunny_cache_ending_filepos);

            t_size = bunny_cache_ending_filepos - bunny_cache_starting_filepos;
          }
          if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
            log_verbose( "ERROR: failed to unlock bunny_cache %d\n", errno );
            g_quit = true;
            done = true;
          }

          struct sembuf semOp;
          semOp.sem_num = 0;
          semOp.sem_op  = 1;
          semOp.sem_flg = 0;

          if( semop( bunny_cache_semdata, &semOp, 1 ) < 0 ) {
            log_verbose("error signaling semdata %d\n", errno);
            g_quit = true;
          }
          //log_verbose("signaled data ready %d\n", t_size);
        }
      } else {
        log_verbose( "buffer full\n" );
      } 
    } 
  }
  int retVal;
  pthread_exit( (void*)&retVal );
}

static char *
getExtension (const char *filename) {
  char *str = NULL;

  str = strrchr ((char*)filename, '.');
  if (str)
    str++;

  return str;
}

int 
bunny_get_info (const char *filename, UpnpFileInfo *info,
                   const void* cookie __attribute__((unused)),
                   const void** requestCookie __attribute__((unused))) {
  extern struct ushare_t *ut;
  struct upnp_entry_t *entry = NULL;
  //struct stat st;
  int upnp_id = 0;
  char *content_type = NULL;
  char *protocol = NULL;
  
  if (!filename || !info)
    return -1;

  pid_t tid;
  tid = syscall(SYS_gettid);

  log_verbose ("bunny_get_info %d, filename : %s\n", tid, filename);

  if (!strcmp (filename, CDS_LOCATION)) {
    set_info_file (info, CDS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return 0;
  }

  if (!strcmp (filename, CMS_LOCATION)) {
    set_info_file (info, CMS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return 0;
  }

  if (!strcmp (filename, MSR_LOCATION)) {
    set_info_file (info, MSR_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return 0;
  }

  if (ut->use_presentation && !strcmp (filename, USHARE_PRESENTATION_PAGE)) {
    if (build_presentation_page (ut) < 0)
      return -1;

    set_info_file (info, ut->presentation->len, PRESENTATION_PAGE_CONTENT_TYPE);
    return 0;
  }

  if (ut->use_presentation && !strncmp (filename, USHARE_CGI, strlen (USHARE_CGI))) {
    if (process_cgi (ut, (char *) (filename + strlen (USHARE_CGI) + 1)) < 0)
      return -1;

    set_info_file (info, ut->presentation->len, PRESENTATION_PAGE_CONTENT_TYPE);
    return 0;
  }

  upnp_id = atoi (strrchr (filename, '/') + 1);
  entry = bunny_upnp_get_entry (ut, upnp_id);
  if (!entry)
    return -1;

  if (!entry->fullpath)
    return -1;

  log_verbose("fullpath: %s\n", entry->fullpath);

  UpnpFileInfo_set_IsReadable(info, 1);

  UpnpFileInfo_set_FileLength(info, entry->size);

  log_verbose("size set to %lld\n", entry->size);

  /* file exist and can be read */
  //UpnpFileInfo_set_FileLength(info, st.st_size);
  //UpnpFileInfo_set_LastModified(info, st.st_mtime);
  //UpnpFileInfo_set_IsDirectory(info, S_ISDIR (st.st_mode));

  protocol = 
#ifdef HAVE_DLNA
    entry->dlna_profile ?
    dlna_write_protocol_info (DLNA_PROTOCOL_INFO_TYPE_HTTP,
                              DLNA_ORG_PLAY_SPEED_NORMAL,
                              DLNA_ORG_CONVERSION_NONE,
                              DLNA_ORG_OPERATION_RANGE,
                              ut->dlna_flags, entry->dlna_profile) :
#endif /* HAVE_DLNA */
    mime_get_protocol (entry->mime_type);

  content_type =
    strndup ((protocol + PROTOCOL_TYPE_PRE_SZ),
             strlen (protocol + PROTOCOL_TYPE_PRE_SZ)
             - PROTOCOL_TYPE_SUFF_SZ);
  free (protocol);

  if (content_type) {
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString (content_type));
    free (content_type);
  }
  else
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString (""));

  return 0;
}

static UpnpWebFileHandle
get_file_memory (const char *fullpath, const char *description,
                 const size_t length) {
  struct web_file_t *file;

  file = (web_file_t*)malloc (sizeof (struct web_file_t));
  file->fullpath = strdup (fullpath);
  file->pos = 0;
  file->type = web_file_t::FILE_MEMORY;
  file->detail.memory.contents = strdup (description);
  file->detail.memory.len = length;

  return ((UpnpWebFileHandle) file);
}

UpnpWebFileHandle 
bunny_open (const char *filename, enum UpnpOpenFileMode mode,
                             const void* cookie __attribute__((unused)),
                             const void* requestCookie __attribute__((unused))) {
  extern struct ushare_t *ut;
  struct upnp_entry_t *entry = NULL;
  struct web_file_t *file;
  int fd, upnp_id = 0;
  static string lastfile; // to remove duplicate entries to the playlog

  bunny_cache_starting_filepos = 0;
  bunny_cache_ending_filepos = 0;

  if (!filename)
    return NULL;

  log_verbose ("bunny_open, filename : %s\n", filename);

  if (mode != UPNP_READ)
    return NULL;

  if (!strcmp (filename, CDS_LOCATION))
    return get_file_memory (CDS_LOCATION, CDS_DESCRIPTION, CDS_DESCRIPTION_LEN);

  if (!strcmp (filename, CMS_LOCATION))
    return get_file_memory (CMS_LOCATION, CMS_DESCRIPTION, CMS_DESCRIPTION_LEN);

  if (!strcmp (filename, MSR_LOCATION))
    return get_file_memory (MSR_LOCATION, MSR_DESCRIPTION, MSR_DESCRIPTION_LEN);

  if (ut->use_presentation && ( !strcmp (filename, USHARE_PRESENTATION_PAGE)
      || !strncmp (filename, USHARE_CGI, strlen (USHARE_CGI))))
    return get_file_memory (USHARE_PRESENTATION_PAGE, ut->presentation->buf,
                            ut->presentation->len);

  upnp_id = atoi (strrchr (filename, '/') + 1);
  entry = bunny_upnp_get_entry (ut, upnp_id);
  if (!entry)
    return NULL;

  if (!entry->fullpath)
    return NULL;

  log_verbose ("Fullpath : %s @ %s\n", entry->fullpath, entry->servername);

  file = (web_file_t*)malloc (sizeof (struct web_file_t));
  file->fullpath = strdup (entry->fullpath);
  //file->servername = strdup( entry->servername );
  file->pos = 0;
  file->type = web_file_t::FILE_LOCAL;
  file->detail.local.entry = entry;
  file->detail.local.fd = 0;
  file->detail.local.bunny_sock = NULL;
  file->detail.local.dssObj = ut->dssObj;

  if( lastfile != string(filename) ) {

    lastfile = string(filename);
 
    SocketIO bunny_sock;
    bunny_sock.useSSL = true;
    string fullserver = entry->servername;
    string bunny_server, server_dir;
    int x = fullserver.find_first_of("/");
    if( x > 0 ) {
      bunny_server = fullserver.substr(0, x - 1);
      server_dir = fullserver.substr(x + 1, fullserver.size() - x - 1 );
    } else {
      bunny_server = fullserver;
      server_dir = "/";
    }
    int bunny_server_port = 443;
   
    int fd = bunny_sock.openClient( bunny_server, bunny_server_port );
   
    //log_verbose("have bunny_sock fd %d, %s, %d\n", fd, bunny_server, bunny_server_port);

    LObj obj;
   
    string fetch, id;

    if( fd >= 0 ) {
      string t = string(entry->fullpath);
      int i = t.find(".");
      if( i > 0 ) {
        id = t.substr(0,i) + ".info";
      } else {
        id = t + ".info";
      }
      //fetch = "GET /echo/bunnyman/" + id + "?hop=30 HTTP/1.0\n";
      //fetch += "host: buuna.dwanta.com\n\n";
      fetch = "GET " + server_dir + "/" + id + "?hop=30 HTTP/1.0\n";
      fetch += "host: " + bunny_server + "\n\n";
    
      log_verbose("using buf %s \n", fetch.c_str());
      bunny_sock.write(fetch.c_str(), fetch.size());
  
      readHeader( &bunny_sock, obj );
    }
    if( obj.packet.size() > 0 ) {
      string logname = INSTALLDIR + string("/cache/playlog");
      log_verbose("writing to cache/playlog %s\n", logname.c_str());

      int fd2 = open(logname.c_str(), O_WRONLY|O_APPEND|O_CREAT, S_IRWXU);
      if( fd2 >= 0 ) {
        string out = "\n**************************************************";
        out += "\n\nplaying html link: https://buuna.dwanta.com/echo/bunnyman/" + id + "\n";
        int rc = write(fd2, out.c_str(), out.size());
        rc = write(fd2, obj.packet.c_str(), obj.packet.size());
        printf("have logfile write rc %d\n", rc);
        close(fd2);
      }
    }
  }
  return ((UpnpWebFileHandle) file);
}

int
bunny_server_connect(UpnpWebFileHandle fh) {
  struct web_file_t *file = (struct web_file_t *) fh;

  file->detail.local.fd = -1;

  file->detail.local.bunny_sock = new SocketIO();
  file->detail.local.bunny_sock->useSSL = true;

  string bunny_server = "buuna.dwanta.com";
  int bunny_server_port = 443;

  if( file->detail.local.entry->servername != NULL && strlen(file->detail.local.entry->servername) < 80 )
    bunny_server = strdup(file->detail.local.entry->servername);
  else {
    log_info("bad servername \n");
  }

  int x = bunny_server.find_first_of("/");
  if( x > 0 )
    bunny_server = bunny_server.substr(0, x);

  file->detail.local.fd = file->detail.local.bunny_sock->openClient( bunny_server, bunny_server_port );

  pid_t tid;
  tid = syscall(SYS_gettid);

  log_verbose("server connect %d %d %s\n", tid, file->detail.local.fd, bunny_server.c_str() );

  return file->detail.local.fd;
}

int 
bunny_read2 (UpnpWebFileHandle fh, char *buf, size_t buflen,
               const void* cookie __attribute__((unused)),
               const void* requestCookie __attribute__((unused))) {

  struct web_file_t *file = (struct web_file_t *) fh;

  if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "error mutex lock\n" );
    g_quit = true;
  }

  bunny_cache_filepos = file->pos;
  bunny_cache_starting_filepos = file->pos;
  if( file->pos == 0 ) bunny_cache_ending_filepos = 0;

  bunny_cache_sock = file->detail.local.bunny_sock;
  bunny_cache_fd   = file->detail.local.fd;
  bunny_cache_filesize = file->detail.local.entry->size;

  log_verbose("bunny_read2 starting %d, size %d, pos %d\n", buflen, bunny_cache_filesize, bunny_cache_filepos);

  string serverdir = "/";
  string server = string(file->detail.local.entry->servername);
  string server2;
  
  int x = server.find_first_of("/");
  //log_verbose("x = %d \n", x);
  if( x > 0 ) {
    server2 = server.substr(0,x);
    //log_verbose("server %s\n", server.c_str());
    serverdir = server.substr(x, server.size() - x);
    //log_verbose("serverdir %s\n", serverdir.c_str());
  } else {
    server2 = server;
  }

  bunny_cache_servername = server2;
  bunny_cache_request = serverdir + "/" + string(file->fullpath);
  
  if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "error mutex unlock\n" );
    g_quit = true;
  }

  int prefetch = 0;

  if( file->pos == 0 ) {
    // check for prefetch config for cache control
    char* file_ext = getExtension( file->fullpath );
    if( file_ext ) {
      log_verbose("file extension %s\n", file_ext);
      if( strcmp( file_ext, "flac" ) == 0 ) {
        log_verbose("flac processing\n");
        prefetch = 10000000;  // trying 3 packets or 10MB, will make it configurable
        if( prefetch > bunny_cache_filesize ) prefetch = bunny_cache_filesize;
      }
    }
  }
  struct sembuf semOp;
  unsigned long len = 0;
  bool done = false;

  while( ! done && ! g_quit ) {

    semOp.sem_num = 0;
    semOp.sem_op  = 1;
    semOp.sem_flg = 0;

    //log_verbose("signaling bunny cache %s, %d, %d, %d\n", bunny_cache_request, bunny_cache_filepos, bunny_cache_fd, bunny_cache_sock);

    if( semop( bunny_cache_sem, &semOp, 1 ) < 0 ) {
      log_verbose("error signaling cache sem %d\n", errno);
      g_quit = true;
    }
 
    //log_verbose("main locking mutex\n"); 
    if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
      log_verbose( "error mutex lock\n" );
      g_quit = true;
    }
    //log_verbose("main locked mutex\n"); 
    unsigned long remaining = bunny_cache_filesize - bunny_cache_ending_filepos;
    if( remaining > buflen ) remaining = buflen;
  
    if( bunny_cache_starting_filepos >= bunny_cache_filesize ) {
      log_verbose( "data done %d\n", bunny_cache_filesize  );
      done = true;
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
      }
    }
    else
    if( prefetch > 0 && (bunny_cache_ending_filepos - bunny_cache_starting_filepos) < prefetch ) {
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
        g_quit = true;
      }
      log_verbose("prefetch buffer, signaling and sleeping %d / %d \n", bunny_cache_ending_filepos - bunny_cache_starting_filepos, prefetch);
  
      semOp.sem_num = 0;
      semOp.sem_op  = 1;
      semOp.sem_flg = 0;
  
      if( semop( bunny_cache_sem, &semOp, 1 ) < 0 ) {
        log_verbose( "error semop\n" );
        g_quit = true;
      }
      semOp.sem_num = 0;
      semOp.sem_op  = -1;
      semOp.sem_flg = 0;
      int rc = semop( bunny_cache_semdata, &semOp, 1 );
      if( rc != 0 ) {
        log_verbose("bad semdata\n");
        done = true;
      } 
      log_verbose("data signaled %d\n", rc);
    }
    else
    if( bunny_cache_starting_filepos >= bunny_cache_ending_filepos - remaining ) {
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
        g_quit = true;
      }
      sleep(2);
      log_verbose("buffer empty, signaling and sleeping\n");
  
      semOp.sem_num = 0;
      semOp.sem_op  = 1;
      semOp.sem_flg = 0;
  
      if( semop( bunny_cache_sem, &semOp, 1 ) < 0 ) {
        log_verbose( "error semop\n" );
        g_quit = true;
      }
      semOp.sem_num = 0;
      semOp.sem_op  = -1;
      semOp.sem_flg = 0;
      int rc = semop( bunny_cache_semdata, &semOp, 1 );
      if( rc != 0 ) {
        log_verbose("bad semdata\n");
        done = true;
      } 
      log_verbose("data signaled %d\n", rc);
    } else {
      if( bunny_cache_filesize - bunny_cache_starting_filepos < buflen )
        len = bunny_cache_filesize - bunny_cache_starting_filepos;
  
      if( bunny_cache_filepos >= bunny_cache_starting_filepos && bunny_cache_filepos <= bunny_cache_ending_filepos && (bunny_cache_ending_filepos - bunny_cache_starting_filepos >= buflen ) ) {
  
        unsigned long offset = bunny_cache_filepos - bunny_cache_starting_filepos;
        unsigned long x = bunny_cache_ending_bufptr - bunny_cache_starting_filepos_bufptr - offset;
  
        int x2 = bunny_cache_size - ( bunny_cache_starting_filepos_bufptr - bunny_cache );

        log_verbose("checking data %d %d %d %d %d\n", bunny_cache_starting_filepos, bunny_cache_ending_filepos, x, bunny_cache_filesize - x, x2);

        if(  bunny_cache_ending_filepos <= bunny_cache_starting_filepos ) {
          if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
            log_verbose( "error mutex unlock\n" );
          }
          log_verbose( "no data %d\n", bunny_cache_starting_filepos );
          semOp.sem_num = 0;
          semOp.sem_op  = 1;
          semOp.sem_flg = 0;
  
          if( semop( bunny_cache_sem, &semOp, 1 ) < 0 ) {
            log_verbose( "error semop\n" );
            g_quit = true;
          }
          sleep(1);
          semOp.sem_num = 0;
          semOp.sem_op  = -1;
          semOp.sem_flg = 0;
          int rc = semop( bunny_cache_semdata, &semOp, 1 );
          if( rc != 0 ) {
            log_verbose("bad semdata\n");
            done = true;
          }
          log_verbose("data signaled %d\n", rc);
        }
        else
        if( x > buflen ) {
          //log_verbose("x > buflen \n");
          memcpy( buf, bunny_cache_starting_filepos_bufptr + offset, buflen );
          bunny_cache_starting_filepos_bufptr += buflen;
          int x = bunny_cache_starting_filepos_bufptr - bunny_cache;
          log_verbose("2 cache buffer offset currently %d %d\n", x, bunny_cache_filepos);
        } else {
          //log_verbose("x < buflen \n");
          unsigned long offset = bunny_cache_filepos - bunny_cache_starting_filepos;
          unsigned long len2 = bunny_cache_ending_bufptr - bunny_cache_starting_filepos_bufptr - offset;

          log_verbose("x2 check %d %d\n", x2, len2);
          len2 = x2;
  
          //if( len2 > sizeof(buf) ) len2 = sizeof(buf);
   
          if( len2 <= 0 ) {
            log_verbose("error in len %d %d %d\n", len2, bunny_cache_ending_bufptr, offset);
            g_quit = true;
            return -1;
          } 
          log_verbose("copy vals %d len2 %d %d %d\n", len, len2, len2 - len, buflen - len2);
          memcpy( buf, bunny_cache_starting_filepos_bufptr + offset, len2 );
          memcpy( buf + len2, bunny_cache, buflen - len2 );
          bunny_cache_starting_filepos_bufptr = bunny_cache + (buflen - len2);

          int x = bunny_cache_starting_filepos_bufptr - bunny_cache;
          log_verbose("1 cache buffer offset currently %d %d\n", x, bunny_cache_filepos);

          if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
            log_verbose( "error mutex unlock\n" );
          }
        }
        if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
          log_verbose( "error mutex unlock\n" );
        }
        log_verbose("copied data, unlocked mutex, done %d\n", buflen);
        done = true;
        file->pos += buflen;
        return buflen;
      } else {
        log_verbose( "outside step %d %d %d %d\n", bunny_cache_filepos, bunny_cache_starting_filepos, bunny_cache_ending_filepos, buflen );
        if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
          log_verbose( "error mutex unlock\n" );
        }
        semOp.sem_num = 0;
        semOp.sem_op  = -1;
        semOp.sem_flg = 0;
        int rc = semop( bunny_cache_semdata, &semOp, 1 );
        if( rc != 0 ) {
          log_verbose("bad semdata\n");
          done = true;
        }
        sleep(1);
      }
      bunny_cache_starting_filepos += len;
  
      unsigned long t_size = bunny_cache_ending_filepos - bunny_cache_starting_filepos;
  
      //log_verbose( "main thread size %d, %d, %d\n", t_size, bunny_cache_starting_filepos, bunny_cache_ending_filepos );
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose("bad mutex\n");
        g_quit = true;
      }
    }
  }
  return 0;
}

int 
bunny_read (UpnpWebFileHandle fh, char *buf, size_t buflen,
               const void* cookie __attribute__((unused)),
               const void* requestCookie __attribute__((unused))) {

  struct web_file_t *file = (struct web_file_t *) fh;
  ssize_t len = -1;

  // vars  cache, cachesize, cachestart, currentpos, fd, servername, request
  // check cache first, if data, send that, signal cache thread about update
  // if cache name different or data not sufficient write fd, pos, and len to global vars and signal cache thread
  //   cache thread will get chunks in 5MB and try to fill 20MB cache
  // wait for semaphore
  // send data

  pid_t tid;
  tid = syscall(SYS_gettid);

  struct timeval tv;
  gettimeofday( &tv, NULL );

  log_verbose ("bunny_read %d %lu file=%p entry=%p timestamp=%d %d\n", tid, buflen, file, file->detail.local.entry, tv.tv_sec, tv.tv_usec);

  if (!file)
    return -1;

  if (file->type == FILE_MEMORY ) {
    log_verbose ("Reading file from memory.\n");
    len = (size_t) MIN (buflen, file->detail.memory.len - file->pos);
    memcpy (buf, file->detail.memory.contents + file->pos, (size_t) len);
    return len;
  }

  if( bunny_cache_enabled ) {
    return bunny_read2( fh, buf, buflen, cookie, requestCookie );
  }

  //log_verbose ("bunny_fd is %d %d\n", file->detail.local.fd, tid );
  //log_info("bunny_fd is %d %d : %s\n", file->detail.local.fd, tid, file->fullpath );

  if (file->detail.local.fd <= 0 ) {
    bunny_server_connect(fh);
    //log_verbose ("bunny_fd is now %d %d\n", file->detail.local.fd, tid);
    if (file->detail.local.fd <= 0) {
      log_verbose("bad bunny_server_connect\n");
      return 0;
    }
  }
  string serverdir = "/";
  string server = string(file->detail.local.entry->servername);
  string server2;

  int x = server.find_first_of("/");
  //log_verbose("x = %d \n", x);
  if( x > 0 ) {
    server2 = server.substr(0,x);
    //log_verbose("server %s\n", server.c_str());
    serverdir = server.substr(x, server.size() - x);
    //log_verbose("serverdir %s\n", serverdir.c_str());
  } else {
    server2 = server;
  }
  string test;
  //test = "GET /echo/bunnycloud/" + string(file->fullpath) + " HTTP/1.0\n";
  test = "GET " + serverdir + "/" + string(file->fullpath) + " HTTP/1.0\n";
  if( file->detail.local.entry->servername != NULL )
    test += "host: " + server2 + "\n";
  else
    test += "host: buuna.dwanta.com\n";

  test += "range: bytes=" + to_string(file->pos) + "-" + to_string(buflen) + "\n\n";
 
  log_verbose("sending %s %d\n", test.c_str(), tid); 

  file->detail.local.bunny_sock->write(test.c_str(), test.size());

  LObj obj;

  readHeader( file->detail.local.bunny_sock, obj );

  int i = buflen;
  if( obj.packet.size() < i )
    i = obj.packet.size();
  else
  if( obj.packet.size() > i )
    log_verbose ("server sent too many bytes (%d), truncating!!!", obj.packet.size());

  if( i>= 0 )
    file->pos += i;

  log_verbose ("Read %d bytes %d.\n", i, tid);

  if( i > 0 )
    memcpy( buf, obj.packet.c_str(), i );
  else {
    buf[0] = 0;
    return -1;
  }

  gettimeofday( &tv, NULL );

  log_verbose ("Read %zd data bytes %d, file pos is %lld, timestamp %d %d.\n", obj.packet.size(), tid, file->pos, tv.tv_sec, tv.tv_usec);

  buflen = i;

  return i;
}

int 
bunny_write (UpnpWebFileHandle fh __attribute__((unused)),
            char *buf __attribute__((unused)),
            size_t buflen __attribute__((unused)),
            const void* cookie __attribute__((unused)),
            const void* requestCookie __attribute__((unused))) {

  log_verbose ("bunny write\n");

  return 0;
}

int 
bunny_seek (UpnpWebFileHandle fh, off_t offset, int origin,
               const void* cookie __attribute__((unused)),
               const void* requestCookie __attribute__((unused))) {
  struct web_file_t *file = (struct web_file_t *) fh;
  off_t newpos = -1;

  log_verbose ("bunny_seek file=%p origin=%d bytes\n", file, origin);

  if (!file)
    return -1;

  switch (origin) {
  case SEEK_SET:
    log_verbose ("Attempting to seek to %lld (was at %lld) in %s\n",
                offset, file->pos, file->fullpath);
    newpos = offset;
    break;
  case SEEK_CUR:
    log_verbose ("Attempting to seek by %lld from %lld in %s\n",
                offset, file->pos, file->fullpath);
    newpos = file->pos + offset;
    break;
  case SEEK_END:
    log_verbose ("Attempting to seek by %lld from end (was at %lld) in %s\n",
                offset, file->pos, file->fullpath);

    if (file->type == web_file_t::FILE_LOCAL) {
      newpos = file->detail.local.entry->size;
    }
    else if (file->type == web_file_t::FILE_MEMORY)
      newpos = file->detail.memory.len + offset;
    break;
  }

  log_verbose("new pos is %lld\n", newpos);

  switch (file->type) {
  case web_file_t::FILE_LOCAL:
    /* Just make sure we cannot seek before start of file. */
    if (newpos < 0) {
      log_verbose ("%s: cannot seek: %s\n", file->fullpath, strerror (EINVAL));
      return -1;
    }
    break;
  case web_file_t::FILE_MEMORY:
    if (newpos < 0 || newpos > file->detail.memory.len) {
      log_verbose ("%s: cannot seek: %s\n", file->fullpath, strerror (EINVAL));
      return -1;
    }
    break;
  case web_file_t::FILE_DSS:
    break;
  }
  file->pos = newpos;

  return 0;
}

int 
bunny_close (UpnpWebFileHandle fh,
                const void* cookie __attribute__((unused)),
                const void* requestCookie __attribute__((unused))) {
  struct web_file_t *file = (struct web_file_t *) fh;

  pid_t tid;
  tid = syscall(SYS_gettid);

  log_verbose ("bunny_close2 %d file=%p\n", tid, file);

  if( file->type == web_file_t::FILE_MEMORY ) {
    if (file->detail.memory.contents)
      free (file->detail.memory.contents);
    return 0;
  }

  if( file->detail.local.bunny_sock != NULL ) {
    file->detail.local.bunny_sock->doClose();
    free(file->detail.local.bunny_sock);
    file->detail.local.fd = -1;
    file->detail.local.bunny_sock = NULL;
  }
  if (!file)
    return -1;

  switch (file->type) {
  case web_file_t::FILE_MEMORY:
    if (file->detail.memory.contents)
      free (file->detail.memory.contents);
    break;
  default:
    //log_verbose ("Unknown file type %d.\n", file->type);
    break;
  }

  if (file->fullpath)
    free (file->fullpath);
  if(file)
    free (file);

  log_verbose("bunny_close done\n");

  return 0;
}

int
readHeader( SocketIO* socket, LObj& lobj ) {
  int rc = 1;
  char buf[MAX_HEADER];
  memset(buf,0,sizeof(buf));
  bool done = false;
  int i = 0;

  pid_t tid;
  tid = syscall(SYS_gettid);

  lobj.header.clear();
  lobj.headers.clear();
  lobj.get.clear();
  lobj.request_type = 0;
  lobj.datasource = 0;
  lobj.content_length = 0;
  lobj.packet = "";

  int timeout = 60;
  if( lobj.keepalive ) 
    timeout = 120;

  while( rc > 0 && ! done ) {
    //log_verbose("about to read, timeout=%d %d %d\n", timeout, tid, socket->useSSL);
    memset( buf, 0, sizeof(buf) );
    rc = socket->doReadLine(buf, sizeof(buf), timeout);
    if( rc > 0 ) {
      int len = strlen(buf);
      //log_verbose("readheader buf %s %d %d %d\n", buf, rc, len, tid);
      if( len == 0 ) { 
        done = true;
      } else {
        lobj.headers[i++] = buf;
        //logger.error("added headers " + string(buf));
      }
    }
  }

  parseHeader( socket, lobj );

  if( lobj.content_length <= 0  || lobj.content_length > MAX_CONTENT ) {
    log_verbose("bad content length %d\n",lobj.content_length);
    return 0;
  }

  char* buf2 = (char*) malloc(lobj.content_length + 1);
  if( ! buf2 ) {
    log_verbose("bad malloc\n");
    return 0;
  }
  memset(buf2,0,lobj.content_length + 1);
  //log_verbose("read content length %d %d %d\n", lobj.content_length, tid, socket->useSSL);

  rc = socket->doRead(buf2, lobj.content_length, 60);
  //log_verbose("read buf2 %d %d\n", rc, tid);

  if( rc != lobj.content_length ) {
    log_verbose("bad header content length\n");
    lobj.packet = "";
    free(buf2);
    return 0;
  }
  lobj.packet = string(buf2, lobj.content_length);

  free(buf2);
  return 1;
}

int parseHeader( SocketIO* socket, LObj& lobj ) {
  
  if( lobj.headers.size() < 1 || lobj.headers.size() > MAX_HEADERS ) {
    log_verbose("headers size error, exiting %lu\n", lobj.headers.size());
    return -1;
  }

  pid_t tid;
  tid = syscall(SYS_gettid);

  int pos = lobj.headers[0].find(" ");
  //log_verbose("header pos is %d\n", pos);
  if( pos > 0 ) {
    lobj.rc = atoi(lobj.headers[0].substr(pos + 1, 3).c_str()); 
    //log_verbose("header rc %d %d\n", lobj.rc, tid);
  } 

  //typedef map<string,string>::const_iterator J;
  //typedef multimap<string,string>::const_iterator J2;
  //typedef map<string,int>::const_iterator J3;

  // check access and type of data request  

  lobj.request_type = 0;
  lobj.datasource = 0;

  // need the cookies here
  map<int,string>::iterator I;
  for( I = lobj.headers.begin(); I != lobj.headers.end(); ++I ) {
    int i = I->second.find(": ");
    if( i > 0 ) {
      string key = I->second.substr(0, i); 
      int size = I->second.size();
      string val = I->second.substr(i + 2, size - i - 2);

      for (char& c : key ) {
        c = std::tolower(c);
      }

      lobj.header[key] = val;
      //log_verbose("header test %s %s %d\n", key.c_str(), val.c_str(), tid);
      if( key == "content-length" ) {
        lobj.content_length = atoi(val.c_str());
      }
    }
  }
  //log_verbose("header test done\n");
  return 0;
}
