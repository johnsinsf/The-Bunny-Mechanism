/*
  Author:  John Sullivan, Dynamic Processing Upnps LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "Upnp.h"
#include "Upnp_ushare.h"
#include "dpsglobal.h"

const key_t Upnp::INITKEY2 = 0x16010000;

extern "C" {
  Upnp* makeUpnp() {
    return new Upnp;
  }
}

int
Upnp::main( DssObject& o ) {
  int       rc    = 0;
  _dpsServer = o.server;
  if( o.hostname.size() == 0 )
    o.hostname= get_hostname();

  logger.error("Upnp main running now " + itoa(geteuid()) + " localid " + o.localID + " on " + o.hostname);

  int id = semget( INITKEY2 + _DpsServerNumber, 1, S_IRWXU );
  if( id )
    semctl( id, 0, IPC_RMID, 0 );
    
  id = semget( INITKEY2 + _DpsServerNumber, 1, S_IRWXU | IPC_CREAT );

  if( id == -1 ) {
    logger.error( "ERROR: error: semget initID" );
    return -1;
  }
  semctl( id, 0, SETVAL, 0 );

  logger.error("Upnp running");

  _dpsServer->sems.getClientSem( o.localidsem, o.semNum, o.offset );

  o.getInterface().setLocalID( o.localID );
  o.getInterface().setHostname( o.hostname );
  logger.error("running model " + o.model);

  //semRespID = o.semRespID;

  Upnp* a = NULL;
  if( o.model == "USHARE" ) {
    logger.error("new ushare host");
    a = new Upnp_ushare;
  } else {
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
