#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "dpsframework.h"
#include "dpsglobal.h"
#include "BaseServer.h"
#include "Icomm.h"
#include "SBase_files.h"
//#include "TestClass.h"
//#include "ShmManN.h"
//#include "ShmManF.h"
//#include "ShmManB.h"
//#include "ShmManV.h"
#include "Json.h"
#include <sys/types.h>
#include <time.h>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>

log4cpp::Category& logger = log4cpp::Category::getRoot();
BaseServer *SBase::_dpsServer = NULL;

using namespace std;

int main(void) {

  std::string initFileName = "log4cpp.conf";
  //std::string initFileName = "log4cpp.properties";
  log4cpp::PropertyConfigurator::configure(initFileName);


  string n = "YnVubnltZWNoLjAzMDkyNi50Z3o=";
  //string n = "Lw==";

  string t = "/";

  char buf[80];
  memset(buf, 0, sizeof(buf));
  dpsencode( t.c_str(), buf, t.size() );

  cout << "buf " << buf << endl;

  memset(buf, 0, sizeof(buf));
  dpsdecode( n.c_str(), buf, n.size());

  cout << "n buf " << n << " " << buf << endl;

  return 0;
}

