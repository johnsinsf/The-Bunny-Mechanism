/*
  File:    logMan/LogMan.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include <stdio.h>
#include <fstream>
#include "LogMan.h"
#include "Log4.h"
#include <sys/syscall.h> 
#include <inttypes.h>
#include <arpa/inet.h>

pthread_mutex_t LogMan::shmMutex    = PTHREAD_MUTEX_INITIALIZER;
BaseServer *SBase::_dpsServer = NULL;

#define MAXBUFSIZE 512000

void
LogMan::loadConfig( void ) {

  string config = string(INSTALLDIR) + "/conf/logMan.conf";
  loadConfigMap(config.c_str());

  return;
}

int
LogMan::initThread( void ) {

  if( pthread_mutex_lock( &shmMutex ) != 0 )
    logger.error( "ERROR: failed to lock mutex" );

  pid_t tid;
  threadID = tid = syscall(SYS_gettid);

  if( pthread_mutex_unlock( &shmMutex ) != 0 )
    logger.error( "ERROR: failed to unlock mutex" );

  return tid;
}

void
LogMan::init( void ) {

  logger.error( "LogMan init beginning " + itoa(threadID) );

  sigset_t iset;
  sigemptyset(&iset);
  sigaddset(&iset, SIGPIPE);
  pthread_sigmask( SIG_BLOCK, &iset, NULL );
  pthread_sigmask( SIG_SETMASK, &iset, NULL );

  logger.error( "LogMan init done" );
  return;
}


int
LogMan::doControl3( pid_t& inThreadID ) {

  logger.error( "LogMan doControl3 thread: " + itoa(inThreadID) + " tgid " + itoa(getpid()) );

  SocketIO* socket = NULL;
  int clientFD = getClientFD(2);
 
  logger.error("have FD " + itoa(clientFD));

  if( clientFD > 0 ) 
    socket = new SocketIO(clientFD); 
  else {
    logger.error("bad client fd");
    return -1;
  }
  socket->useSSL = true;
  socket->isConnected = true;
  socket->startUpSSL();

  socket->isConnected = true;

  bool done = false;

  string packetin, packetout;
  char buf[MAXBUFSIZE];
  memset( buf, 0, sizeof(buf));

  int rc = 0;

  while( ! done && ! g_quit ) {

    logger.error( "logMan reading");

    rc = socket->doReadLine( buf, sizeof(buf), 330 );
    if( rc > 0 ) {
      logger.error("input: " + string(buf));
    } else {
      logger.error("bad read, exiting");
      done = true; 
    }
  }
  return 0;
}

void
LogMan::run( void ) {

  pthread_t tp[LogMan_NUMTHREADS + 1];
  init();

  logger.error("LogMan starting");

  int rc = 0;
  string tag = "logMan";

  log4cpp::NDC::push( itoa(threadID) + " " + tag );

  SocketIO socket;
  socket.useSSL = true;
  //socket.useSSL = false;
  socket._certRequired = false;

  int _socket = socket.openServer( 22222 );
  if( _socket <= 0 )
    g_quit = true;

  while( ! g_quit ) {
    logger.error( "logman accepting ");

    int clientFD = socket.accept( _socket );
    if( clientFD > 0 ) {
        pushClientFD( clientFD, 2 );
        logger.error( "logman accepted:" + itoa(clientFD) );
        if( clientFD <= 0 ) {
          logger.error( "error accepting" );
          g_quit = true;
        } else {
          // 
          startThread( true, 6 );
          sleep(5);  // testing
        }
    }
  }
  logger.error("done with logman");

  return;
}

