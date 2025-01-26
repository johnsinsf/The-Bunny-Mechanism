/*
  File:    dps/Channels.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Channels
#define __Channels

//#include <string>
#include "dpsframework.h"
//#include "Log4.h"
#include "SBase.h"
#include "Interfaces.h"

class Channels : public SBase {
  public:
    void _dss_init( void ) {
      id          = 0;
      interface   = 0;
      fio         = 0;
      type        = 0;
      active      = 0;
      updated     = 0;
      synced      = 0;
      direction   = 0;
      memset(buf, 0, sizeof(buf));
      memset(buf2, 0, sizeof(buf2));
      memset(buf3, 0, sizeof(buf3));
    }
    Channels( void ) : SBase() { 
      _dss_init();
    }
    Channels( unsigned int i ) : SBase() { 
      id = i;
      _dss_init();
    }
    virtual ~Channels( void ) {
    }

    typedef struct {
      string channel;
      int fio;
      int type;
      int direction;
    } ctstruct;

    virtual void setSearch( int by );
    virtual int  insert   ( int opt = 0 );
    virtual void saveData ( dpsID* ptr, int i, Interfaces& f );
    virtual void setId    ( unsigned int i ) { id = i; }
    virtual void setInterface( unsigned int i ) { interface = i; }
    virtual void setLocalId  ( string i ) { localid = i; }
    virtual void setDpsId    ( string i ) { dpsid = i; }
    virtual void setChannel  ( string i ) { channel = i; }
    virtual void setType     ( int i )    { type = i; }
    virtual void setActive   ( int i )    { active = i; }
    virtual void setDirection( int i )    { direction = i; }
    virtual void setStrings( void );
#ifdef _USELIBXML
    virtual int  load     ( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M );
#endif
    virtual bool isDirty  ( void ) { return ! syncIDs.empty(); }
    virtual int  setClean ( void );
    virtual void export_data_XML( string& s );

    virtual int  getId        ( void ) { return id; }
    virtual int  getDirection ( void ) { return direction; }
    virtual string        getLocalId   ( void ) { return localid; }
    virtual unsigned int  getFio       ( void ) { return fio; }
    virtual string        getChannel   ( void ) { return channel; }
    virtual string        getDpsId     ( void ) { return dpsid; }
    virtual unsigned int  getInterface ( void ) { return interface; }
    virtual unsigned int  getType      ( void ) { return type; }
    virtual unsigned int  getActive    ( void ) { return active; }
    virtual time_t        getUpdated   ( void ) { return updated; }
    virtual time_t        getSynced    ( void ) { return synced; }
    virtual dpsID*        getDpsIds    ( int& numdpsids );
    virtual dpsID*        getDpsIds    ( int& numdpsids, ctstruct d[] );

  protected:
    unsigned int  id;
    unsigned int  fio;
    stack<int>    syncIDs;
    string        description;
    string        localid;
    string        dpsid;
    string        channel;
    unsigned int  interface;
    unsigned int  type;
    unsigned int  active;
    time_t        synced;
    int           direction;
    time_t        updated;
    char          buf[80];
    char          buf2[80];
    char          buf3[80];
};
  
#endif
