/*
  File:    dps/Interfaces.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Interfaces
#define __Interfaces

#include <stack>
#include "dpsframework.h"
#include "SBase.h"

class Interfaces : public SBase {
  public:
    void _dss_init( void ) {
      type        = 0;
      active      = 0;
      contacted   = 0;
      id          = 0;
      errors      = 0;
      lasterror   = 0;
      memset(buf1, 0, sizeof(buf1));
      memset(buf2, 0, sizeof(buf2));
      memset(buf3, 0, sizeof(buf3));
      memset(buf4, 0, sizeof(buf4));
    }
    Interfaces( void ) : SBase() { 
      _dss_init();
    }
    Interfaces( unsigned int i ) : SBase() { 
      _dss_init();
      id = i;
    }
    ~Interfaces( void ) {
    }

    void   setSearch ( int by ) override;
    int    insert    ( int opt = 0 ) override;
    void   setStrings( void ) override;

    void   setId     ( unsigned int i ) { id = i; }
    void   setHostname( string s )  { hostname = s; }
    void   setDpsId   ( string s )  { dpsid = s; }
    void   setLocalID( string s )   { localid = s; }
    void   setContacted( time_t t ) { contacted = t; }
    void   setActive   ( int i )    { active = i; }
    void   setType     ( int i )    { type = i; }

    unsigned int getId( void ) { return id; }
    string   getDescription( void ) { return description; }
    string   getSerialNo   ( void ) { return serialno; }
    unsigned int getType   ( void ) { return type; }
    time_t  getActive   ( void ) { return active; }
    time_t  getContacted( void ) { return contacted; }
    time_t  getDateTime( void ) { return updated; }
    time_t  getSynced  ( void ) { return synced; }
    string  getLocalId ( void ) { return localid; }
    string  getHostname( void ) { return hostname; }
    string  getDpsId   ( void ) { return dpsid; }
    bool    isDirty    ( void ) { return ! syncIDs.empty(); }
#ifdef _USELIBXML
    int     load( int nodeid, map<eKey,eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) override;
#endif
    int     markContacted( void );
    int     incrementErrors( void );
    int     setClean( void );
    void    export_data_XML( string& s );

  private:
    unsigned int id;
    stack<int>   syncIDs;
    string       description;
    string       serialno;
    string       localid;
    string       dpsid;
    string       hostname;
    unsigned int type;
    unsigned int errors;
    time_t       active;
    time_t       contacted;
    time_t       lasterror;
    char buf1[80];
    char buf2[80];
    char buf3[80];
    char buf4[80];
};
  
#endif
