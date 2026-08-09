/*
  File:    logMan/logMan.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/


#include "dpsframework.h"
#include "LogMan.h"
#include "getopt.h"
#include "dpsglobal.h"
#include "dpsinit.h"
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>

#define LOGFILE "/tmp/logMan.log"

log4cpp::Category& logger = log4cpp::Category::getRoot();

int 
main(int argc, char *argv[]) {
  std::string initFileName = "log4cpp.properties";
  log4cpp::PropertyConfigurator::configure(initFileName);
  std::string tag = "logMan";
  log4cpp::NDC::push(tag);

  dpsInit( argc, argv );

  LogMan* man = new LogMan();

  logger.info( "Logging Manager running" );

  man->setTag( tag );
  man->loadConfig();

  man->run();

  delete man;

  return EXIT_SUCCESS;
}
