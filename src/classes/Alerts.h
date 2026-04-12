/*
  File:    dps/Alerts.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Alerts
#define __Alerts

#include "dpsframework.h"
#include "SBase.h"

class Alerts : public SBase {
  public:
    void _dss_init( void ) {
      updated     = 0;
      priority    = 5;
    }
    Alerts( void ) : SBase() { 
      _dss_init();
    }
    Alerts( unsigned int s ) : SBase() { 
      _dss_init();
    }
    ~Alerts( void ) {
    }

    void setSearch( int by ) override;
    int  insert   ( int opt = 0 ) override;
    void setStrings( void ) override;

    void export_data_XML( string& s );
    void setAlertMsg( string s ) { alertmsg = s; }
    void setHostname( string s ) { hostname = s; }
    void setSource  ( string s ) { source = s; }
    void setList    ( string s ) { list = s; }
    void setPriority( int i )    { priority = i; }
    void setDateTime( time_t t ) { datetime = t; }
#ifdef _USELIBXML
    int  load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) override;
#endif

    bool     data_sent;

  private:
    string   hostname;
    time_t   datetime;
    string   source;
    string   list;
    string   alertmsg;
    int      priority;
};
  
#endif
