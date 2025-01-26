/*
  File:    dps/Logs.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Logs
#define __Logs

//#include <string>
//#include <stack>
#include "dpsframework.h"
//#include "Log4.h"
#include "SBase.h"

class Logs : public SBase {
  public:
    void _dss_init( void ) {
      type        = 0;
      datetime    = 0;
      datetime_nsec= 0;
      interface   = 0;
      id          = 0;
      synced      = 0;
      memset(buf, 0, sizeof(buf));
      memset(buf2, 0, sizeof(buf2));
      memset(buf3, 0, sizeof(buf3));
      memset(buf4, 0, sizeof(buf4));
    }
    Logs( void ) : SBase() { 
      _dss_init();
    }
    Logs( unsigned int i ) : SBase() { 
      _dss_init();
      id = i;
    }
    virtual ~Logs( void ) {
    }

    virtual void setSearch ( int by );
    virtual int  insert    ( int opt = 0 );
    virtual void setId     ( unsigned int i ) { id = i; }
    virtual int  saveData  ( dpsID* ptr, long id, string& s);
    virtual void setStrings( void );
    virtual void setData   ( string& s ) { data = s; }
    virtual void setDpsId  ( string& s ) { dpsid = s; }
    virtual void setHostname( string& s ) { myhostname = s; }
    virtual void setType   ( int i ) { type = i; }
    virtual int  setClean  ( void );

    virtual unsigned int getId       ( void ) { return id; }
    virtual unsigned int getType     ( void ) { return type; }
    virtual unsigned int getInterface( void ) { return interface; }
    virtual string       getLocalId  ( void ) { return localid; }
    virtual string       getChannel  ( void ) { return channel; }
    virtual string       getHostname ( void ) { return myhostname; }
    virtual string       getDpsId    ( void ) { return dpsid; }
    virtual unsigned int isDirty     ( void ) { return ! syncIDs.empty(); }
    virtual time_t       getDateTime ( void ) { return datetime; }
    virtual string&      getData     ( void ) { return data; }
    virtual void         serialOut   ( string& s );
    virtual void         export_data_XML( string& s);

  private:
    unsigned int  id;
    string        data;
    string        dpsid;
    string        localid;
    string        channel;
    string        myhostname;
    unsigned int  type;
    unsigned int  interface;
    time_t        datetime;
    long          datetime_nsec;
    time_t        synced;
    char          buf[80];
    char          buf2[257];
    char          buf3[257];
    char          buf4[257];
    stack<int>    syncIDs;
};
  
#endif
