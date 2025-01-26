/*
  File:    dpsio/DssSems.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __DssSems
#define __DssSems

//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
//#include <string>
//#include <map>
//#include <stack>

#include "dpsframework.h"

//#include "Log4.h"

extern log4cpp::Category& logger;

using namespace std;

class DssSems {
  public:

    DssSems(key_t k) {
      SEMKEY = k;
      init();
    };
    DssSems(void) {
    };
    ~DssSems() {
    };
    int  semGet     ( int s, int offset = 0, int timeout = -1, int timeoutns = -1 );
    void semRelease ( int s, int offset = 0 );
    int  semClear   ( int semid, int offset );
    int  popQ   ( void );
    int  pushQ  ( int id );


    void setThreadID    ( pid_t i ) { threadID = i; };
    void setThreadID    ( pid_t i, pid_t gid ) { threadID = i; threadGID = gid; };
    void setMaxSemSets  ( int i ) { MAXSEMSETS = i; }
    void setMaxSemWait  ( int i ) { MAXSEMWAIT = i; }
    void setSemSetSize  ( int i ) { SEMSETSIZE = i; }
    void setSemKey      ( int i ) { SEMKEY = i; init(); }

    pid_t getThreadID   ( void ) { return threadID; };
    pid_t getThreadGID  ( void ) { return threadGID; };
    //int   getIndex      ( void );
    bool  getClientSem  ( int offset, int& semNum, int& newOffset );
    int   getUserSemaphore  ( pid_t inThreadID = 0 );
    int  init         ( void );

    key_t SEMKEY;

  private:

    int  initSemaphores( void );

    static pthread_mutex_t  dssRespMutex;
    static pthread_mutex_t  dssMutex;

    pid_t    threadID;
    pid_t    threadGID;
    stack<int> semQ;
    int      currentIndex;
    map<int,int> dssSemIDs;

    int MAXSEMWAIT;
    int MAXSEMSETS;
    int SEMSETSIZE;
};

#endif
