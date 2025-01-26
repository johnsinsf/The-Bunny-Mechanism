/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "MechLJ.h"
#include "Alerts.h"

void
MechLJ::doDataChanged( int id, DssObject& o ) {
  string s;
  int type = o.dpsids[id].type;
  if( type == 1 ) { // digital
    int current_state = o.dpsids[id].inew_state;
    s = "state change " + o.dpsids[id].channel + " state " + itoa(current_state) + " interface " + itoa( o.getInterface().getId() ) ;
  } 
  else
  if( type == 2 ) { // analog
    double current_state = o.dpsids[id].dnew_state;
    s = "state change " + o.dpsids[id].channel + " state " + ftoa(current_state) + " interface " + itoa( o.getInterface().getId() ) ;
  } 
  else {
    logger.error("bad type in doDataChanged " + itoa(type));
    int current_state = o.dpsids[id].inew_state;
    s = "state change " + o.dpsids[id].channel + " state " + itoa(current_state) + " interface " + itoa( o.getInterface().getId() ) ;
  }
  logger.error(s);

  if( o.localstorage ) {
    o.getLog().saveData    ( o.dpsids, id, s );
    o.getChannel().saveData( o.dpsids, id, o.getInterface() );

#ifndef _HAVEMACOS
    semRelease(o.semDataID);
#else 
    dispatch_semaphore_signal( dispatch_sem );
#endif 
  }
  if( o.send_data_alerts ) {
    logger.error("alert " + s);

    string out;
    Alerts alert;
    alert.setHostname(o.hostname);
    alert.setDateTime(time(NULL));
    alert.setSource("Labjack");
    alert.setList  ("default");
    alert.setPriority(2);
    alert.setAlertMsg(s);
    alert.export_data_XML( out );
    o.server->add_client_data( o, out );
  }
  return;
}

void
MechLJ::processDeviceData( DssObject& o, bool opt ) {
  int icurrent_state = -1;
  int ilast_state    = -1;

  double dcurrent_state = 0.0;
  double dlast_state = 0.0;

  for( int j = 0; j < o.numdpsids; j++ ) {
    if(o.dpsids[j].type == 1) { // digital
      ilast_state    = o.dpsids[j].icurrent_state;
      icurrent_state = o.dpsids[j].inew_state;
      if( ilast_state != icurrent_state && opt ) {
        doDataChanged(j, o);
      }
      o.dpsids[j].icurrent_state = icurrent_state;
      if( icurrent_state )
        o.dpsids[j].last_open = time(NULL);
      else
        o.dpsids[j].last_close = time(NULL);
    } 
    else
    if(o.dpsids[j].type == 2) { // analog
      dlast_state    = o.dpsids[j].dcurrent_state;
      dcurrent_state = o.dpsids[j].dnew_state;
      if( dlast_state != dcurrent_state && opt ) {
        doDataChanged(j, o);
      }
      o.dpsids[j].dcurrent_state = dcurrent_state;
    }
  }
  return;
}

