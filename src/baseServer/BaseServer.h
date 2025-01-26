/*
  File:    baseServer/BaseServer.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __BaseServer
#define __BaseServer

#include <stack>
#include <sys/poll.h>

#include "dpsframework.h"
#include "Icomm.h"
#include "SocketIO.h"
#include "DssSems.h"

class Site;
class Device;

class BaseServer {

  public:

    enum {
      ATEND,      // 0
      SENDENQ,    // 1
      SENDACK,    // 2
      READCLIENT,  // 3
      SENDCLIENT,  // 4
      READHOST,    // 5
      SENDHOST,    // 6  
      PROCESSCLIENT,  // 7
      GETENQ,    // 8
      GETACK,    // 9
      SENDEOT,    // 10
      READHEADER,  // 11
      SENDNAK,    // 12
      RUNCONTROLTHREAD,    // 13
      SENDTESTMSG,    //
      RECONNECTED,    //
      RECONNECTFAILED,    //
      SENDBATCH,    // 
      SENDBATCHDONE,    // 
      SENDBATCHDONEREC,    // 
      SENDBATCHREC,    //  20
      SENDCLIENTINIT,  // 
      SENDCLIENTREINIT, // 
      SENDCLIENT2,       // 
      SENDCLIENTERR,       // 
      SENDCLIENTDATA,
      READCLIENTDATA,
      SENDSIGNONACK,
      WAITFORSIGNAL // 28
   };
                                                                                
    enum {
      NoAuthRequired,
      AuthRequired,
      IsoAuthRequired,
      IsoRegistration
    };
/*
    struct LookupType{
      unsigned int siteid;
      unsigned int stan;
    };
*/

    BaseServer( void ) { 
      myRandState   = 0;
      serverType    = NoAuthRequired;
      libmydps_handle = NULL;
      libdpsserver_handle = NULL;
      _threadID     = 0;
      _MainThread     = 0;
      _euid = 0;
      _egid = 0;
    }

    virtual ~BaseServer( void ) {
    };

    BaseServer( const BaseServer& b ) {

      libmydps_handle   = b.libmydps_handle;  
      libdpsserver_handle = b.libdpsserver_handle;  
      serverType        = b.serverType;
      shmMutex          = b.shmMutex;
      shmShlibMutex     = b.shmShlibMutex;
      _threadID         = b._threadID;
      clientFDs[0]      = b.clientFDs[0];
      clientFDs[1]      = b.clientFDs[1];
      clientFDs[2]      = b.clientFDs[2];
      _MainThread       = b._MainThread;
    }

    static void* runThread        ( void* a );
    static void* runWorkerThread  ( void* a );
    static void* runWorkerThread2 ( void* a );
    static void* runWorkerThread3 ( void* a );
    static void* runControlThread ( void* a );
    static void* runControlThread2( void* a );
    static void* runControlThread3( void* a );

    virtual void run     ( int port, int port2, int port3, bool useSSL = false, bool certReq = false, int euid = 0, int egid = 0 ) {};
    virtual void run     ( int port, int port2, bool useSSL = false, bool certReq = false, int euid = 0, int egid = 0 ) {};
    virtual void run     ( int fd, bool useSSL = false, bool certReq = false, int euid = 0, int egid = 0 );
    virtual void run     ( void );
    virtual void runTest ( void );
    virtual void setSeed ( int s ) { mySeed = s; };
    virtual void setTag  ( string s ) { tag = s; };
    virtual void setServerType( int t ) { serverType = t; };
    virtual void setServer( BaseServer* p ) { _dpsServer = p; }
    virtual bool checkLibs  ( time_t& ts );
    virtual void* setLibs  ( void );
    virtual void setLogLevel  ( int l ) { g_logLevel = l; }

    virtual void initThreadSignals( int tid, int tgid );

    void* get_mydps_handle( void ) { return libmydps_handle; };
    void* get_dpsserver_handle( void ) { return libdpsserver_handle; };

    virtual void init ( void );
    int  semGet       ( int s, int offset = 0, int timeout = MAXSEMWAIT, int timeoutns = 0 );
    void semRelease   ( int s, int offset = 0 );

    int  doPoll       ( int fd, int timeOut );

    virtual void dpsVersion  ( void );

    virtual int    doWorkClientPacket( Icomm& hostIn,
                                       Icomm& hostOut,
                                       Site* site,
                                       Device* device,
                                       pid_t threadID ) { return 0; };

    virtual string printMe( void ) { return string("baseServer"); };
    virtual void setLibDps( void* handle ) { libmydps_handle = handle; };

    virtual void loadConfigMap( string configFile );
    virtual map<string,string>& getConfigMap( void ) { return configMap; };

    map<string, string>   configMap;

    static const  key_t  SEMKEY;
    static const  int    MAXSEMTRIES;
    static const  int    MAXSEMWAIT;

    static int   _DpsControlPort;

  protected:

    virtual void   doFinish  ( pid_t inThreadID, SocketIO* socket );
    virtual int    doWork    ( int& inThreadID, SocketIO* socket ) { return 0;}
    virtual int    doWork    ( int& inThreadID ) { return 0;}
    virtual int    doWork2   ( int& inThreadID ) { return 0;}
    virtual int    doWorker2 ( int& inThreadID ) { return 0;}
    virtual int    doControl ( int& inThreadID ) { return 0;}
    virtual int    doControl2( int& inThreadID ) { return 0;}
    virtual int    doControl3( int& inThreadID ) { return 0;}
    virtual pid_t  initThread();
    virtual void   startThread( bool detachOpt = true, int type = 0 );
    virtual void   setThreadType( int type ) { _threadType = type; };
    virtual int    getSeed    ( void ) { return mySeed; }

    virtual int    getClientFD( int i = 0 );
    virtual int    getThreadType( void ) { return _threadType; }
    virtual void   pushClientFD( int fd, int i = 0 );

    virtual bool   send ( SocketIO* socket, Icomm& hostOut );

    //virtual void   get_hostname( void );

    int  _threadID; // not the thread id, just a count, shouldn't be used
    int  _threadType; // to select thread start function

    pid_t _MainThread;
    int   _euid;
    int   _egid;

    static  int  _DpsPort;
    static  int  mySeed;
    static  int  serverSemID;
    unsigned  int  myRandState;
#ifndef _HAVEMACOS
    struct  drand48_data drand48_buf;
#endif
    int    serverType;
    BaseServer* _dpsServer;

    void    *libmydps_handle;
    void    *libdpsserver_handle;

    // used for passing the file descriptors to the threads
    stack<int>    clientFDs[3];

    // needed to lock access to clientFDs
    static  pthread_mutex_t  shmMutex;
    static  pthread_mutex_t  shmShlibMutex;
    map<unsigned int,unsigned int>   siteCompanyMap;
    static  string tag;
    DssSems sems;
    string  certFile;
    string  certPass;
    string  certCAFile;
    string  hostname;
};
/*
class lookupCmp {
     public:
          bool operator () ( const BaseServer::LookupType&,
                             const BaseServer::LookupType& ) const;
};
*/

#endif
