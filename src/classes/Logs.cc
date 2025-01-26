/*
  File:    classes/Logs.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include "dpsframework.h"
#include "Logs.h"

int
Logs::insert( int opt ) {

  setUpdated();
  datetime = updated;
  datetime_nsec = updated_nsec;

  string s = "INSERT logs (id, hostname, type, interface, channel, data, datetime, datetime_nsec) VALUES ( 0, '" + hostname + "', " + itoa(type) + "," + itoa(interface) + ",'" + channel + "','" + data + "'," + itoa(datetime) + "," + itoa(datetime_nsec) + ")";

  return SBase::insert( s );
}

int
Logs::setClean() {

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
  search = "UPDATE logs SET synced = " + itoa(time(NULL)) + " WHERE id IN (" + s + ")";

  return SBase::insert( search );
}

void
Logs::setSearch( int by ) {

  search = "SELECT l.id, l.hostname, l.type, l.interface, l.channel, l.data, l.datetime, l.datetime_nsec, i.localid FROM logs l JOIN interfaces i ON i.id = l.interface";
  readMode = READ;
  switch( by ) {
    case byId:
      search += " WHERE l.id = " + itoa(id);
      break;
    case bySynced:
      search += " WHERE l.synced = 0 ORDER BY l.id";
      readMode = READSEQ;
      break;
  }
  int i = 0;
  addResultVar(&id, i);
  addResultVar(&buf4, i);
  addResultVar(&type, i);
  addResultVar(&interface, i);
  //addResultVar(&channel, i);
  addResultVar(&buf, i);
  addResultVar(&buf2, i);
  addResultVar(&datetime, i);
  addResultVar(&datetime_nsec, i);
  addResultVar(&buf3, i);

  return;
}

int
Logs::saveData( dpsID* ptr, long dpsid, string& s ) {
  
  setUpdated();

  interface = ptr[dpsid].interface;
  channel   = ptr[dpsid].channel;
  data      = s;
  type      = 1;
  datetime  = updated;
  datetime_nsec= updated_nsec;
  synced    = 0;
  hostname  = myhostname;
 
  return insert();
}

void
Logs::setStrings( void ) {

  channel = buf;
  data    = buf2;
  localid = buf3;
  myhostname= buf4;

  return;
}

void
Logs::serialOut( string& s) {

  string t = itoa(getType());
  int len = t.size(); 
  s = "";
  if( len < 10 ) s += "0";
  s += itoa(len) + t;
 
  t = itoa(getInterface());
  len = t.size(); 
  if( len < 10 ) s += "0";
  s += itoa(len) + t;

  t = getChannel();
  len = t.size(); 
  if( len < 10 ) s += "0";
  s += itoa(len) + t;

  t = itoa(getDateTime());
  len = t.size(); 
  if( len < 10 ) s += "0";
  s += itoa(len) + t;

  len = data.size(); 
  if( len < 10 ) {
    s += "00" + itoa(len) + data;
  } else if( len < 100 ) {
    s += "0"  + itoa(len) + data;
  } else if ( len < 256 ) {
    s += itoa(len) + data;
  } else {
    logger.error("Logs data truncated, only 256 bytes transferred.");
    s += "256" + data.substr(0,256);
  }
  return;
}

void
Logs::export_data_XML( string& s) {
  s+= "  <table name=\"logs\" type=\"dssObject\">\n";
  s+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
  s+= "    <hostname>" + getHostname() + "</hostname>\n";
  s+= "    <dpsid>" + getDpsId() + "</dpsid>\n";
  s+= "    <type>" + itoa(getType()) + "</type>\n";
  s+= "    <localid>" + getLocalId() + "</localid>\n";
  s+= "    <channel>" + getChannel() + "</channel>\n";
  s+= "    <datetime>" + itoa(getDateTime()) + "</datetime>\n";
  s+= "    <data>" + data + "</data>\n";
  s+= "  </table>\n";

  syncIDs.push(getId());
}
