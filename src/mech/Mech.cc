/*
  File:    mech/Mech.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  The Bunny Mechanism

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "Mech.h"

BaseServer *SBase::_dpsServer = NULL;
const key_t Mech::INITKEY = 0x15060001;
const key_t Mech::RUNKEY  = 0x15060012;
const key_t Mech::DATAKEY = 0x15060023;
pthread_mutex_t Mech::controlMutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef _HAVEMACOS
dispatch_semaphore_t Mech::dispatch_sem = 0;
#endif

// main loop for a "worker thread", reads the interface and signals main thread when a change occurs

int
Mech::doWork( int& threadID ) {
  logger.error("running worker " + itoa(_threadID) + " thread " + itoa(threadID));
 
  string _localID, hardware, model, maker, library, ipaddress;

  typedef map<string,string>::const_iterator I2;
  int j = 1; 
  string key;

  if( configMap.size() == 0 ) {
    logger.error("no config, exiting");
    return 0;
  }
  for( I2 i=configMap.begin(); i != configMap.end(); ++i ) {
    if( i->first.substr(0,7) == "localid" ) {
      if( j++ == _threadID ) {
        _localID = i->second;
        key = i->first;
        logger.error("found localID: " + _localID);
      }
    }
  }
  int k = key.find("[");
  int m = key.find("]", k);
  if( !k || ! m || ( m - k < 2 ) || ( m - k > 8 ) ) {
    logger.error("unable to parse config file");
    return -1;
  }
  string idx = key.substr(k, m-k+1);
  I2 i = configMap.find("hardware" + idx);
  if( i != configMap.end() ) {
    hardware = i->second;
    logger.error("found hardware: " + hardware);
  }
  i = configMap.find("model" + idx);
  if( i != configMap.end() ) {
    model = i->second;
    logger.error("found model: " + model);
  }
  i = configMap.find("library" + idx);
  if( i != configMap.end() ) {
    library = i->second;
    logger.error("found library: " + library);
  }
  i = configMap.find("maker" + idx);
  if( i != configMap.end() ) {
    maker = i->second;
    logger.error("found maker: " + maker);
  }
  i = configMap.find("ipaddress" + idx);
  if( i != configMap.end() ) {
    ipaddress = i->second;
    logger.error("found ipaddress: " + ipaddress);
  }

  i = configMap.find("localstorage");
  bool localstorage = true;
  if( i != configMap.end() && i->second == "off" ) {
    localstorage = false;
    logger.error("localstorage set");
  }

  i = configMap.find("send_data_alerts");
  bool send_data_alerts = false;
  if( i != configMap.end() && i->second == "on" ) {
    send_data_alerts = true;
    logger.error("send_data_alerts set");
  }

  if( pthread_mutex_lock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return -1;
  }

  int localidsem = sems.getUserSemaphore( threadID );

  if( pthread_mutex_unlock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
    return -1;
  }

  localidSems[_localID] = localidsem;

  int semInitID = semget(INITKEY + _DpsServerNumber, 0, 0);
  int semDataID = semget(DATAKEY + _DpsServerNumber, 0, 0);
  int semRunID  = semget(RUNKEY + _DpsServerNumber, 0, 0);

  if( _localID.size() == 0 ) {
    logger.error("localid not found in config file, exiting worker thread");
    semRelease(semInitID);
    return -1;
  } 
  if( library.size() == 0 || maker.size() == 0 ) {
    logger.error("no library/maker found, exiting");
    semRelease(semInitID);
    return -1;
  }
  dlerror();    /* Clear any existing error */

  void* h1 = dlopen( library.c_str(), RTLD_NOW );

  if (!h1 ) {
    logger.error( "setLibs failed " + string(dlerror()) + " on " + library );
    semRelease(semInitID);
    return -1;
  } 
  logger.error("launching hardware client " + hardware + " " + model);

  void* mkr = dlsym( h1, maker.c_str() );
  if( mkr ) {

    DssObject o;
    o.localidsem = localidsem;
    o.localID   = _localID;
    o.idx       = idx;
    o.threadID  = threadID;
    o.semInitID = semInitID;
    o.semDataID = semDataID;
    o.semRunID  = semRunID;
    o.ipaddress = ipaddress;
    o.model     = model;
    o.hardware  = hardware;
    o.hostname  = hostname;
    o.dpsid     = dpsid;
    o.send_data_alerts = send_data_alerts;
    o.localstorage = localstorage;
    o.server    = this;

    Mech* cl = ((Mech*(*)())(mkr))();
    int rc = cl->main( o );
    delete cl;

    return rc;
  } else {
    semRelease(semInitID);
  }
  logger.error("unable to launch client, exiting");

  return 0;
}


// main thread, starts the control thread and worker threads and communicates to the host

int
Mech::doWork( int& threadID, SocketIO* socket ) {
  bool done  = false;
  int  rc    = 0;
  int  stan  = 1;
  char c     = '\0';
  DssObject o;
  string host, localssl, localcerts;
  int port = -1;

  typedef map<string,string>::const_iterator I;

  if( configMap.size() == 0 ) {
    logger.error("no config, exiting");
    return 0;
  }

  I i = configMap.find("dsshost");
  i != configMap.end() ? host = i->second : host = "";
  
  i = configMap.find("dssmode");
  i != configMap.end() ? o.runMode = i->second : o.runMode = "remote";
  
  i = configMap.find("connectport");
  i != configMap.end() ? port = atoi(i->second.c_str()) : port = 22925;
  
  i = configMap.find("certfile");
  i != configMap.end() ? o.certFile = i->second : o.certFile = "dss.pem";

  i = configMap.find("certpass");
  i != configMap.end() ? o.certPass = i->second : o.certPass = "";

  i = configMap.find("localssl");
  i != configMap.end() ? localssl = i->second : localssl = "";

  i = configMap.find("localcerts");
  i != configMap.end() ? localcerts = i->second : localcerts = "";

  i = configMap.find("dpsid");
  if( i != configMap.end() ) {
    dpsid = o.dpsid = i->second;
  }
  i = configMap.find("siteid");
  if( i != configMap.end() ) {
    o.siteid = i->second;
  }
  i = configMap.find("companyid");
  if( i != configMap.end() ) {
    o.companyid = i->second;
  }
  i = configMap.find("hostname");
  if( i != configMap.end() ) {
    hostname = o.hostname = i->second;
  } else {
    hostname = o.hostname = get_hostname();
  }
  i = configMap.find("databasesync");
  o.databasesync = false;
  if( i != configMap.end() ) {
    if( i->second == "on" ) {
      o.databasesync = true;
      logger.error("setting database sync on");    
    }
  }
  sems.setSemKey(0x14770040 + _DpsServerNumber );

  o.getLog().setServer(this);
  o.getLog().setHostname(o.hostname);
  o.getLog().setDpsId(o.dpsid);
  o.getInterface().setDpsId(o.dpsid);
  o.getInterface().setHostname(o.hostname);
  o.getChannel().setDpsId(o.dpsid);
  o.getChannelAnalog().setDpsId(o.dpsid);
  o.getChannelDigital().setDpsId(o.dpsid);

  _MainThread = threadID;

  logger.error("starting worker");

  int numThreads = 0;

  for( I i=configMap.begin(); i != configMap.end(); ++i ) {
    if( i->first.substr(0,7) == "localid" ) 
      numThreads++;
  }
  logger.error("will launch " + itoa(numThreads) + " clients " + o.certPass + " " + o.certFile + " " + o.hostname + ".");

  o.semInitID = semget( INITKEY + _DpsServerNumber, 1, S_IRWXU );
  if( o.semInitID )
    semctl( o.semInitID, 0, IPC_RMID, 0 );

  o.semInitID = semget( INITKEY + _DpsServerNumber, 1, S_IRWXU | IPC_CREAT );

  if( o.semInitID == -1 ) {
    logger.error( "ERROR: error: semget initID" );
    return -1; 
  }
  else
    semctl( o.semInitID, 0, SETVAL, 0 );

  o.semRunID = semget( RUNKEY + _DpsServerNumber, 1, S_IRWXU );
  if( o.semRunID )
    semctl( o.semRunID, 0, IPC_RMID, 0 );
  o.semRunID = semget( RUNKEY + _DpsServerNumber, 1, S_IRWXU | IPC_CREAT );
  
  if( o.semRunID == -1 ) {
    logger.error( "ERROR: error: semget runID" );
    return -1; 
  }
  else
    semctl( o.semRunID, 0, SETVAL, 0 );

#ifdef _HAVEMACOS
  dispatch_sem = dispatch_semaphore_create(0);
#endif

  o.semDataID = semget( DATAKEY + _DpsServerNumber, 1, S_IRWXU );
  if( o.semDataID )
    semctl( o.semDataID, 0, IPC_RMID, 0 );

  o.semDataID = semget( DATAKEY + _DpsServerNumber, 1, S_IRWXU | IPC_CREAT );
  
  if( o.semDataID == -1 ) {
    logger.error( "ERROR: error: semget dataID" );
    return -1; 
  }
  else
    semctl( o.semDataID, 0, SETVAL, 0 );

  logger.error("have semDataID " + itoa(o.semDataID));

  _threadID = 0;
  int j = 0;
  while( j++ < numThreads ) {
    startThread(true, 1);
    semGet( o.semInitID );
  }
  if( geteuid() == 0 ) {
    i = configMap.find("runuid");
    if( i != configMap.end() ) {
      o.uid = i->second;
    }
    i = configMap.find("rungid");
    if( i != configMap.end() ) {
      o.gid = i->second;
    }
    if( o.uid != "" && o.gid != "" ) {
      union semun {
        int val;
        struct semid_ds *buf;
        unsigned short  *array;
      } arg;
      struct semid_ds ds;
      ds.sem_perm.uid = atoi(o.uid.c_str());
      ds.sem_perm.gid = atoi(o.gid.c_str());
      ds.sem_perm.mode = 700;
      arg.buf = &ds;
      semctl( o.semInitID, 1, IPC_SET, arg );
      semctl( o.semRunID, 1, IPC_SET, arg );
      semctl( o.semDataID, 1, IPC_SET, arg );
#ifndef _HAVEMACOS
      //seteuid( atoi(o.uid.c_str()) );
      //setegid( atoi(o.gid.c_str()) );
      setresuid( atoi(o.uid.c_str()), atoi(o.uid.c_str()), 0 );
      setresgid( atoi(o.gid.c_str()), atoi(o.gid.c_str()), 0 );
      logger.error("uid set to " + o.uid + " gid set to " + o.gid + " euid " + itoa(geteuid()));
#endif
    }
  }
  j = 0;
  // signal threads they can continue, non-root is set
  logger.error("signaling threads they can continue");
  while( j++ < numThreads ) {
    semRelease( o.semRunID );
  }

  // start control thread

  i = configMap.find("startcontrolport");
  logger.error("checking for controlport");
  if( i != configMap.end() && i->second == "on" ) {
    logger.error("controlport found");
    startThread(true, 2);
    semGet( o.semInitID );
  }
  if( socket == NULL ) {
    logger.error("creating new socket");
    socket = new SocketIO();
  } else {
    logger.error("using old socket");
  }

  // start listener thread for local clients

  logger.error("checking for listener");

  i = configMap.find("listenerport");
  if( i != configMap.end() && i->second == "on" ) {
    logger.error("starting listener");
    startThread(true, 4);

    socket->setCertName(o.certFile);
    socket->setCertPass(o.certPass);
    socket->_certRequired = true;
    socket->useSSL = true;
    logger.error("using SSL");

  } else {
    if( localssl == "on" ) {
      logger.error("using SSL");
      socket->useSSL = true;
      socket->_certRequired = false;
      if( localcerts == "on" ) {
        socket->setCertName(o.certFile);
        socket->setCertPass(o.certPass);
        socket->_certRequired = true;
        logger.error("using certs");
      }
    } else {
      socket->useSSL = false;
      logger.error("not using SSL");
    }
  }
  int hostFD = -1;
  if( host == "" ) {
    logger.error("open host failed, no host set, looping.");
    while( ! g_quit )
      sleep(15);
    return -1;
  }
  if( socket != NULL ) {
    while( hostFD < 0 && ! g_quit ) {
      hostFD = socket->openClient( host, port, true, false );
      if( hostFD <= 0 ) {
        logger.error("unable to open " + host + ", trying again.");
        socket->doClose();
        sleep(10);
      }
    }
  } else {
    logger.error( "can't connect, exiting" );
    done = true;
  } 
  if( ! hostFD ) {
    socket->doClose();
    socket->doFinish();
    delete socket;
    return -1;
  }
  bool   headerDone       = false;
  bool   headerProcessing = false;
  string auth_token, indata, dpsid, siteid;
  Icomm  hostOut, hostIn;
  time_t wakeup  = 0;
  int    infile  = -1;

  o.socket = socket;

  int  step     = SENDCLIENT;

  while( ! done & ! g_quit ) {
    if( g_quit ) step = SENDEOT;
    if( g_logLevel > 0 )
      logger.error("step " + itoa(step));
    switch( step ) {
      case GETACK:
        rc = socket->doRead( &c, 1 );
        if( ( ! rc ) || ( c != ACK ) ) {
          logger.error( "GETACK bad read:" + itoa(c) );
          step = reconnectHost(socket, host, port);
        } else {
          if( g_logLevel > 0 )
            logger.debug("ACK found:" + itoa(c) );
          step = SENDCLIENT;
        }
        break;
          
      case GETENQ:
        rc = socket->doRead( &c, 1 );
        if( ( ! rc ) || ( c != ENQ ) ) {
          logger.error( "GETENQ bad read:" + itoa(c) );
          step = reconnectHost(socket, host, port);
        } else {
          if( g_logLevel > 0 )
            logger.debug("ENQ found:" + itoa(c) );
          step = SENDCLIENT;
        }
        break;
          
      case SENDACK:  
        rc = socket->write( ACK );
        if( ! rc )
          step = reconnectHost(socket, host, port);
        else
          step  = PROCESSCLIENT;
        break;

      case SENDTESTMSG:
        step = createTestMsg(hostOut, o);
        add_host_record(o);
        break;
 
      case RECONNECTED:
        logger.error("reconnected");
        headerDone = false;
        step = SENDCLIENT;
        break;

      case RECONNECTFAILED:
        logger.error("reconnect failed");
        headerDone = false;
        sleep(10);
        step = reconnectHost(socket, host, port);
        break;

      case PROCESSCLIENT:
        wakeup = time(NULL) + _TESTMSGINTERVAL;
        hostOut.clear();
        step = processClient(infile, wakeup, hostOut, o);
        break;

      case SENDSIGNONACK:
        step = createSignonAckMsg( hostOut, o );
        break;
 
      case READCLIENT:
        if( ! socket->isOpen() ) {
          logger.error( "READCLIENT socket is not open" );
          step = PROCESSCLIENT;
        } else {
          step = SENDCLIENT;
          //logger.error("Xreading");
          rc = hostOut.read( socket, 15 );
          //logger.error("Xread " + itoa(rc));
          if( rc != 1 ) {
            if( rc != 2 ) { // not a pass char
              if( rc == 0 ) { // interrupt
                logger.error("interrupted");
                sleep(1);
                step = READCLIENT;
              } else {
                sleep(10);
                logger.error("bad read " + itoa(rc));
                step = reconnectHost(socket, host, port);
              }
            } else {
              step = PROCESSCLIENT;
              if( o.getLog().isDirty() )
                o.getLog().setClean();
              if( o.getInterface().isDirty() )
                o.getInterface().setClean();
              if( o.getChannel().isDirty() )
                o.getChannel().setClean();
              if( o.getChannelDigital().isDirty() )
                o.getChannelDigital().setClean();
              if( o.getChannelAnalog().isDirty() )
                o.getChannelAnalog().setClean();

              hostOut.clear();

              if( g_logLevel > 0 )
                logger.error("ACKed, going to processClient");
            }
          } else {
            if( headerDone ) {
              step = processHostMsg(hostOut, o);
            }
            else
            if( headerProcessing && ! headerDone ) {
              int type = hostOut.getType();
              logger.error("worker signon processing complete " + itoa(rc) + " " + itoa(type));
              if( type == Icomm::signonResponse || type == Icomm::signonAck ) {
                headerDone = true;
                headerProcessing = false;
                step = SENDSIGNONACK;
              } else {
                logger.error("bad response to signon, exiting");
                step = ATEND;
              }
            } else {
              indata = ""; // data has now been sent to host, erase it
            } 
          }
          if( rc && hostOut.test(Icomm::authData) ) {
            auth_token = hostOut.get(Icomm::authData);
            logger.debug("new auth token " + auth_token);
          }
        }
        break;

      case SENDCLIENTDATA:
        if( auth_token.size() )
          hostOut.set(Icomm::authData, auth_token);
        if( g_logLevel > 0 )
          logger.error("XXXXsendclientdata " + itoa(o.dataout.size()));
        hostOut.set(Icomm::userData, o.dpsid);
        hostOut.set(Icomm::siteid, o.siteid);
        rc = send( socket, hostOut );
        if( g_logLevel > 0 )
          logger.error("XXXXsendclientdata send1 rc " + itoa(rc));
        step = READCLIENT;
        if( ! rc ) {
          step = reconnectHost(socket, host, port);
        } else {
          if( g_logLevel >= 5 )
            logger.error("XXXXsocket writing " + o.dataout);
          int s = o.dataout.size();
          rc = socket->write( (unsigned char*)o.dataout.c_str(), s );
          if( ! rc ) {
            step = reconnectHost(socket, host, port);
          }
        }
        break;
 
      case SENDCLIENT:
        if( g_logLevel > 0 )
          logger.error("sendclient");
        step = READCLIENT;
        if( ! socket->isOpen() ) {
          logger.error("socket is not open");
          step = reconnectHost(socket, host, port);
        } 
        else 
        if( ! headerDone ) {
          headerProcessing = true;
          createHeaderMsg( hostOut, o );
          hostOut.set(Icomm::stan, stan++);
          hostOut.set(Icomm::passedOpt, Icomm::NormalThread);
          if( g_logLevel > 0 )
            logger.error("sending header");
          rc = send( socket, hostOut );
          if( ! rc ) {
            step = reconnectHost(socket, host, port);
          }
        } else {
          if( auth_token.size() )
            hostOut.set(Icomm::authData, auth_token);
          hostOut.set(Icomm::userData, o.dpsid);
          hostOut.set(Icomm::siteid, o.siteid);
          hostOut.set(Icomm::stan, stan++);
          if( g_logLevel > 0 )
            logger.error("sending msg");
          rc = send( socket, hostOut );
          if( ! rc ) {
            step = reconnectHost(socket, host, port);
          }
        }
        break;

      case SENDEOT:  
        logger.info( "sending EOT" );
        if( socket->isOpen() )
           rc = socket->write( EOT );
        step = ATEND;
        break;

      case ATEND:
        logger.error( "done ending" );
        sleep(5);
        step = SENDCLIENT; // keep looping until we connect
        break;

      default:
        logger.error( "default step: " + itoa(step) );
        done = true;
    }
  }
  logger.error( "all done");
  if( socket )  {
    socket->doClose();
    socket->doFinish();
    delete socket;
  }
  return 0;
}

// main loop for the "control thread", receives messages from host and processes them

int
Mech::doControl( int& threadID ) {
  bool done = false;
  int  step  = SENDCLIENT;
  int  rc    = 0;
  int  stan  = 1;
  string host;
  int  port = -1;
  DssObject  o;

  typedef map<string,string>::const_iterator I;
  I i = configMap.find("certfile");
  i != configMap.end() ? o.certFile = i->second : o.certFile = "dss.pem";
  i = configMap.find("certpass");
  i != configMap.end() ? o.certPass = i->second : o.certPass = "";
  i = configMap.find("dssmode");
  i != configMap.end() ? o.runMode = i->second : o.runMode = "remote";
  i = configMap.find("dsshost");
  i != configMap.end() ? host = i->second : host = "localhost";
  i = configMap.find("connectport");
  i != configMap.end() ? port = atoi(i->second.c_str()) : port = 22925;

  logger.error( "doControl Mech open host: " + itoa(threadID) + " is " + host  + " port: " + itoa(port) );

  // to let the listener start up
  sleep(5);

  SocketIO* socket = new SocketIO();
  if( o.runMode == "remote" ) {
    logger.error("using SSL");
    socket->useSSL = true;
    socket->setCertName(o.certFile);
    socket->setCertPass(o.certPass);
  }
  int hostFD = -1;
  if( socket ) {
    while( hostFD < 0 && ! g_quit ) {
      hostFD = socket->openClient( host, port, true, false );
      if( hostFD <= 0 ) {
        logger.error("unable to open " + host + ", trying again.");
        socket->doClose();
        sleep(10);
      }
    }
  } else {
    logger.error( "can't connect, exiting" );
    done = true;
  } 
  if( ! hostFD ) {
    socket->doClose();
    socket->doFinish();
    delete socket;
    return -1;
  }

  o.semInitID = semget( INITKEY + _DpsServerNumber, 0, 0);

  // signal main thread that we're running

  semRelease(o.semInitID);

  sleep(2); // just to help in debugging

  if( geteuid() == 0 ) {
    logger.error("running as root, checking config");
    i = configMap.find("runuid");
    if( i != configMap.end() ) {
      o.uid = i->second;
      logger.error("found uid: " + o.uid);
    }
    i = configMap.find("rungid");
    if( i != configMap.end() ) {
      o.gid = i->second;
      logger.error("found gid: " + o.gid);
    }
    if( o.uid != "" || o.gid != "" ) {
      int semRunID = semget(RUNKEY + _DpsServerNumber, 0, 0);
      logger.error("control waiting for run signal");
      semGet(semRunID);
    }
  }
  bool   headerDone       = false;
  bool   headerProcessing = false;
  string auth_token;
  Icomm  hostOut, hostIn;
  o.socket = socket;

  while( ! done && ! g_quit ) {
    switch( step ) {

      case SENDACK:  
        rc = socket->write( ACK );
        if( ! rc ) {
          step = reconnectHost(socket, host, port);
        } else
          step  = READCLIENT;
        break;

      case RECONNECTED:
        headerDone = false;
        step = SENDCLIENT;
        break;

      case RECONNECTFAILED:
        logger.error("control reconnect failed");
        headerDone = false;
        step = reconnectHost(socket, host, port);
        break;

      case READCLIENT:
        step = SENDACK;
        if( ! socket->isOpen() ) {
          logger.error( "control READCLIENT socket is not open" );
          step = reconnectHost(socket, host, port);
        } else {
          logger.debug("control thread reading for 305 seconds");
          rc = hostOut.read( socket, 305 );

          if( rc != 1 ) {
            if( rc == 2 ) 
              step = READCLIENT; // was a pass char, an ACK
            else {
              if( rc == 0 ) { // interrupt
                logger.error("interrupted");
                sleep(1);
                step = READCLIENT;
              } else {
                step = reconnectHost(socket, host, port);
              }
            }
          } else {
            if( headerDone ) {
              step = processHostMsg(hostOut, o);
            }
            else
            if( headerProcessing && ! headerDone ) {
              logger.debug("control signon processing complete");
              headerDone = true;
              headerProcessing = false;
              step = SENDSIGNONACK;
            } 
          }
          if( rc > 0 && hostOut.test(Icomm::authData) ) {
            auth_token = hostOut.get(Icomm::authData);
            logger.debug("control new auth token " + auth_token);
          }
        }
        break;

      case SENDSIGNONACK:
        step = createSignonAckMsg( hostOut, o );
        break;

      case SENDCLIENT:
        step = READCLIENT;
        if( ! socket->isOpen() ) {
          logger.error("socket is not open");
          rc = -1;
          step = reconnectHost(socket, host, port);
        } 
        else 
        if( ! headerDone ) {
          headerProcessing = true;
          createHeaderMsg( hostOut, o );
          hostOut.set(Icomm::stan, stan++);
          hostOut.set(Icomm::passedOpt, Icomm::ControlThread);

          rc = send( socket, hostOut );
          if( ! rc ) { 
            step = reconnectHost(socket, host, port);
          }
        } else {
          if( auth_token.size() )
            hostOut.set(Icomm::authData, auth_token);
          hostOut.set( Icomm::userData, o.dpsid );
          hostOut.set( Icomm::siteid, o.siteid );
          hostOut.set(Icomm::stan, stan++);
          rc = send( socket, hostOut );
          if( ! rc ) { 
            step = reconnectHost(socket, host, port);
          }
        }
        break;

      case SENDEOT:  
        if( socket->isOpen() )
           rc = socket->write( EOT );
        step = ATEND;
        break;

      case ATEND:
        logger.error( "done ending" );
        done = true;
        break;

      default:
        logger.error( "error step: " + itoa(step) );
        done = true;
    }
  }
  logger.error( "exiting" );
  if( socket )  {
    socket->doClose();
    socket->doFinish();
    delete socket;
  }
  return 0;
}

int
Mech::processHostCommand(string& json) {
#ifdef _USEJSON
  char* endptr = NULL;
  JsonValue value;
  JsonAllocator allocator;

  int rc = jsonParse((char*)json.c_str(), &endptr, &value, allocator);
  if( rc != 0 ) { 
    logger.error("ERROR: can't parse input");
    return SENDACK;
  }
  JsonIterator i = begin(value);
  JsonIterator j = end(value);

  string type; //op, which, localid;
  int tag = 0;
  commType c;

  while( i.p != j.p ) {
    tag  = i.p->value.getTag();

    if( tag == JSON_ARRAY || tag == JSON_OBJECT ) {   
      JsonIterator i2 = begin(i.p->value);
      JsonIterator j2 = end(i.p->value);

      while( i2.p != j2.p ) {
        tag = i2.p->value.getTag();
        type = string(i2.p->key);
        if( tag == JSON_NUMBER ) {
          long n = i2.p->value.toNumber();
          if( type == "channel" )
            c.channel = itoa(n);
          else
          if( type == "duration" )
            c.duration = n;
          else
          if( type == "value" ) {
            double d = i2.p->value.toNumber();
            c.val = ftoa(d);
          }
          else
          if( type == "localid" )
            c.localid = itoa(n);
        }
        else
        if( tag == JSON_STRING ) {
          char *p = i2.p->value.toString();
          string n(p);
          if( type == "op" )
            c.op = n;
          else
          if( type == "which" )
            c.which = n;
          else
          if( type == "localid" )
            c.localid = n;
          else
          if( type == "value" )
            c.val = n;
          else
          if( type == "channel" )
            c.channel = n;
          else
          if( type == "channela" )
            c.channela = n;
          else
          if( type == "channelb" )
            c.channelb = n;
          else
          if( type == "channelc" )
            c.channelc = n;
          else
          if( type == "channeld" )
            c.channeld = n;
          else
          if( type == "channeltrig" )
            c.channeltrig = n;
          else
          if( type == "direction" )
            c.direction = n;
          else
          if( type == "speed" )
            c.speed = n;
          else
          if( type == "duration" )
            c.duration = n;
        }
        i2.p = i2.p->next;
      }
    } 
    // now do commands
    logger.error("op " + c.op + " which " + c.which + " val " + c.val + " localid " + c.localid + " chan " + c.channel);
    doCommand(c);
    i.p = i.p->next;
  }
#endif
  return SENDACK;
}

// signal appropriate worker thread to do the command

int
Mech::doCommand( commType& c ) {
  int rc = 0;

  if( pthread_mutex_lock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return -1;
  }

  localidComm.insert(make_pair(c.localid, c));

  map<string, int>::const_iterator I = localidSems.find(c.localid);
  if( I != localidSems.end() ) {
    int semNum = 0, offset = 0;
    sems.getClientSem(I->second, semNum, offset);
    if( semNum && offset ) {
      sems.semRelease(semNum, offset);
      logger.error("sent signal to " + itoa(I->second));
    }
  } else {
    logger.error("can't find localid " + c.localid + " for command " + c.op);
    rc = -1;
  }
  if( pthread_mutex_unlock(&controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
    return -1;
  }

  return rc;
}

int
Mech::importHostData(string& xml) {
  int rc = -1;

#ifdef _USELIBXML
  DssSaxParser parser;

  try {
    parser.set_substitute_entities(true);

    if(xml.size()) {
      parser.parse_chunk(xml);
    }
    parser.finish_chunk_parsing();
    rc = 0; 
  }
  catch(const xmlpp::exception& ex) {
    logger.error( "libxml++ exception: " + string(ex.what()));
  }
  if( rc == 0 ) {
    parser.print_maps();
  } else {
    logger.error("ERROR: XML import failed, data NOT imported");
    return -1;
  }
  typedef map<string,string> smap;
  typedef map<int,smap>::const_iterator I2;
  typedef map<string,string>::const_iterator I3;
  for( I2 i = parser.attributeMap.begin(); i != parser.attributeMap.end(); ++i ) {
    bool found = false;
    string tablename;
    for( I3 j = i->second.begin(); j != i->second.end(); ++j ) {
      if( j->first == "type" && j->second == "dssObject") {
        found = true;
      } else if( j->first == "name" ) {
        tablename = j->second;
      }
      if( found && tablename.size() ) {
        rc = processTable( i->first, tablename, parser );
      }
    }  
  }  
#endif
  return rc;
}

#ifdef _USELIBXML
int 
Mech::processTable( int table_nodeid, string& t, DssSaxParser& parser ) {
  SBase* p = NULL;
  int   rc = -1;
  std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::tolower(c); });

  if( t == "interfaces" ) 
    p = new Interfaces();
  else
  if( t == "channels" )  {
    p = new Channels();
  } 
  if( t == "channels_digital" )  {
    p = new Channels_digital();
  } 
  else
  if( t == "locations" ) 
    p = new Locations();
  else {
    logger.error("invalid table name, not importing " + t);
    return rc;
  }
  eKey ek;
  ek.nodeid = table_nodeid;
  ek.node = "table";
  typedef map<eKey,eData>::const_iterator I4;
  I4 k = parser.elementMap.find(ek);
  ++k; // skip "table"

  p->load( table_nodeid, k, parser.elementMap ); 
  bool found = p->read(SBase::byKey);

  if( found ) {
    k = parser.elementMap.find(ek);
    ++k; // skip "table"
    p->load( table_nodeid, k, parser.elementMap );  //reload data that was overwritten during read
  }
  rc = p->insert( found );
  if( ! rc ) {
    logger.error("error saving data for " + t);
    delete p;
    return rc;
  }
  delete p;
 
  return 0;
}
#endif

int 
Mech::processHostData( string& s ) {
  int  prev = 0, pos = 0, hcount= 0, payloadsize = 0;
  int  len  = s.size();
  string h[6];

  pos   = s.find("\n", prev);
  if( pos > 0 && pos < len ) {
    string header = s.substr(prev,pos);
    int i = 0;
    stringstream ss(header);
    while( ss.good() && i < 6 ) {
      ss >> h[i++];
    }
    // type: xml size: 267 records: 2
    if( i == 6 && h[0] == "type:" && h[1] == "xml" ) {
      if( h[2] == "size:" ) {
        payloadsize = atoi(h[3].c_str());
      }
      if( h[4] == "records:" ) {
        hcount = atoi(h[5].c_str());
      }
      if( !hcount || !payloadsize ) {
        logger.error("header data incorrect, exiting data import");
      } else {
        string xml = s.substr(pos,len-pos);
        importHostData(xml);
      }
    }
    else
    // type: json size: 167 
    if( i == 4 && h[0] == "type:" && h[1] == "json" ) {
      string json = s.substr(pos,len-pos);
      processHostCommand(json);
    }
    else {
      logger.error("bad header, exiting data import");
    }
  } else { 
    logger.error("no header, exiting data import");
  }
  logger.debug("done with host message");

  return SENDACK;
}

int
Mech::processHostMsg( Icomm& hostIn, DssObject& o ) {
  int msgType = hostIn.getType();
  int actionCode = 0;
  int len = 0;
  if( hostIn.test(Icomm::actionCode) )
    actionCode = hostIn.getInt(Icomm::actionCode);
  if( hostIn.test(Icomm::dataLength) )
    len = hostIn.getInt(Icomm::dataLength);

  if( msgType == Icomm::fileUpdateResponse ) {
    //logger.error("host sent file update response " + itoa(actionCode));
    o.dataout = "";
    //if( actionCode != Icomm::success ) {
      //string errorText = "host rejected data update";
      //logger.error(errorText);
    //} 
    if( o.getLog().isDirty() )
      o.getLog().setClean();
    if( o.getInterface().isDirty() )
      o.getInterface().setClean();
    if( o.getChannel().isDirty() )
      o.getChannel().setClean();
    if( o.getChannelDigital().isDirty() )
      o.getChannelDigital().setClean();
    if( o.getChannelAnalog().isDirty() )
      o.getChannelAnalog().setClean();
  }
  if( (msgType == Icomm::fileUpdateResponse && len ) || (actionCode == Icomm::dataSend && len > 0 && len < _MAXDATASIZE ) ) {
    logger.error("reading data for " + itoa(len) );
    o.databuf[len] = 0;
    int rc = o.socket->doRead( o.databuf, len );
    if( rc ) {
      string s(o.databuf);
      return processHostData(s);
    }
  }
  return SENDACK;
}

int
Mech::createTestMsg ( Icomm& msg, DssObject& o ) {
  msg.setType( Icomm::testRequest );
  msg.set( Icomm::date,   getDate() );
  msg.set( Icomm::time,   getTime() );
  msg.set( Icomm::siteid, o.siteid );

  return SENDCLIENT;
}

int
Mech::createSignonAckMsg ( Icomm& msg, DssObject& o ) {
  msg.setType( Icomm::signonAck );
  msg.set( Icomm::date,   getDate() );
  msg.set( Icomm::time,   getTime() );
  if( o.siteid != "" )
    msg.set( Icomm::siteid, o.siteid );
  msg.set( Icomm::additionalData, "signon complete");
  logger.error("sending signon ack");
  return SENDCLIENT;
}

void
Mech::createHeaderMsg ( Icomm& msg, DssObject& o ) {
  msg.clear();

  msg.setType( Icomm::signonRequest );
  msg.set( Icomm::date,   getDate() );
  msg.set( Icomm::time,   getTime() );

  o.pass = "";
  o.siteid = "";
  o.dpsid = "";
  o.companyid = "";

  map<string,string>::const_iterator I = configMap.find("password_encrypted");
  if( I != configMap.end() ) {
    o.pass = configMap["password_encrypted"];
  }

  I = configMap.find("siteid");
  if( I != configMap.end() ) {
    o.siteid = configMap["siteid"];
  }
  I = configMap.find("dpsid");
  if( I != configMap.end() ) {
    o.dpsid = configMap["dpsid"];
  }
  I = configMap.find("companyid");
  if( I != configMap.end() ) {
    o.companyid = configMap["companyid"];
  }
  msg.set( Icomm::siteid, o.siteid );
  msg.set( Icomm::userData, o.dpsid );
  msg.set( Icomm::additionalData, o.pass );
  msg.set( Icomm::passedData, o.companyid );
  msg.set( Icomm::passedData2, _DSSCLIENTVERSION );

  logger.error("created header");

  return;
}

int
Mech::processClient( int fd, time_t wakeup, Icomm& comm, DssObject& o ) {
  int  recsFound = 0;
  bool semReceived = false;

  string s;
  if( o.runMode == "remote" )
    s = "<dssObject xmlns=\"http://www.dwanta.com\">\n";

  if( g_logLevel > 0 )
    logger.error("processClient " + itoa(wakeup));

  while( ! recsFound && ! g_quit ) { 
    time_t t = time(NULL);
    if( semReceived || t >= wakeup ) {
      if( g_logLevel > 0 )
        logger.error("database scan check");
      if( o.databasesync ) {
        logger.error("database scan");
        // this should be put in subclass and use add_client_data instead
        int rc = o.getLog().read(SBase::bySynced);
        while( rc ) {
          recsFound++;
          o.getLog().export_data_XML(s);
          rc = o.getLog().readmo();
        }
        rc = o.getInterface().read(SBase::bySynced);
        while( rc ) {
          recsFound++;
          o.getInterface().export_data_XML(s);
          rc = o.getInterface().readmo();
        }
        rc = o.getChannel().read(SBase::bySynced);
        while( rc ) {
          recsFound++;
          o.getChannel().export_data_XML(s);
          rc = o.getChannel().readmo();
        }
        rc = o.getChannelAnalog().read(SBase::bySynced);
        while( rc ) {
          recsFound++;
          o.getChannelAnalog().export_data_XML(s);
          rc = o.getChannelAnalog().readmo();
        }
        rc = o.getChannelDigital().read(SBase::bySynced);
        while( rc ) {
          recsFound++;
          o.getChannelDigital().export_data_XML(s);
          rc = o.getChannelDigital().readmo();
        }
      }
    } 
    // now check for any client data
    int rc = exportClientDataXML(s, o);
    if( rc > 0 ) {
      recsFound += rc;
      if( g_logLevel > 0 )
        logger.error("recs now " + itoa(recsFound));
    }
    if( ! recsFound ) {
      if( t >= wakeup ) {
        return SENDTESTMSG;
      } else {
        int n = 10;
        if( t >= wakeup - 10 ) 
          n = wakeup - t;
        if( n < 0 ) n = 10;
        if( g_logLevel > 2 )
          logger.error("wakeup in " + itoa(n) + " wakeup " + itoa(wakeup));
#ifndef _HAVEMACOS
        int i = semGet( o.semDataID, 0, n );  
        i == 0 ? semReceived = true : semReceived = false;
        if( semReceived ) {
          if( g_logLevel > 2 )
            logger.error("pinged");
          semctl( o.semDataID, 0, SETVAL, 0 );
        }
#else
        dispatch_time_t dt = dispatch_time(DISPATCH_TIME_NOW, n * NSEC_PER_SEC);
        if( g_logLevel > 2 )
          logger.error("waiting");
        int i = dispatch_semaphore_wait( dispatch_sem, dt );
        if( g_logLevel > 2 )
          logger.error("pinged");
        if( i < 0 ) 
          logger.error("dispatch error " + itoa(i));
        i == 0 ? semReceived = true : semReceived = false;
#endif
      }
    }
  }
  if( g_quit ) {
    logger.error("received quit");
    return ATEND;
  }

  o.dataout = s;

  if( o.runMode == "remote" ) {
    o.dataout += "</dssObject>\n"; 

    string header = "type: xml size: " + itoa(o.dataout.size()) + " records: 1\n";
    o.dataout = header + o.dataout;
  }
  comm.set( Icomm::dataLength, o.dataout.size() );
  comm.set( Icomm::actionCode , Icomm::dataSend );
  comm.setType( Icomm::fileUpdateRequest );

  if( g_logLevel > 2 )
    logger.error("return send client data " + itoa(o.dataout.size()));

  return SENDCLIENTDATA;
}

int
Mech::exportClientDataXML( string& out, DssObject& o ) {
  time_t now = time(NULL);

  if( pthread_mutex_lock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return -1;
  }

  map<int, dataType>::iterator I;
  int found = 0;
  int recs = 0;
  int donerecs = 0;
  bool done = false;
  string tmsg;
  string tdata;

  for( I = clientData.begin(); I != clientData.end() && ! done; ++I ) {
    recs++;
    if( I->second.synced > 0 ) 
      donerecs++;
    if( ( I->second.synced == 0 ) && ( I->second.datetime_sent == 0 || I->second.datetime_sent < now - 300 ) ) {
      if( I->second.tries++ < 3 ) {
        found++;
        o.clientDataTS = I->second.datetime_sent = now;

        // hardcoding this for now, should wait for response packet
        I->second.synced = now;

        if( tmsg == I->second.msgid ) {
          if( tdata.size() > 0 ) tdata += " ";
          tdata += I->second.data;
          if( g_logLevel > 2 )
            logger.error("tdata set");
        } else {

          string key1 = I->second.hostname + ":" + I->second.localid;
          string key2 = key1 + ":" + I->second.channel;

          clientDataMap[key1] = 1;
          clientDataMap[key2] = 1;

          out+= "<table name=\"logs\" type=\"dssObject\">\n";
          out+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
          out+= "    <type>" + itoa(I->second.datatype) + "</type>\n";
          out+= "    <hostname>" + I->second.hostname + "</hostname>\n";
          out+= "    <msgid>" +I->second.msgid + "</msgid>\n";
          out+= "    <msgtime>" + I->second.msgtime + "</msgtime>\n";
          out+= "    <localid>" + I->second.localid + "</localid>\n";
          out+= "    <channel>" + I->second.channel + "</channel>\n";
          if( tdata.size() > 0 ) 
            out+= "    <data>" + tdata + "</data>\n";
          else
            out+= "    <data>" + I->second.data + "</data>\n";
          out+= "  </table>\n";

          tmsg = I->second.msgid;
          tdata = "";
        }
      } else {
        logger.error("too many tries on rec");
        I->second.synced = now;
      }
    }
    if( out.size() > _SIZELIMIT1 ) {
      if( g_logLevel > 2 )
        logger.error("max size reached");
      done = true;
    }
  }
  if( ! done && recs ) {
    clientData.clear();
    if( g_logLevel > 0 )
      logger.error("cleared clientData");
  }
  if( recs > 100 ) {
    if( g_logLevel > 0 )
      logger.error("clientData size is " + itoa(recs));
  }
  
  // now add in any data from local clients

  if( g_logLevel > 0 )
    logger.error("localData size is " + itoa(localData.size()));

  if( localData.size() > 0 ) {
    done = false;
    recs = 0;
    map<int, localDataType>::iterator I;
    for( I = localData.begin(); ! done && I != localData.end(); ++I ) {
      recs++;
      if( g_logLevel > 0 )
        logger.error("done " + itoa(I->second.done) + " " + itoa(I->second.data.size()));
      if( I->second.done == 0 ) {
        if( out.size() + I->second.data.size() < _MAXDATASIZE ) {
          out += I->second.data;
          I->second.recs ? found += I->second.recs : found++;
          I->second.done = 1;
        } 
        else 
        if( I->second.data.size() > _SIZELIMIT1 ) {
          logger.error("DATA TOO LARGE, DROPPING " + itoa(I->second.data.size()) );
          I->second.done = 1;
        } else {
          done = true;
        }
      }
    }
    if( recs == (int)localData.size() ) {
      localData.clear();
      if( g_logLevel > 0 )
        logger.error("cleared localData");
      localrecid = 0;
    }
  }
  if( pthread_mutex_unlock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
    return -1;
  }

  return found;
}

int
Mech::reconnectHost( SocketIO*& socket, string& host, int port ) {
  if( g_quit || ! socket ) 
    return ATEND;

  socket->doClose();
  
  sleep(1);

  // set to blocking so it will return when connected
  int hostFD = socket->openClient( host, port, true, false );

  if( hostFD > 0 ) {
    logger.error("reconnected to host " + itoa(hostFD));
    return RECONNECTED;
  }
  return RECONNECTFAILED;
}

// main loop for a "listener thread", accepts connections from local clients and forwards to host

int
Mech::doWork2( int& threadID ) {
  // get the config and create object
  logger.error("running worker2 " + itoa(_threadID) + " thread " + itoa(threadID));

  SocketIO socket;
  string localssl, localcerts;

  typedef map<string,string>::const_iterator I;

  I i = configMap.find("localssl");
  if( i != configMap.end() ) 
    localssl = i->second;

  i = configMap.find("localcerts");
  if( i != configMap.end() ) 
    localcerts = i->second;

  if( localssl == "on" ) {
    socket.useSSL = true;
    logger.error("using SSL");
  }
  else
    socket.useSSL = false;

  if( localcerts == "on" )
    socket._certRequired = true;
  else
    socket._certRequired = false;

  int port = 22926;

  i = configMap.find("listenport");
  if( i != configMap.end() ) 
    port = atoi(i->second.c_str());

  int _socket = socket.openServer( port );
  if( _socket <= 0 )
    return -1;
  int errors = 0;
  bool done = false;

  while( ! done ) {
    int rc = doPoll(_socket, 10);
    if( rc == 1 ) {
      int clientFD = socket.accept( _socket );

      logger.error( "Mech local listener accepted:" + itoa(clientFD) + " err " + itoa(errno) );

      if( clientFD <= 0 ) {
        logger.error( "Mech error accepting" );
        if( errors++ > 100 ) done = true;
        sleep(1);
      } else {
        pushClientFD( clientFD, 0 );
        logger.error("starting client thread");
        startThread( true, 5 );
        errors = 0;
      }
    }
  }
  logger.error("exiting");

  return 0;
}

void
Mech::startThread( bool detachOpt, int type ) {
  pthread_t tp;
  int rc = -1;

  logger.debug( "Mech startThread " + printMe() );

  if( type == 0 )
    rc = pthread_create( &tp, NULL, runThread, (void*)this);
  else
  if( type == 1 )
    rc = pthread_create( &tp, NULL, runWorkerThread, (void*)this);
  else
  if( type == 2 )
    rc = pthread_create( &tp, NULL, runControlThread, (void*)this);
  else
  if( type == 3 )
    rc = pthread_create( &tp, NULL, runControlThread2, (void*)this);
  else
  if( type == 4 )
    rc = pthread_create( &tp, NULL, runWorkerThread2, (void*)this);
  else
  if( type == 5 )
    rc = pthread_create( &tp, NULL, runWorkerThread3, (void*)this);

  if( rc != 0 ) {
    logger.error( "error creating thread:" + itoa(rc) );
  }
  else
  if( ! detachOpt ) {
    logger.debug( "waiting for thread to finish" );
    rc = pthread_join( tp, NULL );
  } else {
    rc = pthread_detach( tp );
    if( rc != 0 ) {
      logger.error( "error detaching: " + itoa(rc) );
    } else {
      logger.info( "Mech detached thread" );
    }
  }
  return;
}

// local thread listener, accepts local client connections and forwards data to host

int
Mech::doWorker2( int& threadID ) {
  logger.error("running worker2 thread " + itoa(_threadID) + " thread " + itoa(threadID));

  int    clientFD         = getClientFD(0);
  SocketIO* socket        = NULL;
  DssObject o;

  o.semDataID = semget(DATAKEY + _DpsServerNumber, 0, 0);
  
  if( clientFD > 0 )  {
    socket = new SocketIO(clientFD); 
    socket->useSSL = true;
    if( socket->useSSL ) {
      logger.error("starting SSL");
      socket->startUpSSL();
    }
    logger.error("created new socket " + itoa(clientFD));
  } else {
    logger.error("no client fds, exiting");
    return -1;
  }

  bool done = false;
  int step = READCLIENT;
  string packetin, packetout;
  Icomm localin;

  char buf[100000];
  int rc = 0;
  while( ! done  ) {
    if( g_logLevel > 0 )
      logger.error("listener step " + itoa(step) + " " + itoa(o.signon_status));
    switch( step ) {
      case READCLIENT:
        step = SENDACK;
        localin.clear();
        rc = localin.read( socket, 305 );
        if( g_logLevel > 0 )
          logger.error("listener read " + itoa(rc) + " type " + itoa(localin.getType()));

        if( rc != 1 ) {
          if( rc == 2 ) 
            step = READCLIENT; // was a pass char, an ACK
          else {
            logger.error("local bad read");
            step = ATEND;
          }
        } else {
          step = PROCESSCLIENT;
        }
        break;

      case PROCESSCLIENT:  
        o.socket = socket;
        step = processLocalMsg( localin, o );
        localin.clear();
        break;

      case SENDACK:  
        //logger.error("control sending ACK");
        buf[0] = ACK;
        socket->write(buf, 1);
        step = READCLIENT;
        break;

      case SENDSIGNONACK:
        step = createSignonAckMsg( localin, o );
        break;

      case SENDCLIENT:
        step = READCLIENT;
        rc = send( socket, localin );
        if( ! rc ) {
          step = ATEND;
        }
        break;

      case ATEND:  
        buf[0] = 4;
        socket->write(buf, 1);
        done = true;
        break;

      default:
        logger.error( "local default step: " + itoa(step) );
        done = true;
    }
  }
  socket->doClose();
  socket->doFinish();
  delete socket;
  socket = NULL;

  return 1;
}

int
Mech::processLocalMsg( Icomm& localIn, DssObject& o ) {
  int msgType = localIn.getType();
  int actionCode = 0;
  int len = 0;
  if( localIn.test(Icomm::actionCode) )
    actionCode = localIn.getInt(Icomm::actionCode);
  if( localIn.test(Icomm::dataLength) )
    len = localIn.getInt(Icomm::dataLength);

  if( g_logLevel > 0 )
    logger.error("processLocalMsg type " + itoa(msgType) + " ac " + itoa(actionCode) + " len " + itoa(len) + " " + itoa(o.signon_status));

  if( msgType == Icomm::testRequest ) {
    return SENDACK;
  }
  else
  if( msgType == Icomm::signonRequest ) {
    // to do: some testing on password etc...
    o.signon_status = 1;
    if( g_logLevel > 0 )
      logger.error("send signon ack");
    return SENDSIGNONACK;
  }
  else
  if( msgType == Icomm::signonResponse || msgType == Icomm::signonAck ) {
    // to do: some testing on password etc...
    o.signon_status = 2;
    return SENDACK;
  }
  else
  if( o.signon_status > 0 ) {
    if( msgType == Icomm::fileUpdateRequest && actionCode == Icomm::dataSend && len > 0 && len < _MAXDATASIZE ) {
      o.databuf[len] = 0;
      
      int rc = o.socket->doRead( o.databuf, len );
      if( rc > 0 ) {
        add_client_data( o, string(o.databuf) );
      } else {
        logger.error("bad read in process local msg");
  
        //return ATEND;
        return SENDACK;
      }
    } else {
      logger.error("bad action code in process local msg " + itoa(actionCode) + " " + itoa(len));

      //return ATEND;
      return SENDACK;
    }
  } else {
    logger.error("bad action code in process local msg " + itoa(actionCode) + " " + itoa(len));

    //return ATEND;
    return SENDACK;
  }
  return SENDACK;
}

void
Mech::add_host_record( DssObject& o ) {
  string buf;
  o.getHost().export_data_XML( buf );

  if( buf.size() )
    add_client_data( o, buf );

  return;
}

void
Mech::add_client_data( DssObject& o, dataType& d ) {
  if( pthread_mutex_lock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return;
  }

  clientData[clientDataIndex++] = d;

  if( pthread_mutex_unlock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
    return;
  }

#ifdef _HAVEMACOS
  dispatch_semaphore_signal( o.server->dispatch_sem );
#else 
  semRelease(o.semDataID);
#endif 
 
  return;
}

void
Mech::add_client_data( DssObject& o, string databuf ) {

  if( pthread_mutex_lock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return;
  }

  map<int, localDataType>::iterator I;
  localDataType t;
  t.data = databuf;
  t.done = 0;
  t.recs = 0; // ??
  if( localData.find(localrecid) != localData.end() ) {
    logger.error("ERROR INSERTING LOCALDATA");
  } else {
    localData[localrecid] = t;
  }
  localrecid++;
  if( pthread_mutex_unlock( &controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
    return;
  }

#ifdef _HAVEMACOS
  //dispatch_semaphore_signal( o.server->dispatch_sem );
#else 
  semRelease(o.semDataID);
#endif 

  return;  
}
