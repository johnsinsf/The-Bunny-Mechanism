/*
  File:    mech/Mech.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Mech
#define __Mech

#include "dpsframework.h"
#include "BaseServer.h"
#include "Icomm.h"
#include "Logs.h"
#include "Channels.h"
#include "Channels_digital.h"
#include "Channels_analog.h"
#include "Hosts.h"
#include "Interfaces.h"
#include "Locations.h"
#include "DssSems.h"
#ifdef _USELIBXML
#include "DssSaxParser.h"
#endif

#ifdef _USEJSON
#include "Json.h"
#endif

#ifdef _HAVEMACOS
#include <dispatch/dispatch.h>
#endif

#define _DSSCLIENTVERSION "0.0.1"

#define _TESTMSGINTERVAL 200
#define _SIZELIMIT1 32000
#define _SIZELIMIT2 64000
//#define _SIZELIMIT1 64000
#define _MAXDATASIZE 256000

typedef struct {
  int    structtype;
  int    datatype;
  string hostname;
  string msgid;
  string msgtime;
  string localid;
  string channel;
  string data;
  int    datetime_sent;
  int    synced;
  int    tries;
} dataType;

typedef struct {
  string data;
  int    done;
  int    recs;
} localDataType;
 
typedef struct {
      string localid;
      string op;
      string which;
      string val;
      string duration;
      string direction;
      string speed;
      string channel;
      string channela;
      string channelb;
      string channelc;
      string channeld;
      string channeltrig;
} commType;

class Mech;

class DssObject {
  public:
    DssObject( void ) {
      socket = NULL;
      semNum = 0;
      offset = 0;
      timestamp = 0;
      clientDataTS = 0;
      dpsids    = NULL;
      numdpsids = 0;
      semInitID = 0;
      semDataID = 0;
      semRunID = 0;
      threadID  = 0;
      localidsem = 0;
      motor_running = 0;
      motor_duration = 0;
      current_motor_step = 0;
      motor_ts_next.tv_sec = 0;
      motor_ts_next.tv_usec = 0;
      server = NULL;
      signon_status = 0;
      databasesync = false;
      send_data_alerts = false;
      localstorage = true;
      memset(databuf,0,sizeof(databuf));
    }
    void setLocalId  ( string s ) { localID = s; }
    void setModel    ( string s ) { model = s; }
    void setHardware ( string s ) { hardware = s; }
    void setLog      ( Logs& o )  { log = o; }
    void setHost     ( Hosts& o )  { host = o; }
    void setChannel  ( Channels& o ) { channel = o; }
    void setInterface( Interfaces& o ) { interface = o; }
    void setHostname ( string s ) { hostname = s; }
 
    Interfaces& getInterface ( void ) { return interface; }
    Hosts& getHost           ( void ) { return host; }
    Channels& getChannel     ( void ) { return channel; }
    string getLocalId        ( void ) { return localID; }
    string getHostname       ( void ) { return hostname; }
    Channels_analog& getChannelAnalog  ( void ) { return channel_analog; }
    Channels_digital& getChannelDigital( void ) { return channel_digital; }
    Logs& getLog   ( void ) { return log; }

    SocketIO* socket;
    Mech* server;
    string dataout;
    time_t timestamp;
    string pass, siteid, companyid, dpsid, uid, gid;
    int semInitID;
    int semDataID;
    int semRunID;
    int semNum;
    int offset;
    int clientDataTS;
    int numdpsids;
    dpsID* dpsids;
    ofstream outfile;
    const char* aNames[MAX_CHANNELS_PER_INTERFACE];
    double aValues[MAX_CHANNELS_PER_INTERFACE];
    const char* cNames[MAX_CHANNELS_PER_INTERFACE];
    double cValues[MAX_CHANNELS_PER_INTERFACE];
    string hostname;
    string localID;
    string model;
    string hardware;
    string ipaddress;
    string certFile;
    string certPass;
    string runMode;
    bool   databasesync;
    bool   send_data_alerts;
    bool   localstorage;
    pid_t  threadID;
    int    localidsem;
    string motor_channela;
    string motor_channelb;
    string motor_channelc;
    string motor_channeld;
    string motor_channeltrig;
    string motor_direction;
    string motor_speed; 
    int    motor_duration;
    int    motor_running;
    int    current_motor_step;
    struct timeval motor_ts_next;
    multimap<string, commType> *localidComm;
    int    signon_status;
    map<string, int> clientDataMap;
    string idx;
    char        databuf[_MAXDATASIZE];

  protected:
    Channels_digital channel_digital;
    Channels_analog  channel_analog;
    Channels    channel;
    Interfaces  interface;
    Hosts       host;
    Logs        log;
};

class Mech : public BaseServer {
  public:
    Mech( void ) : BaseServer() { 
      clientDataIndex = 0;
      localrecid      = 0;
    }
    ~Mech( void ) {
    }

    virtual int  main( DssObject& o ) { return 0;}

    virtual int  doWork( int& inThreadID, SocketIO* socket ); // client/server thread
    virtual int  doWork( int& inThreadID ); //  interface data collection thread
    virtual int  doWork2( int& inThreadID );  //  local device data collection thread
    virtual int  doWorker2( int& inThreadID );  //  local device data collection thread
    virtual int  doControl( int& inThreadID ); // host control thread

    virtual void createHeaderMsg ( Icomm& msg, DssObject& o );
    virtual int  createSignonAckMsg( Icomm& msg, DssObject& o );
    virtual int  createTestMsg   ( Icomm& msg, DssObject& o );
    virtual int  processHostMsg  ( Icomm& msg, DssObject& o );
    virtual int  processHostData ( string& s );
    virtual int  importHostData  ( string& xml );
    virtual int  processHostCommand( string& json );
    virtual int  processClient   ( int infile, time_t wakeup, Icomm& comm, DssObject& o );
    virtual int  processLocalMsg ( Icomm& localIn, DssObject& o );
    virtual int  reconnectHost   ( SocketIO*& socket, string& authHost, int authPort );
    virtual void add_client_data   ( DssObject& o, dataType& d );
    virtual void add_client_data   ( DssObject& o, string databuf );
    virtual void add_host_record   ( DssObject& o );
    virtual int  exportClientDataXML( string& out, DssObject& o );
    virtual int  getClientDataIndex ( void ) { return clientDataIndex++; }
    virtual void startThread       (bool detach, int type);

#ifdef _USELIBXML
    virtual int  processTable    ( int nodeid, string& tablename, DssSaxParser& parser );
#endif

    virtual int  doCommand       ( commType& c );
    virtual void setServer       ( Mech* i ) { _dpsServer = i; }

    multimap<string, commType> localidComm;
    map<int, dataType> clientData;
    map<string, int> clientDataMap;
    map<int, localDataType> localData;
    Mech* _dpsServer;
    DssSems sems;
    int localrecid;
    string dpsid;

  protected:
    int    clientDataIndex;

    map<string, int> localidSems;  // localid, semid
    static const  key_t  INITKEY;
    static const  key_t  DATAKEY;
    static const  key_t  RUNKEY;
    static pthread_mutex_t controlMutex;
#ifdef _HAVEMACOS
    static dispatch_semaphore_t dispatch_sem;
#endif
};

#endif
