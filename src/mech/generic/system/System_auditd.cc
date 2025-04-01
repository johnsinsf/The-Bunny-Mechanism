/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "System_auditd.h"
/*
#include <sys/time.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
*/

int 
System_auditd::main( DssObject& o ) {
  int nl_sock;
  int rc = -1;
  
  logger.error("System_auditd starting");

  nl_sock = audisp_connect();

  logger.error("System_auditd sock " + itoa(nl_sock));

  semRelease( o.semInitID );

  if( nl_sock > -1 ) {
    load_defines();
    rc = run( nl_sock, o );
    close(nl_sock);
  }
  logger.error("System_auditd done");

  return rc;
}

int 
System_auditd::audisp_connect() {
  #define MY_SOCK_PATH "/run/audispd_events"
  //#define MY_SOCK_PATH "/run/audispd_events"
  //#define MY_SOCK_PATH "/usr/local/run/audispd_events"

  int sfd;
  struct sockaddr_un my_addr;

  sfd = socket( AF_UNIX, SOCK_STREAM, 0 );
  if (sfd == -1) {
    logger.error("unable to open audisp socket " + itoa(errno));
    return -1;
  }

  memset(&my_addr, '\0', sizeof(struct sockaddr_un));
  my_addr.sun_family = AF_UNIX;

  strncpy(my_addr.sun_path, MY_SOCK_PATH, sizeof(my_addr.sun_path) - 1);

  if( connect( sfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un ) ) == -1 ) {
    int e = errno;
    logger.error("bad audispd_events connect: " + itoa(e));
    return -1;
  }
  return sfd;
}

int 
System_auditd::run( int nl_sock, DssObject& o ) {
  int rc = -1;
  bool done = false;

  #define BUFSIZE 16384

  int size = BUFSIZE;
  if( o.audit_mode == F_BINARY )
    size = 8986;

  char buf[size];
  string out;

  logger.error("System_auditd running");

  while( ! done && ! g_quit ) {
    memset(buf, 0, sizeof(buf));
    if( g_logLevel > 2 )
      logger.error("System_auditd recv");
    rc = recv( nl_sock, &buf, sizeof(buf), 0 );
    if( g_logLevel > 2 )
      logger.error("System_auditd recv " + itoa(rc));
    if( rc > 0 ) {
      if( rc != size ) {
        out += string(buf,rc);
        if( o.audit_mode == F_BINARY )  {
          if( g_logLevel > 20 )
            logger.error("have size " + to_string(out.size()) );
          parse_buf_binary( out, o );
        } else {
          parse_buf( out, o );
        }
        out = "";
      } else {
        if( o.audit_mode == F_BINARY )  {
          out = string(buf,size);
          if( g_logLevel > 20 )
            logger.error("have size " + to_string(out.size()) );
          parse_buf_binary( out, o );
        } else {
          out += string(buf,rc);
        }
      }
    }
    else
    if( rc == 0 ) {
      logger.error("audisp recv eof");
      done = true;
    } 
    else 
    if (rc <= -1) {
      if (errno != EINTR) {
        logger.error("audisp recv error " + itoa(errno));
        rc = -1;
        done = true;
      }
      else {
        logger.error("audisp interrupted");
        done = true;
      }
    }
  }
  logger.error("System_auditd done");
  return rc;
}

// loop datavals and create clientData entries

void
System_auditd::send_data( DssObject& o, int msgid ) {
  dataType d;
  d.structtype = 0;
  d.datatype   = 4;
  d.localid    = o.localID;
  d.channel    = "AUDITD";
  d.synced     = 0;
  d.tries      = 0;
  d.datetime_sent = 0;

  map<string,string>::iterator I;
  map<int, map<string,string> >::iterator I1;

  int i = 0;
  string out, index;

  I1 = datavals.find(msgid);

  if( g_logLevel > 0 )
    logger.error("have msg " + itoa(msgid));

  for( I = I1->second.begin(); I != I1->second.end(); ++I ) { 
    //logger.error("have " + I->first + " " + I->second);
    if( I->first == "msg" ) {
       // skip
    }
    else
    if( I->first == "saddr" ) {
       // skip
    }
    else
    if( I->first == "msgtime" ) {
      d.msgtime = I->second;
    }
    else
    if( I->first == "msgid" ) {
      d.msgid = I->second;
    }
    else
    if( I->first == "node" ) {
      d.hostname = I->second;
    } else {

      bool skipVal = false;

      if( I->first == "title" )
        if( datavals[msgid]["titlex"] > "" )
          skipVal = true;

      if( I->first == "syscall" )
        if( datavals[msgid]["syscallx"] > "" )
          skipVal = true;

      if( ! skipVal ) {
        if( i++ > 0 ) out += " ";    
        out += I->first + "=" + I->second;
      } 
      if( I->first == "exe" || I->first == "family" || I->first == "syscall" || I->first == "syscallx" || 
          I->first == "user" || I->first == "port" || I->first == "ip" || I->first == "file" ) {
        index += I->second;
      } 
    } 
  }
  if( index.size() )
    replaceAll(index, " ", "-");
  else
    index = "none";

  if( has_binary_data( out.c_str(), out.size() ) ) {
    if( g_logLevel > 2 )
      logger.error("skipping binary data");
    char buf[out.size() + 1];
    memset(buf, 0, sizeof(buf));
    memcpy( buf, out.c_str(), out.size());
    remove_binary_data( (char*)buf, out.size() ); 
    out = buf;
  }

  d.data = index + "\t" + out;
 
  o.server->add_client_data( o, d );
  
  datavals[msgid].clear();

  return;
}

void 
System_auditd::parse_buf_binary( string buf, DssObject& o ) {
  if( g_logLevel > 20 )
    logger.error("parse binary size2 " + to_string(buf.size())  );
  string msg;
/*
  for( int x = 0; x < 1000; x+=80 ) {
    for( int i = x; i < 80 + x; i++ ) {
      msg +=   to_string(buf[i]) + ":";
      if( buf[i] == 0 )  {
        i = 1001; x = 1001;
      }
    }
  }
*/
  if( g_logLevel > 20 ) {
    for( int i = 0; i < 80; i++ ) {
      msg +=   to_string(buf[i]) + ":";
    }
    logger.error("msg " + msg);

    unsigned int i1;

    memcpy(&i1, (char*)(buf.substr(0, 4).c_str()), 4 );
    logger.error("have i0 " + itoa(i1));

    memcpy(&i1, buf.substr(4, 4).c_str(), 4 );
    logger.error("have i1 " + itoa(i1));

    memcpy(&i1, buf.substr(8, 4).c_str(), 4 );
    logger.error("have i2 " + itoa(i1));

    memcpy(&i1, buf.substr(12, 4).c_str(), 4 );
    logger.error("have i3 " + itoa(i1));
  }

  bool done = false;
  int x = 16;
  string t1, buf2;

  while( ! done && x < 200 ) {
    char a;
    memcpy( &a, (char*)(buf.substr(x, 1).c_str()), 1 );
    //logger.error("have a " + to_string(a));
    if( a == 32 ) {
      if( g_logLevel > 20 )
        logger.error("have cr string " + t1);
      if( buf2.size() ) {
        buf2 += " ";
      }
      if( t1.substr(0,4) == "msg=" ) {
        if( t1.substr(t1.size() - 1, 1) == ":" ) {
          t1 = t1.substr(0, (t1.size() - 1) )  + " ";
        }
      }
      buf2 += t1;
      t1 = "";
    }
    else
    if( a == 0 ) {
      t1 = "";
      done = true;
    } else {
      t1 += buf.substr(x,1);
    }
    x++;
  }
  if( buf2.size() ) {
    if( g_logLevel > 20 )
      logger.error("have buf " + buf2);
    return parse_buf( buf2, o );
  }
  return;
}

void 
System_auditd::parse_buf( string buf, DssObject& o ) {
  int k = 0;
  int i = buf.find("\n");
  int msgid = 0;
  bool eoeFound = false;

  if( g_logLevel > 0 )
    logger.error("System_auditd parse " + buf + " i " + itoa(i) + " curr " + itoa(currentmsgid));

  if( i < 0 ) {
    parse_msgid( buf, o, msgid, eoeFound );
    if( currentmsgid == 0 ) currentmsgid = msgid;
    if( eoeFound ) {
      send_data ( o, msgid );
      currentmsgid = msgid;
    } else {
      parse_buf2( buf, msgid );
    }
  }
  else
  while( i > 0 ) {
    string buf2 = buf.substr(k, i - k);
    parse_msgid( buf2, o, msgid, eoeFound );
    if( g_logLevel > 0 )
      logger.error("msgid " + itoa(msgid) + " " + itoa(eoeFound) + " " + itoa(currentmsgid));
    if( currentmsgid == 0 ) currentmsgid = msgid;
    if( eoeFound ) {
      send_data ( o, msgid );
      currentmsgid = msgid;
    } else {
      parse_buf2( buf2, msgid );
    }
    k = i + 1;
    i = buf.find("\n", k );
  }
}

int
System_auditd::parse_msgid( string buf, DssObject& o, int& msgid, bool& eoeFound ) {
  int rc = -1;
  int i = buf.find("msg=");
  if( i >= 0 ) {
    int e = buf.find(" ", i + 1);
    if( e > 0 ) {
      string msg = buf.substr( i + 4, e - i - 4);
      if( msg.substr(0,6) == "audit(" ) {
        int i = msg.find(":",6);
        //logger.error("have i " + to_string(i));
        if( i > 0 ) {
          int j = msg.find(")",i);
          //logger.error("have j " + to_string(j));
          if( j > 0 ) {
            rc = atoi( msg.substr(i+1,j-i-1).c_str() );
            msgid = rc;
          }
        }
      }
    }
  }
/*
 record type = AUDIT_EOE (audit end of event type record), or
              record type = AUDIT_PROCTITLE (we note the AUDIT_PROCTITLE is always the last record), or
              record type = AUDIT_KERNEL (kernel events are one record events), or
              record type < AUDIT_FIRST_EVENT (only single record events appear before this type), or
              record type >= AUDIT_FIRST_ANOM_MSG (only single record events appear after this type), or
              record type >= AUDIT_MAC_UNLBL_ALLOW && record type <= AUDIT_MAC_CALIPSO_DEL (these are also one record events), or
              for the stream being processed, the time of the event is over end_of_event_timeout seconds old.
*/
  i = buf.find(" type=");
  string type;
  eoeFound = false;
  if( i >= 0 ) {
    int e = buf.find(" ", i + 1);
    if( e > 0 ) {
      type = buf.substr( i + 6, e - i - 6);
      if( type == "EOE" || type == "PROCTITLE" || type == "KERNEL" )
      //if( type == "EOE" )
        eoeFound = true;
    }
  }
  return rc;
}

int
System_auditd::parse_buf2( string buf, int msgid ) {
  int rc = -1;
  int e = 0;
  int i = buf.find("node=");

  if( g_logLevel > 0 )
    logger.error("System_auditd parse2 " + buf + " curr " + itoa(msgid));

  if( i >= 0 ) {
    e = buf.find(" ", i + 1);
    if( e > 0 ) {
      string node = buf.substr( i + 5, e - i - 5 );
      datavals[msgid]["node"] = node;
    }
  }
  //i = buf.find(" type=", e + 1);
  i = buf.find(" type=", e);
  //logger.error("have i " + itoa(i));
  string type;
  if( i >= 0 ) {
    e = buf.find(" ", i + 1);
    //logger.error("have e " + itoa(e));
    if( e > 0 ) {
      type = buf.substr( i + 6, e - i - 6);
      //logger.error("have type " + type);
      if( datavals[msgid]["type"] > "" )
        datavals[msgid]["type"] += " type=" + type;
      else
        datavals[msgid]["type"] = type;
    }
  }
  i = buf.find("msg=", e + 1);
  if( i >= 0 ) {
    e = buf.find(" ", i + 1);
    if( e > 0 ) {
      string msg = buf.substr( i + 4, e - i - 4);
      if( msg.substr(0,6) == "audit(" ) {
        datavals[msgid]["msg"] = msg;
        int i = msg.find(":",6);
        if( i > 0 ) {
          datavals[msgid]["msgtime"] = msg.substr(6,i-6);
          int j = msg.find(")",i);
          if( j > 0 ) {
            datavals[msgid]["msgid"] = msg.substr(i+1,j-i-1);
            rc = atoi(datavals[msgid]["msgid"].c_str());
          }
        }
      }
      else {
        datavals[msgid]["msg2"] = msg;
      }
    }
  }
  int msgend = e;
  if( type == "PROCTITLE" ) {
    i = buf.find("proctitle=", msgend);
    if( i >= 0 ) {
      e = buf.find_first_of(" \n\0", i + 1);
      if( e < 0 ) e = buf.length();
      if( e > 0 ) {
        string title = buf.substr( i + 10, e - i - 10);
        string title2;
        if( title.substr(0,1) != "/" && title.substr(0,1) != "." ) {
          hex2ascii( title, title2 );
        }
        datavals[msgid]["title"] = title;
        if( title2.size() > 0 )
          datavals[msgid]["titlex"] = title2;
      }
    }
  }
  if( type == "CWD" ) {
    i = buf.find("cwd=", msgend);
    if( i >= 0 ) {
      e = buf.find_first_of( " \n\0", i + 1 );
      if( e < 0 ) e = buf.length();
      if( e > 0 ) {
        string cwd = buf.substr( i + 4, e - i - 4 );
        datavals[msgid]["cwd"] = cwd;
      }
    }
  }

  if( type == "SYSCALL" ) {
    i = buf.find("arch=", msgend);
    string arch, syscall, success, exitcode, a0, a1, a2, a3, ppid, pid, uid, exe, key;
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        arch = buf.substr( i + 5, e - i - 5);
        datavals[msgid]["arch"] = arch;
      }
    }
    i = buf.find("syscall="); // could be at start of rec
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        syscall = buf.substr( i + 8, e - i - 8);
        datavals[msgid]["syscall"] = syscall;
        if( arch == "c000003e" )
          datavals[msgid]["syscallx"] = callvals64[syscall];
        else
        if( arch == "40000003" )
          datavals[msgid]["syscallx"] = callvals86[syscall];
      }
    }
    i = buf.find("success=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        success = buf.substr( i + 8, e - i - 8);
        datavals[msgid]["success"] = success;
      }
    }
    i = buf.find("exit=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        exitcode = buf.substr( i + 5, e - i - 5);
        datavals[msgid]["exitcode"] = exitcode;
      }
    }
    i = buf.find("a0=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        a0 = buf.substr( i + 3, e - i - 3);
        datavals[msgid]["a0"] = a0;
      }
    }
    i = buf.find("a1=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        a1 = buf.substr( i + 3, e - i - 3);
        datavals[msgid]["a1"] = a1;
      }
    }
    i = buf.find("a2=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        a2 = buf.substr( i + 3, e - i - 3);
        datavals[msgid]["a2"] = a2;
      }
    }
    i = buf.find("a3=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        a3 = buf.substr( i + 3, e - i - 3);
        datavals[msgid]["a3"] = a3;
      }
    }
    i = buf.find("ppid=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        ppid = buf.substr( i + 5, e - i - 5);
        datavals[msgid]["ppid"] = ppid;
      }
    }
    i = buf.find(" pid=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        pid = buf.substr( i + 5, e - i - 5);
        datavals[msgid]["pid"] = pid;
      }
    }
    i = buf.find(" uid=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        uid = buf.substr( i + 5, e - i - 5);
        datavals[msgid]["uid"] = uid;
      }
    }
    i = buf.find("exe=", e + 1);
    if( i >= 0 ) {
      logger.error("TEST " + buf);
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        exe = buf.substr( i + 4, e - i - 4);
        datavals[msgid]["exe"] = exe;
        logger.error("TEST2 " + exe);
      }
    }
    i = buf.find("key=", e + 1);
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        key = buf.substr( i + 4, e - i - 4);
        datavals[msgid]["key"] = key;
      }
    }
  }
  if( type == "PATH" ) {
    i = buf.find("name=", msgend);
    string name;
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e > 0 ) {
        name = buf.substr( i + 5, e - i - 5);
        datavals[msgid]["name"] = name;
      }
    }
  }
  if( type == "SOCKADDR" ) {
    i = buf.find("saddr=", msgend);
    string saddr;
    if( i >= 0 ) {
      e = buf.find(" ", i + 1);
      if( e < 0 ) e = buf.length();
      if( e > 0 ) {
        saddr = buf.substr( i + 6, e - i - 6);
        datavals[msgid]["saddr"] = saddr;
        unsigned long s[8];

        for( int i = 0, j = 0; i < 8; i++, j += 2 ) {
          string s1 = saddr.substr(j,2);
          s[i] = hex2dec(s1); 
        }
        unsigned long family = 256 * s[1] + s[0];
        datavals[msgid]["family"] = itoa(family);
        if( family == 2 ) {
          unsigned long port = 256 * s[2] + s[3];
          datavals[msgid]["port"] = itoa(port);
          string ip = ltoa(s[4]) + "." + ltoa(s[5]) + "." + ltoa(s[6]) + "." + ltoa(s[7]);
          datavals[msgid]["ip"] = ip;
        } 
        else
        if( family == 1 ) {
          string s1 = saddr.substr(4, saddr.length() - 4);
          string file;
          hex2ascii( s1, file );
          datavals[msgid]["file"] = file;
        }
        else {
          string s1 = saddr.substr(4, saddr.length() - 4);
          datavals[msgid]["file"] = s1;
        }
      }
    }
  }
  return rc;
}

void
System_auditd::load_defines( void ) {
  std::ifstream f1("/usr/include/linux/audit.h");
  std::stringstream buffer;
  buffer << f1.rdbuf();

  int i = buffer.str().find("#define AUDIT_");

  while( i >= 0 ) {
    int j = buffer.str().find_first_of(" \t", i);
    int k = buffer.str().find_first_of(" \t", j+1);
    string key = buffer.str().substr(j + 1,k-j-1);
    j = buffer.str().find_first_not_of(" \t", k);
    k = buffer.str().find_first_of(" \t", j+1);
    string val = buffer.str().substr(j,k-j);
    string key2 = key.substr(6, key.size() - 6);
    vals[key2] = val;
    if( val == "2000" )
      i = -1;
    else
      i = buffer.str().find("#define AUDIT_", k);
  }

  string buf = string(INSTALLDIR) + "/includes/syscalls.x86_64";
  std::ifstream f2(buf.c_str());
  std::stringstream buffer2;
  buffer2 << f2.rdbuf();
  i = 0;
  if(( buffer2.str().substr(0,1).c_str() < "0" ) || ( buffer2.str().substr(0,1).c_str() > "9" ))  {
    // skip first line if not numeric first char
    i = buffer2.str().find_first_of("\n");
  }

  int k = buffer2.str().find_first_of(" \t", i);
  while( k >= 0 ) {
    string key, val;
    key = buffer2.str().substr(i+1, k-i-1);
    i = buffer2.str().find_first_not_of(" \t", k);
    k = buffer2.str().find_first_of("\n", i);
    val = buffer2.str().substr(i, k-i);
    i = k;
    k = buffer2.str().find_first_of(" \t", i);
    callvals64[key] = val;
  }

  buf = string(INSTALLDIR) + "/includes/syscalls.i386";
  std::ifstream f3(buf.c_str());
  std::stringstream buffer3;
  buffer3 << f3.rdbuf();
  i = 0;
  if(( buffer3.str().substr(0,1).c_str() < "0" ) || ( buffer3.str().substr(0,1).c_str() > "9" ))  {
    // skip first line if not numeric first char
    i = buffer3.str().find_first_of("\n");
  }

  k = buffer3.str().find_first_of(" \t", i);
  while( k >= 0 ) {
    string key, val;
    key = buffer3.str().substr(i+1, k-i-1);
    i = buffer3.str().find_first_not_of(" \t", k);
    k = buffer3.str().find_first_of("\n", i);
    val = buffer3.str().substr(i, k-i);
    i = k;
    k = buffer3.str().find_first_of(" \t", i);
    callvals86[key] = val;
  }
  return;
}

