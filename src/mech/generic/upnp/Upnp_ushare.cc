#include "Upnp_ushare.h"

extern "C" {
  Upnp_ushare* makeUpnp_ushare() {
    return new Upnp_ushare;
  }
}

extern int ushare_main( DssObject& o );

int 
Upnp_ushare::main( DssObject& o ) {

  semRelease( o.semInitID );

  logger.error("ushare checking for controlport");
  typedef map<string,string>::const_iterator I;

  I i = o.server->configMap.find("startcontrolport");

  _dpsServer = o.server;
  semRespID = o.semRespID;

  if( i != o.server->configMap.end() && i->second == "on" ) {
    logger.error("ushare controlport found");
    startThread(true, 2);
  }
  else
    logger.error("ushare controlport NOT found");

  ushare_main(o);

  return 0;
}

void
Upnp_ushare::send_data( DssObject& o, string& msg ) {

  dataType d;
  d.structtype = 0;
  d.datatype   = 5;
  d.localid    = o.localID;
  d.channel    = "ushare";
  d.synced     = 0;
  d.tries      = 0;
  d.datetime_sent = 0;
  d.data       = msg;
  d.msgid      = ltoa(msgid++);
  d.msgtime    = ltoa(time(NULL));
 
  o.server->add_client_data( o, d );

  return;
}

// main loop for the "control thread", receives messages from host and processes them

int
Upnp_ushare::doControl( int& threadID ) {

  logger.error("ushare in doControl");

  int localidsem = 0, semNum = 0, offset = 0;
  string idx, ip, localId;

  typedef map<string,string>::const_iterator I;
  I i = _dpsServer->configMap.find("configidx");

  if( i != _dpsServer->configMap.end() )
    idx = i->second;

  i = _dpsServer->configMap.find("localidsem" + idx);
  if( i != _dpsServer->configMap.end() ) {
    localidsem = atoi(i->second.c_str());
  }

  logger.error("checking avdevice" + idx);

  i = _dpsServer->configMap.find("avdevice" + idx);

  if( i != _dpsServer->configMap.end() ) {
    ip = i->second;
  }

  i = _dpsServer->configMap.find("localid" + idx);

  if( i != _dpsServer->configMap.end() ) {
    localId = i->second;
  }

  localidsem = _dpsServer->localidSems[localId];

  _dpsServer->sems.getClientSem( localidsem, semNum, offset );

  logger.error("handleCommand ip is " + string(ip) + " localid " + localId + " sem " + to_string(semNum) + " offset " + to_string(offset) + " localidsem " + itoa(localidsem));

  while ( ! g_quit ) {
    time_t timeoutns = 25000000;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = timeoutns - 10;
    nanosleep(&ts, NULL);
    int rc = _dpsServer->sems.semGet( semNum, offset, 0, 10 );
    if( rc == 0 ) { 
      logger.error("received a signal, doing commands");
      int commandrc = handleCommand( localId, localidsem, ip );
#ifdef _HAVEMACOS
      logger.error("have macos command resp " + itoa(commandrc) + " signaling " + itoa(semRespID) );
      if( ! semRespID ) {
        logger.error("dispatch error");
      } else {
        dispatch_semaphore_signal( semRespID );
        logger.error("done");
      }
#else   
      logger.error("have command resp " + itoa(commandrc) + " signaling " + itoa(semRespID) );
      semRelease( semRespID );
      logger.error("done");
#endif  
    }
  }
  return 0;
}

int
Upnp_ushare::handleCommand( string localId, int localidsem, string ip ) {

  int rc = 0;
  if( pthread_mutex_lock( &_dpsServer->controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
  } else {
    multimap<string, commType>::iterator I2 = _dpsServer->localidComm.find( localId );
    while( I2 != _dpsServer->localidComm.end() ) {

      logger.error("found command " + I2->second.op + " cmd " + I2->second.yxc_cmd + " val " + I2->second.strval);

      if( I2->second.op == "yxc" ) {
        string req = I2->second.yxc_cmd;
        string resp = sendCommand( req, ip );
        I2->second.resp = resp;
        I2->second.status = 2;
        logger.error("have resp " + resp);
        rc++;
      }
      ++I2;
    }
  }
  if( pthread_mutex_unlock( &_dpsServer->controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
  }
  return rc;
}

string
Upnp_ushare::sendCommand( string req, string ip ) {

  if( req.size() && ip.size() ) {
    SocketIO bunny_sock;
    bunny_sock.useSSL = false;

    int fd = bunny_sock.openClient( ip, 80 );

    logger.error("have fd " + to_string(fd) + " req " + req);
 
    LObj obj;
   
    string fetch, id;
    if( fd >= 0 ) {
      fetch = "GET " +  req + " HTTP/1.0\n";
      fetch += "host: " + ip + "\n\n";
    
      logger.error("using buf " + fetch);
      bunny_sock.write(fetch.c_str(), fetch.size());
  
      readHeader( &bunny_sock, obj );
    }
    if( obj.packet.size() > 0 ) {
      logger.error("have packet " + obj.packet);
      return obj.packet;
    }
  }
  return "failed";
}
