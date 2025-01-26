/*
  File:    dps/Locations.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __Locations
#define __Locations

//#include <string>
#include "dpsframework.h"
//#include "Log4.h"
#include "SBase.h"

class Locations : public SBase {
  public:
    void _dss_init( void ) {
      type        = 0;
      active      = 0;
      updated     = 0;
      memset(buf2, 0, sizeof(buf2));
    }
    Locations( void ) : SBase() { 
      _dss_init();
    }
    Locations( unsigned int s ) : SBase() { 
      _dss_init();
    }
    virtual ~Locations( void ) {
    }

    virtual void setSearch( int by );
    virtual int  insert   ( int opt = 0 );
    virtual void setId    ( unsigned int i ) { id = i; }
    virtual void setStrings( void );
#ifdef _USELIBXML
    virtual int  load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M );
#endif

  private:
    unsigned int  id;
    string        description;
    unsigned int  type;
    unsigned int  active;
    time_t        updated;
    char          buf2[80];
};
  
#endif
