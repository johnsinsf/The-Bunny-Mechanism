/*
  File:    dpsio/Init.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include "dpsframework.h"
//#include "Log4.h"

//#include <sys/poll.h>
//#include <sys/syscall.h>
//#include <dlfcn.h>
//#include <fstream>
//#include <algorithm>
//#include <cctype>

#include <log4cpp/BasicConfigurator.hh>

extern log4cpp::Category& logger;
/*
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
  if(from.empty())
    return;
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

inline std::string& sanitize(std::string& s) {
#ifdef _HAVETRANSFORM
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { if(c == '\\' || c == '\'') c = ' '; return c; });
#else
  replaceAll(s, "\'", " ");
  replaceAll(s, "\\", " ");
#endif
  return s;
}
*/
extern void dump_stack ( void ) ;

// need to use sigaction here...
void dpsSigHandler( int inSig ) {
  //int status   = 0;
  //pid_t child   = 0;
   
  // disable this signal for now
  signal( inSig, SIG_IGN );

  switch( inSig ) {
    case SIGALRM:
      logger.error( "timer!" );
      g_timer = true;
      break;
#ifndef _HAVEMACOS
    case SIGPOLL:
      logger.error( "poll!" );
      //g_timer = true;
      break;
#endif
    case SIGHUP:
      logger.error( "hangup received, ignoring" );
      break;

    case SIGPIPE:
      logger.error( "pipe closed" );
      break;

    case SIGSEGV:
      logger.error( "sigsegv" );
      //dump_stack();
      _exit(2);
      break;

    case SIGINT:
      logger.error( "caught interrupt signal: " +  itoa(inSig) );
      //pid_t tid;
      //tid = syscall(SYS_gettid);
      //pthread_mutex_lock( &dapIntMutex );

      //intMap[tid] = true;

      //pthread_mutex_unlock( &dapIntMutex );

      break; 

    case SIGQUIT:
    case SIGTERM:
      logger.error( "caught quit signal: " +  itoa(inSig) );
      g_quit = true;
      break; 

    default:
      logger.error( "caught default signal: " + itoa(inSig) );
      g_quit = true;
      break; 
  }  
  // reset this signal
  signal( inSig, dpsSigHandler );
}

void dpsSigInit( void ) {
  signal( SIGINT,  dpsSigHandler );
  signal( SIGHUP,  dpsSigHandler );
  // let the core dump for debugging
  //signal( SIGSEGV, dpsSigHandler );
  signal( SIGALRM, dpsSigHandler );
  signal( SIGQUIT, dpsSigHandler );
  signal( SIGPIPE, dpsSigHandler );
#ifndef _HAVEMACOS
  signal( SIGPOLL, dpsSigHandler );
#endif
  signal( SIGTERM, dpsSigHandler );

  logger.error("Signals set");
  return;
}

void dpsInit( int argc, char **argv ) {
  int c;
  dpsSigInit();
  while( 1 ) {
    c = getopt( argc, argv, "S:n:s:f:eiF" );

    if( c == -1 )
      break;
    switch( c ) {
      case 'f':
        configFile = optarg;
        break;
      case 'n':
        numThreads = atoi( optarg );
        break;
      case 'S':
        mySeed = atoi( optarg );
        break;
      case 's':
        _DpsServerNumber = atoi( optarg );
        break;
      case 'e':
        g_useSSL = true;
        break;
      case 'i':
        g_isInit = true;
        break;
      case 'F':
        g_isFileServer = true;
        break;
      default:
        break;
    }
  }
  return;
}

void doDetach( void ) {
  int pid = fork();

  if( pid < 0 ) {
    // should not happen
    logger.error( "fork" );
    exit( 1 );
  } 
  else 
  if( pid > 0 ) {
    logger.info( "child is " + itoa( pid ) );
    cout << "child is " << pid << endl;
    exit( 0 );
   }
   close( fileno( stderr ) );
   close( fileno( stdout ) );
   close( fileno( stdin ) );
   setsid( );
   return;
}

void* getdpsdlsym(void* handle, string s, pthread_mutex_t& mutex ) {

  if( handle == NULL ) {
    logger.error( "handle is null" );
    return NULL;
  }
  dlerror();

  void* rc = dlsym(handle, s.c_str() );
  if( rc == NULL ) {
    string err = dlerror();
    logger.error( "ERROR: dlsym failed with " + err );
  }
  return rc;
}

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
  dlerror();    /* Clear any existing error */

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
