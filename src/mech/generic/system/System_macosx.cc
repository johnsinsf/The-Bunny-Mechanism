/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "System_macosx.h"

int 
System_macosx::main( DssObject& o ) {
  int sock;
  int rc = -1;
  
  get_hostname();

  sock = socketfilter_connect();

  // signal we are ready 
  semRelease( o.semInitID );

  if( o.semRunID > 0 )
    semGet( o.semRunID );

  if( sock > -1 ) {
    // loop on events
    rc = run( sock, o );
  }
  if( sock > -1 )
    close( sock );

  return rc;
}

/*
* connect to kext socket filter 
* returns socket, or -1 on error
*/

int 
System_macosx::socketfilter_connect() {

  int sock = -1;

  sock = socket( PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL );

  if( sock < 0 ) {
    logger.error("socket SYSPROTO_CONTROL " + itoa(errno));
    return -1;
  }

  bzero(&ctl_info, sizeof(struct ctl_info));

  strcpy(ctl_info.ctl_name, MYBUNDLEID);

  errno = 0;
  if( ioctl( sock, CTLIOCGINFO, &ctl_info ) == -1 ) {
    logger.error("ioctl CTLIOCGINFO");
    return -1;
  } else {
    //printf("ctl_id: 0x%x for ctl_name: %s\n", ctl_info.ctl_id, ctl_info.ctl_name);
    //logger.error("ctl_name: " + string(ctl_info.ctl_name));
  }
  memset(&sc, 0, sizeof(sc));

  sc.sc_len     = sizeof(struct sockaddr_ctl);
  sc.sc_family  = AF_SYSTEM;
  sc.ss_sysaddr = SYSPROTO_CONTROL;
  sc.sc_id      = ctl_info.ctl_id;
  sc.sc_unit    = 0;

  if( connect(sock, (struct sockaddr *)&sc, sizeof(struct sockaddr_ctl)) ) {
    logger.error("connect failed " + itoa(errno));
    return -1;
  }
  logger.error("mac connected " + itoa(sock)); 
  return sock;
}

void
System_macosx::get_hostname( void ) {

  char buf[32];
  memset(buf, 0, sizeof(buf));
  int i = gethostname( buf, sizeof(buf) );
  if( i == 0 )
    hostname = buf;

  logger.error("mac hostname " + hostname);

  return;
}

/*
* handle process events
*/

int 
System_macosx::run( int sock, DssObject& o ) {
  int n = -1;
  bool done = false;
  string msg;

  logger.error("mac running");

  while( ! g_quit && ! done ) {
    n = doPoll( sock, 5 );
    logger.error("polling " + to_string(n));
    if( n > 0 ) {
      n = recv( sock, &tlp, sizeof (tlp), 0);
      if( n == sizeof(tlp) ) {
        msg = parse_buf();
        if( msg.size() ) {
          send_data( o, msg );
        }
      } else {
        logger.error("bad read, exiting");
        done = true;
      }
    } 
    else 
    if( n != 0 ) {
      logger.error("bad poll, exiting");
      done = true;
    }
  }
  return 0;
}

void
System_macosx::send_data( DssObject& o, string& msg ) {

  dataType d;
  d.structtype = 0;
  d.datatype   = 4;
  d.localid    = o.localID;
  d.channel    = "STD";
  d.synced     = 0;
  d.tries      = 0;
  d.datetime_sent = 0;
  d.hostname   = hostname;
  d.data       = msg;
  d.msgid      = ltoa(msgid);
  d.msgtime    = ltoa(time(NULL));
 
  o.server->clientData[o.server->getClientDataIndex()] = d;

  //dispatch_semaphore_signal( dispatch_sem );

  return;
}

string
System_macosx::parse_buf( void ) {
  string msg, type;

  switch (tlp.tli_state) {
    case TLS_CONNECT_IN:
      type = "IN ";
      break;
    case TLS_CONNECT_OUT:
      type = "OUT";
      break;
    case TLS_LISTENING:
      type = "LISTEN";
      break;
    default:
    case 0:
      type = "NONE  ";
      break;
  }

  msgid = tlp.tli_genid;

  if (tlp.tli_protocol == AF_INET) {
    msg = type + " FROM " + inet_ntoa(tlp.tli_local.addr4.sin_addr) + ":" + itoa(tlp.tli_local.addr4.sin_port);
    msg += " TO " + string(inet_ntoa(tlp.tli_remote.addr4.sin_addr)) + ":" + itoa(tlp.tli_remote.addr4.sin_port);
  } else {
    char  addrString[256];

    inet_ntop(AF_INET6, &(tlp.tli_local.addr6.sin6_addr), (char*)addrString, sizeof(addrString));
    msg = type + " FROM " + string(addrString) + ":" + itoa(tlp.tli_local.addr6.sin6_port);
           
    inet_ntop(AF_INET6, &(tlp.tli_remote.addr6.sin6_addr), (char*)addrString, sizeof(addrString));
    msg += " TO " + string(addrString) + + ":" + itoa(tlp.tli_remote.addr6.sin6_port);
  }
  msg += " pkts in: " + itoa(tlp.tli_pkts_in) + " out: " + itoa(tlp.tli_pkts_out);
  msg += " bytes in: " + itoa(tlp.tli_bytes_in) + " out: " + itoa(tlp.tli_bytes_out);
  msg += " uid: " + itoa(tlp.tli_uid);
  msg += " pid: " + itoa(tlp.tli_pid);

  double elapsed;
  struct timeval tdiff;

  tdiff.tv_sec = tlp.tli_stop.tv_sec - tlp.tli_start.tv_sec;
  tdiff.tv_usec = tlp.tli_stop.tv_usec - tlp.tli_start.tv_usec;
  if (tdiff.tv_usec < 0)
    tdiff.tv_sec--, tdiff.tv_usec += 1000000;
  elapsed = tdiff.tv_sec + ((double)tdiff.tv_usec) / 1000000;
  if( elapsed < 0.00001 )  elapsed = 0.00001;
 
  msg += " time: " + ftoa(elapsed);

  return msg;
}
