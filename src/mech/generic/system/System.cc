/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "System.h"
#ifndef _HAVEMACOS
#include "System_netlink.h"
#include "System_auditd.h"
#include "System_host.h"
#else
#include "System_auditd_macosx.h"
#include "System_host.h"
#endif

#include "dpsglobal.h"

const key_t System::INITKEY2 = 0x15060034;

extern "C" {
  System* makeSystem() {
    return new System;
  }
}

int
System::main( DssObject& o ) {
  int       rc    = 0;
  _dpsServer = o.server;
  if( o.hostname.size() == 0 )
    o.hostname= get_hostname();

  logger.error("System main running now " + itoa(geteuid()) + " localid " + o.localID + " on " + o.hostname);

  int id = semget( INITKEY2 + _DpsServerNumber, 1, S_IRWXU );
  if( id )
    semctl( id, 0, IPC_RMID, 0 );
    
  id = semget( INITKEY2 + _DpsServerNumber, 1, S_IRWXU | IPC_CREAT );

  if( id == -1 ) {
    logger.error( "ERROR: error: semget initID" );
    return -1;
  }
  semctl( id, 0, SETVAL, 0 );

  logger.error("System running");

  _dpsServer->sems.getClientSem( o.localidsem, o.semNum, o.offset );

  o.getInterface().setLocalID( o.localID );
  o.getInterface().setHostname( o.hostname );
  logger.error("running model " + o.model);

  System* a = NULL;
#ifndef _HAVEMACOS
  if( o.model == "NETLINK" ) {
    a = new System_netlink;
  }
  else
  if( o.model == "AUDITD" ) {
    logger.error("new auditd");
    a = new System_auditd;
  }
  else
  if( o.model == "SYSHOST" ) {
    logger.error("new sys host");
    a = new System_host;
  }
  else
#else
  if( o.model == "AUDITD" ) {
    logger.error("new macosx auditd");
    a = new System_auditd_macosx;
  }
  else
  if( o.model == "SYSHOST" ) {
    logger.error("new sys host");
    a = new System_host;
  }
  else
#endif
  {
    logger.error("no model found, exiting");
    rc = -1;
  }

  if( a ) {
    logger.error("running main client");
    a->main( o );
    logger.error("done running main client");
    delete a;
  }
  if( o.dpsids )
    delete[]o.dpsids;

  return rc;
}

int
System::doPoll( int fd, int t ) {
  struct pollfd fds[1];

  fds[0].fd       = fd;
  fds[0].events   = POLLIN | POLLPRI;
  fds[0].revents  = 0;

  int timeOut = t * 1000; // seconds

  int retval = ::poll( fds, 1, timeOut );

  logger.error( "poll rc : " + itoa(retval) );

  if( ( fds[0].revents & POLLERR ) ||
      ( fds[0].revents & POLLNVAL ) ||
      ( fds[0].revents & POLLHUP ) ) {
    logger.error( "doPoll error: invalid " + itoa(errno) + " fd " + itoa(fd) );
    return -1;
  }

  if( retval == -1 ) {
    logger.error( "doPoll failed: " + itoa(errno) + " to: " + itoa(fd) );
  }
  else
  if( retval == 0 ) {
    logger.error( "doPoll timedout: " + itoa(t) + " to: " + itoa(fd) );
  }
  else
  if( retval == 1 ) {
    if( !( fds[0].revents & POLLIN ) ) {
      logger.error( "doPoll failed: " + itoa(errno) );
      return -1;
    } else {
      logger.error( "doPoll interrupted");
      sleep(2);
      return 1;
    }
  }
  return retval;
}
