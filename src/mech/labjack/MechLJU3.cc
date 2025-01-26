/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "MechLJU3.h"
#include "Channel_types.h"
#include "Alerts.h"
#include "dpsglobal.h"

//#include <sys/time.h>

extern "C" {
  MechLJU3* makeLJU3() {
    return new MechLJU3;
  }
}

int 
MechLJU3::readDeviceLoop( HANDLE hDevice, DssObject& o ) {

  int isDAC1Enabled = 0;
  int error = 0;
  int count = 0;
  u3CalibrationInfo caliInfo;
  string filename = string(INSTALLDIR) + "/status/interface_reader-" + o.getLocalId();

  error = dpsLJConf( hDevice, &isDAC1Enabled );

  if( ! error )
    error = getCalibrationInfo( hDevice, &caliInfo );

  // prime the dpsids so we don't get alerts during startup
  if( ! error ) {
    error = dpsLJRead( hDevice, o, &caliInfo, isDAC1Enabled );
    if( ! error )
      processDeviceData( o, false );
  }
  while( ! error && ! g_quit ) {
    error = dpsLJRead( hDevice, o, &caliInfo, isDAC1Enabled );
    if( ! error ) {
      processDeviceData( o, true );
      usleep(500000);
      if( count++ > 60 ) {
        count = 0;
        string s = "worker " + o.getLocalId() + " is still running";
        logger.error("MechLJU3: " + s);
        utimes(filename.c_str(), NULL);
        time_t now = time(NULL);
        o.getInterface().setContacted(now);
        o.getInterface().markContacted();
        semRelease(o.semDataID);
      }
    }
  }
  logger.error("MechLJU3 readDeviceLoop returning error");

  return error;
}

int
MechLJU3::main( DssObject& o ) {
  int       error     = 1;
  time_t    lastalert = 0;
  HANDLE    hDevice   = NULL;

  o.semInitID = semget(INITKEY + _DpsServerNumber, 0, 0);
  o.semDataID = semget(DATAKEY + _DpsServerNumber, 0, 0);

  logger.error("u3 signaling in 1 sec " + itoa(o.semInitID) + " uid " + itoa(geteuid()) + " host " + o.hostname );

  int euid = geteuid();

  // signal main thread that we're running
  semRelease(o.semInitID);

  if( euid == 0 ) {
    logger.error("running as root, checking config");
    typedef map<string,string>::const_iterator I;
    I i = configMap.find("runuid");
    string uid, gid;
    if( i != configMap.end() ) {
      uid = i->second;
      logger.error("found uid: " + uid);
    }
    i = configMap.find("rungid");
    if( i != configMap.end() ) {
      gid = i->second;
      logger.error("found gid: " + gid);
    }
    if( uid != "" || gid != "" ) {
      int semRunID = semget(RUNKEY + _DpsServerNumber, 0, 0);
      logger.error("worker waiting for run signal");
      semGet(semRunID);
    }
  }

  _dpsServer = o.server;

  o.server->sems.getClientSem( o.localidsem, o.semNum, o.offset );

  o.getInterface().setLocalID( o.localID );
  o.getInterface().setHostname( o.hostname );
  o.getInterface().setDpsId   ( o.dpsid );
  o.getLog().setHostname( o.hostname );
  o.getLog().setDpsId( o.dpsid );

  int interfaceFound = o.getInterface().read( SBase::byKey );

  while( ! g_quit ) {

    hDevice = openUSBConnection( atoi(o.localID.c_str()) ); // numeric only?

    if( hDevice != NULL ) {
  
      error = 0; 

      if( interfaceFound != 1 ) {
        logger.error("interface not found in database, adding default record");
        time_t contacted = time(NULL);
        o.getInterface().setContacted( contacted );
        o.getInterface().setHostname( o.hostname );
        o.getInterface().setType(1);
        o.getInterface().setActive(1);
        o.getInterface().insert( false );
        int rc = o.getInterface().read( SBase::byKey ); // gets the new id number
        logger.error("have ID " + itoa(o.getInterface().getId()) + " rc " + itoa(rc));
        o.getChannel().setInterface(o.getInterface().getId());
        o.getChannel().setType(1); // digital
        o.getChannel().setDirection(1);
        o.getChannel().setActive(1);
        Channel_types ct;
        ct.setInterfacetype(1);
        rc = ct.read(SBase::byKey);
        while( rc ) {
          o.getChannel().setChannel(ct.getChannel());
          o.getChannel().insert(false);
          //o.getChannel().read( SBase::byKey ); // gets the new id number
          int id = o.getChannel().last_insert_id();
          o.getChannelDigital().setId( id ); 
          o.getChannelDigital().insert(false);
          o.getChannelAnalog().setId( id );
          o.getChannelAnalog().insert(false);
          rc = ct.readmo();
        }
      }
      o.getChannel().setInterface( o.getInterface().getId() );
      o.getChannel().setLocalId( o.localID );

      logger.error("getting dpsids");
      o.dpsids = o.getChannel().getDpsIds( o.numdpsids ) ; // gets the channels to monitor for this interface

      if( o.numdpsids > 0 ) {

        dpsLJPrint( o.numdpsids, o.dpsids );
        error = readDeviceLoop( hDevice, o );

      } else {
        logger.error("no channels found, exiting");
        closeUSBConnection( hDevice );

        return 0;
      }
      closeUSBConnection( hDevice );
      hDevice = NULL;
    }
    if( error != 0 ) {
      string s = "MechLJU3 device error, trying again on localid " + o.localID;
      logger.error(s);
      time_t t = time(NULL);
      if( lastalert == 0 || (t - lastalert > 600)) {
        lastalert = t;
        string out;
        Alerts alert;
        alert.setHostname(o.hostname);
        alert.setDateTime(time(NULL));
        alert.setSource("LabjackU3");
        alert.setList  ("default");
        alert.setAlertMsg(s);
        alert.export_data_XML( out );
        o.server->add_client_data( o, out );
      }
      o.getInterface().incrementErrors();
      semRelease(o.semDataID);

      sleep(1);
    }
  }
  if( o.dpsids )
    delete[]o.dpsids;

  return 1;
}
