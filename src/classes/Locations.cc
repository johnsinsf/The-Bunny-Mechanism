/*
  File:    classes/Locations.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include "dpsframework.h"
#include "Locations.h"

int
Locations::insert( int update ) {

  string s = "INSERT locations (id, description, type, active, updated) VALUES ( 0, '" + description + "'," + itoa(type) + "," + itoa(active) + "," + itoa(updated) + ")";

  return SBase::insert( s );
}

void
Locations::setSearch( int by ) {

  search = "SELECT id, description, type, active, updated FROM locations";

  switch( by ) {
    case byKey:
    case byId:
      search += " WHERE id = " + itoa(id);
      break;
  }
  int i = 0;
  addResultVar(&id, i);
  addResultVar(&buf2, i);
  addResultVar(&type, i);
  addResultVar(&active, i);
  addResultVar(&updated, i);
  return;
}

void
Locations::setStrings(void) {
  description = buf2;
  return;
}

#ifdef _USELIBXML
int 
Locations::load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) {

  bool done = false;
  while( !done ) {
    if( I != M.end() && I->second.parentid == nodeid ) {
      if( I->first.node == "description" ) 
        description = I->second.text;
      if( I->first.node == "type" ) 
        type = atoi(I->second.text.c_str());
      else
      if( I->first.node == "active" ) 
        active = atoi(I->second.text.c_str());
      else
      if( I->first.node == "updated" ) 
        updated = atoi(I->second.text.c_str());
      else
      if( I->first.node == "version" ) 
        version = I->second.text;
      else
        logger.error("unknown element " + I->first.node + ", consider upgrading");

      ++I;

    } else {
      done = true;
      logger.error("found end of table");
    }
  }
  return 0;
}
#endif
