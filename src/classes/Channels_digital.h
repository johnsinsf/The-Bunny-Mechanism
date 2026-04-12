/*
  File:    dps/Channels_digital.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Channels_digital
#define __Channels_digital

#include "dpsframework.h"
#include "Interfaces.h"
#include "Channels.h"

class Channels_digital : public Channels {
  public:
    void _dss_init( void ) {
      id          = 0;
      updated     = 0;
      contacted   = 0;
      synced      = 0;
      open_count  = 0;
      last_open   = 0;
      last_close  = 0;
      current_status = 0;
    }
    Channels_digital( void ) : Channels() { 
      _dss_init();
    }
    Channels_digital( unsigned int i ) : Channels() { 
      id = i;
      _dss_init();
    }
    ~Channels_digital( void ) {
    }

    void setSearch( int by ) override;
    int  insert   ( int opt = 0 ) override;
    void setStrings( void ) override;

    void saveData ( dpsID* ptr, int i, Interfaces& f ) override;

    void setStatus( int i ) {current_status = i;}
#ifdef _USELIBXML
    int  load     ( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) override;
#endif
    int  setClean ( void ) override;
    void export_data_XML( string& s ) override;

    int  getStatus    ( void ) { return current_status; }
    time_t        getContacted ( void ) { return contacted; }
    unsigned int  getOpenCount ( void ) { return open_count; }
    unsigned int  getLastOpen  ( void ) { return last_open; }
    unsigned int  getLastClose ( void ) { return last_close; }

  private:
    unsigned int  id;
    stack<int>    syncIDs;
    unsigned int  open_count;
    unsigned int  last_open;
    unsigned int  last_close;
    int           current_status;
    time_t        updated;
    time_t        contacted;
};
  
#endif
