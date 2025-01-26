/*
  File:    classes/Interfaces.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "dpsframework.h"
#include "Interfaces.h"

int
Interfaces::insert( int update ) {

  setUpdated();

  if( !update ) {
    search = "INSERT interfaces (id, hostname, localid, description, type, active, updated, synced, contacted, errors, lasterror) VALUES ( " + itoa(id) + ",'" + hostname + "','" + localid + "','" + description + "'," + itoa(type) + "," + itoa(active) + "," + itoa(updated) + "," + itoa(synced) + "," + itoa(contacted) + "," + itoa(errors) + "," + itoa(lasterror) + ")";
  } else {
    search = "UPDATE interfaces SET hostname = '" + hostname + "', SET localid = '" + localid + "', description = '" + description + "', type = " + itoa(type) + ", active = " + itoa(active) + ", updated = " + itoa(updated) + ", synced = " + itoa(synced) + ", contacted = " + itoa(contacted) + ", errors = " + itoa(errors) + ", lasterror = " + itoa(lasterror) + " WHERE id = " + itoa(id);
  }
  return SBase::insert( search );
}

int
Interfaces::setClean( void ) {

  time_t t = time(NULL);
  string s;
  while( ! syncIDs.empty() ) {
    if( s.size() ) s += ",";
    s += itoa(syncIDs.top());
    syncIDs.pop();
  }
  if( ! s.size() ) {
    logger.error("nothing to clean");
    return 0;
  }
  search = "UPDATE interfaces SET synced = " + itoa(t) + " WHERE id IN (" + s + ")";
  
  return SBase::insert( search );
}

int
Interfaces::incrementErrors( void ) {

  time_t t = time(NULL);
  search = "UPDATE interfaces SET errors = errors + 1, lasterror = " + itoa(t) + " WHERE id = " + itoa(id);
  
  return SBase::insert( search );
}

int
Interfaces::markContacted( void ) {

  search = "UPDATE interfaces SET synced = 0, contacted = " + itoa(contacted) + " WHERE id = " + itoa(id);
  
  return SBase::insert( search );
}

void
Interfaces::setSearch( int by ) {

  readMode = READ;
  search = "SELECT id, hostname, localid, description, serialno, type, active, updated, contacted, errors, lasterror FROM interfaces";

  switch( by ) {
    default:
    case byId:
      search += " WHERE id = " + itoa(id);
      break;
    case byKey:
      search += " WHERE localid = '" + localid + "' AND hostname = '" + hostname + "'";
      break;
    case bySynced:
      search += " WHERE synced = 0";
      readMode = READSEQ;
      break;
  }

  int i = 0;
  addResultVar(&id, i);
  addResultVar(&buf1, i);
  addResultVar(&buf2, i);
  addResultVar(&buf3, i);
  addResultVar(&buf4, i);
  addResultVar(&type, i);
  addResultVar(&active, i);
  addResultVar(&updated, i);
  addResultVar(&contacted, i);
  addResultVar(&errors, i);
  addResultVar(&lasterror, i);

  return;
}

void
Interfaces::setStrings( void ) {

  hostname    = buf1;
  localid     = buf2;
  description = buf3;
  serialno    = buf4;
  
  return;
}

#ifdef _USELIBXML
int 
Interfaces::load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) {

  synced = time(NULL);
  bool done = false;

  while( !done ) {
    if( I != M.end() && I->second.parentid == nodeid ) {
      if( I->first.node == "description" ) 
        description = I->second.text;
      else
      if( I->first.node == "serialno" ) 
        serialno = I->second.text;
      else
      if( I->first.node == "hostname" ) 
        hostname = I->second.text;
      else
      if( I->first.node == "localid" ) 
        localid = I->second.text;
      else
      if( I->first.node == "type" ) 
        type = atoi(I->second.text.c_str());
      else
      if( I->first.node == "active" ) 
        active = atoi(I->second.text.c_str());
      else
      if( I->first.node == "updated" ) 
        updated = atoi(I->second.text.c_str());
      else
      if( I->first.node == "contacted" ) 
        contacted = atoi(I->second.text.c_str());
      else
      if( I->first.node == "errors" ) 
        errors = atoi(I->second.text.c_str());
      else
      if( I->first.node == "lasterror" ) 
        lasterror = atoi(I->second.text.c_str());
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

void
Interfaces::export_data_XML( string& s) {

  s+= "  <table name=\"interfaces\" type=\"dssObject\">\n";
  s+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
  s+= "    <hostname>" + hostname + "</hostname>\n";
  s+= "    <localid>" + localid + "</localid>\n";
  s+= "    <dpsid>" + getDpsId() + "</dpsid>\n";
  s+= "    <description>" + description + "</description>\n";
  s+= "    <serialno>" + serialno + "</serialno>\n";
  s+= "    <type>" + itoa(type) + "</type>\n";
  s+= "    <active>" + itoa(active) + "</active>\n";
  s+= "    <updated>" + itoa(updated) + "</updated>\n";
  s+= "    <contacted>" + itoa(contacted) + "</contacted>\n";
  s+= "    <errors>" + itoa(errors) + "</errors>\n";
  s+= "    <lasterror>" + itoa(lasterror) + "</lasterror>\n";
  s+= "  </table>\n";

  syncIDs.push(getId());

  return;
}
