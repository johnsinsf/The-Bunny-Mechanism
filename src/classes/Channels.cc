/*
  File:    dps/Channels.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "dpsframework.h"
#include "Channels.h"
#include "Channels_digital.h"
#include "Channels_analog.h"

int
Channels::insert( int update ) {

  if( ! update ) {
    search = "INSERT channels (id, description, interface, channel, direction, type, active, updated, synced) VALUES ( 0,'" + description + "'," + itoa(interface) + ",'" + channel + "'," + itoa(direction) + "," + itoa(type) + "," + itoa(active) + "," + itoa(updated) + "," + itoa(synced) + ")";
  } else {
    search = "UPDATE channels SET updated = " + ltoa(time(NULL)) + ", synced = " + itoa(synced) + ", description = '" + description + "', direction = " + itoa(direction) + ", active = " + itoa(active) + " WHERE interface = " + itoa(interface) + " AND channel = '" + channel + "'";
  }
  return SBase::insert( search );
}

void
Channels::setSearch( int by ) {
  search = "SELECT c.id, c.description, c.interface, c.channel, c.type, c.active, c.updated, c.direction, c.synced, f.localid, ct.fio FROM channels c ";
  search += " JOIN interfaces f ON c.interface = f.id";
  search += " LEFT JOIN channel_types ct ON ct.channel = c.channel AND ct.interfacetype = f.type";

  readMode = READ;
  switch( by ) {
    default:
    case byId:
      search += " WHERE c.id = " + itoa(id);
      break;
    case byKey:
      search += " WHERE c.interface = " + itoa(interface) + " AND c.channel = '" + channel + "'";
      break;
    case byInterface:
      search += " WHERE c.interface = " + itoa(interface);
      readMode = READSEQ;
      break;
    case bySynced:
      search += " WHERE c.synced = 0";
      readMode = READSEQ;
      break;
  }
  int i = 0;
  addResultVar( &id, i );
  addResultVar( &buf2, i );
  addResultVar( &interface, i );
  addResultVar( &buf, i );
  addResultVar( &type, i );
  addResultVar( &active, i );
  addResultVar( &updated, i );
  addResultVar( &direction, i );
  addResultVar( &synced, i );
  addResultVar( &buf3, i );
  addResultVar( &fio, i );

  return;
}

void
Channels::saveData( dpsID* ptr, int i, Interfaces& f ) {

  int type = ptr[i].type;

  if( type == 1 ) {
    Channels_digital d;
    d.setId(ptr[i].id);
    d.setInterface(ptr[i].interface);
    d.saveData( ptr, i, f ); 
  } else {
    Channels_analog d;
    d.setId(ptr[i].id);
    d.setInterface(ptr[i].interface);
    d.saveData( ptr, i, f ); 
  }
  return;
}

void
Channels::setStrings( void ) {
  channel = buf;
  description = buf2;
  localid = buf3;

  return;
}

#ifdef _USELIBXML
int 
Channels::load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) {

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
      if( I->first.node == "version" ) 
        version = I->second.text;
      else
        logger.error("unknown element " + I->first.node + ", consider upgrading");

      ++I;

    } else {
      done = true;
      logger.debug("found end of Channels table");
    }
  }
  return 0;
}
#endif

int
Channels::setClean() {

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
  search = "UPDATE channels SET synced = " + itoa(time(NULL)) + " WHERE id IN (" + s + ")";

  return SBase::insert( search );
}

void
Channels::export_data_XML( string& s) {
  s+= "  <table name=\"channels\" type=\"dssObject\">\n";
  s+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
  s+= "    <description>" + description + "</description>\n";
  s+= "    <interface>" + getLocalId() + "</interface>\n";
  s+= "    <dpsid>" + getDpsId() + "</dpsid>\n";
  s+= "    <channel>" + getChannel() + "</channel>\n";
  s+= "    <type>" + itoa(getType()) + "</type>\n";
  s+= "    <direction>" + itoa(getDirection()) + "</direction>\n";
  s+= "    <active>" + itoa(getActive()) + "</active>\n";
  s+= "    <updated>" + itoa(getUpdated()) + "</updated>\n";
  s+= "  </table>\n";

  syncIDs.push(getId());

  return;
}

dpsID*
Channels::getDpsIds(int& numdpsids) {

  int rc = read( SBase::byInterface );  

  numdpsids = 0;
  dpsID* ptr = new dpsID[MAX_CHANNELS_PER_INTERFACE];

  while( rc == 1 ) {
    //logger.error("step1");

    //logger.error("have dpsid " + itoa(id) + " channel " + channel + " interface " + itoa(interface) + " type " + itoa(type) + " description " + description);

    ptr[numdpsids].id      = id;
    ptr[numdpsids].fio     = fio;
    ptr[numdpsids].channel = channel;
    ptr[numdpsids].interface = interface;
    ptr[numdpsids].direction = direction;
    ptr[numdpsids].name    = description;
    ptr[numdpsids].type    = type;
    ptr[numdpsids].icurrent_state = 0;
    ptr[numdpsids].inew_state     = 0;
    ptr[numdpsids].dcurrent_state = 0;
    ptr[numdpsids].dnew_state     = 0;
    ptr[numdpsids].last_open      = 0;
    ptr[numdpsids].last_close     = 0;
    ptr[numdpsids].ignore_changes = 0;

    if( ++numdpsids < MAX_CHANNELS_PER_INTERFACE )
      rc = readmo();
    else
      rc = 0;
  }
  return ptr;
}

dpsID*
Channels::getDpsIds(int& numdpsids, Channels::ctstruct d[]) {


  numdpsids = 0;
  dpsID* ptr = new dpsID[MAX_CHANNELS_PER_INTERFACE];

  while( d[numdpsids].fio >= 0 ) {

    ptr[numdpsids].id      = numdpsids+1;
    ptr[numdpsids].fio     = d[numdpsids].fio;
    ptr[numdpsids].channel = d[numdpsids].channel;
    ptr[numdpsids].interface = 1;
    ptr[numdpsids].direction = d[numdpsids].direction;
    ptr[numdpsids].name    = d[numdpsids].channel;
    ptr[numdpsids].type    = d[numdpsids].type;
    ptr[numdpsids].icurrent_state = 0;
    ptr[numdpsids].inew_state     = 0;
    ptr[numdpsids].dcurrent_state = 0;
    ptr[numdpsids].dnew_state     = 0;
    ptr[numdpsids].last_open      = 0;
    ptr[numdpsids].last_close     = 0;
    ptr[numdpsids].ignore_changes = 0;

    ++numdpsids;
  }
  return ptr;
}
