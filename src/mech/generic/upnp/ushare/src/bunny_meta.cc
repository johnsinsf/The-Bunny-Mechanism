/*
 * bunny_meta.c : bunny-uShare Web Server handler.
 * Copyright (C) 2025 John Sullivan <john@dwanta.com>
 *
 * original header:
 * metadata.c : GeeXboX uShare CDS Metadata DB.
 * Originally developped for the GeeXboX project.
 * Copyright (C) 2005-2007 Benjamin Zores <ben@geexbox.org>
 *
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "mime.h"
#include "metadata.h"
#include "bunny_meta.h"
#include "util_iconv.h"
#include "content.h"
#include "gettext.h"
#include "trace.h"
#include "Mech.h"
#include "LObj.h"

#define TITLE_UNKNOWN "unknown"

#define MAX_URL_SIZE 32
#define MAX_URLS 32000
#define MINCACHESIZE 256

extern int readHeader( SocketIO* socket, LObj& lobj );

struct upnp_entry_lookup_t {
  int id;
  struct upnp_entry_t *entry_ptr;
};

static char *
getExtension (const char *filename) {
  char *str = NULL;

  str = strrchr ((char*)filename, '.');
  if (str)
    str++;

  return str;
}

static struct mime_type_t *
getMimeType (const char *extension) {

  extern struct mime_type_t MIME_Type_List2[];
  struct mime_type_t *list;

  if (!extension)
    return NULL;

  list = &MIME_Type_List2[0];
  while (list->extension) {
    if (!strcasecmp (list->extension, extension))
      return list;
    list++;
  }
  return NULL;
}

static bool
is_valid_extension (const char *extension) {
  if (!extension)
    return false;

  if (getMimeType (extension))
    return true;

  return false;
}

static int
get_list_length (void **list) {
  void **l = list;
  int n = 0;

  while (*(l++))
    n++;

  return n;
}

static xml_convert_t xml_convert[] = {
  {'"' , "&quot;"},
  {'&' , "&amp;"},
  {'\'', "&apos;"},
  {'<' , "&lt;"},
  {'>' , "&gt;"},
  {'\n', "&#xA;"},
  {'\r', "&#xD;"},
  {'\t', "&#x9;"},
  {0, NULL},
};

static char *
get_xmlconvert (int c) {
  int j;
  for (j = 0; xml_convert[j].xml; j++) {
    if (c == xml_convert[j].charac)
      return xml_convert[j].xml;
  }
  return NULL;
}

static char *
convert_xml (const char *title) {
  char *newtitle, *s, *t, *xml;
  int nbconvert = 0;

  /* calculate extra size needed */
  for (t = (char*) title; *t; t++) {
    xml = get_xmlconvert (*t);
    if (xml)
      nbconvert += strlen (xml) - 1;
  }
  if (!nbconvert)
    return NULL;

  newtitle = s = (char*) malloc (strlen (title) + nbconvert + 1);

  for (t = (char*) title; *t; t++) {
    xml = get_xmlconvert (*t);
    if (xml) {
      strcpy (s, xml);
      s += strlen (xml);
    }
    else
      *s++ = *t;
  }
  *s = '\0';

  return newtitle;
}

static struct mime_type_t Container_MIME_Type =
  { NULL, "object.container.storageFolder", NULL};

static struct upnp_entry_t *
upnp_entry_new (struct ushare_t *ut, const char *name, const char *fullpath, const char* servername,
                struct upnp_entry_t *parent, off_t size, int dir) {
  struct upnp_entry_t *entry = NULL;
  char *title = NULL, *x = NULL;
  char url_tmp[MAX_URL_SIZE] = { '\0' };
  char *title_or_name = NULL;

  if (!name)
    return NULL;

  entry = (struct upnp_entry_t *) malloc (sizeof (struct upnp_entry_t));

#ifdef HAVE_DLNA
  entry->dlna_profile = NULL;
  entry->url = NULL;
  if (ut->dlna_enabled && fullpath && !dir) {
    dlna_profile_t *p = dlna_guess_media_profile (ut->dlna, fullpath);
    if (!p) {
      log_verbose( "guess media profile failed\n");
      free (entry);
      return NULL;
    }
    entry->dlna_profile = p;
  }
#endif /* HAVE_DLNA */
 
  if (ut->xbox360) {
    if (ut->root_entry)
      entry->id = ut->starting_id + ut->nr_entries++;
    else
      entry->id = 0; /* Creating the root node so don't use the usual IDs */
  }
  else
    entry->id = ut->starting_id + ut->nr_entries++;
  
  //if( ut->verbose )
    //log_verbose( "%d %d %d %d %s\n", entry->id, ut->root_entry, ut->nr_entries, ut->xbox360, servername);
  entry->fullpath = fullpath ? strdup (fullpath) : NULL;
  entry->parent = parent;
  entry->child_count =  dir ? 0 : -1;
  entry->title = NULL;
  if( servername != NULL )
    entry->servername = strdup( servername );

  entry->childs = (struct upnp_entry_t **)
    malloc (sizeof (struct upnp_entry_t *));
  *(entry->childs) = NULL;

  if (!dir) /* item */
    {
#ifdef HAVE_DLNA
      if (ut->dlna_enabled)
        entry->mime_type = NULL;
      else
      {
#endif /* HAVE_DLNA */
      struct mime_type_t *mime = getMimeType (getExtension (fullpath));
      if (!mime) {
        --ut->nr_entries; 
        upnp_entry_free (ut, entry);
        log_error ("Invalid Mime type for %s, entry ignored", fullpath);
        return NULL;
      }
      entry->mime_type = mime;
#ifdef HAVE_DLNA
      }
#endif /* HAVE_DLNA */
      
      if (snprintf (url_tmp, MAX_URL_SIZE, "%d.%s",
                    entry->id, getExtension (fullpath)) >= MAX_URL_SIZE)
        log_error ("URL string too long for id %d, truncated!!", entry->id);

      /* Only malloc() what we really need */
      entry->url = strdup (url_tmp);

    }
  else /* container */
    {
      entry->mime_type = &Container_MIME_Type;
      entry->url = NULL;
      if( fullpath )
        entry->url = strdup(fullpath);
    }

  /* Try Iconv'ing the name but if it fails the end device
     may still be able to handle it */
  title = iconv_convert (name);
  if (title)
    title_or_name = title;
  else {
    if (ut->override_iconv_err) {
      title_or_name = strdup (name);
      log_error ("Entry invalid name id=%d [%s]\n", entry->id, name);
    } else {
      upnp_entry_free (ut, entry);
      log_error ("Freeing entry invalid name id=%d [%s]\n", entry->id, name);
      return NULL;
    }
  }

  if (!dir) {
    x = strrchr (title_or_name, '.');
    if (x)  /* avoid displaying file extension */
      *x = '\0';
  }
  x = convert_xml (title_or_name);
  if (x) {
    free (title_or_name);
    title_or_name = x;
  }
  entry->title = title_or_name;

  if (!strcmp (title_or_name, "")) /* DIDL dc:title can't be empty */ {
    free (title_or_name);
    entry->title = strdup (TITLE_UNKNOWN);
  }

  entry->size = size;
  entry->fd = -1;

  if( ut->verbose ) {
    if (entry->id && entry->url) {
      log_verbose ("[%d] %s,%s\n", entry->id, entry->title, entry->url);
    } else {
      log_verbose ("[%d] container %s\n", entry->id, entry->title);
    }
  }
  return entry;
}

static void
upnp_entry_add_child (struct ushare_t *ut,
                      struct upnp_entry_t *entry, struct upnp_entry_t *child) {
  struct upnp_entry_lookup_t *entry_lookup_ptr = NULL;
  struct upnp_entry_t **childs;
  int n;

  if (!entry || !child)
    return;

  for (childs = entry->childs; *childs; childs++)
    if (*childs == child)
      return;

  n = get_list_length ((void **) entry->childs) + 1;
  entry->childs = (struct upnp_entry_t **)
    realloc (entry->childs, (n + 1) * sizeof (*(entry->childs)));
  entry->childs[n] = NULL;
  entry->childs[n - 1] = child;
  entry->child_count++;

  entry_lookup_ptr = (struct upnp_entry_lookup_t *)
    malloc (sizeof (struct upnp_entry_lookup_t));
  entry_lookup_ptr->id = child->id;
  entry_lookup_ptr->entry_ptr = child;

  if (rbsearch ((void *) entry_lookup_ptr, ut->rb) == NULL)
    log_info (_("Failed to add the RB lookup tree\n"));
}

struct upnp_entry_t *
bunny_upnp_get_entry (struct ushare_t *ut, int id)
{
  struct upnp_entry_lookup_t *res, entry_lookup;

  log_verbose ("Bunny Looking for entry id %d\n", id);
  if (id == 0) // We do not store the root (id 0) as it is not a child 
    return ut->root_entry;

  entry_lookup.id = id;
  res = (struct upnp_entry_lookup_t *)
    rbfind ((void *) &entry_lookup, ut->rb);

  if (res) {
    log_verbose ("Found at %p %d %d fullpath %s server %s title %s url %s\n",
                 ((struct upnp_entry_lookup_t *) res)->entry_ptr,
                 ((struct upnp_entry_lookup_t *) res)->id,
                 ((struct upnp_entry_lookup_t *) res)->entry_ptr->id,
                 ((struct upnp_entry_lookup_t *) res)->entry_ptr->fullpath,
                 ((struct upnp_entry_lookup_t *) res)->entry_ptr->servername,
                 ((struct upnp_entry_lookup_t *) res)->entry_ptr->title,
                 ((struct upnp_entry_lookup_t *) res)->entry_ptr->url
    );
    if( (((struct upnp_entry_lookup_t *) res)->entry_ptr->mime_type == &Container_MIME_Type) && 
         ((struct upnp_entry_lookup_t *) res)->entry_ptr->child_count <= 0 ) {

      log_verbose("getting XML bunny list \n" );
/*
      char *xmltext2 = "\
<media>\
  <header>\
    <filetype>bunny-ushare</filetype>\
    <version>000.000.001</version>\
    <files>\
      <filetype>file</filetype>\
      <filename>file1</filename>\
      <filesize>11202033</filesize>\
      <bunnyname>xHT8143oZusSPU6Q9MQlg.m4a</bunnyname>\
      <bunnyserver>buuna.dwanta.com</bunnyserver>\
    </files>\
    <files>\
      <filetype>file</filetype>\
      <filename>file2</filename>\
      <filesize>5665382</filesize>\
      <bunnyname>QkdNbxawS3OJAkSx5Eiw4.m4a</bunnyname>\
      <bunnyserver>buuna.dwanta.com</bunnyserver>\
    </files>\
    <files>\
      <filetype>file</filetype>\
      <filename>file3</filename>\
      <filesize>232789050</filesize>\
      <bunnyname>HN0WsOIupcuCJjoFgUvsz.flac</bunnyname>\
      <bunnyserver>buuna.dwanta.com</bunnyserver>\
    </files>\
    <files>\
      <filetype>file</filetype>\
      <filename>file4</filename>\
      <filesize>257695138</filesize>\
      <bunnyname>LnNwr1CuzsgOL8p1MvOqA.flac</bunnyname>\
      <bunnyserver>buuna.dwanta.com</bunnyserver>\
    </files>\
  </header>\
</media>";

      //ut->xml_doc = ixmlParseBuffer(xmltext2);
      int rc = ixmlParseBufferEx(xmltext2, &ut->xml_doc);
      log_verbose("ixml rc is %d\n", rc);
*/
      bool cacheOK = false;

      if( ut->use_cache ) {
        string filename = INSTALLDIR + string("/cache/") + string(((struct upnp_entry_lookup_t *) res)->entry_ptr->fullpath);
        log_info("trying cache %s\n", filename.c_str());
        int fd2 = open(filename.c_str(), O_RDONLY);
        struct stat st;
        int rc = fstat(fd2, &st);
        if( ( rc == 0 ) && st.st_size > 0 && st.st_mtime > time(NULL) - 24*60*60 ) {
          char buf[st.st_size + 1];
          memset(buf, 0, sizeof(buf));
          rc = read(fd2, buf, sizeof(buf));
          log_info("have cache read rc %d\n", rc);
          close(fd2);
          rc = ixmlParseBufferEx(buf, &ut->xml_doc);
          log_info("cache ixml rc is %d\n", rc);
          if( rc == 0 )
            cacheOK = true;
        } 
      }
      if( ! cacheOK ) {
        SocketIO bunny_sock;
        bunny_sock.useSSL = true;
  
        string bunny_server = ((struct upnp_entry_lookup_t *) res)->entry_ptr->servername;
        int bunny_server_port = 443;
  
        int fd = bunny_sock.openClient( bunny_server, bunny_server_port );
  
        log_verbose("have fd %d\n", fd);
  
        if( fd >= 0 ) {
          string test;
          test = "GET /echo/bunnyman/" + string(((struct upnp_entry_lookup_t *) res)->entry_ptr->fullpath) + "?hop=30 HTTP/1.0\n";
          test += "host: buuna.dwanta.com\n\n";
    
          log_verbose("using buf %s \n", test.c_str());
          bunny_sock.write(test.c_str(), test.size());
  
          LObj obj;
  
          readHeader( &bunny_sock, obj );
  
          int rc = ixmlParseBufferEx(obj.packet.c_str(), &ut->xml_doc);
          log_verbose("ixml rc is %d\n", rc);
  
          bunny_sock.doClose();
  
          if( ut->use_cache && obj.packet.size() > MINCACHESIZE ) {
            string filename = INSTALLDIR + string("/cache/") + string(((struct upnp_entry_lookup_t *) res)->entry_ptr->fullpath);
            log_verbose("writing to cache %s\n", filename.c_str());
            int fd2 = open(filename.c_str(), O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU);
            if( fd2 >= 0 ) { 
              rc = write(fd2, obj.packet.c_str(), obj.packet.size());
              printf("have cache write rc %d\n", rc);
              close(fd2);
            }
          }
        }
      }
      metadata_add_container (ut, ((struct upnp_entry_lookup_t *) res)->entry_ptr, ((struct upnp_entry_lookup_t *) res)->entry_ptr->title);

      return ((struct upnp_entry_lookup_t *) res)->entry_ptr;
    }
    return ((struct upnp_entry_lookup_t *) res)->entry_ptr;
  }
  log_verbose ("Not Found\n");

  return NULL;
}

static void
metadata_add_file (struct ushare_t *ut, struct upnp_entry_t *entry,
                   const char *file, const char *name, const char *fsize ) {
  if (!entry || !file || !name) {
    log_verbose ("no entry, file, name\n");
    return;
  }
  if( ut->verbose )
    log_verbose ("filesize: %s\n", fsize);

#ifdef HAVE_DLNA
  if (ut->dlna_enabled || is_valid_extension (getExtension (file)))
#else
  if (is_valid_extension (getExtension (file)))
#endif
  {
    struct upnp_entry_t *child = NULL;

    long int i = atol(fsize);
    if( i <= 0 ) i = 1;
    child = upnp_entry_new (ut, name, file, entry->servername, entry, i, false);
    if (child)
      upnp_entry_add_child (ut, entry, child);
  } else {
    log_verbose ("bad extension: %s\n", file);
  }
}

static void
metadata_add_container (struct ushare_t *ut,
                        struct upnp_entry_t *entry, const char *container) {
  typedef struct {
    char* d_name;
    char* d_size;
    char* d_alias;
    char* d_server;
    int   d_type;
  } namelist_type;

  namelist_type* namelist[MAX_URLS] = { NULL, NULL, NULL, 0 };

  int n,i;

  if (!entry || !container) {
    log_error (_("bad entry or container\n"));
    return;
  }

  log_verbose ("Scanning directory %s, %d\n", container, ut->root_entry);
  n = 0;
  i = 0;

  if ( !ut->xml_doc ) {
    log_verbose("bad request\n");
  } else {
    if( ut->verbose )
      log_verbose("XML request OK\n");

    IXML_Node* node = (IXML_Node *) ut->xml_doc;

    if (!node) {
      log_verbose ("Invalid action request document\n");
      return;
    }
    node = ixmlNode_getFirstChild (node);
    if (!node) {
      log_verbose ("Invalid action request document\n");
      return;
    }
    if( ut->verbose )
      log_verbose("first node is %s - %s\n", ixmlNode_getNodeName (node), ixmlNode_getNodeValue(node) );

    node = ixmlNode_getFirstChild (node);

    if( ut->verbose )
      log_verbose("XML node OK %s -  %s\n", ixmlNode_getNodeName (node), ixmlNode_getNodeValue(node) );

    node = ixmlNode_getFirstChild (node);

    if( ut->verbose )
      log_verbose("XML node OK %s - %s \n", ixmlNode_getNodeName (node), ixmlNode_getNodeValue(node) );

    char* t_name = NULL;
    char* t_size = NULL;
    char* t_alias = NULL;
    char* t_server = NULL;
    int   t_type = 0;

    for (; node; node = ixmlNode_getNextSibling (node)) {
      log_verbose("node is %s -  %s \n", ixmlNode_getNodeName (node), ixmlNode_getNodeValue(node) );
      if( strcmp( ixmlNode_getNodeName (node), "filetype" ) == 0 ) {
        IXML_Node* node2 = ixmlNode_getFirstChild (node);
        if (!node2) {
          log_verbose("empty\n");
        } else {
          //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
        }
        if( strcmp( ixmlNode_getNodeValue(node2), "directory" ) == 0 ) {
          t_type = 1;
        } 
        else
        if( strcmp( ixmlNode_getNodeValue(node2), "file" ) == 0 ) {
          t_type = 2;
        } 
      } 
      else
      if( strcmp( ixmlNode_getNodeName (node), "filename" ) == 0 ) {
        IXML_Node* node2 = ixmlNode_getFirstChild (node);
        if (!node2) {
          log_verbose("empty\n");
        } else {
          //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
          t_name = strdup( ixmlNode_getNodeValue (node2) );
        }
      }
      else
      if( strcmp( ixmlNode_getNodeName (node), "filesize" ) == 0 ) {
        IXML_Node* node2 = ixmlNode_getFirstChild (node);
        if (!node2) {
          log_verbose("empty\n");
        } else {
          //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
          t_size = strdup( ixmlNode_getNodeValue (node2) );
        }
      }
      else
      if( strcmp( ixmlNode_getNodeName (node), "files" ) != 0 ) {
        IXML_Node* node2 = ixmlNode_getFirstChild (node);
        if (!node2) {
          log_verbose("empty\n");
        } else {
          //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
        }
      } else {
        IXML_Node* node_top = node;
        //while( node_top ) {
        {
          node = ixmlNode_getFirstChild (node);
          for (; node; node = ixmlNode_getNextSibling (node)) {
            //log_verbose("2 node is %s - %s \n", ixmlNode_getNodeName (node), ixmlNode_getNodeValue(node) );
            if( strcmp( ixmlNode_getNodeName (node), "filetype" ) == 0 ) {
              IXML_Node* node2 = ixmlNode_getFirstChild (node);
              if (!node2) {
                log_verbose("empty\n");
              } else {
                //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
              }
              if( strcmp( ixmlNode_getNodeValue(node2), "directory" ) == 0 ) {
                t_type = 1;
              } 
              else
              if( strcmp( ixmlNode_getNodeValue(node2), "file" ) == 0 ) {
                t_type = 2;
              } 
            } 
            else
            if( strcmp( ixmlNode_getNodeName (node), "filename" ) == 0 ) {
              IXML_Node* node2 = ixmlNode_getFirstChild (node);
              if (!node2) {
                log_verbose("empty\n");
              } else {
                //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
                t_name = strdup( ixmlNode_getNodeValue (node2) );
              }
            }
            else
            if( strcmp( ixmlNode_getNodeName (node), "filesize" ) == 0 ) {
              IXML_Node* node2 = ixmlNode_getFirstChild (node);
              if (!node2) {
                log_verbose("empty\n");
              } else {
                //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
                t_size = strdup( ixmlNode_getNodeValue (node2) );
              }
            }
            else
            if( strcmp( ixmlNode_getNodeName (node), "bunnyname" ) == 0 ) {
              IXML_Node* node2 = ixmlNode_getFirstChild (node);
              if (!node2) {
                log_verbose("empty\n");
              } else {
                //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
                t_alias = strdup( ixmlNode_getNodeValue (node2) );
              }
            }
            else
            if( strcmp( ixmlNode_getNodeName (node), "bunnyserver" ) == 0 ) {
              IXML_Node* node2 = ixmlNode_getFirstChild (node);
              if (!node2) {
                log_verbose("empty\n");
              } else {
                //log_verbose("node value %s\n", ixmlNode_getNodeValue (node2) );
                t_server = strdup( ixmlNode_getNodeValue (node2) );
              }
            }
          }
        }
        node = node_top;
        if( ut->verbose )
          log_verbose("set node to node_top\n");
      }
      if( t_name && n < MAX_URLS ) {
        namelist[n] = (namelist_type*) malloc( sizeof(namelist_type) );
        if( t_name )
          namelist[n]->d_name = strdup(t_name);
        else
          namelist[n]->d_name = NULL;
        if( t_size )
          namelist[n]->d_size = strdup(t_size);
        else
          namelist[n]->d_size = NULL;
        if( t_alias )
          namelist[n]->d_alias = strdup(t_alias);
        else
          namelist[n]->d_alias = NULL;
        namelist[n]->d_type = t_type;
        if( t_server )
          namelist[n]->d_server = strdup(t_server);
        else
          namelist[n]->d_server = NULL;
        n++;
        if( ut->verbose )
          log_verbose("added %s %s %s\n", t_name, t_alias, t_server);
        //sleep(10);
        free(t_name);
        free(t_size);
        free(t_alias);
        free(t_server);
        t_name = NULL;
        t_size = NULL;
        t_alias = NULL;
        t_server = NULL;
        t_type = 0;
      }
    }
  }
  for( i = 0; i < n; i++ ) {
    char *fullpath = NULL;
    fullpath = (char *) malloc (strlen (container) + strlen (namelist[i]->d_name) + 2);

    sprintf (fullpath, "%s%s", container, namelist[i]->d_name);
    if( ut->verbose )
      log_verbose ("fullpath: %s %s %s\n", fullpath, namelist[i]->d_alias, namelist[i]->d_server);

    if ( namelist[i]->d_type == 1 ) { // a directory 
      struct upnp_entry_t *child = NULL;

      child = upnp_entry_new (ut, namelist[i]->d_name, namelist[i]->d_alias, namelist[i]->d_server, entry, 0, true);

      if (child) {
        if( ut->verbose ) {
          log_verbose ("add child %d %s %s\n", child->id, namelist[i]->d_name, namelist[i]->d_alias);
        }
        upnp_entry_add_child (ut, entry, child);
      }
    }
    else {
      if( ut->verbose )
        log_verbose ("add file %s %s, %d\n", namelist[i]->d_name, namelist[i]->d_alias, entry->id);
      metadata_add_file (ut, entry, namelist[i]->d_alias, namelist[i]->d_name, namelist[i]->d_size );
    }

    if( namelist[i] )
      free (namelist[i]);
    if( fullpath )
      free (fullpath);
    namelist[i] = NULL;
    fullpath = NULL;
  }
  if( namelist[0] )
    free (namelist[0]);
}

void
free_bunny_metadata_list (struct ushare_t *ut) {
  ut->init = false;
  if (ut->root_entry)
    upnp_entry_free (ut, ut->root_entry);
  ut->root_entry = NULL;
  ut->nr_entries = 0;

  if (ut->rb) {
    rbdestroy (ut->rb);
    ut->rb = NULL;
  }

  ut->rb = rbinit (bunny_rb_compare, NULL);
  if (!ut->rb)
    log_error (_("Cannot create RB tree for lookups\n"));
}

void
build_bunny_metadata_list (struct ushare_t *ut) {
  int i;
  log_info (_("Building Bunny Metadata List ...\n"));

  /* build root entry */
  if (!ut->root_entry) {
    ut->root_entry = upnp_entry_new (ut, "root", NULL, "", NULL, -1, true);
    log_verbose("new root id: %d\n", ut->root_entry->id);
  }

  map<string,string> configMap;
  configMap = ut->dssObj->server->getConfigMap( );
  string bunny_user, bunny_server; 
  int bunny_server_port = 443;

  if( configMap.size() > 0 ) {
    map<string,string>::iterator I = configMap.find("bunny_user");
    if( I != configMap.end() ) {
      log_verbose("have bunny user %s\n", I->second.c_str());
      bunny_user = I->second;
    }
    I = configMap.find("bunny_server");
    if( I != configMap.end() ) {
      log_verbose("have bunny server %s\n", I->second.c_str());
      bunny_server = I->second;
    }
    I = configMap.find("bunny_server_port");
    if( I != configMap.end() ) {
      log_verbose("have bunny server port %s\n", I->second.c_str());
      bunny_server_port = atoi(I->second.c_str());
    }
  }
  bool cacheOK = false;

  if( ut->use_cache ) {
    string filename = INSTALLDIR + string("/cache/") + bunny_user;
    log_info ("reading bunny user cache %s\n", filename.c_str());
    int fd2 = open(filename.c_str(), O_RDONLY);
    struct stat st;
    int rc = fstat(fd2, &st);
    if( ( rc == 0 ) && st.st_size > 0 && st.st_mtime > time(NULL) - 24*60*60 ) {
      char buf[st.st_size + 1];
      memset(buf, 0, sizeof(buf));
      rc = read(fd2, buf, sizeof(buf));
      log_info("have cache read rc %d\n", rc);
      close(fd2);
      rc = ixmlParseBufferEx(buf, &ut->xml_doc);
      log_info("cache ixml rc is %d\n", rc);
      if( rc == 0 )
        cacheOK = true;
    } 
  }
  if( ! cacheOK ) {
    SocketIO bunny_sock;
    bunny_sock.useSSL = true;

    int fd = bunny_sock.openClient( bunny_server, bunny_server_port );

    log_verbose("have fd %d\n", fd);

    if( fd >= 0 ) {
      string test;
      test = "GET /echo/bunnyman/" + bunny_user + "?hop=30 HTTP/1.0\n";
      test += "host: buuna.dwanta.com\n\n";
  
      log_verbose("using buf %s \n", test.c_str());
      bunny_sock.write(test.c_str(), test.size());

      LObj obj;

      readHeader( &bunny_sock, obj );

      bunny_sock.doClose();

      //log_verbose("have %s\n", obj.packet.c_str());
      int rc = ixmlParseBufferEx(obj.packet.c_str(), &ut->xml_doc);
      log_info("ixml rc is %d\n", rc);

      if( ut->use_cache && obj.packet.size() > MINCACHESIZE ) {
        string filename = INSTALLDIR + string("/cache/") + bunny_user;
        log_info("writing bunny user cache %s\n", filename.c_str()); 
        int fd2 = open(filename.c_str(), O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU);
        if( fd2 >= 0 ) { 
          rc = write(fd2, obj.packet.c_str(), obj.packet.size());
          log_info("have write rc %d\n", rc);
          close(fd2);
        }
      }
    }
  }
/*
  char* xmltext = "\
<media>\
  <header>\
    <filetype>bunny-ushare</filetype>\
    <version>000.000.001</version>\
    <files>\
      <filetype>directory</filetype>\
      <filename>dir1</filename>\
      <filesize>0</filesize>\
      <bunnyname>e9WBvPedXB2R1teW6qfM0</bunnyname>\
      <bunnyserver>buuna.dwanta.com</bunnyserver>\
    </files>\
  </header>\
</media>\
";
  ut->xml_doc = ixmlParseBuffer(xmltext);
*/

  /* content set from config should be bunnyname/title */
  /* load each contentlist content separately on each loop */

  /* add files from content directory */
  for (i=0 ; i < ut->contentlist->count ; i++) {
    struct upnp_entry_t *entry = NULL;
    char *title = NULL;
    int size = 0;

    log_info (_("Looking for files in content directory : %s\n"),
              ut->contentlist->content[i]);

    size = strlen (ut->contentlist->content[i]);
    if (ut->contentlist->content[i][size - 1] == '/')
      ut->contentlist->content[i][size - 1] = '\0';
    title = strrchr (ut->contentlist->content[i], '/');
    if (title)
      title++;
    else {
      /* directly use content directory name if no '/' before basename */
      title = ut->contentlist->content[i];
    }

    entry = upnp_entry_new (ut, title, ut->contentlist->content[i], "", ut->root_entry, -1, true);

    if (!entry) {
      log_error(_("bad entry %s %s...\n"), title, ut->contentlist->content[i]);
      continue;
    } else{
      log_error(_("entry OK %s %s...\n"), title, ut->contentlist->content[i]);
    }
    upnp_entry_add_child (ut, ut->root_entry, entry);
    metadata_add_container (ut, entry, ut->contentlist->content[i]);
  }

  log_info (_("Found %d files and subdirectories.\n"), ut->nr_entries);
  ut->init = true;

/*
  // just some testing here
  for( int ii = 100000; ii < 100003; ii++ ) {

    struct upnp_entry_lookup_t *res, entry_lookup;
  
    log_verbose ("Bunny Looking for entry id %d\n", ii);
    if (ii == 0) {
      log_verbose ("Root at %d %s %s %s %d\n",
                   ut->root_entry->id,
                   ut->root_entry->fullpath,
                   ut->root_entry->title,
                   ut->root_entry->url,
                   ut->root_entry->child_count
      );
    } else {
      entry_lookup.id = ii;
      res = (struct upnp_entry_lookup_t *)
        rbfind ((void *) &entry_lookup, ut->rb);
  
      if (res) {
        log_verbose ("Found at %p %d %d %s %s %s %s %d\n",
                   ((struct upnp_entry_lookup_t *) res)->entry_ptr,
                   ((struct upnp_entry_lookup_t *) res)->id,
                   ((struct upnp_entry_lookup_t *) res)->entry_ptr->id,
                   ((struct upnp_entry_lookup_t *) res)->entry_ptr->fullpath,
                   ((struct upnp_entry_lookup_t *) res)->entry_ptr->title,
                   ((struct upnp_entry_lookup_t *) res)->entry_ptr->url,
                   ((struct upnp_entry_lookup_t *) res)->entry_ptr->servername,
                   ((struct upnp_entry_lookup_t *) res)->entry_ptr->child_count
        );
      }
    }
  }
*/
}

int
bunny_rb_compare (const void *pa, const void *pb,
            const void *config __attribute__ ((unused))) {
  struct upnp_entry_lookup_t *a, *b;

  a = (struct upnp_entry_lookup_t *) pa;
  b = (struct upnp_entry_lookup_t *) pb;

  if (a->id < b->id)
    return -1;

  if (a->id > b->id)
    return 1;

  return 0;
}
