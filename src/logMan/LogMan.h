/*
  File:    mcMan/LogMan.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.

  LogMan - Memcache Storage threaded manager

  performs the actual storage operations of open, close, read and save
  using the ShmMan as the default communications to the clients.  Each
  client sends their requests to LogMan, which does the action requested,
  and gets the response through ShmMan(shared memory manager).

  mysql databases are the only ones supported at this time.

  databases must be opened and associated with each table that will 
  be used.  multiple databases can be opened by the clients.

  operations can be totally asynchronous, the clients can continue with
  other operations and come back to check status at a later time.
*/

#ifndef __LogMan
#define __LogMan

#include <string>
#include <map>
#include <stack>
#include <queue>
#include "dpsframework.h"
#include "BaseServer.h"
#include "SocketIO.h"
#include "Log4.h"
#include "../classes/SBase.h"

#define LogMan_MAXSERVERS  25
#define LogMan_NUMTHREADS  25

class LogMan : public BaseServer {
  public:
   
    LogMan( void ) { 
      threadID   = 0;
      _dpsServer = NULL;
    }
    ~LogMan( void ) {
    }
    void run( void ) override;
    int doControl3(pid_t& t) override;

    void  loadConfig      ( void );

  private:
  
    void  init       ( void ) override;
    int  initThread( void ) override;

    static pthread_mutex_t shmMutex;
    pid_t threadID;
    LogMan* _dpsServer;

};
  
#endif
