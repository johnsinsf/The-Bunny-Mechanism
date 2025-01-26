/*
  File:    classes/Alerts.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include "dpsframework.h"
#include "Alerts.h"

int
Alerts::insert( int update ) {

  return 0;
}

void
Alerts::setSearch( int by ) {

  return;
}

void
Alerts::setStrings(void) {
  return;
}

void    
Alerts::export_data_XML( string& s ) {
 
  updated = time(NULL);
  if( hostname.size() == 0 )
    hostname = get_hostname();

  s+= "  <table name=\"alerts\" type=\"dssObject\">\n";
  s+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
  s+= "    <hostname>" + hostname + "</hostname>\n";
  s+= "    <datetime>" + itoa(updated) + "</datetime>\n";
  s+= "    <priority>" + itoa(priority) + "</priority>\n";
  s+= "    <source>" + source + "</source>\n";
  s+= "    <list>" + list + "</list>\n";
  s+= "    <alertmsg>" + alertmsg + "</alertmsg>\n";
  s+= "  </table>\n";

  data_sent = true;

  return;
}

