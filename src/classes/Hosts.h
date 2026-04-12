/*
  File:    dps/Hosts.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Hosts
#define __Hosts

#include "dpsframework.h"
#include "SBase.h"

class Hosts : public SBase {
  public:
    void _dss_init( void ) {
      updated     = 0;
      data_sent   = false;
      memset(buf, 0, sizeof(buf));
    }
    Hosts( void ) : SBase() { 
      _dss_init();
    }
    Hosts( unsigned int s ) : SBase() { 
      _dss_init();
    }
    ~Hosts( void ) {
    }

    void setSearch( int by ) override;
    int  insert   ( int opt = 0 ) override;
    void setStrings( void ) override;

    void export_data_XML( string& s );
    void setVar1   ( string& s ) { var1 = s; }
    void setVar2   ( string& s ) { var2 = s; }
    void setVar3   ( string& s ) { var3 = s; }
    void setHostname( string& s ) { hostname = s; }
#ifdef _USELIBXML
    int  load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) override;
#endif

    bool     data_sent;

  private:
    string   hostname;
    time_t   updated;
    char     buf[256];
    string   var1;
    string   var2;
    string   var3;
};
  
#endif
