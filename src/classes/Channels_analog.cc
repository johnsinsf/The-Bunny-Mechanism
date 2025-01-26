/*
  File:    classes/Channels_analog.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "dpsframework.h"
#include "Channels_analog.h"

int
Channels_analog::insert( int update ) {

  if( ! update ) {
    search = "INSERT channels_analog (id, current_status, synced, contacted) VALUES ( " + itoa(id) + "," + itoa(current_status) + "," + itoa(synced) + ", " + itoa(contacted) + ")";
  } else {
    search = "UPDATE channels_analog SET current_status = " + ftoa(current_status) + ", synced = 0, contacted = " + itoa(contacted);
    search += " WHERE id = " + itoa(id);
  }
  return SBase::insert( search );
}

void
Channels_analog::setSearch( int by ) {

  search = "SELECT ca.id, ca.current_status, ca.synced, f.localid, c.channel, ca.contacted FROM channels_analog ca";
  search += " JOIN channels c ON c.id = ca.id";
  search += " JOIN interfaces f ON c.interface = f.id";

  readMode = READ;
  switch( by ) {
    default:
    case byId:
      search += " WHERE ca.id = " + itoa(id);
      break;
    case byKey:
      search += " WHERE c.interface = " + itoa(interface) + " AND c.channel = '" + channel + "'";
      break;
    case bySynced:
      search += " WHERE ca.synced = 0";
      readMode = READSEQ;
      break;
  }
  int i = 0;
  addResultVar( &id, i );
  addResultVar( &current_status, i );
  addResultVar( &synced, i );
  addResultVar( &buf3, i );
  addResultVar( &buf, i );
  addResultVar( &contacted, i );

  return;
}

void
Channels_analog::saveData( dpsID* ptr, int i, Interfaces& f ) {

  current_status = ptr[i].dnew_state;

  interface = ptr[i].interface;
  channel   = ptr[i].fio;
  contacted = time(NULL);
  
  insert(true); 

  return;
}

void
Channels_analog::setStrings( void ) {

  channel = buf;
  localid = buf3;

  return;
}

#ifdef _USELIBXML
int 
Channels_analog::load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) {

  synced = time(NULL);
  bool done = false;
  while( !done ) {
    if( I != M.end() && I->second.parentid == nodeid ) {
      if( I->first.node == "interface" ) 
        interface = atoi(I->second.text.c_str());
      else
      if( I->first.node == "localid" ) 
        localid = I->second.text;
      else
      if( I->first.node == "channel" ) 
        channel = I->second.text;
      else
      if( I->first.node == "contacted" ) 
        contacted = atoi(I->second.text.c_str());
      else
      if( I->first.node == "current_status" ) 
        current_status = atoi(I->second.text.c_str());
      else
      if( I->first.node == "version" ) 
        version = I->second.text;
      else
        logger.error("unknown element " + I->first.node + ", consider upgrading");

      ++I;

    } else {
      done = true;
      logger.debug("found end of Channels_analog table");
    }
  }
  return 0;
}
#endif

int
Channels_analog::setClean() {

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
  search = "UPDATE channels_analog SET synced = " + itoa(time(NULL)) + " WHERE id IN (" + s + ")";

  return SBase::insert( search );
}

void
Channels_analog::export_data_XML( string& s) {
  s+= "  <table name=\"channels_analog\" type=\"dssObject\">\n";
  s+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
  s+= "    <interface>" + getLocalId() + "</interface>\n";
  s+= "    <dpsid>" + getDpsId() + "</dpsid>\n";
  s+= "    <channel>" + getChannel() + "</channel>\n";
  s+= "    <contacted>" + itoa(getContacted()) + "</contacted>\n";
  s+= "    <current_status>" + ftoa(getStatus()) + "</current_status>\n";
  s+= "  </table>\n";

  syncIDs.push(getId());

  return;
}
