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

bool bunny_dspcache_enabled = false;
unsigned int bunny_dspcache_retain = 0;
unsigned int bunny_cache_size = 20000000;
unsigned int bunny_cache_fd = -1;
unsigned int bunny_cache_sem = 0;
unsigned int bunny_cache_seminit = 0;
unsigned int bunny_cache_semdata = 0;

unsigned long bunny_cache_starting_filepos = 0;
unsigned long bunny_cache_filepos = 0;
unsigned long bunny_cache_filesize = 0;

SocketIO *bunny_cache_sock = NULL;

string bunny_cache_servername;
string bunny_cache_request;
string bunny_cache_filename;
string bunny_cache_directory;
string bunny_avdevice;
string bunny_avdevice_mode;
bool   bunny_avdevice_flac_pause;

pthread_mutex_t bunny_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *
getExtension (const char *filename) {
  char *str = NULL;

  str = strrchr ((char*)filename, '.');
  if (str)
    str++;

  return str;
}


// file-based cache version

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

  if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "ERROR: failed to unlock bunny_cache %d\n", errno );
    g_quit = true;
  }
  extern struct ushare_t *ut;
  struct sembuf semOp;
  semOp.sem_num = 0;
  semOp.sem_op  = 1;
  semOp.sem_flg = 0;

  bunny_cache_directory = ut->installdir + string("/cache/");

  if( semop( bunny_cache_seminit, &semOp, 1 ) < 0 ) {
    log_verbose("error signaling seminit %d\n", errno);
    g_quit = true;
  }

  if( bunny_cache_sem < 0 ) {
    log_verbose("error getting cache semaphore %d\n", errno);
    g_quit = true;
  }

  string idx;
  if( ut->dssObj->server ) {
    log_verbose("checking dspcachedir\n");
    map<string,string>::const_iterator I = ut->dssObj->server->configMap.find("dspcachedir");
    if( I != ut->dssObj->server->configMap.end() ) {
      bunny_cache_directory = strdup(I->second.c_str());
      if( bunny_cache_directory.substr(bunny_cache_directory.size() - 1, 1) != "/" )
        bunny_cache_directory += "/";
    }
    log_verbose("checking dspcachedir %s\n", bunny_cache_directory.c_str());
    I = ut->dssObj->server->configMap.find("configidx");
      
    if( I != ut->dssObj->server->configMap.end() )
      idx = I->second;

    I = ut->dssObj->server->configMap.find("avdevice" + idx);
  
    if( I != ut->dssObj->server->configMap.end() )
      bunny_avdevice = I->second;

    I = ut->dssObj->server->configMap.find("avdevice_mode" + idx);
  
    if( I != ut->dssObj->server->configMap.end() )
      bunny_avdevice_mode = I->second;

    I = ut->dssObj->server->configMap.find("avdevice_flac_pause" + idx);
  
    if( I != ut->dssObj->server->configMap.end() ) {
      if( I->second == "yes" )
        bunny_avdevice_flac_pause = true;
    }
  }


  int connect_errors = 0;

  while( ! g_quit ) {
    struct sembuf semOp;
    semOp.sem_num = 0;
    semOp.sem_op  = -1;
    semOp.sem_flg = 0;
 
    // wait for cache signal
    log_verbose("cache thread waiting\n");

    int rc = semop( bunny_cache_sem, &semOp, 1 );

    string cachefile = "";

    if( rc == -1 )
      g_quit = true;

    if( rc == 0 ) {
      if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
        g_quit = true;
      }
      if( bunny_cache_fd != -1 ) {
        close(bunny_cache_fd);
        bunny_cache_fd = -1;
      }
      if( bunny_cache_fd == -1 ) {
        cachefile = bunny_cache_directory + bunny_cache_filename;
        //log_verbose("opening cache file %s\n", cachefile.c_str());
        bunny_cache_fd = open( cachefile.c_str(), O_WRONLY|O_APPEND|O_CREAT, S_IRWXU);
      }
      struct stat st;
      bool fileok = false;
      bool do_flac = false;

      if( bunny_cache_fd != -1 ) {
        rc = fstat( bunny_cache_fd, &st );
        if( rc == 0 ) {
          bunny_cache_starting_filepos = st.st_size;
          fileok = true;
        } else {
          log_verbose("bad file %s\n", cachefile.c_str());
        }
      } else {
        log_verbose("bad file %s\n", cachefile.c_str());
      }
      unsigned long t_filesize = 0;
      unsigned long t_pos = 0;
      string t_request, t_servername;

      if( fileok ) { 
        t_filesize = bunny_cache_filesize;
        t_pos = bunny_cache_filepos;

        t_request = bunny_cache_request;
        t_servername = bunny_cache_servername;

        char* file_ext = getExtension( bunny_cache_filename.c_str() );
        do_flac = false;
        bunny_cache_size = 5000000;
        if( file_ext ) {
          if( strcmp( file_ext, "flac" ) == 0 ) {
            do_flac = true;
            bunny_cache_size = 15000000;
          }
        }
      }
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "ERROR: failed to unlock bunny_cache %d\n", errno );
        g_quit = true;
      }

      //log_verbose("cache thread pos %d, starting %d, filesize %d\n", t_pos, bunny_cache_starting_filepos, t_filesize);

      if( ! fileok ) {
        log_verbose("cache thread sleeping, bad file found\n");
        sleep(1);
      }
      if( t_pos >= t_filesize ) {
        log_verbose("bad pos %d %d\n", t_pos, t_filesize);
      }
      else
      if( (bunny_cache_starting_filepos - t_pos) <= bunny_cache_size && bunny_cache_starting_filepos < t_filesize && ! g_quit && fileok ) {
        bool done = false;
        bool finished = false;
        LObj obj;
        while( (bunny_cache_starting_filepos - t_pos) <= bunny_cache_size && ! finished && ! done && ! g_quit && bunny_cache_starting_filepos < t_filesize ) {

          if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
            log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
            g_quit = true;
          }
          string request;
          int buflen = 1048576;
          if( do_flac ) {
            if( bunny_cache_filepos == 0  )
              buflen = 4000000;
            else
              buflen = 2000000;
          }
          unsigned long remaining = bunny_cache_filesize - bunny_cache_starting_filepos;
          if( remaining < buflen ) {
            buflen = remaining;
            finished = true;
          }
          if( t_servername != bunny_cache_servername || t_request != bunny_cache_request ) {
            done = true;
            if( bunny_cache_fd != -1 ) {
              close(bunny_cache_fd);
              bunny_cache_fd = -1;
            }
            log_verbose("changed cache request, exiting\n");
          }
          if( ! done ) {
            request = "GET " + bunny_cache_request + " HTTP/1.0\n";
            request += "host: " + bunny_cache_servername + "\n";
            request += "range: bytes=" + to_string(bunny_cache_starting_filepos) + "-" + to_string(buflen) + "\n\n";

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
   
            int fd = bunny_sock.openClient( bunny_server, bunny_server_port );

            if( fd < 0 ) {
              log_verbose("connect failed, trying again %s, %d\n", bunny_server.c_str(), errno);
              fd = bunny_sock.openClient( bunny_server, bunny_server_port );
              if( fd < 0 ) {
                if( connect_errors++ > 10 ) {
                  done = true;
                  log_verbose("bad socket, sleeping\n");
                  sleep(60);
                } else { 
                  log_verbose("bad socket, retrying\n");
                  sleep(5);
                }
              }
            } else {
              log_verbose("cache sending %s %d\n", request.c_str(), fd );
              bunny_sock.write(request.c_str(), request.size());
              connect_errors = 0;
              int rc2 = readHeader( &bunny_sock, obj );
              if( rc2 != 1 ) {
                done = true;
                log_verbose("bad readHeader rc %d\n", rc2);
              }
              bunny_sock.doClose();
            }
            if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
              log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
              g_quit = true;
              done = true;
            }
          }
          if( ! done && ! g_quit ) {
            if( t_servername != bunny_cache_servername || t_request != bunny_cache_request ) {
              done = true;
              if( bunny_cache_fd != -1 ) {
                close(bunny_cache_fd);
                bunny_cache_fd = -1;
              }
              log_verbose("changed cache request, exiting\n");
            } 
            else {
              if( bunny_cache_fd == -1 ) {
                string cachefile = bunny_cache_directory + bunny_cache_filename;
                bunny_cache_fd = open( cachefile.c_str(), O_WRONLY|O_APPEND|O_CREAT, S_IRWXU);
              }
              if( bunny_cache_fd != -1 ) {
                rc = write( bunny_cache_fd, obj.packet.c_str(), obj.packet.size() );
                if( rc != obj.packet.size() ) {
                  log_verbose("bad cache write %s %d\n", bunny_cache_filename.c_str(), errno);
                }
                struct stat st;
                rc = fstat( bunny_cache_fd, &st );
                if( rc == 0 ) {
                  unsigned long int t = bunny_cache_starting_filepos + obj.packet.size();
                  if( t != st.st_size ) {
                    log_verbose("cache consistency error, filepos %d %d %d\n", bunny_cache_starting_filepos, st.st_size, t);
                  }
                  bunny_cache_starting_filepos = st.st_size;
                } else {
                  log_verbose("stat error %s, %d\n", bunny_cache_filename.c_str(), bunny_cache_fd);
                  bunny_cache_starting_filepos += obj.packet.size();
                }
              } else {
                log_verbose("cache error\n");
                bunny_cache_starting_filepos += obj.packet.size();
                done = true;
              }
            }
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
          //log_verbose("signaled data ready %d\n", bunny_cache_starting_filepos);
        }
        if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
          log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
          g_quit = true;
        }
        if( bunny_cache_fd != -1 ) {
          close(bunny_cache_fd);
          bunny_cache_fd = -1;
        }
        if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
          log_verbose( "ERROR: failed to unlock bunny_cache %d\n", errno );
          g_quit = true;
        }
      } else {
        log_verbose( "buffer full, signaling\n" );
        struct sembuf semOp;
        semOp.sem_num = 0;
        semOp.sem_op  = 1;
        semOp.sem_flg = 0;

        if( semop( bunny_cache_semdata, &semOp, 1 ) < 0 ) {
          log_verbose("error signaling semdata %d\n", errno);
          g_quit = true;
        }
        if( do_flac ) {
          if( bunny_cache_filepos <= 12582913 ) {
            // see if it's stuck and send the playnext command
            string resp = sendCommand("/YamahaExtendedControl/v1/netusb/getPlayInfo", bunny_avdevice);
            // "play_time":19,
            int x = resp.find("play_time\":0,");
            log_verbose("have resp %s, x=%d, pos=%d\n", resp.c_str(), x, bunny_cache_filepos);
            if( x > 0 ) {
              sendCommand("/YamahaExtendedControl/v1/netusb/setPlayback?playback=next", bunny_avdevice);
            }
          }
        }
      } 
    } 
  }
  if( bunny_cache_filename != "" && bunny_cache_directory != "" ) {
    string cachefile = bunny_cache_directory + bunny_cache_filename;
    if( bunny_dspcache_retain == 2 && bunny_cache_filesize > 10000000 ) {
      int fd = open( cachefile.c_str(), O_WRONLY, S_IRWXU);
      if( fd >= 0 ) {
        log_verbose("truncating cache file %s\n", cachefile.c_str());
        int rc = lseek(fd, 10000000, SEEK_SET);
        if( rc == 10000000 ) {
          rc = ftruncate(fd, 10000000);
          if( rc != 0 ) {
            log_verbose("error truncating %s\n", bunny_cache_filename.c_str());
          }
        }
        close(fd);
      }
    }
  }
  else 
  if( bunny_dspcache_retain == 0 ) {
    string cachefile = bunny_cache_directory + bunny_cache_filename;
    int rc = unlink( cachefile.c_str() );
    if( rc != 0 ) {
      log_verbose("error removing cache file %s %d\n", cachefile.c_str(), errno);
    }
  }
  int retVal;
  pthread_exit( (void*)&retVal );
}

int 
bunny_get_info (const char *filename, UpnpFileInfo *info,
                   const void* cookie __attribute__((unused)),
                   const void** requestCookie __attribute__((unused))) {
  extern struct ushare_t *ut;
  struct upnp_entry_t *entry = NULL;
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

  if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "error mutex lock\n" );
    g_quit = true;
  }

  bunny_cache_filepos = 0;
  bunny_cache_starting_filepos = 0;
  bunny_cache_filename = strdup (entry->fullpath);
  bunny_cache_filesize = entry->size;

  if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "error mutex unlock\n" );
    g_quit = true;
  }


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
      fetch = "GET " + server_dir + "/" + id + "?hop=30 HTTP/1.0\n";
      fetch += "host: " + bunny_server + "\n\n";
    
      log_verbose("using buf %s \n", fetch.c_str());
      bunny_sock.write(fetch.c_str(), fetch.size());
  
      readHeader( &bunny_sock, obj );
    }
    if( obj.packet.size() > 0 ) {
      string logname = ut->installdir + string("/cache/playlog");
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
  if( file->pos == 0 )
    bunny_cache_starting_filepos = file->pos;

  bunny_cache_sock = file->detail.local.bunny_sock;
  bunny_cache_filesize = file->detail.local.entry->size;

  log_verbose("bunny_read2 starting buflen %d, size %d, pos %d\n", buflen, bunny_cache_filesize, bunny_cache_filepos);

  string serverdir = "/";
  string server = string(file->detail.local.entry->servername);
  string server2;
  
  int x = server.find_first_of("/");
  if( x > 0 ) {
    server2 = server.substr(0,x);
    serverdir = server.substr(x, server.size() - x);
  } else {
    server2 = server;
  }

  bunny_cache_servername = server2;
  bunny_cache_request = serverdir + "/" + string(file->fullpath);
  bunny_cache_filename = string(file->fullpath);
  
  string cachefile = bunny_cache_directory + bunny_cache_filename;
  if( file->pos == 0 ) {
    bunny_cache_fd = open( cachefile.c_str(), O_RDONLY );
    if( bunny_cache_fd > 0 ) {
      struct stat st;
      int rc = fstat( bunny_cache_fd, &st);
      if( rc == 0 ) {
        bunny_cache_starting_filepos = st.st_size;
        log_verbose("set starting pos = %d\n", bunny_cache_starting_filepos);
      }
      close(bunny_cache_fd);
      bunny_cache_fd = -1;
    }
  }

  if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "error mutex unlock\n" );
    g_quit = true;
  }

  int prefetch = 0;

  if( file->pos == 0 ) {
    // check for prefetch config for cache control
    char* file_ext = getExtension( file->fullpath );
    if( file_ext ) {
      if( strcmp( file_ext, "flac" ) == 0 ) {
        log_verbose("flac processing\n");
        prefetch = 10000000;  // trying 3 packets or 12MB, will make it configurable
        if( prefetch > bunny_cache_filesize ) prefetch = bunny_cache_filesize;
      }
    }
    union semun {
      int val;
      struct semid_ds *buf;
      unsigned short *array;
    } arg;
    arg.val = 0;
    struct sembuf sbuf;
    sbuf.sem_num = 0;
    sbuf.sem_op = 0; 
    sbuf.sem_flg = 0;
    if (semctl(bunny_cache_semdata, 0, SETVAL, arg) == -1 || semop(bunny_cache_semdata, &sbuf, 1) == -1) {
      log_verbose("IPC error: semop"); 
      g_quit = true;
    }
  }
  struct sembuf semOp;
  unsigned long len = 0;
  bool done = false;

  semOp.sem_num = 0;
  semOp.sem_op  = 1;
  semOp.sem_flg = 0;

  if( semop( bunny_cache_sem, &semOp, 1 ) < 0 ) {
    log_verbose("error signaling cache sem %d\n", errno);
    g_quit = true;
  }
  if( bunny_cache_starting_filepos < prefetch && bunny_avdevice_flac_pause && bunny_avdevice_mode == "yxc" ) {
    log_verbose("pausing %s for cache fill\n", bunny_avdevice.c_str());
    sendCommand("/YamahaExtendedControl/v1/netusb/setPlayback?playback=pause", bunny_avdevice);
    bool pause_done = false;
    int x = 0;
    while( ! pause_done && ! g_quit ) {
      if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex lock\n" );
        g_quit = true;
      }
      unsigned long int t_pos = bunny_cache_filepos;
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
        g_quit = true;
      }
      if( t_pos >= prefetch || x++ > 5 ) {
        pause_done = true;
      } else {
        sleep(1); 
      }
    }
    log_verbose("start playing %s\n", bunny_avdevice.c_str());
    sendCommand("/YamahaExtendedControl/v1/netusb/setPlayback?playback=play", bunny_avdevice);
  } 
  while( ! done && ! g_quit ) {

    if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
      log_verbose( "error mutex lock\n" );
      g_quit = true;
    }
    unsigned long remaining = bunny_cache_filesize - bunny_cache_starting_filepos;
    if( remaining > buflen ) remaining = buflen;
  
    if( bunny_cache_filepos >= bunny_cache_filesize ) {
      log_verbose( "data done %d\n", bunny_cache_filesize  );
      done = true;
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
      }

    }
    else
    if( prefetch > 0 && bunny_cache_starting_filepos < prefetch ) {
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
        g_quit = true;
      }
      log_verbose("prefetch buffer, signaling and sleeping %d / %d \n", bunny_cache_starting_filepos, prefetch);
  
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
      log_verbose("prefetch data signaled %d\n", rc);
    }
    else
    if( (bunny_cache_starting_filepos - bunny_cache_filepos) < buflen ) {
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
        g_quit = true;
      }
      log_verbose("buffer empty, signaling and sleeping %d %d %d %d\n", 
       bunny_cache_starting_filepos, bunny_cache_filepos, remaining, buflen );
      //sleep(2);
  
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
      log_verbose("empty buffer data signaled %d\n", rc);
    } else {
      if( bunny_cache_filesize - bunny_cache_starting_filepos < buflen )
        len = bunny_cache_filesize - bunny_cache_starting_filepos;
  
      log_verbose("checking data %d %d \n", bunny_cache_starting_filepos, bunny_cache_filesize);

      if(  bunny_cache_starting_filepos <= bunny_cache_filepos ) {
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
        log_verbose("no data signaled %d\n", rc);
      }
      if( bunny_cache_fd != -1 ) {
        close(bunny_cache_fd);
        bunny_cache_fd = -1;
      }
      if( bunny_cache_fd == -1 ) {
        string cachefile = bunny_cache_directory + bunny_cache_filename;
        bunny_cache_fd = open( cachefile.c_str(), O_RDONLY );
        //struct stat st;
        //int rc = fstat( bunny_cache_fd, &st);
      }
      if( bunny_cache_fd != -1 ) {
        int rc = lseek( bunny_cache_fd, file->pos, SEEK_SET );
        if( rc == file->pos ) {
          rc = read( bunny_cache_fd, buf, buflen );
          if( rc != buflen ) {
            log_verbose("read error %d\n", rc);
          }
          file->pos += rc;
          struct stat st;
          rc = fstat( bunny_cache_fd, &st);
          if( rc == 0 ) {
            bunny_cache_starting_filepos = st.st_size;
          }
        }
        close(bunny_cache_fd);
        bunny_cache_fd = -1;
      }
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "error mutex unlock\n" );
      }
      //log_verbose("copied data, unlocked mutex, done %d %d\n", buflen, file->pos, bunny_cache_starting_filepos);
      done = true;

      if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
        g_quit = true;
      }
      if( file->pos >= bunny_cache_filesize ) {
        log_verbose("done, checking cache file %d\n", bunny_dspcache_retain);
        if( bunny_dspcache_retain == 1 ) {
          if( bunny_cache_filename != "" && bunny_cache_directory != "" ) {
            log_verbose("keeping cache file %s/%s\n", bunny_cache_directory.c_str(), bunny_cache_filename.c_str());
          }
        }
        else
        if( bunny_cache_filename != "" && bunny_cache_directory != "" ) {
          string cachefile = bunny_cache_directory + bunny_cache_filename;
          if( bunny_dspcache_retain == 2 && bunny_cache_filesize > 10000000 ) {
            int fd = open( cachefile.c_str(), O_WRONLY, S_IRWXU);
            if( fd >= 0 ) {
              log_verbose("truncating cache file %s\n", cachefile.c_str());
              int rc = lseek(fd, 10000000, SEEK_SET);
              if( rc == 10000000 ) {
                rc = ftruncate(fd, 10000000);
                if( rc != 0 ) {
                  log_verbose("error truncating %s\n", bunny_cache_filename.c_str());
                }
              }
              close(fd);
            }
          }
          else 
          if( bunny_dspcache_retain == 0 ) {
            int rc = unlink( cachefile.c_str() );
            if( rc != 0 ) {
              log_verbose("error removing cache file %s %d\n", cachefile.c_str(), errno);
            }
          }
        }
      }
      if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
        log_verbose( "ERROR: failed to unlock bunny_cache %d\n", errno );
        g_quit = true;
      }
      return buflen;
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

  if( bunny_dspcache_enabled ) {

    int rc = bunny_read2( fh, buf, buflen, cookie, requestCookie );
    log_verbose("read2 rc %d %d\n", rc, buflen);
    return rc;
  }

  if (file->detail.local.fd <= 0 ) {
    bunny_server_connect(fh);
    if (file->detail.local.fd <= 0) {
      log_verbose("bad bunny_server_connect\n");
      return 0;
    }
  }
  string serverdir = "/";
  string server = string(file->detail.local.entry->servername);
  string server2;

  int x = server.find_first_of("/");
  if( x > 0 ) {
    server2 = server.substr(0,x);
    serverdir = server.substr(x, server.size() - x);
  } else {
    server2 = server;
  }
  string request;
  request = "GET " + serverdir + "/" + string(file->fullpath) + " HTTP/1.0\n";
  if( file->detail.local.entry->servername != NULL )
    request += "host: " + server2 + "\n";
  else
    request += "host: buuna.dwanta.com\n";

  request += "range: bytes=" + to_string(file->pos) + "-" + to_string(buflen) + "\n\n";
 
  log_verbose("sending %s %d\n", request.c_str(), tid); 

  file->detail.local.bunny_sock->write(request.c_str(), request.size());

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

  if( pthread_mutex_lock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
    g_quit = true;
  }
  log_verbose("close check %d %d\n", bunny_cache_filesize, bunny_dspcache_retain);

  if( bunny_cache_fd != -1 )
    close(bunny_cache_fd);
  bunny_cache_fd = -1;

/*
  if( bunny_dspcache_retain == 1 ) {
    if( bunny_cache_filename != "" && bunny_cache_directory != "" ) {
      log_verbose("keeping cache file %s/%s\n", bunny_cache_directory.c_str(), bunny_cache_filename.c_str()); 
    }
  }
  else
  if( bunny_cache_filename != "" && bunny_cache_directory != "" ) {
    string cachefile = bunny_cache_directory + bunny_cache_filename;
    if( bunny_dspcache_retain == 2 && bunny_cache_filesize > 10000000 ) {
      int fd = open( cachefile.c_str(), O_WRONLY, S_IRWXU);
      if( fd >= 0 ) {
        log_verbose("truncating cache file %s\n", cachefile.c_str());
        int rc = lseek(fd, 10000000, SEEK_SET);
        if( rc == 10000000 ) {
          int rc = ftruncate(fd, 10000000);
          if( rc != 0 ) {
            log_verbose("error truncating %s\n", bunny_cache_filename.c_str());
          }
        }
        close(fd);
      }
    }
    else
    if( bunny_dspcache_retain == 0 ) {
      int rc = unlink( cachefile.c_str() );
      if( rc != 0 ) {
        log_verbose("error removing cache file %s %d\n", cachefile.c_str(), errno);
      }
    }
  }
*/
  bunny_cache_filepos = 0;
  bunny_cache_filesize = 0;
  bunny_cache_filename = "";
  bunny_cache_starting_filepos = 0;

  if( pthread_mutex_unlock( &bunny_cache_mutex) != 0 ) {
    log_verbose( "ERROR: failed to lock bunny_cache %d\n", errno );
    g_quit = true;
  }
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
    memset( buf, 0, sizeof(buf) );
    rc = socket->doReadLine(buf, sizeof(buf), timeout);
    if( rc > 0 ) {
      int len = strlen(buf);
      if( len == 0 ) { 
        done = true;
      } else {
        lobj.headers[i++] = buf;
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

  rc = socket->doRead(buf2, lobj.content_length, 60);

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

  if( pos > 0 ) {
    lobj.rc = atoi(lobj.headers[0].substr(pos + 1, 3).c_str()); 
    //log_verbose("header rc %d %d\n", lobj.rc, tid);
  } 

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
      if( key == "content-length" ) {
        lobj.content_length = atoi(val.c_str());
      }
    }
  }
  return 0;
}

string
sendCommand( string req, string ip ) {

  log_verbose("sendCommand %s %s\n", req.c_str(), ip.c_str());
  if( req.size() && ip.size() ) {
    SocketIO bunny_sock;
    bunny_sock.useSSL = false;

    log_verbose("opening device\n");
    g_logLevel = 5;
    int fd = bunny_sock.openClient( ip, 80, false, false );

    LObj obj;
    string fetch, id;

    log_verbose("sendCommand fd %d, %s\n ", fd, req.c_str());

    if( fd >= 0 ) {
      fetch = "GET " +  req + " HTTP/1.0\n";
      fetch += "host: " + ip + "\n\n";
    
      log_verbose("using buf %s\n", fetch.c_str());
      bunny_sock.write(fetch.c_str(), fetch.size());
  
      readHeader( &bunny_sock, obj );
    }
    if( obj.packet.size() > 0 ) {
      log_verbose("have output packet %s\n", obj.packet.c_str());
      return obj.packet;
    }
  }
  return "failed";
}
