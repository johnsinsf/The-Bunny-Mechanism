/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "MechLJT4.h"
#include "Channel_types.h"
#include "Alerts.h"
#include <sys/time.h>

extern "C" {
  MechLJT4* makeLJT4() {
    return new MechLJT4;
  }
}

int 
MechLJT4::dpsLJRead( int hDevice, DssObject& o ) {
  int rc = 0;
  double current_state = -1;
  int errorAddress = 0;

  if( o.numdpsids > MAX_CHANNELS_PER_INTERFACE )
    return -1;

  int j = 0;
  for( int i = 0; i < o.numdpsids; i++ ) {
    if( ! o.motor_running ) {
      o.aNames[j++] = o.dpsids[i].channel.c_str();
    } else {
      if( o.dpsids[i].channel != o.motor_channela &&
          o.dpsids[i].channel != o.motor_channelb &&
          o.dpsids[i].channel != o.motor_channelc &&
          o.dpsids[i].channel != o.motor_channeld ) {

        o.aNames[j++] = o.dpsids[i].channel.c_str();
      }
    }
  }
  rc = LJM_eReadNames( hDevice, j, (const char **)o.aNames, o.aValues, &errorAddress );

  if( rc == 0 ) {
    for( int i = 0; i < j; i++ ) {
      bool done = false;
      int x = 0;
      while( ! done ) {
        if( o.dpsids[x].channel == o.aNames[i] ) {
          done = true;
          current_state = o.aValues[i];
          
          if( o.motor_running && o.dpsids[x].type == 2 ) { // check analog inputs for jitter
            if( o.dpsids[x].dnew_state - current_state > .5 ) {
              o.dpsids[x].dnew_state = current_state;
              o.dpsids[x].inew_state = current_state;
            }
          } else {
            o.dpsids[x].dnew_state = current_state;
            o.dpsids[x].inew_state = current_state;
          }
        }
        else
        if( x++ > o.numdpsids ) {
          logger.error("channel not found " + itoa(i) + " " + o.dpsids[x].channel) ;
          done = true;
        }
      }
    }
  }
  return rc;
}

int 
MechLJT4::dpsLJMotor( int hDevice, DssObject& o ) {
  int rc = 0, errorAddress = 0;
  static int a[] = { 1,0,0,0 };
  static int b[] = { 0,0,1,0 };
  static int c[] = { 0,1,0,0 };
  static int d[] = { 0,0,0,1 };

  if( o.motor_running ) {
    struct timeval ts, tsr;
    tsr.tv_sec = tsr.tv_usec = 0;
    gettimeofday(&ts, NULL);
    if( o.motor_ts_next.tv_sec > 0 ) {
      timersub( &o.motor_ts_next, &ts, &tsr );
    }
    tsr.tv_sec = 0;
    tsr.tv_usec = 100000000;  // adjust this later based on steps remaining to speed it up
    timeradd( &ts, &tsr, &o.motor_ts_next );

    o.cNames[0] = o.motor_channela.c_str();
    o.cNames[1] = o.motor_channelb.c_str();
    o.cNames[2] = o.motor_channelc.c_str();
    o.cNames[3] = o.motor_channeld.c_str();
   
    if( ++o.current_motor_step < o.motor_duration ) {
      int i = o.current_motor_step % 4;
      if( o.motor_direction == "close" ) {
        switch(i) {
          case 0: i = 3; break;
          case 1: i = 2; break;
          case 2: i = 1; break;
          case 3: i = 0; break;
        }
      }
      o.cValues[0] = a[i];
      o.cValues[1] = b[i];
      o.cValues[2] = c[i];
      o.cValues[3] = d[i];
    } else {
      o.motor_running = 0;
      o.cValues[0] = 0;
      o.cValues[1] = 0;
      o.cValues[2] = 0;
      o.cValues[3] = 0;
    }
    logger.error("stepping " + itoa(o.current_motor_step));

    int err = LJM_eWriteNames( hDevice, 4, (const char **)o.cNames, o.cValues, &errorAddress );
    if( err > 0 ) {
      char errName[LJM_MAX_NAME_SIZE];
      LJM_ErrorToString(err, errName);
      logger.error("eWriteNames err " + string(errName));
    }
  }
  return rc;
}

int 
MechLJT4::readDeviceLoop( int hDevice, DssObject& o ) {
  int error = 0;
  int count = 0;

  o.semInitID = semget( INITKEY + _DpsServerNumber, 0, 0);
  o.semDataID = semget( DATAKEY + _DpsServerNumber, 0, 0);

  // prime the dpsids so we don't get alerts during startup
  error = dpsLJRead( hDevice, o );
  if( ! error )
    processDeviceData( o, false);

  while( ! error && ! g_quit ) {
    error = dpsLJRead( hDevice, o );

    if( ! error ) {
      processDeviceData( o, true );

      dpsLJMotor( hDevice, o );

      // 1/2 second pause 
      //int timeoutns = 500000000;

      // 250 ms
      int timeoutns = 250000000;
      int timeout = 0;
      if( o.motor_running ) {
        timeoutns = 25000000; 
        timeout = 0;
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = timeoutns - 10;
        nanosleep(&ts, NULL);
        int rc = o.server->sems.semGet( o.semNum, o.offset, 0, 10);
        if( rc == 0 ) { // should probably return a 1 here 
          logger.error("received a signal, doing commands");
          rc = handleCommand( hDevice, o );
        }
      } else {
        int rc = o.server->sems.semGet( o.semNum, o.offset, timeout, timeoutns);

        if( rc == 0 ) { // should probably return a 1 here 
          logger.error("received a signal, doing commands");
          rc = handleCommand( hDevice, o );
        }
      }
      // for display to the logs
      if( count++ > 600 ) {
        count = 0;
        string s = "worker " + o.getLocalId() + " is still running";
        logger.error("MechLJT4: " + s);
        string filename = string(INSTALLDIR) + "/status/interface_reader-" + o.getLocalId();
        utimes(filename.c_str(), NULL);
        if( o.localstorage ) {
          time_t now = time(NULL);
          o.getInterface().setContacted(now);
          o.getInterface().markContacted();
          semRelease(o.semDataID);
        }
      }
    }
  }
  logger.error("MechLJT4 readDeviceLoop returning error " + itoa(error));

  return error;
}

int
MechLJT4::handleCommand( int hDevice, DssObject& o ) {
  int errorAddress = 0;

  if( pthread_mutex_lock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return -1;
  }

  typedef multimap<string, commType>::const_iterator I2;
  for( I2 i = _dpsServer->localidComm.begin(); i != _dpsServer->localidComm.end(); ++i ) {
    logger.error("list is " + i->first);
  }
  multimap<string, commType>::const_iterator I = _dpsServer->localidComm.find( o.getLocalId() );
  int numUpdateFrames = 0;
  bool conflict = false;
  while( I != _dpsServer->localidComm.end() && numUpdateFrames < MAX_CHANNELS_PER_INTERFACE ) {

    logger.error("found command " + I->second.op + " chan " + I->second.channel + " val " + I->second.val);

    if( I->second.op == "update" ) {
      if( o.motor_running ) {
        if( o.motor_channela == I->second.channel ||
            o.motor_channelb == I->second.channel ||
            o.motor_channelc == I->second.channel ||
            o.motor_channeld == I->second.channel ) {
          conflict = true;
          logger.error("have a conflict in command, waiting");
        }
      }
      if( ! conflict ) {
        o.cNames[numUpdateFrames] = I->second.channel.c_str();
        o.cValues[numUpdateFrames] = atof(I->second.val.c_str());
        ++numUpdateFrames;
        logger.error("updating " + I->second.channel + " to " + I->second.val);
      }
    }
    else
    if( I->second.op == "start_motor" && o.motor_running ) {
      conflict = true;
      logger.error("have a conflict in command, waiting");
    } 
    else
    if( I->second.op == "start_motor" && ! o.motor_running ) {
      o.motor_channela = o.cNames[0] = I->second.channela.c_str();
      o.motor_channelb = o.cNames[1] = I->second.channelb.c_str();
      o.motor_channelc = o.cNames[2] = I->second.channelc.c_str();
      o.motor_channeld = o.cNames[3] = I->second.channeld.c_str();

      o.motor_channela = string(o.cNames[0]);
      o.motor_channelb = string(o.cNames[1]);
      o.motor_channelc = string(o.cNames[2]);
      o.motor_channeld = string(o.cNames[3]);
      //logger.error("have channela " + o.motor_channela); 
      // set all to low, i.e. high to stop gate current!
      o.cValues[0] = 1;
      o.cValues[1] = 1;
      o.cValues[2] = 1;
      o.cValues[3] = 1;
      o.current_motor_step = 0;
      o.motor_running = 1;
      o.motor_direction = I->second.direction;
      o.motor_speed = I->second.speed;
      o.motor_duration = atoi(I->second.duration.c_str());
      logger.error("duration is " + itoa(o.motor_duration));
      if( o.motor_duration < 1 ) o.motor_duration = 5; // testing!
      int err = LJM_eWriteNames( hDevice, 4, (const char **)o.cNames, o.cValues, &errorAddress );
      if( err > 0 ) {
        char errName[LJM_MAX_NAME_SIZE];
        LJM_ErrorToString(err, errName);
        logger.error("start_motor eWriteNames err " + string(errName));
        o.motor_running = 0;
      }
    }
    ++I;
  }
  if( numUpdateFrames > 0 ) {
    int err = LJM_eWriteNames( hDevice, numUpdateFrames, (const char **)o.cNames, o.cValues, &errorAddress );
    if( err > 0 ) {
      char errName[LJM_MAX_NAME_SIZE];
      LJM_ErrorToString(err, errName);
      logger.error("eWriteNames err " + string(errName));
    }
  }
  bool done = false;
  if( ! conflict ) {
    while( ! done ) {
      I = _dpsServer->localidComm.find(o.getLocalId());
      if( I != _dpsServer->localidComm.end() )  {
        _dpsServer->localidComm.erase(I);
        logger.error("erased a command");
      } else {
        done = true;
      }
    }
  }
  if( pthread_mutex_unlock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
  }

  return 0;
}

int
MechLJT4::main( DssObject& o ) {
  int       error = 1;
  int       rc    = 0;
  int       hDevice = -1;
  time_t    lastalert = 0;
  _dpsServer = o.server;

  o.semInitID = semget(INITKEY + _DpsServerNumber, 0, 0);
  o.semDataID = semget(DATAKEY + _DpsServerNumber, 0, 0);

  logger.error("t4 signaling in 1 sec " + itoa(o.semInitID) + " uid " + itoa(geteuid()) + " hostname " + o.hostname );
  //sleep(1);

  int euid = geteuid();
  logger.error("before uid " + itoa(euid));

  // signal main thread that we're running
  semRelease(o.semInitID);

  //sleep(1);

  //logger.error("after uid " + itoa(euid));

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

  o.server->sems.getClientSem( o.localidsem, o.semNum, o.offset );

  o.getInterface().setLocalID( o.localID );
  o.getLog().setHostname( o.hostname );

  int interfaceFound = false;
  if( o.localstorage ) 
    interfaceFound = o.getInterface().read( SBase::byKey );

  while( ! g_quit ) {
    if( o.ipaddress.size() )
      rc = LJM_Open(LJM_dtT4, LJM_ctTCP, o.ipaddress.c_str(), &hDevice);
    else
      rc = LJM_Open(LJM_dtT4, 0, o.localID.c_str(), &hDevice);

    logger.error("opened " + itoa(hDevice) + " " + itoa(rc));

    if( hDevice > 0 ) {

      LJM_eWriteName(hDevice, "DIO_INHIBIT", 0xF87);
      LJM_eWriteName(hDevice, "DIO_ANALOG_ENABLE", 0x000); 
      LJM_eWriteName(hDevice, "AIN0_RESOLUTION_INDEX", 0);
      LJM_eWriteName(hDevice, "AIN0_SETTLING_US", 0);

      error = 0; 

      if( o.localstorage && interfaceFound != 1 ) {
        logger.error("interface not found in database, adding default record");
        time_t contacted = time(NULL);
        o.getInterface().setContacted( contacted );
        o.getInterface().setActive(1);
        o.getInterface().setType(2);
        o.getInterface().insert( false );
        o.getInterface().read( SBase::byKey ); // gets the new id number
        o.getChannel().setInterface( o.getInterface().getId() );
        o.getChannel().setType(1);
        o.getChannel().setDirection(2);
        o.getChannel().setActive(1);
        Channel_types ct;
        ct.setInterfacetype(2);
        rc = ct.read( SBase::byKey );
        while( rc ) {
          o.getChannel().setChannel( ct.getChannel() );
          o.getChannel().setType( ct.getType() );
          o.getChannel().setDirection( ct.getDirection() );
          o.getChannel().insert(false);
          int id = o.getChannel().last_insert_id();
          logger.error("have last id " + itoa(id));
          o.getChannelDigital().setId(id);
          o.getChannelDigital().insert(false);
          o.getChannelAnalog().setId(id);
          o.getChannelAnalog().insert(false);
          rc = ct.readmo();
        }
      }
      if( o.localstorage ) {
        o.getChannel().setInterface( o.getInterface().getId() );
        o.getChannel().setLocalId( o.localID );
        logger.error("t4 getting dpsids");
        o.dpsids = o.getChannel().getDpsIds( o.numdpsids ) ; // gets the channels to monitor for this interface
      } else {
        logger.error("t4 init dpsids");
        o.dpsids = o.getChannel().getDpsIds( o.numdpsids, init_channels ) ; // gets the channels to monitor for this interface
      }

      if( o.numdpsids > 0 ) {
        error = readDeviceLoop( hDevice, o );

      } else {
        logger.error("no channels found, exiting");

        return 0;
      }
      LJM_Close(hDevice);
      hDevice = -1;
    }
    if( error != 0 ) {
      string s = "MechLJT4 device error, trying again on localid " + o.localID;
      logger.error(s);
      time_t t = time(NULL);
      if( lastalert == 0 || (t - lastalert > 600)) {
        lastalert = t;
        string out;
        Alerts alert;
        alert.setHostname(o.hostname);
        alert.setDateTime(time(NULL));
        alert.setSource("LabjackT4");
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
