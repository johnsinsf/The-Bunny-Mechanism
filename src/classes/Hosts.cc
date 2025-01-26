/*
  File:    classes/Hosts.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include "dpsframework.h"
#include "Hosts.h"
#include <sys/utsname.h>

int
Hosts::insert( int update ) {

  updated = time(NULL);

  string s = "INSERT hosts (hostname, updated) VALUES ( '" + hostname + "'," + itoa(updated) + ")";

  return SBase::insert( s );
}

void
Hosts::setSearch( int by ) {

  search = "SELECT hostname, updated FROM hosts";

  switch( by ) {
    case byKey:
    case byId:
      search += " WHERE hostname = '" + hostname + "'";
      break;
  }
  int i = 0;
  addResultVar(&buf, i);
  addResultVar(&updated, i);
  return;
}

void
Hosts::setStrings(void) {
  hostname = buf;
  return;
}

void    
Hosts::export_data_XML( string& s ) {
 
  if( data_sent ) {
    logger.error("data already sent");
    //return;
  }
  updated = time(NULL);
  if( hostname.size() == 0 )
    hostname = get_hostname();

  tzset ();

  long int offset = -1 * timezone / 60;

  struct utsname uts;
  int rc = uname( &uts );
  string version;
  if( rc == 0 ) {
    //if( strlen(uts.sysname) < 80 )
      //version += uts.sysname;
    if( strlen(uts.release) < 80 ) {
      if( version.size() ) version += " ";
      version += uts.sysname;
    }
    if( strlen(uts.version) < 80 ) {
      if( version.size() ) version += " ";
      version += uts.version;
    }
    if( strlen(uts.machine) < 80 ) {
      if( version.size() ) version += " ";
      version += uts.machine;
    }
  }
  s+= "  <table name=\"hosts\" type=\"dssObject\">\n";
  s+= "    <version name=\"" + string(_DSSCLIENT_VERSION) + "\"/>\n";
  s+= "    <hostname>" + hostname + "</hostname>\n";
  s+= "    <minutes_offset>" + ltoa(offset) + "</minutes_offset>\n";
  s+= "    <updated>" + itoa(updated) + "</updated>\n";
  if( version.size() ) {
    s+= "    <osversion>" + version + "</osversion>\n";
  }
  if( var1.size() ) {
    s+= "    <var1>" + var1 + "</var1>\n";
  }
  if( var2.size() ) {
    s+= "    <var2>" + var2 + "</var2>\n";
  }
  if( var3.size() ) {
    s+= "    <var3>" + var3 + "</var3>\n";
  }
  s+= "  </table>\n";

  data_sent = true;

  return;
}

#ifdef _USELIBXML
int 
Hosts::load( int nodeid, map<eKey, eData>::const_iterator& I, map<eKey, eData, class nodeCmp>& M ) {

  return 0;
}
#endif
