/*
  File:    dps/Interfaces.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Interfaces
#define __Interfaces

//#include <string>
#include <stack>
#include "dpsframework.h"
//#include "Log4.h"
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
    virtual ~Interfaces( void ) {
    }

    virtual void   setSearch ( int by );
    virtual int    insert    ( int opt = 0 );
    virtual void   setId     ( unsigned int i ) { id = i; }
    virtual void   setStrings( void );
    virtual void   setHostname( string s )  { hostname = s; }
    virtual void   setDpsId   ( string s )  { dpsid = s; }
    virtual void   setLocalID( string s )   { localid = s; }
    virtual void   setContacted( time_t t ) { contacted = t; }
    virtual void   setActive   ( int i )    { active = i; }
    virtual void   setType     ( int i )    { type = i; }

    virtual unsigned int getId( void ) { return id; }
    virtual string   getDescription( void ) { return description; }
    virtual string   getSerialNo   ( void ) { return serialno; }
    virtual unsigned int getType   ( void ) { return type; }
    virtual time_t  getActive   ( void ) { return active; }
    virtual time_t  getContacted( void ) { return contacted; }
    virtual time_t  getDateTime( void ) { return updated; }
    virtual time_t  getSynced  ( void ) { return synced; }
    virtual string  getLocalId ( void ) { return localid; }
    virtual string  getHostname( void ) { return hostname; }
    virtual string  getDpsId   ( void ) { return dpsid; }
    virtual bool    isDirty    ( void ) { return ! syncIDs.empty(); }
#ifdef _USELIBXML
    virtual int     load( int nodeid, map<eKey,eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M );
#endif
    virtual int     markContacted( void );
    virtual int     incrementErrors( void );
    virtual int     setClean( void );
    virtual void    export_data_XML( string& s );

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
