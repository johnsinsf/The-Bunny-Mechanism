/*
  File:    dps/Channels_analog.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Channels_analog
#define __Channels_analog

//#include <string>
#include "dpsframework.h"
//#include "Log4.h"
#include "Interfaces.h"
#include "Channels.h"

class Channels_analog : public Channels {
  public:
    void _dss_init( void ) {
      id          = 0;
      updated     = 0;
      synced      = 0;
      contacted   = 0;
      current_status = 0;
    }
    Channels_analog( void ) : Channels() { 
      _dss_init();
    }
    Channels_analog( unsigned int i ) : Channels() { 
      id = i;
      _dss_init();
    }
    virtual ~Channels_analog( void ) {
    }

    virtual void setSearch( int by );
    virtual int  insert   ( int opt = 0 );
    virtual void saveData ( dpsID* ptr, int i, Interfaces& f );
    virtual void setId    ( unsigned int i ) { id = i; }
    virtual void setStrings( void );
    virtual void setStatus( int i ) {current_status = i;}
#ifdef _USELIBXML
    virtual int  load     ( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M );
#endif
    virtual bool isDirty  ( void ) { return ! syncIDs.empty(); }
    virtual int  setClean ( void );
    virtual void export_data_XML( string& s );

    virtual int  getId        ( void ) { return id; }
    virtual double getStatus    ( void ) { return current_status; }
    virtual time_t        getUpdated   ( void ) { return updated; }
    virtual time_t        getContacted ( void ) { return contacted; }

  private:
    unsigned int  id;
    stack<int>    syncIDs;
    double        current_status;
    time_t        updated;
    time_t        contacted;
};
  
#endif
