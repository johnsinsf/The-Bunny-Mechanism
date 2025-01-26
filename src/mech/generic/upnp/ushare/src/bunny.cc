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
#include "dpsio/SocketIO.h"

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
    string bunny_server = entry->servername;
    int bunny_server_port = 443;
   
    int fd = bunny_sock.openClient( bunny_server, bunny_server_port );
   
    log_verbose("have fd %d\n", fd);

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
      fetch = "GET /echo/bunnyman/" + id + "?hop=30 HTTP/1.0\n";
      fetch += "host: buuna.dwanta.com\n\n";
    
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

  file->detail.local.fd = file->detail.local.bunny_sock->openClient( bunny_server, bunny_server_port );

  pid_t tid;
  tid = syscall(SYS_gettid);

  log_verbose("server connect %d %d %s\n", tid, file->detail.local.fd, bunny_server.c_str() );

  return file->detail.local.fd;
}

int 
bunny_read (UpnpWebFileHandle fh, char *buf, size_t buflen,
               const void* cookie __attribute__((unused)),
               const void* requestCookie __attribute__((unused))) {
  struct web_file_t *file = (struct web_file_t *) fh;
  ssize_t len = -1;

  pid_t tid;
  tid = syscall(SYS_gettid);

  log_verbose ("bunny_read %d %lu file=%p entry=%p\n", tid, buflen, file, file->detail.local.entry);

  if (!file)
    return -1;

  if (file->type == FILE_MEMORY ) {
    log_verbose ("Reading file from memory.\n");
    len = (size_t) MIN (buflen, file->detail.memory.len - file->pos);
    memcpy (buf, file->detail.memory.contents + file->pos, (size_t) len);
    return len;
  }

  log_verbose ("bunny_fd is %d %d\n", file->detail.local.fd, tid );

  if (file->detail.local.fd <= 0 ) {
    bunny_server_connect(fh);
    log_verbose ("bunny_fd is now %d %d\n", file->detail.local.fd, tid);
    if (file->detail.local.fd <= 0) {
      log_verbose("bad bunny_server_connect\n");
      return 0;
    }
  }
  string test;
  test = "GET /echo/bunnycloud/" + string(file->fullpath) + " HTTP/1.0\n";
  if( file->detail.local.entry->servername != NULL )
    test += "host: " + string(file->detail.local.entry->servername) + "\n";
  else
    test += "host: buuna.dwanta.com\n";

  test += "range: bytes=" + to_string(file->pos) + "-" + to_string(buflen) + "\n\n";
  
  log_verbose("sending %s %d\n", test.c_str(), tid);

  file->detail.local.bunny_sock->write(test.c_str(), test.size());

  LObj obj;

  readHeader( file->detail.local.bunny_sock, obj );

  int i = buflen;
  if( obj.packet.size() != i ) 
    i = obj.packet.size();

  if( i>= 0 )
    file->pos += i;

  log_verbose ("Read %d bytes %d.\n", i, tid);

  if( i > 0 )
    memcpy( buf, obj.packet.c_str(), i );
  else {
    buf[0] = 0;
    return -1;
  }

  log_verbose ("Read %zd data bytes %d, file pos is %lld.\n", obj.packet.size(), tid, file->pos);

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
    log_verbose ("Unknown file type %d.\n", file->type);
    break;
  }

  if (file->fullpath)
    free (file->fullpath);
  free (file);

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
    log_verbose("about to read, timeout=%d %d %d\n", timeout, tid, socket->useSSL);
    memset( buf, 0, sizeof(buf) );
    rc = socket->doReadLine(buf, sizeof(buf), timeout);
    if( rc > 0 ) {
      int len = strlen(buf);
      log_verbose("readheader buf %s %d %d %d\n", buf, rc, len, tid);
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
  log_verbose("read content length %d %d %d\n", lobj.content_length, tid, socket->useSSL);

  rc = socket->doRead(buf2, lobj.content_length, 60);
  log_verbose("read buf2 %d %d\n", rc, tid);

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
  log_verbose("header pos is %d\n", pos);
  if( pos > 0 ) {
    lobj.rc = atoi(lobj.headers[0].substr(pos + 1, 3).c_str()); 
    log_verbose("header rc %d %d\n", lobj.rc, tid);
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
      log_verbose("header test %s %s %d\n", key.c_str(), val.c_str(), tid);
      if( key == "content-length" ) {
        lobj.content_length = atoi(val.c_str());
      }
    }
  }
  log_verbose("header test done\n");
  return 0;
}
