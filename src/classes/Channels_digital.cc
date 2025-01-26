/*
  File:    classes/Channels_digital.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include "dpsframework.h"
#include "Channels_digital.h"

int
Channels_digital::insert( int update ) {

  if( ! update ) {
    search = "INSERT channels_digital (id, open_count, last_open, last_close, current_status, synced, contacted) VALUES ( " + itoa(id) + "," + itoa(open_count) + "," + itoa(last_open) + "," + itoa(last_close) + "," + itoa(current_status) + "," + itoa(synced) + ", " + itoa(contacted) + ")";
  } else {
    if( current_status ) {
      search = "UPDATE channels_digital SET open_count = open_count + 1, last_open = " + ltoa(last_open) + ", current_status = 1, synced = 0, contacted = " + itoa(contacted);
    } else {
      search = "UPDATE channels_digital SET last_close = " + ltoa(last_close) + ", current_status = 0, synced = 0, contacted = " + itoa(contacted);
    }
    search += " WHERE id = " + itoa(id);
  }
  return SBase::insert( search );
}

void
Channels_digital::setSearch( int by ) {

  search = "SELECT cd.id, cd.open_count, cd.last_open, cd.last_close, cd.current_status, cd.synced, f.localid, c.channel, cd.contacted FROM channels_digital cd";
  search += " JOIN channels c ON c.id = cd.id";
  search += " JOIN interfaces f ON c.interface = f.id";
  readMode = READ;
  switch( by ) {
    default:
    case byId:
      search += " WHERE cd.id = " + itoa(id);
      break;
    case byKey:
      search += " WHERE cd.interface = " + itoa(interface) + " AND cd.channel = '" + channel + "'";
      break;
    case bySynced:
      search += " WHERE cd.synced = 0";
      readMode = READSEQ;
      break;
  }
  int i = 0;
  addResultVar( &id, i );
  addResultVar( &open_count, i );
  addResultVar( &last_open, i );
  addResultVar( &last_close, i );
  addResultVar( &current_status, i );
  addResultVar( &synced, i );
  addResultVar( &buf3, i );
  addResultVar( &buf, i );
  addResultVar( &contacted, i );

  return;
}

void
Channels_digital::saveData( dpsID* ptr, int i, Interfaces& f ) {

  current_status = ptr[i].inew_state;

  interface = ptr[i].interface;
  channel   = ptr[i].fio;

  current_status = ptr[i].inew_state;
  current_status ? last_open = time(NULL) : last_close = time(NULL);
  contacted = time(NULL);

  insert(true); 

  return;
}

void
Channels_digital::setStrings( void ) {

  channel = buf;
  localid = buf3;

  return;
}

#ifdef _USELIBXML
int 
Channels_digital::load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) {

  synced = time(NULL);
  bool done = false;
  while( !done ) {
    if( I != M.end() && I->second.parentid == nodeid ) {
      if( I->first.node == "description" ) 
        description = I->second.text;
      else
      if( I->first.node == "interface" ) 
        interface = atoi(I->second.text.c_str());
      else
      if( I->first.node == "localid" ) 
        localid = I->second.text;
      else
      if( I->first.node == "channel" ) 
        channel = I->second.text;
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
      if( I->first.node == "open_count" ) 
        open_count = atoi(I->second.text.c_str());
      else
      if( I->first.node == "last_open" ) 
        last_open = atoi(I->second.text.c_str());
      else
      if( I->first.node == "last_close" ) 
        last_close = atoi(I->second.text.c_str());
      else
      if( I->first.node == "current_status" ) 
        current_status = atoi(I->second.text.c_str());
      else
      if( I->first.node == "contacted" ) 
        contacted = atoi(I->second.text.c_str());
      else
      if( I->first.node == "version" ) 
        version = I->second.text;
      else
        logger.error("unknown element " + I->first.node + ", consider upgrading");

      ++I;

    } else {
      done = true;
      logger.debug("found end of Channels_digital table");
    }
  }
  return 0;
}
#endif

int
Channels_digital::setClean() {

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
  search = "UPDATE channels_digital SET synced = " + itoa(time(NULL)) + " WHERE id IN (" + s + ")";

  return SBase::insert( search );
}

void
Channels_digital::export_data_XML( string& s) {
  s+= "  <table name=\"channels_digital\" type=\"dssObject\">\n";
  s+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
  s+= "    <description>" + description + "</description>\n";
  s+= "    <interface>" + getLocalId() + "</interface>\n";
  s+= "    <dpsid>" + getDpsId() + "</dpsid>\n";
  s+= "    <channel>" + getChannel() + "</channel>\n";
  s+= "    <type>" + itoa(getType()) + "</type>\n";
  s+= "    <active>" + itoa(getActive()) + "</active>\n";
  s+= "    <open_count>" + itoa(getOpenCount()) + "</open_count>\n";
  s+= "    <last_open>" + itoa(getLastOpen()) + "</last_open>\n";
  s+= "    <last_close>" + itoa(getLastClose()) + "</last_close>\n";
  s+= "    <updated>" + itoa(getUpdated()) + "</updated>\n";
  s+= "    <contacted>" + itoa(getContacted()) + "</contacted>\n";
  s+= "    <current_status>" + itoa(getStatus()) + "</current_status>\n";
  s+= "  </table>\n";

  syncIDs.push(getId());

  return;
}
