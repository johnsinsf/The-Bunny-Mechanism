/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "System_host.h"
#include <sys/statvfs.h>
#include <math.h>
#include <dirent.h>
#include <iomanip>

int 
System_host::main( DssObject& o ) {
  load_config( o ); 
  host.setHostname( o.hostname );

  if( directories.size() || processes.size() || scripts.size() ) {
    run( o );
  } else {
    logger.error("no directories/processes found, exiting");
  }
  return 0;
}

void
System_host::load_config( DssObject& o ) {
  typedef map<string,string>::const_iterator I;
  string d;

  I i = o.server->configMap.find("directories" + o.idx );

  if( i != o.server->configMap.end() ) {
    d = i->second;
    d = trim(d);

    istringstream s (d);

    int m = 0;
    while( ! s.eof() ) {
      string t;
      s >> t;
      directories[m++] = trim(t);
    }
  }
  i = o.server->configMap.find("warn_percent" + o.idx);
  if( i != o.server->configMap.end() )  {
    warn_percent = atof(i->second.c_str());
    logger.error("have warn percent " + ftoa(warn_percent));
  }
  i = o.server->configMap.find("load_warn" + o.idx);
  if( i != o.server->configMap.end() )  {
    load_warn = atoi(i->second.c_str());
    logger.error("have load warn " + itoa(load_warn));
  }
  i = o.server->configMap.find("loop_interval" + o.idx);
  if( i != o.server->configMap.end() )  {
    loop_interval = atoi(i->second.c_str());
  } 
  logger.error("have loop interval " + itoa(loop_interval));

  i = o.server->configMap.find("processes" + o.idx );

  if( i != o.server->configMap.end() ) {
    d = i->second;
    d = trim(d);

    istringstream s (d);

    int m = 0;
    while( ! s.eof() ) {
      string t;
      s >> t;
      processes[m++] = trim(t);
    }
  }
  i = o.server->configMap.find("scripts" + o.idx );

  if( i != o.server->configMap.end() ) {
    d = i->second;
    d = trim(d);

    istringstream s (d);

    int m = 0;
    while( ! s.eof() ) {
      //string t;
      //s >> t;
      //scripts[m++] = trim(t);
      char buf[2048];
      s.getline(buf, 2048);
      int pos = 0;
      string t(buf);
      string t2 = getWord( t, pos );
      while( t2 != "" ) {
        scripts[m++] = trim(t2);
        logger.error("have script " + t2);
        t2 = getWord( t, pos );
      }
    }
  }

  return;
}

int
System_host::read_proc_entry(int pid, int opt) {
  string s = "/proc/" + itoa(pid) + "/exe";
  int rc = -1;
  char fbuf[MAXFILENAME];

  memset(fbuf, 0, sizeof(fbuf));
  
  int i = readlink(s.c_str(), fbuf, MAXFILENAME-1);
  if( i > 0 )
    rc = 1;
  else {
    if( opt == 1 )
      logger.error("error reading " + s);
    return -1;
    //rc = -1;
  }
  string s2(fbuf);
  if( rc && opt ) {
    procs[s2] = pid;
  }
  if( ! rc ) {
    procs[s2] = 0;
    cmdlines[s2] = 0;
  }

  s = "/proc/" + itoa(pid) + "/cmdline";
  char cbuf[128];

  memset(cbuf, 0, sizeof(cbuf));
  
  int fd = open( s.c_str(), O_RDONLY );
  if( fd > 0 )
    i = read(fd, cbuf, sizeof(cbuf));
  if( i > 0 ) {
    rc = 1;
    bool done = false;
    for( int x = 0; x < i && ! done; x++ ) {
      if( cbuf[x] == 0 ) {
        cbuf[x] = ' ';
        done = true;
      }
    }
  } else {
    logger.error("error reading " + s);
    rc = -1;
  }
  if( fd > 0 ) {
    close( fd );
    fd = -1;
  }
  string s3(cbuf);
  if( rc && opt ) {
    cmdlines[s3] = pid;
  }
  if( ! rc ) {
    cmdlines[s3] = 0;
  }
  
  logger.error("read proc " + s2 + " " + s3 + " " + itoa(pid));

  return rc;
}

void
System_host::load_processes( void ) {
  struct dirent **namelist;
  int n;

  n = scandir("/proc/", &namelist, NULL, alphasort);
  if( n != -1 ) {
    while( n-- ) {
      if( isdigit(*namelist[n]->d_name) ) {
        int i = atoi(namelist[n]->d_name);
        if( i >= 0 && i <= 9999999 ) {
          read_proc_entry(i, 1);
        }
      }
      free(namelist[n]);
    }
    free(namelist);
  }
}

int
System_host::check_process( string p ) {

  int rc = -1;

  logger.error("checking " + p);

  int pid = procs[p];

  if( pid < 1 )
    pid = cmdlines[p];

  if( pid > 0 ) {
    rc = read_proc_entry(pid, 0);
  }
  if( rc < 1 ) {
    // reload to see if a new pid
    logger.error("reloading " + p);
    procs[p] = 0;
    cmdlines[p] = 0;
    load_processes();
    int pid = procs[p];
    if( pid < 1 )
      pid = cmdlines[p];
    if( pid > 0 )
      rc = 1;
    else {
      map<string, int>::iterator i;
      for( i = procs.begin(); i != procs.end(); ++i ) {
        logger.error( "proc: " + to_string(i->second) );
      }
    }
  }
  //logger.error("check " + p + " " + itoa(pid) + " rc " + itoa(rc));

  return rc;
}

void
System_host::run( DssObject& o ) {
  // signal we are ready
  semRelease( o.semInitID );

  logger.error("System_host is running on directories:");

  map<int, string>::iterator i;
  for( i = directories.begin(); i != directories.end(); ++i ) {
    logger.error( "directory: " + i->second );
  }

  for( i = processes.begin(); i != processes.end(); ++i ) {
    logger.error( "process: " + i->second );
  }

  for( i = scripts.begin(); i != scripts.end(); ++i ) {
    logger.error( "script: " + i->second );
  }

  while( ! g_quit ) {
    string var2, msg;

    for( i = scripts.begin(); i != scripts.end(); ++i ) {
      if( check_process( i->second ) < 1 ) {
        if( msg.size() ) msg += "\n";
        msg += "script " + i->second + " not running on " + o.hostname;
      }
    }

    for( i = processes.begin(); i != processes.end(); ++i ) {
      if( check_process( i->second ) < 1 ) {
        if( msg.size() ) msg += "\n";
        msg += "process " + i->second + " not running on " + o.hostname;
      }
    }

    for( i = directories.begin(); i != directories.end(); ++i ) {
      struct statvfs vfs;
      int rc = statvfs( i->second.c_str(), &vfs );
      if( rc == 0 ) {
        if( vfs.f_blocks > 0 ) {

          double b = ((double)(vfs.f_blocks - vfs.f_bavail) / (double)vfs.f_blocks ) * 100.0;
          double f = ((double)(vfs.f_files - vfs.f_favail) / (double)vfs.f_files ) * 100.0;

          if( g_logLevel > 0 )
            logger.error("directory " + i->second + " at " + string(ltoa(roundl(b))) + " inodes " + string(ltoa(roundl(f))));

          if( var2.size() ) var2 += " "; 
          var2 += i->second + "=" + string(ltoa(roundl(b)));

          if( roundl(b) >= warn_percent || roundl(f) >= warn_percent ) {
            if( msg.size() ) msg += "\n";
            msg += i->second + " over warning limit on " + o.hostname;
          }
        }
      } else {
        logger.error("unable to stat directory:" + i->second + ".");
      }
    }
#ifndef _HAVEMACOS
    int fd = open( "/proc/loadavg", O_RDONLY );
    if( fd > 0 ) {
      char buf[80];
      memset(buf, 0, sizeof(buf));
      int rc = read( fd, &buf, sizeof(buf) );
      if( rc > 0 ) {
        string s(buf);
        s = rtrim(s);
        host.setVar1( s );
      }
      close(fd);
    }
#else
    double loadavg[3];
    int rc = getloadavg( loadavg, 3 );
    if( rc == 3 ) {  
      ostringstream s;
      s << setprecision(3) << loadavg[0] << ", " << setprecision(3) << loadavg[1] << ", " << setprecision(3) << loadavg[2];
      string s2 = s.str();
      host.setVar1( s2 );
    }
#endif

    string out;
    host.setVar2( var2 );
    host.data_sent = false;

    host.export_data_XML( out );

    if( msg.size() ) {
      alert.setHostname(o.hostname);
      alert.setDateTime(time(NULL));
      alert.setSource("System_host");
      alert.setPriority(1);
      alert.setList  ("default");
      alert.setAlertMsg(msg);
      alert.export_data_XML( out );
      logger.error("sending alert " + msg);
      msg = "";
    }

    send_data( o, out );

    time_t nextloop = time(NULL) + loop_interval;

    while( time(NULL) < nextloop && ! g_quit ) {
      sleep(5);
    }
  }
  return;
}

void
System_host::send_data( DssObject& o, const string& msg ) {
  if( g_logLevel > 2 )
    logger.error("send_data -> add_client_data " + msg);

  o.server->add_client_data( o, msg );

  return;
}
