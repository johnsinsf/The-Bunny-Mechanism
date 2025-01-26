/*
  File:    dps/SBase.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __SBase
#define __SBase

#include "dpsframework.h"
#include "BaseServer.h"
#include <mysql.h>
#include "DssSaxParser.h"

extern log4cpp::Category& logger;

#define MAXRESULTVARS 100
#define _DSSCLIENT_VERSION "v0.1"

class SBase {       

  public:
    
    enum {
      byAll,
      byId,
      byInterface,
      bySynced,
      byKey
    };

    enum {
      READ,
      READMO,
      READSEQ,
      READSEQEND
    };

    struct tstruct {
      std::string table;
      std::string db;
    };

    struct dstruct {
      std::string db;
      std::string host;
      std::string user;
      std::string pass;
      int port;
    };

    struct bindata {
      int length;
      unsigned char* ptr;
    };

    struct tstruct tables[1] = { 
      { "END", "" },
    };

    struct dstruct databases[1] = { 
      { "END",    "", "", "", 0 },
    };

    SBase( void ) { 
      searchBy    = 0;
      threadID    = 0;
      readMode    = 0;
      mySeed      = 0;
      myRandState = 0;
      readStart   = 0;
      readMax     = 0;
      isOpen      = false;
      synced      = 0;
      updated     = 0;
      updated_nsec= 0;
      _dps_initialized = false;
      mysql       = NULL;
      result      = NULL;
      disable_database = false;
    };

    virtual ~SBase( void ) {
      if( mysql ) { 
        mysql_close(mysql);
        mysql = NULL;
      }
      if( result )
        mysql_free_result(result);
    }

    virtual int  open  ( bool reOpen = false, string in_db = "" );

    virtual int  close ( void );
    virtual int  read  ( int by );
    virtual int  read  ( const string& c, int mode = 0 );
    virtual int  update  ( const string& c );
    virtual int  insert  ( const string& c );
    virtual int  insert  ( int opt = 0 ) { return 0; }
    virtual int  readmo  ();
    virtual void read_end();
    virtual void setStrings ( void ) {  }

    virtual int  getReadMode( void ) { return readMode; }
    virtual int  getSearch  ( void ) { return searchBy; }
    virtual bool getIsOpen  ( void ) { return isOpen; }
    virtual void setSynced  ( time_t t ) { synced = t; }
    virtual void setUpdated ( time_t t ) { updated = t; }
    virtual void setUpdated ( void ) {
      struct timespec tp;
      int rc = clock_gettime(CLOCK_REALTIME, &tp);
      if( rc == 0 ) {
        updated = tp.tv_sec;
        updated_nsec = tp.tv_nsec;
      }
    }
    virtual void setReadMode( int i ) { readMode= i; }

    virtual int  serialIn   ( const string& s ) { return -1;}
    virtual void serialOut  ( string& s ) {}

    virtual string getDBname  ( void );
    virtual string getDBname  ( string table );
    virtual struct dstruct getDBstruct( string& db );
    virtual BaseServer* getServer( void ) { return _dpsServer; }
    virtual void   setServer( BaseServer* p ) { _dpsServer = p; }
    virtual int   import_data_XML( const string& s) { return -1;}
#ifdef _USELIBXML
    virtual int   load( int nodeid, map<eKey,eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) { return 0;}
#endif
    virtual long  last_insert_id( void );

  protected:
    bool   disable_database;
    int    searchBy;
    int    readMode;
    int    readStart;
    int    readMax;
    bool   isOpen;
    time_t synced;
    time_t updated;
    long   updated_nsec;

    string  search;
    string  database;
    string  username;
    string  password;
    string  hostname;
    string  tablename;
    string  version;
    bool   _dps_initialized;  
    pid_t  threadID;   
    unsigned int  myRandState;
    int  mySeed;
    struct  drand48_data  drand48_buf;

    virtual void  dps_init  ( void );
    virtual void  setSearch  ( int inSearch ) {};
    virtual void  setResults( string& results ) { };
    virtual void  getResults(int num_fields, unsigned long* lengths, MYSQL_ROW& r, MYSQL_FIELD* f );
    virtual void  addResultVar(void* p, int& i);

    MYSQL*  mysql;
    MYSQL_RES *result;

    void*  result_var[MAXRESULTVARS];
    static BaseServer* _dpsServer;
};
  
#endif
