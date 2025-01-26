/*
  File:    dps/Alerts.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Alerts
#define __Alerts

//#include <string>
#include "dpsframework.h"
//#include "Log4.h"
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
    virtual ~Alerts( void ) {
    }

    virtual void setSearch( int by );
    virtual int  insert   ( int opt = 0 );
    virtual void setStrings( void );
    virtual void export_data_XML( string& s );
    virtual void setAlertMsg( string s ) { alertmsg = s; }
    virtual void setHostname( string s ) { hostname = s; }
    virtual void setSource  ( string s ) { source = s; }
    virtual void setList    ( string s ) { list = s; }
    virtual void setPriority( int i )    { priority = i; }
    virtual void setDateTime( time_t t ) { datetime = t; }
#ifdef _USELIBXML
    virtual int  load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M );
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
