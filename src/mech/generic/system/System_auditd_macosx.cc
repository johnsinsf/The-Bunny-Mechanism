/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2025 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "System_auditd_macosx.h"

int 
System_auditd_macosx::main( DssObject& o ) {
  int rc = -1;
  
  // TESTING!
  //g_logLevel = 5;

  logger.error("System_auditd_macosx starting");

  int fd = open("/dev/auditpipe", O_RDONLY);

  if( fd < 0 ) {
    logger.error("unable to open /dev/auditpipe");
    return -1;
  }
  semRelease( o.semInitID );

  if( fd > -1 ) {
    load_defines();
    load_procs();
    rc = run( fd, o );
    close(fd);
  }
  logger.error("System_auditd_macosx done");

  return rc;
}

int 
System_auditd_macosx::run( int fd_pipe, DssObject& o ) {
  int rc = -1;
  bool done = false;

  #define BUFSIZE 16384
  char buf[BUFSIZE];
  string out;

  if( g_logLevel > 2 )
    logger.error("System_auditd_macosx running " + itoa(fd_pipe));

  while( ! done && ! g_quit ) {
    memset(buf, 0, sizeof(buf));
    if( g_logLevel > 2 )
      logger.error("System_auditd_macosx recv " + itoa(fd_pipe));
    rc = read( fd_pipe, &buf, sizeof(buf) );
    string out2 = "";
    if( g_logLevel > 1 ) {
      int i = 0;
      while( i < rc )
        out2 += itoa((unsigned char)buf[i++]) + ":";
      out2 += "\n";
      logger.error("read " + out2);
    }
    if( g_logLevel > 2 )
      logger.error("System_auditd_macosx recv " + itoa(rc));
    if( rc > 0 ) {
      if( rc != BUFSIZE ) {
        out += string(buf,rc);
        parse_buf( out, o );
        out = "";
      } else {
        out += string(buf, BUFSIZE);
      }
    }
    else
    if( rc == 0 ) {
      logger.error("pipe recv eof");
      done = true;
    } 
    else 
    if (rc <= -1) {
      if (errno != EINTR) {
        logger.error("pipe recv error " + itoa(errno));
        rc = -1;
        done = true;
      }
      else {
        logger.error("pipe interrupted");
        done = true;
      }
    }
  }
  logger.error("System_auditd_macosx done");
  return rc;
}

// loop datavals and create clientData entries

void
System_auditd_macosx::send_data( DssObject& o, unsigned int msgid, unsigned int msgtime ) {

  dataType d;
  d.structtype = 0;
  d.datatype   = 4;
  d.localid    = o.localID;
  d.channel    = "AUDITD";
  d.synced     = 0;
  d.tries      = 0;
  d.msgid      = itoa(msgid);
  d.msgtime    = itoa(msgtime);
  d.datetime_sent = 0;
  d.hostname   = o.hostname;

  map<string,string>::iterator I;
  map<int, map<string,string> >::iterator I1;

  int i = 0;
  string out, index;
  bool sendVal = true;

  I1 = datavals.find(msgid);

  if( g_logLevel > 2 )
    logger.error("have msg " + itoa(msgid));

  for( I = I1->second.begin(); I != I1->second.end(); ++I ) { 
    if( g_logLevel > 2 )
      logger.error("have " + I->first + " " + I->second);
    if( I->first == "node" ) {
      d.hostname = I->second;
    } else {

      bool skipVal = false;

      if( I->first == "title" )
        if( datavals[msgid]["titlex"] > "" )
          skipVal = true;

      if( I->first == "syscall" ) {
        if( datavals[msgid]["syscallx"] > "" )
          skipVal = true;
        if( I->second == "213" ) { // munmap
          sendVal = false;
        }
      }
      if( ! skipVal ) {
        if( i++ > 0 ) out += " ";    
        out += I->first + "=" + I->second;
      } 
      if( I->first == "exe" || I->first == "family" || I->first == "syscall" || I->first == "syscallx" || 
          I->first == "user" || I->first == "port" || I->first == "ip" || I->first == "file" ) {
        if( I->second.size() < 40 )
          index += I->second;
        else {
          int x = I->second.find(" ");
          if( x <= 0 || x > 40 ) x = 40;
          index += I->second.substr(0,x);
        }
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
  // memcached is limited to 250 bytes keys, leaving some room for my keys
  if( index.size() > 200 ) {
    logger.error("truncating index, too large: " + index);
    string index2 = index.substr(0,200);
    index = index2;
  } 
  d.data = index + "\t" + out;
 
  if( g_logLevel > 2 )
    logger.error("adding data " + d.data);

  if( sendVal )
    o.server->add_client_data( o, d );
  
  datavals[msgid].clear();

  //logger.error("Pinging data ready");

  //dispatch_semaphore_signal( dispatch_sem );

  return;
}


/*
  sample messages

  to figure out how to decode a message type use praudit -r /var/audit/current to compare to the raw data 

20,79,11,184,0,1661028558,674
45,1,0x1c,fd
36,501,501,20,501,20,252,100004,50331650,0.0.0.0
39,0,1
19,79
20,139,11,191,0,1661028558,674
45,1,0x1d,fd
130,1,
35,/Users/johns/Library/Containers/com.apple.mail/Data/
36, 501,501,20,501,20,252,100004,50331650,0.0.0.0
36,  -1,0,  0,   0, 0,154,100007,312,     0.0.0.0
122,501,0,  0, 501,20,154,100007,50331650,0.0.0.0

36:  0:0:1:245: 0:0:1:245: 0:0:0:20 :0:0:1:245: 0:0:0:20: 0:0:11:106: 0:1:134:201 :3:0:0:2:  0:0:0:0
122: 0:0:1:245: 0:0:0:0:   0:0:0:20 :0:0:1:245 :0:0:0:20: 0:0:6:187:  0:0:6:187   :16:0:0:2: 0:0:0:4 :0:0:0:0:
     39:0:0:0:0:0:237:0:0:0:1:0:16:99:111:109

35:0:31:47:117:115:114:47:115:104:97:114:101:47:108:111:99:97:108:101:47:108:111:99:97:108:101:46:97:108:105:97:115:0:62:0:0:129:164:0:0:0:0:0:0:0:0:1:0:0:2:0:0:0:0:0:195:240:99:0:0:0:0:

35:0:27:47:85:115:101:114:115:47:106:111:104:110:115:47:46:98:97:115:104:95:104:105:115:116:111:114:121:0:

29 chars 62:0:0:129:128:0:0:0:0:0:0:0:20:1:0:0:2:0:0:0:0:0:5:209:204:0:0:0:0:

36:0:0:1:245:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:12:3:0:1:134:164:3:0:0:2:0:0:0:0:

20:0:0:0:101:11:0:213:0:0:99:1:92:127:0:0:3:194:113:1:0:0:127:199:90:48:0:0:0:5:97:100:100:114:0:113:2:0:0:0:0:0:16:0:0:0:4:108:101:110:0:

45:1:0:0:0:4:0:3:102:100:0:
45:2:0:0:0:23:0:3:102:100:0:

113:1:0:0:0:1:16:35:144:0:0:5:97:100:100:114:0:
113:2:0:0:0:0:0:0:16:0:0:4:108:101:110:0:

45,1,0x2,domain
45,2,0x1,type
45,3,0x0,protocol

*/

void 
System_auditd_macosx::parse_buf( string buf, DssObject& o ) {
  unsigned int msgid = 0;
  int cnt = 0;
  unsigned int msgtime = 0;
  bool eoeFound = false;
  const char* bufptr = buf.c_str();
  int len = buf.size();

  if( g_logLevel > 0 )
    logger.error("System_auditd_macosx parse_buf " + itoa(len));

  int rc = parse_msgid( bufptr, len, o, msgid, msgtime, cnt );

  while( rc == 1 ) {
    rc = parse_buf2( bufptr, len, msgid, eoeFound, cnt );
    if( rc && eoeFound ) {
      send_data ( o, msgid, msgtime );
    } 
    if( rc ) {
      rc = parse_msgid( bufptr, len, o, msgid, msgtime, cnt );
    }
  }
}

int
System_auditd_macosx::parse_msgid( const char* bufptr, int len, DssObject& o, unsigned int& msgid, unsigned int& msgtime, int& cnt ) {
  int rc = -1;
  //logger.error("parse_msgid " + itoa(cnt) + " int size " + itoa(sizeof(int)));
  unsigned int t1 = 0;
  msgid = 0; msgtime = 0;
 
  if( cnt < len ) {
    if( *(bufptr+cnt) == 20 ) {
      cnt++;
      memcpy(&t1, (unsigned char*)(bufptr+cnt), sizeof(int));
      msgid = htonl(t1);
      //logger.error("20 have msgid " + itoa(msgid));
      cnt += 5;
      unsigned short int s1, s2;
      memcpy(&s1, (unsigned char*)(bufptr+cnt), 2);
      s2 = htons(s1);
      //logger.error("have header val " + itoa(s2));
      cnt += 4;
      
      map<string,string>::iterator I = callvals.find(itoa(s2));

      datavals[msgid]["syscall"] = itoa(s2);

      if( I != callvals.end() ) {
        //logger.error("found callval " + I->second);
        datavals[msgid]["syscallx"] = I->second;
      }
      memcpy(&t1, (unsigned char*)(bufptr+cnt), sizeof(int));
      msgtime = htonl(t1);
      //logger.error("20 have msgtime " + itoa(msgtime));
      //datavals[msgid]["msgtime"] = itoa(msgtime);
      cnt += 8;
      rc = 1;
    }
    else {
      logger.error("bad new msg id " + itoa(*(bufptr+cnt)) 
        + " - " + itoa(*(bufptr+cnt-1))
        + " + " + itoa(*(bufptr+cnt+2)) );
    }
  }
  return rc;
}

int
System_auditd_macosx::parse_buf2( const char* bufptr, int len, int msgid, bool& eoeFound, int& cnt ) {
  int rc = -1;
  unsigned int pid = 0, uid = 0;
  string exe;

  if( g_logLevel > 0 )
    logger.error("System_auditd_macosx parse_buf2 msgid " + itoa(msgid) + " cnt " + itoa(cnt));

  eoeFound = false;
  while( ! eoeFound && cnt < len ) {
    if( g_logLevel > 2 )
      logger.error("HAVE CNT " + itoa(cnt) + " " + itoa(*(bufptr+cnt)));
    if( *(bufptr + cnt) == 19 ) {
      if( g_logLevel > 2 )
        logger.error("EOE found");
      eoeFound = true;
      cnt += 3;
      unsigned int t1 = 0, t2 = 0;
      memcpy(&t1, (bufptr+cnt), 4);
      t2 = htonl(t1);
      if( t2 != msgid ) {
        logger.error("mismatched MsgIDs! " + itoa(t2) );
      }
      cnt += 4;
      rc = 1;
    } 
    else
    if( *(bufptr + cnt) == 20 ) {
      logger.error("found header in parse_buf2, bailing");
      return -1;
    }
    else
    if( *(bufptr + cnt) == 36 ) {
      if( g_logLevel > 2 )
        logger.error("found type 36");
      // 36: 0:0:1:245: 0:0:1:245: 0:0:0:20 :0:0:1:245: 0:0:0:20: 0:0:11:106: 0:1:134:201 :3:0:0:2: 0:0:0:0
      cnt++;
      unsigned int i1, i2;
      memcpy(&i1, (bufptr+cnt), 4);
      i2 = htonl(i1);
      //logger.error("36 have uid " + itoa(i2));
      datavals[msgid]["uid"] = itoa(i2);
      uid = i2;
      //cnt += 36;
      cnt += 20;

      memcpy(&i1, (bufptr+cnt), 4);
      i2 = htonl(i1);
      //logger.error("36 have PID " + itoa(i2));
      pid = i2;
      cnt += 16;
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 122 ) {
      if( g_logLevel > 2 )
        logger.error("found type 122");
      // 122,501,0,0,501,20,154,100007,50331650,0.0.0.0
      cnt++;
      unsigned int i1, i2;
      memcpy(&i1, (bufptr+cnt), 4);
      i2 = htonl(i1);
      //logger.error("36 have uid " + itoa(i2));
      datavals[msgid]["uid"] = itoa(i2);
      uid = i2;
      //cnt += 36;
      cnt += 20;

      memcpy(&i1, (bufptr+cnt), 4);
      i2 = htonl(i1);
      //logger.error("36 have PID " + itoa(i2));
      pid = i2;
      //cnt += 16;
      cnt += 20;
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 33 ) {
      if( g_logLevel > 2 )
        logger.error("found type 33");
      cnt += 24;
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 62 ) {
      if( g_logLevel > 2 )
        logger.error("found type 62");
      cnt += 29;
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 35 ) {
      if( g_logLevel > 2 )
        logger.error("found type 35");
      cnt++;
      short int s1, s2;
      memcpy(&s1, (bufptr+cnt), 2);
      s2 = htons(s1);
      //logger.error("35 have len " + itoa(s2));
      char sbuf[s2];
      cnt += 2;
      memcpy(&sbuf, (bufptr+cnt), s2);
      //logger.error("35 have sbuf " + string(sbuf));
      cnt += s2;
      //logger.error("35 have cnt " + itoa(cnt) + " len " + itoa(len));
      datavals[msgid]["name"] = "\"" + string(sbuf) + "\"";
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 39 ) {
      if( g_logLevel > 2 )
        logger.error("found type 39");
      cnt += 6;
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 45 ) {
      if( g_logLevel > 2 )
        logger.error("found type 45");
      cnt += 1;
      int type = 0;
      if( *(bufptr+cnt) == 1 ) {
        //logger.error("45 type 1" );
        type = 1;
      }
      else
      if( *(bufptr+cnt) == 2 ) {
        //logger.error("45 type 2" );
        type = 2;
      }
      else
      if( *(bufptr+cnt) == 3 ) {
        //logger.error("45 type 3" );
        type = 3;
      }
      else
        logger.error("45 type UNKNOWN " + string((bufptr+cnt)) );
      cnt+=1;
      int i1, i2 = 0;
      memcpy( &i1, (bufptr+cnt), 4);
      i2 = htonl(i1);
      //logger.error("have type 45 val " + itoa(i2));
      short int s1, s2 = 0;
      cnt += 4;
      memcpy( &s1, (bufptr+cnt), 2);
      s2 = htons(s1);
      //logger.error("have type 45 len " + itoa(s2));
      cnt += (s2 + 2);
      rc = 1;
    }
    else
    if( (unsigned char)*(bufptr + cnt) == 128 ) {
      if( g_logLevel > 2 )
        logger.error("found type 128");
      // 128:0:2:46:224:207:7:131:35
      short int s1, s2 = 0;
      cnt++; 
      cnt+=2;
      memcpy( &s1, (bufptr+cnt), 2);
      s2 = htons(s1);
      if( g_logLevel > 2 )
        logger.error("have type 128 port " + itoa(s1) + " " + itoa(s2));
      datavals[msgid]["port"] = itoa(s2);
      cnt += 2;
      unsigned char x;
      string ipaddr;
      x = *(bufptr+cnt);
      ipaddr = itoa(x) + ".";
      x = *(bufptr+cnt+1);
      ipaddr += itoa(x) + ".";
      x = *(bufptr+cnt+2);
      ipaddr += itoa(x) + ".";
      x = *(bufptr+cnt+3);
      ipaddr += itoa(x);
      if( g_logLevel > 2 )
        logger.error("ip addr " + ipaddr);
      datavals[msgid]["ip"] = ipaddr;
      datavals[msgid]["family"] = "2";
      cnt += 4;
      rc = 1;
      string t= "";
      int i = 0;
      while( i < len )
        t += itoa((unsigned char)*(bufptr+i++)) + ":";
      t += "\n";
    }
    // 129:0:26:0:192:254:128:0:0:0:0:0:0:16:91:110:194:95:143:14:189  :36:0:0:1:245:0:0:1:245:0:0:0:20:0:0:1:245:0:0:0:20:0:0:45:236:0:1:134
    // 129:0:26:241:245:254:128:0:0:0:0:0:0:128:213:246:173:0:209:36:30
    // socket-inet6,26,53,2001:4860:4860::8888
    // 129,26,53,2001:4860:4860::8888
/*
    else
    if( (unsigned char)*(bufptr + cnt) == 129 ) {
      //if( g_logLevel > 2 )
        logger.error("found type 129");
      cnt += 21;
    }
*/
    else
    if( *(bufptr + cnt) == 113 ) {
      if( g_logLevel > 2 )
        logger.error("found type 113");
      int type = 0;
      cnt++;
      if( *(bufptr+cnt) == 1 ) {
        //logger.error("113 type 1" );
        type = 1;
      }
      else
      if( *(bufptr+cnt) == 2 ) {
        //logger.error("113 type 2" );
        type = 2;
      }
      if( type == 1 )
        cnt += 9;
      if( type == 2 )
        cnt += 9;
      short int s1, s2 = 0;
      memcpy(&s1, (bufptr+cnt), 2);
      s2 = htons(s1);
      //logger.error("have 113 len " + itoa(s2));
      cnt += (s2 + 2);
      rc = 1;
    }
    else
    if( *(unsigned char*)(bufptr + cnt) == 237 || *(bufptr + cnt) == -19 ) {
      if( g_logLevel > 2 )
        logger.error("found type 237");
      int type = 0;
      cnt++;
      if( *(bufptr+cnt) == 1 ) {
        //logger.error("237 type 1" );
        type = 1;
      }
      else
      if( *(bufptr+cnt) == 2 ) {
        //logger.error("237 type 2" );
        type = 2;
      }
      cnt += 4;
      short int s1, s2 = 0;
      memcpy(&s1, (bufptr+cnt), 2);
      s2 = htons(s1);
      if( g_logLevel > 2 )
        logger.error("have 237 len " + itoa(s2));
      cnt += 2;
      char sbuf[s2];
      memcpy(&sbuf, (bufptr+cnt), s2);
      if( g_logLevel > 2 )
        logger.error("237 have sbuf " + string(sbuf));
      cnt += s2;
      cnt++;

      memcpy(&s1, (bufptr+cnt), 2);
      s2 = htons(s1);
      if( g_logLevel > 2 )
        logger.error("have 237 len2 " + itoa(s2));
      cnt += 2;
      char sbuf2[s2];
      memcpy(&sbuf2, (bufptr+cnt), s2);
      if( g_logLevel > 2 )
        logger.error("237 have sbuf2 " + string(sbuf2));
      datavals[msgid]["exe"] = "\"" + string(sbuf) + " " + string(sbuf2) + "\"" ;
      cnt += s2;
      cnt += 23; // stuff
      //cnt++;
      //eoeFound = true;
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 40 ) {
      if( g_logLevel > 2 )
        logger.error("found type 40");
      //40:0:68:86:101:114:105:102:121:32:112:97:115:115:119:111:114:100:32:102:111:114:32:114:101:99:111:114:100:32:116:121:112:101:32:85:115:101:114:115:32:39:106:111:104:110:115:39:32:110:111:100:101:32:39:47:76:111:99:97:108:47:68:101:102:97:117:108:116:39:0
      cnt++;
      cnt++;
      string t;
      while( *(bufptr+cnt) != 0 && cnt < len ) {
        t += *(bufptr+cnt++);
      }
      if( g_logLevel > 2 )
        logger.error("have type 40 " + t);
      datavals[msgid]["aa"] = t;
      cnt++;
      rc = 1;
    }
    else
    if( *(bufptr + cnt) == 60 ) {
      if( g_logLevel > 2 )
        logger.error("found type 60");
      // 60:0:0:0:1:99:97:116:0:
      // 60:0:0:0:2:118:105:0:83:121:115:116:101:109:95:97:117:100:105:116:100:95:109:97:99:111:115:120:46:99:99:0
      cnt += 4;
      unsigned int x = *(bufptr + cnt);
      cnt++;
 
      //logger.error("have type 60 arg cnt " + itoa(x));
      string t;
      while( x-- > 0 ) {
        //string t;
        while( *(bufptr+cnt) != 0 && cnt < len ) {
          t += *(bufptr+cnt++);
        }
        if( x > 0 ) t += " ";
        //logger.error("have type 60 " + t);
        cnt++;
      }
      //logger.error("have type 60 " + t);
      exe = datavals[msgid]["exe"] = "\"" + t + "\"";
      //exe = t;
      rc = 1;
    }
    else
    if( (unsigned char)*(bufptr + cnt) == 130 ) {
      if( g_logLevel > 2 )
        logger.error("found type 130");
      // 130:0:1:47:116
      cnt += 3;
      if( *(bufptr+cnt) == 0 ) {
        cnt++;
      } else {
        char buf[len];
        memset(buf, 0, sizeof(buf));
        int i = 0;
        while( i < len && *(bufptr+cnt) != 0 ) {
          buf[i++] = *(bufptr+cnt++);
        }
        cnt++;
        if( g_logLevel > 2 )
          logger.error("have 130 status text " + string(buf));
        datavals[msgid]["unix-socket"] = string(buf);
      }
    }
    else {
      rc = -1;
      logger.error("DEFAULT TYPE FOUND " + itoa(cnt) 
        +  " " + itoa((unsigned char)*(bufptr+cnt)) 
        + " - " + itoa((unsigned char)*(bufptr+cnt -1)) 
        + " + " + itoa((unsigned char)*(bufptr+cnt +1)) 
      );
      cnt = len;
      string t= "";
      int i = 0;
      while( i < len )
        t += itoa((unsigned char)*(bufptr+i++)) + ":";
      t += "\n";
      logger.error("input " + t);
/*
use praudit -r /var/audit/current to get record to see how to format

this means at location 49 type 74 was found, previous value on the line is 102 and the next is 36.
DEFAULT TYPE FOUND 49 74 - 102 + 36

input 20:0:0:0:147:11:0:191:0:0:103:145:119:44:0:0:3:109:45:1:0:0:0:10:0:3:102:100:0:129:0:26:97:215:254:128:0:0:0:0:0:0:231:164:60:232:130:142:102:74:36:0
:0:1:245:0:0:1:245:0:0:0:20:0:0:1:245:0:0:0:20:0:0:185:64:0:1:134:167:3:0:0:2:0:0:0:0:39:0:0:0:0:95:237:0:0:0:0:0:13:98:117:110:110:121:95:99:108:105:101:1
10:116:0:0:0:1:0:0:0:20:200:86:19:159:195:67:73:53:116:2:181:95:70:119:125:215:160:187:61:50:19:177:5:0:0:0:147:

bash-3.2# 1737797524 ERROR  -1 dssClient: DEFAULT TYPE FOUND 49 189 - 14 + 36
1737797524 ERROR  -1 dssClient: input 20:0:0:0:147:11:0:34:0:0:103:148:175:148:0:0:0:225:45:1:0:0:0:5:0:3:102:100:0:129:0:26:0:192:254:128:0:0:0:0:0:0:16:91:110:194:95:143:14:189:36:0:0:1:245:0:0:1:245:0:0:0:20:0:0:1:245:0:0:0:20:0:0:231:238:0:1:134:167:3:0:0:2:0:0:0:0:39:0:0:0:0:0:237:0:0:0:0:0:13:98:117:110:110:121:95:99:108:105:101:110:116:0:0:0:1:0:0:0:20:200:86:19:159:195:67:73:53:116:2:181:95:70:119:125:215:160:187:61:50:19:177:5:0:0:0:147:20:0:0:0:126:11:168:132:0:0:103:148:175:148:0:0:0:225:45:1:0:0:0:5:0:3:102:100:0:36:0:0:1:245:0:0:1:245:0:0:0:20:0:0:1:245:0:0:0:20:0:0:231:238:0:1:134:167:3:0:0:2:0:0:0:0:39:0:0:0:0:0:237:0:0:0:0:0:13:98:117:110:110:121:95:99:108:105:101:110:116:0:0:0:1:0:0:0:20:200:86:19:159:195:67:73:53:116:2:181:95:70:119:125:215:160:187:61:50:19:177:5:0:0:0:126:20:0:0:0:160:11:0:183:0:0:103:148:175:148:0:0:0:225:45:1:0:0:0:2:0:7:100:111:109:97:105:110:0:45:2:0:0:0:1:0:5:116:121:112:101:0:45:3:0:0:0:0:0:9:112:114:111:116:111:99:111:108:0:36:0:0:1:245:0:0:1:245:0:0:0:20:0:0:1:245:0:0:0:20:0:0:231:238:0:1:134:167:3:0:0:2:0:0:0:0:39:0:0:0:0:6:237:0:0:0:0:0:13:98:117:110:110:121:95:99:108:105:101:110:116:0:0:0:1:0:0:0:20:200:86:19:159:195:67:73:53:116:2:181:95:70:119:125:215:160:187:61:50:19:177:5:0:0:0:160:20:0:0:0:135:11:0:34:0:0:103:148:175:148:0:0:0:225:45:1:0:0:0:6:0:3:102:100:0:128:0:2:0:0:127:0:0:1:36:0:0:1:245:0:0:1:245:0:0:0:20:0:0:1:245:0:0:0:20:0:0:231:238:0:1:134:167:3:0:0:2:0:0:0:0:39:0:0:0:0:0:2

*/
    }
  }
  if( pid > 0 && exe != "" ) {
    procvals[pid] = exe;
  } else {
    if( pid ) {
      map<unsigned int,string>::const_iterator I = procvals.find(pid);
      if( I != procvals.end() ) {
        datavals[msgid]["exe"] = I->second;
        if( g_logLevel > 1 )
          logger.error("set exe " + I->second + " pid " + itoa(pid));
      }
    }
  }
  if( g_logLevel > 1 )
    logger.error("parse_buf2 returns " + itoa(rc));

  return rc;
}

void
System_auditd_macosx::load_defines( void ) {
  string filename = "/etc/security/audit_event";

  ifstream file;
  file.open ( filename.c_str(), ios::in );
  if( ! file.is_open() ) {
    logger.error("audit_events file is missing");
    return;
  }
  string buf, key, val ;
  for( string buf; getline(file, buf); ) {
    //std::cout << line << std::endl;

    if( buf.substr(0,1) != "#" ) {
      int i = buf.find(":");
      if( i > 0 ) {
        key = buf.substr(0, i);
        int j = buf.find(":", i + 1);
        if( j > 0 ) { 
          int k = buf.find(":", j + 1); // skip over descr
          int l = buf.find("(", j + 1); // skip over ()
          if( k > 0 ) { 
            if( l > 0 && l < k ) k = l;
            val = buf.substr(j + 1, k - j - 1);
            callvals[key] = val;
            //logger.error("defines set " + key + " to " + val);
          }
        }
      }
    }
  }
  file.close();
  return;
}

void
System_auditd_macosx::load_procs( void ) {
  string filename = "/tmp/ps.list";

  ifstream file;
  file.open ( filename.c_str(), ios::in );
  if( ! file.is_open() ) {
    logger.error("ps.list is missing");
    return;
  }
  string buf;
  for (string buf; getline(file, buf); ) {
    logger.error("read " + buf);
    if( buf.size() > 11 ) {
      string uid = buf.substr(0,5);
      string pid = buf.substr(6,5);
      string cmd = buf.substr(11,buf.size() - 11);
      unsigned int x = atoi(pid.c_str());
      procvals[x] = cmd;
      logger.error("loaded " + cmd + " for uid " + uid + " pid " + pid);
    }
  }
  file.close();
  return;
}

