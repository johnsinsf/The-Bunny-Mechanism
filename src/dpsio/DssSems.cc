/*
  File:    dpsio/DssSems.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "DssSems.h"

pthread_mutex_t DssSems::dssMutex       = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t DssSems::dssRespMutex   = PTHREAD_MUTEX_INITIALIZER;

int DssSems::initSemaphores( void ) {
  if( pthread_mutex_lock( &dssMutex ) != 0 )
    logger.error( "error: failed to lock init mutex" );

  int k = 0;
  if( currentIndex < MAXSEMSETS )
    k = currentIndex++;
  else {
    logger.error( "too many semaphores");

    return -1;
  }
  if( pthread_mutex_unlock( &dssMutex ) != 0 ) {
    logger.error( "failed to unlock mutex" );

    return -1;
  }
  dssSemIDs[k] = semget( SEMKEY+k, SEMSETSIZE,  S_IRWXU );

  if( dssSemIDs[k] == -1 ) {
    dssSemIDs[k] = semget( SEMKEY+k, SEMSETSIZE,  S_IRWXU | IPC_CREAT );
  }
  if( dssSemIDs[k] == -1 ) {
    logger.error( "dsssems error: semget clientID" );
  } else {
    unsigned short arr[SEMSETSIZE];
    memset( arr, 0, sizeof(arr) );
    semctl( dssSemIDs[k] , 0, SETALL, arr );
  }
  for( int i = 0; i < SEMSETSIZE; i++ ) {
    int l = k*SEMSETSIZE + i;
    semQ.push(l);
  }
  if( pthread_mutex_unlock( &dssMutex ) != 0 )
    logger.error( "failed to unlock mutex" );
  
  return 1;
}

int DssSems::init( void ) {
  threadID      = -1;
  threadGID     = -1;
  currentIndex  = 0; 
  MAXSEMWAIT    = 10;
  MAXSEMSETS    = 10;
  SEMSETSIZE    = 24;
  initSemaphores();

  return 1;
}

int DssSems::popQ( void ) {
  if( pthread_mutex_lock( &dssRespMutex ) != 0 ) {
    logger.error( "ERROR: locking dssRespMutex" );
  }
  int rc = semQ.top();
  semQ.pop();

  if( pthread_mutex_unlock( &dssRespMutex ) != 0 ) {
    logger.error( "ERROR: unlocking dssRespMutex" );
  }
  return rc;
}

int DssSems::pushQ( int id ) {
  if( pthread_mutex_lock( &dssRespMutex ) != 0 ) {
    logger.error( "ERROR: locking dssRespMutex" );
  }
  semQ.push(id);

  if( pthread_mutex_unlock( &dssRespMutex ) != 0 ) {
    logger.error( "ERROR: unlocking dssRespMutex" );
  }
  return 0;
}

int DssSems::semClear( int semid, int offset ) {
  int rc = semctl(semid, offset, SETVAL, 0);

  return rc;
}

#ifndef _HAVEMACOS
int DssSems::semGet( int s, int offset, int timeout, int timeoutns ) {
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
  return rc;
}
#else

int DssSems::semGet( int s, int offset, int timeout, int timeoutns ) {
  struct sembuf semOp;
  semOp.sem_num = offset;
  semOp.sem_op  = -1;
  semOp.sem_flg = 0;
  //int timeleft = timeout;
 
  int rc   = -1;

  rc = semop( s, &semOp, 1 );
  if( rc != 0 ) {
    if( g_debugging )
      logger.error( "shman timeout in semop lock " + itoa(s) + " off: " + itoa(offset) );
  } 
  return rc;
}
#endif

void DssSems::semRelease( int s, int offset ) {
  struct sembuf semOp;
  semOp.sem_num = offset;
  semOp.sem_op  = 1;
  semOp.sem_flg = 0;

  if( semop( s, &semOp, 1 ) < 0 ) {
    logger.error( "error in semop unlock " + itoa(s) + " off: " + itoa(offset) );
  } else {
    if( g_debugging ) 
      logger.debug( "unlocked mutex: " + itoa(s) + " off: " + itoa(offset) );
  }
  return;
}

bool 
DssSems::getClientSem( int offset, int& semNum, int& newOffset ) {
  semNum = 0;
  newOffset = 0;

  // typical case
  if( offset < SEMSETSIZE ) {
    newOffset  = offset;
    semNum  = dssSemIDs[0];
  } else {
    newOffset = offset % SEMSETSIZE;
    if( newOffset > SEMSETSIZE )
      newOffset = 0;
    else {
      int t = offset / SEMSETSIZE;
      if( t < MAXSEMSETS ) 
        semNum = dssSemIDs[t];
      else {
        logger.error( "getClientSem ERROR: " + itoa(offset));
        return false;
      }
    }
  }
  return true;
}

int 
DssSems::getUserSemaphore  ( pid_t inThreadID ) {
  int rc = -1;
  if( pthread_mutex_lock( &dssMutex ) != 0 )
    logger.error( "ERROR: failed to lock mutex" );
    
  if( semQ.empty() ) {
    logger.error( "ERROR: out of semaphores" );
    if( pthread_mutex_unlock( &dssMutex ) != 0 )
      logger.error( "ERROR: failed to unlock mutex" );

    rc = initSemaphores();

    if( rc != 1 ) {
      logger.error("failed to get more semaphores");
      return -1;
    }
    if( pthread_mutex_lock( &dssMutex ) != 0 )
      logger.error( "ERROR: failed to lock mutex" );
  } 
  rc = semQ.top();
  semQ.pop();

  if( pthread_mutex_unlock( &dssMutex ) != 0 )
    logger.error( "ERROR: failed to unlock mutex" );

  return rc;
}
