/*
  File:    baseServer/BaseServer.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include <fstream>
#include <dlfcn.h>
#include "dpsframework.h"
#include "BaseServer.h"
#include "dpsinit.h"
#include "openssl/ssl.h"
#include <sys/types.h> 
#include <sys/syscall.h> 
#include <fstream>
#include <algorithm>
#include <cctype>

pthread_mutex_t BaseServer::shmMutex      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t BaseServer::shmShlibMutex = PTHREAD_MUTEX_INITIALIZER;

const key_t  BaseServer::SEMKEY = 0x13760060;
const key_t  BaseServer::MAXSEMTRIES = 3;
const key_t  BaseServer::MAXSEMWAIT = 15;

int  BaseServer::serverSemID     = -1;
int  BaseServer::_DpsPort        = 0;
int  BaseServer::_DpsControlPort = 0;
int  BaseServer::mySeed          = 0;
string BaseServer::tag;

bool
BaseServer::send( SocketIO* socket, Icomm& hostOut ) {
  ostringstream o;
  hostOut.print( o );

  int len = o.str().length();
  char buf[len+4];
  memset( buf, '\0', len + 4 );  
  unsigned char lrc;

  lrc = 0;
  buf[0] = STX;
  memcpy( (char*)&buf[1], o.str().c_str(), len );

  for( int i = 1; i < len + 1; i++ )
    lrc ^= buf[i];

  lrc ^= ETX;

  buf[len+1] = ETX;
  buf[len+2] = lrc;
  buf[len+3] = '\0';
    
  if( ! hostOut.useBCD ) {
    //logger.debug( "sending packet: " + itoa(len) + " " + o.str() + " lrc " + itoa(lrc) );
  } else {
    char tbuf[8];
    unsigned char lbuf[3];
    snprintf( tbuf, 8, "%.4d", len );
    socket->strToBcd( tbuf, 4, lbuf );
    logger.debug( "send len: " + string(tbuf) );
    // send the two byte bcd length
    socket->write( (char*)&lbuf, 2 );
  }
  return socket->write( buf, len+3 );
}

bool
BaseServer::checkLibs( time_t& ts ) {

  if( ( libmydps_handle != NULL ) && ( ! g_reload_lib ) ) {
    logger.error( "exiting setLibs, no reload " );
    return false;
  }

  bool doReload = false;
  if(  g_reload_lib ) {
    struct stat stbuf;
    string filename = string(INSTALLDIR) + "/lib/libfile";
    if( stat( filename.c_str(), &stbuf ) == 0 ) {
      logger.debug( "lib file found: " + filename  );
      if( ts == 0 ) {
        logger.debug( "reload lib date 0: " + itoa(ts) + " " + itoa(stbuf.st_mtime));
        ts = stbuf.st_mtime;
      }
      else
      if( stbuf.st_mtime > ts ) {
        doReload = true;
        logger.debug( "reload lib date diff: " + itoa(ts) + " " + itoa(stbuf.st_mtime));
        ts = stbuf.st_mtime;
      }
    }
  }
  return doReload;
}

void *
BaseServer::setLibs( void ) {

  if( ( libmydps_handle != NULL ) && ( ! g_reload_lib ) ) {
    logger.error( "exiting setLibs, no reload " );
    return libmydps_handle;
  }
  return resetHandles( &libdpsserver_handle, &libmydps_handle );
}

void
BaseServer::doFinish( pid_t inThreadID, SocketIO* socket ) {

  //logger.error( "BaseServer doFinish: " + itoa(inThreadID) + " sock " + itoa(socket) );

  if( libmydps_handle == NULL ) {
    //logger.error( "handle is null " + itoa(inThreadID) );
  }
  if( socket ) {
    //logger.error("BaseServer closing sockets");
    socket->doClose();
    //socket->doFinish();
    delete socket;
    socket = NULL;
  }
  return;
}

void
BaseServer::initThreadSignals( int tid, int tgid ) {

  struct sigaction sigs, osigs;
  sigs.sa_handler = dpsSigHandler;
  sigemptyset(&sigs.sa_mask);
  sigs.sa_flags = SA_RESTART;
  int rc = sigaction(SIGINT, &sigs, &osigs);
  if( rc != 0 )
    logger.error("error in sigaction: " + itoa(rc));

  sigset_t iset;
  sigemptyset(&iset);
  sigaddset(&iset, SIGINT);
  sigaddset(&iset, SIGPIPE);
  pthread_sigmask( SIG_BLOCK, &iset, NULL );
  pthread_sigmask( SIG_SETMASK, &iset, NULL );

  logger.debug("initThreadSignals rc " );

  return;
}

void* 
BaseServer::runWorkerThread( void* a ) {

  BaseServer *p = (BaseServer*)a;

  pid_t threadID  = p->initThread();
  pid_t threadGID = getpid();

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  logger.info( "BaseServer runWorkerThread tid "  + itoa(threadID) + " tgid " + itoa(threadGID)  + " tag " + tag + " " + p->printMe());

  p->initThreadSignals( threadID, threadGID );

  p->doWork( threadID );

  logger.info( "BaseServer thread closing:" + itoa(threadID));

  int retVal;
  pthread_exit( (void*)&retVal );

  logger.debug( "thread exit:" + itoa(retVal) );

  return NULL;
} 

void* 
BaseServer::runWorkerThread2( void* a ) {

  BaseServer *p = (BaseServer*)a;

  pid_t threadID  = p->initThread();
  pid_t threadGID = getpid();

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  logger.error( "BaseServer runWorkerThread2 tid "  + itoa(threadID) + " tgid " + itoa(threadGID)  + " tag " + tag + " " + p->printMe());

  p->initThreadSignals( threadID, threadGID );

  p->doWork2( threadID );

  logger.error( "BaseServer thread closing:" + itoa(threadID));

  int retVal;
  pthread_exit( (void*)&retVal );

  logger.debug( "thread exit:" + itoa(retVal) );

  return NULL;
} 

void* 
BaseServer::runWorkerThread3( void* a ) {

  BaseServer *p = (BaseServer*)a;

  pid_t threadID  = p->initThread();
  pid_t threadGID = getpid();

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  logger.error( "BaseServer runWorkerThread2 tid "  + itoa(threadID) + " tgid " + itoa(threadGID)  + " tag " + tag + " " + p->printMe());

  p->initThreadSignals( threadID, threadGID );

  p->doWorker2( threadID );

  logger.error( "BaseServer thread closing:" + itoa(threadID));

  int retVal;
  pthread_exit( (void*)&retVal );

  logger.debug( "thread exit:" + itoa(retVal) );

  return NULL;
} 

void* 
BaseServer::runControlThread( void* a ) {

  BaseServer *p = (BaseServer*)a;

  pid_t threadID  = p->initThread();
  pid_t threadGID = getpid();

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  logger.error( "BaseServer runControlThread tid "  + itoa(threadID) + " tgid " + itoa(threadGID)  + " tag " + tag); 

  p->initThreadSignals( threadID, threadGID );

  p->doControl( threadID );

  logger.info( "BaseServer thread closing:" + itoa(threadID));

  int retVal;
  pthread_exit( (void*)&retVal );

  logger.debug( "thread exit:" + itoa(retVal) );

  return NULL;
} 

void* 
BaseServer::runControlThread2( void* a ) {

  BaseServer *p = (BaseServer*)a;

  pid_t threadID  = p->initThread();
  pid_t threadGID = getpid();

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  logger.error( "BaseServer runControlThread2 tid "  + itoa(threadID) + " tgid " + itoa(threadGID)  + " tag " + tag + " " + p->printMe());

  p->initThreadSignals( threadID, threadGID );

  logger.error("threadID address " + itoa(&p));

  p->doControl2( threadID );

  logger.error( "BaseServer thread closing:" + itoa(threadID));

  int retVal;
  pthread_exit( (void*)&retVal );

  logger.debug( "thread exit:" + itoa(retVal) );

  return NULL;
} 

void* 
BaseServer::runControlThread3( void* a ) {

  BaseServer *p = (BaseServer*)a;

  pid_t threadID  = p->initThread();
  pid_t threadGID = getpid();

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  logger.error( "BaseServer runControlThread3 tid "  + itoa(threadID) + " tgid " + itoa(threadGID)  + " tag " + tag + " " + p->printMe());

  p->initThreadSignals( threadID, threadGID );

  logger.error("threadID address " + itoa(&p));

  p->doControl3( threadID );

  sleep(5); //testing

  logger.error( "BaseServer thread closing:" + itoa(threadID));

  int retVal;
  pthread_exit( (void*)&retVal );

  logger.debug( "thread exit:" + itoa(retVal) );

  return NULL;
} 

void* 
BaseServer::runThread( void* a ) {

  BaseServer *p = (BaseServer*)a;
  pid_t threadID  = p->initThread();
  pid_t threadGID = getpid();
  int clientFD    = p->getClientFD(0);

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  logger.info( "BaseServer runThread tid "  + itoa(threadID) + " tgid " + itoa(threadGID)  + " tag " + tag ); 

  SocketIO* socket = NULL;

  if( clientFD > 0 ) 
    socket = new SocketIO(clientFD);

  p->initThreadSignals( threadID, threadGID );

  if( g_useSSL ) {
    if( clientFD >= 0 ) {
      logger.error( "using ssl" );
      socket->useSSL = true;
      int rc  = socket->startUpSSL();
      logger.error( "using ssl " + itoa(rc) );
      if( rc != 1 ) {
        logger.error("bad SSL, exiting");
        socket->doClose();
        delete socket;
        return NULL;
      }
    } else {
      logger.error( "no client fd, not using ssl" );
    }
  } else {
    socket->useSSL = false;
  }
  logger.error( "BaseServer runThread socket "  + itoa(socket));

  // if a lot of test threads, give them a chance to load before working
  if( g_testMode && ( numThreads > 50 ) )
    sleep( 1 + ( threadID / 100 ) );

  struct stat stbuf;
  string filename = string(INSTALLDIR) + "/bin/debugging";
  if( stat( filename.c_str(), &stbuf ) == 0 ) {
    logger.debug( "debugging file found"  );
    g_debugging = true;
  }
  if( p != NULL ) {
    p->doWork( threadID, socket );
    if( socket != NULL ) {
      p->doFinish( threadID, socket );
    }
  }
  int retVal = 0;
  pthread_exit( (void*)&retVal );

  logger.debug( "thread exit:" + itoa(retVal) );

  return NULL;
} 

pid_t
BaseServer::initThread() {

  if( _threadID++ > 32000 )  // not used
    _threadID = 1;
  
  pid_t tid;
  tid = syscall(SYS_gettid);

  logger.info( "BaseServer set threadID to " + itoa(tid) );

  return tid;
}

void
BaseServer::init() {

  logger.debug( "BaseServer init" );

  if( serverSemID != -1 )
    return;

  // try open existing first
  serverSemID = semget( BaseServer::SEMKEY, 1,  S_IRWXU );

  if( serverSemID < 1 ) {
    logger.debug( "BaseServer creating semaphore" );
     serverSemID = semget( BaseServer::SEMKEY, 1,  S_IRWXU | IPC_CREAT );
  }
  if( serverSemID < 1 ) {
    logger.error( "BaseServer semaphore FAILED" );
  } else {
    int rc = semctl( serverSemID, 0, SETVAL, 1 );
    logger.debug( "BaseServer semaphore set, " + itoa(rc) );
  }
  return;
}

void
BaseServer::startThread( bool detachOpt, int type ) {

  pthread_t tp;
  int rc = -1;

  logger.error( "BaseServer startThread " + printMe() );

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
  else
  if( type == 6 )
    rc = pthread_create( &tp, NULL, runControlThread3, (void*)this);

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
      logger.info( "BaseServer detached thread" );
    }
  }
  return;
}

int
BaseServer::doPoll( int fd, int timeOut ) {

  struct pollfd fds[1];
  fds[0].fd       = fd;
  fds[0].events   = POLLIN | POLLPRI;
  fds[0].revents  = 0;

  errno = 0;
  int t = -1;

  // convert to seconds
  if( timeOut )
    t = timeOut * 1000;

  int retval = ::poll( fds, 1, t );

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
    //logger.error( "doPoll timedout: " + itoa(t) + " to: " + itoa(fd) );
  }
  else
  if( retval == 1 ) {
    if( !( fds[0].revents & POLLIN ) ) {
      logger.error( "doPoll failed: " + itoa(errno) );
      return -1;
    }
    //logger.error( "doPoll OK");
  }
  return retval;
}
/*
  should use something like a string structure to pass in parms
  thread0:
    Port: number;
    host: name;
    certfile: name;
    ssl: bool;
    certreq: bool;
    euid: num;
    egid: num;
  thread1:
    Port: number;
    host: name;
    certfile: name;
    ssl: bool;
    certreq: bool;
    euid: num;
    egid: num;

*/
  
void
BaseServer::run( int port, bool useSSL, bool certReq, int euid, int egid ) {

  this->init();
  _DpsPort = port;   // for logging later on
  map<string,string>::const_iterator i = configMap.find("certfile");
  i != configMap.end() ? certFile = i->second : certFile = string(INSTALLDIR) + "/ssl/dss.pem";
  i = configMap.find("certcafile");
  i != configMap.end() ? certCAFile = i->second : certCAFile = "";

  logger.info( "BaseServer accepting on port:"  + itoa(port));

  SocketIO socket;
  socket.useSSL = useSSL;
  socket.setCertName( certFile );
  socket.setCertCAName( certCAFile );

  int _socket = socket.openServer( port, euid, egid );
  if( _socket <= 0 )
    g_quit = true;

  while( ! g_quit ) {

    int rc = doPoll(_socket, 10);
    if( rc == 1 ) {
      logger.error( "BaseServer accepting " + itoa(libmydps_handle) );

      int clientFD = socket.accept( _socket );
      pushClientFD( clientFD, 0 );

      logger.error( "BaseServer accepted:" + itoa(clientFD) );

      if( clientFD <= 0 ) {
        logger.error( "BaseServer error accepting" );
        g_quit = true;
      } else {
        logger.error( "BaseServer starting thread " + itoa(libmydps_handle) );
        startThread( true );
      }
    }
  }
  logger.info( "BaseServer exiting, received quit" );

  return;
}

void
BaseServer::run( void ) {

  this->init();

  startThread( false );

  logger.info( "BaseServer ending" );

  return;
}

void
BaseServer::runTest( void ) {

  this->init();

  for( int i = 0; i < numThreads; i++ ) {
    startThread( true );
  }
  // wait here until signaled
  pause();

  return;
}

int 
BaseServer::getClientFD( int i ) {

  int rc = -1;

  if( pthread_mutex_lock( &shmMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return -1;
  }

  if( ! clientFDs[i].empty() ) {
    rc = clientFDs[i].top();
    clientFDs[i].pop();
  } else {
    logger.error( "no clientFDS!");
  }
  if( pthread_mutex_unlock( &shmMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
    return -1;
  }

  logger.debug( "returning clientFD: " + itoa(rc) );
 
  return rc;
}

void
BaseServer::pushClientFD( int fd, int i ) {

  logger.error( "pushing clientFD: " + itoa(fd) );

  if( fd < 0 )
    return;

  if( pthread_mutex_lock( &shmMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
    return;
  }

  clientFDs[i].push( fd );

  if( pthread_mutex_unlock( &shmMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
    return;
  }

  return;
}

#ifndef _HAVEMACOS
int
BaseServer::semGet( int s, int offset, int timeout, int timeoutns ) {

  if( g_debugging )
    logger.error( "locking mutex: " + itoa(s) + " off: " + itoa(offset) );

  struct sembuf semOp;
  semOp.sem_num = offset;
  semOp.sem_op  = -1;
  semOp.sem_flg = 0;
  int timeleft = timeout;
 
  int rc   = -1;
  struct timespec ts;
  bool timedOut = false;

  // loops every 10 seconds if using a timeout to allow signals to set g_quit

  while( (rc != 0) && ! timedOut && ! g_quit ) {
    if( timeleft > 10 ) {
      ts.tv_sec = 10;
      ts.tv_nsec = 0;
      timeleft -= ts.tv_sec;
    } 
    else
    if( timeout > 0 ) {
      ts.tv_sec = timeleft;
      ts.tv_nsec = 0;
      timeleft = 0;
    } 
    else
    if( timeoutns > 0 ) {
      ts.tv_sec = 0;
      ts.tv_nsec = timeoutns;
      timeleft = 0;
    } 
    else {
      ts.tv_sec = 10;
      ts.tv_nsec = 0;
    }
    rc = semtimedop( s, &semOp, 1, &ts );
    if( rc != 0 ) {
      if( g_debugging )
        logger.error( "shman timeout in semop lock " + itoa(s) + " off: " + itoa(offset) );
    } 
    if( (timeout > 0 || timeoutns > 0) && timeleft <= 0 ) 
      timedOut = true;
  }
  //logger.error( "semGet done");
  return rc;
}
#else
int
BaseServer::semGet( int s, int offset, int timeout, int timeoutns ) {

  if( g_debugging )
    logger.debug( "locking mutex: " + itoa(s) + " off: " + itoa(offset) );

  struct sembuf semOp;
  semOp.sem_num = offset;
  semOp.sem_op  = -1;
  semOp.sem_flg = 0;
 
  int rc   = -1;
  rc = semop( s, &semOp, 1 );

  if( rc != 0 ) {
    if( g_debugging )
      logger.error( "shman timeout in semop lock " + itoa(s) + " off: " + itoa(offset) );
  } 
  return rc;
}
#endif

void
BaseServer::semRelease( int s, int offset ) {

  if( g_debugging )
    logger.error( "unlocking mutex: " + itoa(s) + " off: " + itoa(offset) );

  struct sembuf semOp;
  semOp.sem_num = offset;
  semOp.sem_op  = 1;
  semOp.sem_flg = 0;

  if( semop( s, &semOp, 1 ) < 0 ) {
    logger.error( "error in semop unlock " + itoa(s) + " off: " + itoa(offset) );
  } else {
    //logger.info( "unlocked mutex: " + itoa(s) + " off: " + itoa(offset) );
  }
  return;
}

void 
BaseServer::dpsVersion  ( void ) { 
  logger.info("baseServer version 0.001"); 
}

void
BaseServer::loadConfigMap( string config ) {

  if( config > "" && configFile == "" ) 
    configFile = config;

  if( configFile == "" )
    configFile = string(INSTALLDIR) + "/conf/dss.conf";

  ifstream conffile;
  conffile.open ( configFile.c_str(), ios::in );

  if( conffile.is_open() ) {
    int c = conffile.get();

    while( ! conffile.eof() ) {
      bool commentline = false, delimfound = false, newlinefound = false, linestarted = false, valstarted = false;
      string key, value;

      while( ! newlinefound && ! conffile.eof() ) {
        if( ! linestarted && c != ' ' && c != '\t' && c != '\n' ) {
          linestarted = true;
          if( c == '#' )
            commentline = true;
        } 
        if( linestarted ) {
          if( c == '\n' ) {
            newlinefound = true;
            delimfound   = false;
            linestarted  = false;
            valstarted   = false;
            commentline  = false;
          }
          else
          if( c == '=' )
            delimfound = true;
          else
          if( ! delimfound && (c == ' ' || c == '\t') ) {
            // skips all spaces and tabs in the keys !
          }
          else
          if( ! commentline ) {
            if( ! delimfound && ! newlinefound ) {
              key += tolower(c);
            }
            else
            if( delimfound && ! newlinefound ) {
              if( valstarted || c != ' ' ) {
                value += c;
                valstarted = true;
              }
            }
          }
        }
        c = conffile.get();
      }
      value = rtrim(value);
      configMap[key] = value;
      logger.error("configMap:" + key + "=" + value + ".");
      if( key == "loglevel" )
        g_logLevel = atoi(value.c_str());
      //if( key == "skipnotify" )
        //g_useNotify = false;
    }
  }
  return;
}
/*
void* resetHandles( void** h1, void** h2 ) {
  logger.error("resetHandles " + itoa(h1) + " " + itoa(h2));

  string libfilename = string(INSTALLDIR) + "/lib/libfile";
  ifstream libfile;
  libfile.open ( libfilename.c_str(), ios::in );
  if( ! libfile.is_open() ) {
    logger.error("libfile is missing");
    return NULL;
  }
  if( *h1 ) {
    logger.error("closing h1");
    dlclose(*h1);
  }
  if( *h2 ) {
    logger.error("closing h2");
    dlclose(*h2);
  }
  *h1 = NULL;  *h2 = NULL;

  string filename, filename2;

  libfile >> filename;
  libfile >> filename2;
  if( ! filename.size() || ! filename2.size() ) {
    logger.error("bad libfiles");
    return NULL;
  }
  dlerror();    

  *h1 = dlopen (filename2.c_str(), RTLD_NOW);
  if (!*h1 ) {
    logger.error( ", setLibs failed " + string(dlerror()) );
  } else {
    logger.error( ", setLibs ok  " + filename2 );
  }

  *h2 = dlopen (filename.c_str(), RTLD_NOW);
  if (!*h2) {
    logger.error( ", setLibs failed " + string(dlerror()) );
  } else {
    logger.error( ", setLibs ok " + filename );
  }
  logger.error("resetHandles " + itoa(h1) + " " + itoa(h2));

  return dlsym( *h1, "maker");
}
*/
