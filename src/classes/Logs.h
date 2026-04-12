/*
  File:    dps/Logs.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Logs
#define __Logs

#include "dpsframework.h"
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
    ~Logs( void ) {
    }

    void setSearch ( int by ) override;
    int  insert    ( int opt = 0 ) override;
    void setStrings( void ) override;

    void setId     ( unsigned int i ) { id = i; }
    int  saveData  ( dpsID* ptr, long id, string& s);
    void setData   ( string& s ) { data = s; }
    void setDpsId  ( string& s ) { dpsid = s; }
    void setHostname( string& s ) { myhostname = s; }
    void setType   ( int i ) { type = i; }
    int  setClean  ( void );

    unsigned int getId       ( void ) { return id; }
    unsigned int getType     ( void ) { return type; }
    unsigned int getInterface( void ) { return interface; }
    string       getLocalId  ( void ) { return localid; }
    string       getChannel  ( void ) { return channel; }
    string       getHostname ( void ) { return myhostname; }
    string       getDpsId    ( void ) { return dpsid; }
    unsigned int isDirty     ( void ) { return ! syncIDs.empty(); }
    time_t       getDateTime ( void ) { return datetime; }
    string&      getData     ( void ) { return data; }
    void         serialOut   ( string& s );
    void         export_data_XML( string& s);

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
