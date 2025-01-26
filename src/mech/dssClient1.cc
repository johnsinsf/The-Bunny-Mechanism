/*
  File:    mech/dssClient1.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Runs the Bunny Mechanism

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "dpsframework.h"
#include "Mech.h"
#include "getopt.h"
#include "dpsglobal.h"
#include "dpsinit.h"
    
log4cpp::Category& logger = log4cpp::Category::getRoot();

int 
main( int argc, char *argv[] ) {
  std::string initFileName = "log4cpp.properties";
  log4cpp::PropertyConfigurator::configure(initFileName);
  std::string tag = "bunnymech";
  log4cpp::NDC::push(tag);

#ifdef _USEMYSQL
  mysql_library_init(0, NULL, NULL);
#endif

  dpsInit( argc, argv );
  g_isDss = true;

  logger.info( "bunnymech running" );
  
  Mech* c = new Mech();

  c->setTag(tag);
  c->loadConfigMap(string(INSTALLDIR) + "/conf/dss.conf");

  c->run();

#ifdef _USEMYSQL
  mysql_library_end();
#endif

  delete c;

  return EXIT_SUCCESS;
}
