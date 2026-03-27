#include "Dsp_share.h"
#include <dirent.h>
#include <sys/stat.h>

#define MAXLEVELS 1000

extern "C" {
  Dsp_share* makeDsp_share() {
    return new Dsp_share;
  }
}

int g_Debugging = 0;

int 
Dsp_share::share_main( DssObject& o ) {

  bool done = false;
  string resp, respheader;
  string sharedir = "/var/dspshare"; // the default
  string req = "/";
  string dir_end;
  string headin;
  string tailin;
  int index = 0;
  int start_index = 0;
  bool moreData = false;
  bool initialize = true;
  stack<string> dirs;

  typedef map<string,string>::const_iterator I;

  I i = _dpsServer->configMap.find("configidx");

  string idx;
  if( i != _dpsServer->configMap.end() )
    idx = i->second;

  i = o.server->configMap.find("sharedir" + idx);

  if( i != o.server->configMap.end() ) {
    logger.error("sharedir found");
    sharedir = i->second;
  }
  i = o.server->configMap.find("localid" + idx);

  if( i != o.server->configMap.end() ) {
    logger.error("localid found");
    o.localid = i->second;
  }

  i = o.server->configMap.find("makefilespublic" + idx);

  makeFilesPublic = false;

  if( i != o.server->configMap.end() ) {
    logger.error("makefilespublic found");
    if( i->second == "yes" || i->second == "on" ) {
      makeFilesPublic = true;
      logger.error("making files PUBLIC");
    }
  }

  logger.error("share_main " + sharedir + req);

  int cnt = 0;

  char logname[128];
  struct tm t;
  time_t now = time(NULL);
  localtime_r( &now, &t );
  strftime( logname, sizeof(logname), "%Y%m%d%H%i%s", &t );

  string fname = string(INSTALLDIR) + "/logfiles/" + logname;

  int fd = open( fname.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU );
  int rc = -1;

  if( fd != -1 )
    rc = create_dirents_file( sharedir, fd );

  struct stat sb1, sb2;
  
  int rc1 = -1, rc2 = -1;

  if( fd > 0 ) {
    rc1 = fstat( fd, &sb1 );
  }
  string fname2 = string(INSTALLDIR) + "/logfiles/current";
  int fd2 = open( fname2.c_str(), O_RDONLY );
  if( fd2 > 0 ) {
    rc2 = fstat( fd2, &sb2 );
  }

  if( rc1 == 0 && rc2 == 0 ) {
    if( sb1.st_size != sb2.st_size ) {
      logger.error("file directory listing has changed, sending up the new list, size " + to_string(sb1.st_size) );
    } else {
      // same size, compare line by line
      char buf1[4096]; 
      char buf2[4096]; 
      bool comparedone = false;
      int errors = 0;
      close( fd );
      fd = open( fname.c_str(), O_RDONLY );
      while( ! comparedone ) {
        memset(buf1, 0, sizeof(buf1));
        memset(buf2, 0, sizeof(buf2));
        rc1 = read( fd, &buf1, sizeof(buf1) );
        rc2 = read( fd2, &buf2, sizeof(buf2) );
        if( rc1 <= 0 || rc2 <= 0 ) { 
          comparedone = true;
        } else {
          if( memcmp(buf1, buf2, sizeof(buf1)) != 0 ) {
            comparedone = true;
            errors++;
            logger.error("files are different, sending new directory listing.");
          }
        }
      }
      if( errors == 0 ) {
        logger.error("files are the same, not sending directory listing.");
        done = true;
      }
    }
  }

  if( fd > 0 ) {
    close( fd );
  }
  if( fd2 > 0 ) {
    close( fd2 );
  }

  // remove the "current" file
  unlink(fname2.c_str());

  // make a link from "current" to the latest file
  symlink( fname.c_str(), fname2.c_str() );

  while( ! done && ! g_quit ) {

    int rc = getDirectory( resp, respheader, headin, tailin, req, SIZELIMIT1, sharedir, moreData, dir_end, start_index, index, dirs, initialize, o, fname );

    //if( g_Debugging )
      logger.error("GETDIR: req " + req + " more " + itoa(moreData) + " end " + dir_end + " index " + itoa(index));

    if( rc == 0 ) {
      o.server->add_client_data( o, resp );
      string fname = string(INSTALLDIR) + "/test/xml." + to_string(mylogctr++);
      int fd = open( fname.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU );
      if( fd > 0 ) {
        write( fd, resp.c_str(), resp.size() );
        close( fd );
      }
    } else {
      g_quit = true;
      logger.error("error in getDirectory");
    }

    // testing
    //if( cnt++ > 1 ) done = true;
    //done = true;
    //cnt++;

    initialize = false;
    if( ! moreData ) 
      done = true;

    if( ! g_quit && ! done ) {
      resp = "";
      int x = dirs.size();
      while( dirs.size() > 0 ) {
        string s = dirs.top();
        if( g_Debugging )
          logger.error("TEST s = " + s);
        if( s != dir_end ) {
          int i = s.rfind("/");
          if( i > 0 ) {
            s = s.substr( i + 1, s.size() - i - 1);
            if( g_Debugging )
              logger.error("NEW s = " + s);
          }
          if( g_Debugging )
            logger.error("pushing add dir entry " + s);
          string m;
          for( int i = 0; i < x - dirs.size(); i++ ) m += " ";
          char buf[80 + s.size() * 2];
          memset(&buf, 0, sizeof(buf));
          dpsencode( s.c_str(), buf, s.size() );

          headin += "\n   " + m + "<entry><n>" + string(buf) + "</n><md>-1</md><cd>-1</cd><fs>-1</fs><t>d</t>";
        } else {
          if( g_Debugging )
            logger.error("skipped " + s);
        }
        dirs.pop();
      }
      moreData = false;
      start_index = index + 1;
      if( g_Debugging )
        logger.error("MORE starting at " + itoa(start_index) + " resp " + resp );
      index = 0;
      while( dirs.size() > 0 ) {
        if( g_Debugging )
          logger.error("popping " + itoa(dirs.size() ));
        dirs.pop();
      }
    }
  }

  done = false;

  while( ! done && ! g_quit ) {
    sleep(60);
    logger.error("sleeping");
    // todo wait for directory/file changes and send updates
  }

  return 0;
}

int 
Dsp_share::main( DssObject& o ) {

  semRelease( o.semInitID );

  logger.error("share checking for controlport");
  typedef map<string,string>::const_iterator I;

  I i = o.server->configMap.find("startcontrolport");

  _dpsServer = o.server;
  semRespID = o.semRespID;

  logger.error("have dispatch sem " + itoa(o.semDataID));
  logger.error("have dispatch sem2 " + itoa(o.semRespID));

  if( i != o.server->configMap.end() && i->second == "on" ) {
    logger.error("share controlport found");
    startThread(true, 2);
  }
  else
    logger.error("share controlport NOT found");

  share_main(o);

  return 0;
}

void
Dsp_share::send_data( DssObject& o, string& msg ) {

  dataType d;
  d.structtype = 0;
  d.datatype   = 5;
  d.localid    = o.localid;
  d.channel    = "dsp share";
  d.synced     = 0;
  d.tries      = 0;
  d.datetime_sent = 0;
  d.data       = msg;
  d.msgid      = ltoa(msgid++);
  d.msgtime    = ltoa(time(NULL));
 
  o.server->add_client_data( o, d );

  return;
}

// main loop for the "control thread", receives messages from host and processes them

int
Dsp_share::doControl( int& threadID ) {

  logger.error("share in doControl");

  int localidsem = 0, semNum = 0, offset = 0;
  string idx, ip, localid, sharedir;

  typedef map<string,string>::const_iterator I;
  I i = _dpsServer->configMap.find("configidx");

  if( i != _dpsServer->configMap.end() )
    idx = i->second;

  i = _dpsServer->configMap.find("localidsem" + idx);
  if( i != _dpsServer->configMap.end() ) {
    localidsem = atoi(i->second.c_str());
  }

  logger.error("checking sharedir " + idx);

  i = _dpsServer->configMap.find("sharedir" + idx);

  if( i != _dpsServer->configMap.end() ) {
    sharedir = i->second;
  }

  logger.error("sharedir is " + sharedir);

  i = _dpsServer->configMap.find("localid" + idx);

  if( i != _dpsServer->configMap.end() ) {
    localid = i->second;
  }

  i = _dpsServer->configMap.find("makefilespublic" + idx);

  makeFilesPublic = false;

  if( i != _dpsServer->configMap.end() ) {
    logger.error("makefilespublic found");
    if( i->second == "yes" || i->second == "on" ) {
      logger.error("making files PUBLIC");
      makeFilesPublic = true;
    }
  }

  localidsem = _dpsServer->localidSems[localid];

  _dpsServer->sems.getClientSem( localidsem, semNum, offset );

  logger.error("handleCommand ip is " + string(ip) + " localid " + localid + " sem " + to_string(semNum) + " offset " + to_string(offset) + " localidsem " + itoa(localidsem));

  DssObject o;
  o.localid = localid;

  while ( ! g_quit ) {
    time_t timeoutns = 25000000;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = timeoutns - 10;
    nanosleep(&ts, NULL);
    int rc = _dpsServer->sems.semGet( semNum, offset, 0, 10 );
    if( rc == 0 ) { 
      logger.error("dspshare received a signal, doing commands, localid " + localid + " " + to_string(localidsem) + " " + sharedir);
      int commandrc = handleCommand( localid, localidsem, sharedir );
#ifdef _HAVEMACOS
      logger.error("have macos command resp " + itoa(commandrc) + " signaling " + itoa(semRespID) );
      if( ! semRespID ) {
        logger.error("dispatch error");
      } else {
        dispatch_semaphore_signal( semRespID );
        logger.error("done");
      }
#else   
      logger.error("have command resp " + itoa(commandrc) + " signaling " + itoa(semRespID) );
      semRelease( semRespID );
      logger.error("done");
#endif  
    }
  }
  return 0;
}

int
Dsp_share::handleCommand( string localid, int localidsem, string sharedir ) {

  logger.error("handleCommand on " + sharedir);

  DssObject o;
  o.localid = localid;

  int rc = 0;
  if( pthread_mutex_lock( &_dpsServer->controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to lock mutex" );
    g_quit = true;
  } else {
    multimap<string, commType>::iterator I2 = _dpsServer->localidComm.find( localid );
    while( I2 != _dpsServer->localidComm.end() ) {

      logger.error("found command " + I2->second.op + " cmd " + I2->second.which + " val " + I2->second.strval);

      if( I2->second.op == "getfile" ) {
        string req = I2->second.which;
        if( req == "" ) req = "/";
        string resp, respheader;
        bool moreData = false;
        int rc2 = getFile( resp, respheader, req, SIZELIMIT1, sharedir, moreData, I2->second.strval );
        I2->second.resp = resp;
        I2->second.respheader = respheader;
        I2->second.status = 2;
        I2->second.rc = rc2;
      }
      else
      if( I2->second.op == "getfilestatus" ) {
        string req = I2->second.which;
        if( req == "" ) req = "/";
        string resp, respheader;
        int rc2 = getFileStatus( resp, respheader, req, sharedir, I2->second.strval );
        I2->second.resp = resp;
        I2->second.respheader = respheader;
        I2->second.status = 2;
        I2->second.rc = rc2;
      }
      else
      if( I2->second.op == "getdirectory" ) {
        string req = I2->second.which;
        if( req == "" ) req = "/";
        string resp, respheader;
        string dir_end;
        string headin, tailin;
        int index, start_index;
        bool moreData = false;
        index = start_index = 0;
        stack<string> dirs;
        int rc2 = getDirectory( resp, respheader, headin, tailin, req, SIZELIMIT1, sharedir, moreData, dir_end, start_index, index, dirs, false, o, "" );
        I2->second.resp = resp;
        I2->second.respheader = respheader;
        I2->second.status = 2;
        I2->second.rc = rc2;
        //logger.error("have resp " + resp);
        logger.error("have resp " + itoa(resp.size()));
        rc++;
      }
      ++I2;
    }
  }
  if( pthread_mutex_unlock( &_dpsServer->controlMutex ) != 0 ) {
    logger.error( "ERROR: failed to unlock mutex" );
    g_quit = true;
  }
  return rc;
}

// saves the directory to a file in the INSTALLDIR/logfiles directory

int
Dsp_share::create_dirents_file( string dir_in, int fd ) {

  struct dirent **namelist;
  struct stat statbuf;
  int rc = 0;
  int n;
  int index = 0;
  int level = 0;

  //if( g_Debugging || index >= start_index )
    //logger.error("get_dirents " + dir_in + " level " + itoa(level) + " max " + itoa(maxResp) + " index " + itoa(index) + " start " + itoa(start_index) + " inlen " + itoa(inlen));

  int i = scandir(dir_in.c_str(), &namelist, NULL, alphasort);

  string out;

  if( i != -1 ) {
    for( int n = 0; n < i; n++ ) {
      index++;
      //if( g_Debugging )
        //logger.error("index = " + itoa(index));

      string s = string(namelist[n]->d_name);

      int t = namelist[n]->d_type;
      if( s.substr(0,1) != "." ) {
        rc = stat( (dir_in + "/" + s).c_str(), &statbuf );
        if( rc == 0 ) {
          string x = dir_in + "/" + s;
          out += x + "\n";

          if( t == 4 ) {
            level++;
            create_dirents_file( x, fd );
            level--;
          }
        } else {
          if( g_Debugging )
            logger.error("error stat: " + itoa(rc) + " " + s);
        }
      }
      free(namelist[n]);
    }
    free(namelist);
  }
  if( fd > 0 ) {
    write( fd, out.c_str(), out.size() );
  }
  return 1;
}

void
Dsp_share::mapdirs( string s, map<int, string> &m ) {

  if( s.size() < 1 )
    return;

  int idx = 0;
  int y = 0;
  int x = s.find('/', y + 1);

  while( x > 0 ) {
    string s2 = s.substr( y, y - x );
    int x2 = s2.find('/');
    if( x2 > 0 )
      s2 = s2.substr(0, x2);
    m[idx++] = s2;
    y = x + 1;
    x = s.find('/', y);
  }
  if( y < s.size() ) {
    string s2 = s.substr(y, s.size() - y);
    m[idx] = s2;
  }
}

string
Dsp_share::get_dirents( string infilename, string sharedir, int maxResp, bool& moreData, string& end, int start_index, int& index, stack<string>& dirs ) {

  struct stat statbuf;
  int rc = 0;
  bool done = false;
  string out;
  string currdir, prevdir;
  ifstream infile;

  infile.open ( infilename.c_str(), ios::in );

  if( ! infile.is_open() ) {
    logger.error("can't open input file " + infilename);
    return "";
  }

  char line[2048];

  // skip over files already sent, send in batches of 500K bytes
  if( start_index > 0 ) {
    infile.getline((char*)&line, sizeof(line), '\n');
    done = false;
    while( ! infile.eof() && ! done && ! g_quit ) {
      if( index++ >= start_index ) {
        done = true;
      } else {
        infile.getline((char*)&line, sizeof(line), '\n');
      } 
    } 
  } 

  done = false;

  while( ! infile.eof() && ! done && ! g_quit ) {
    memset(line, 0, sizeof(line));
    infile.getline((char*)&line, sizeof(line), '\n');
    if( strlen(line) > sharedir.size() ) {
      string s(line);
      // strip the sharedir from the front
      s = s.substr(sharedir.size() + 1, s.size() - sharedir.size() - 1);
      if( g_Debugging )
        logger.error("stripped s is " + s + " sharedir " + sharedir);
      int level = 0;
      for( int i = sharedir.size(); i < strlen(line); i++ )
        if( line[i] == '/' ) level++;
      int x = s.find_last_of("/");
      if( x > 0 ) {
        currdir = s.substr(0, x);
        s = s.substr( x + 1, s.size() - x - 1 );
        if( g_Debugging )
          logger.error("s is " + s + " curr " + currdir);
      } else {
        currdir = "";
      }

      if( g_Debugging )
        logger.error("test s is 2 " + s + " curr " + currdir + " prev " + prevdir);

      if( currdir != prevdir ) { 
        map<int, string> currmap;
        map<int, string> prevmap;
        mapdirs( currdir, currmap );
        mapdirs( prevdir, prevmap );
        typedef map<int, string>::const_iterator I;
        I i;
        bool done2 = false;
        int x = currmap.size();
        int y = prevmap.size();
        string s1, s2;
  
        // first close any directories that aren't in this file
        if( prevdir != "" ) {
          int a = y;
          while( ! done2 ) {
            i = currmap.find(a);
            i != currmap.end() ?  s1 = i->second: s1 = "";
            i = prevmap.find(a);
            i != prevmap.end() ?  s2 = i->second: s2 = "";
            if( s1 != s2 && s2 != "" ) {
              string m;
              for( int i = 0; i < a + 4; i++ ) m += " ";
              out += m + "</entry> /* " + dpsaddslashes(s2.c_str(), s2.size()) + " */ \n";
            }
            if( a-- <= 0 ) done2 = true;
          }
        }
        done2 = false;
        // open any new directories in this file
        x = 0;
        while( ! done2 ) {
          i = currmap.find(x);
          i != currmap.end() ?  s1 = i->second: s1 = "";
          i = prevmap.find(x);
          i != prevmap.end() ?  s2 = i->second: s2 = "";

          if( g_Debugging )
            logger.error("currmap " + itoa(currmap.size()) + " s1 " + s1 + " s2 " + s2);

          if( x < currmap.size() &&  s1 != s2 ) {
            rc = stat( (sharedir + "/" + currdir).c_str(), &statbuf ); 
            if( g_Debugging )
              logger.error("statting dir " + sharedir + "/" + currdir + " rc " + itoa(rc));
            if( rc == 0 ) {
              if( ((statbuf.st_mode & S_IFMT) == S_IFDIR) && (s.substr(0,1) != ".") ) {
                string m;
                for( int i = 0; i < x + 4; i++ ) m += " ";

                string n = currmap[x];
                char buf[80 + s.size() * 2];
                memset(&buf, 0, sizeof(buf));
                dpsencode( n.c_str(), buf, n.size() );

                out +=  m + "<entry><n>" + string(buf) + "</n>";
                out += "<md>" + to_string(statbuf.st_mtime) + "</md>";
                out += "<cd>" + to_string(statbuf.st_ctime) + "</cd>";
                out += "<fs>" + to_string(statbuf.st_size) + "</fs>";
                if( makeFilesPublic ) {
                  // make all file open to the public using encoded URLs
                  out += "<ac>world</ac>";
                }
                out += "<t>d</t> \n";
              }
            }
          }
          x++;
          if( x > currmap.size() ) done2 = true;
        }
        //logger.error("done");
        prevdir = currdir;
      }
      if( ! infile.eof() && s.size() <= 255 && out.size() < maxResp ) {
        index++;
        if( g_Debugging )
          logger.error("statting file " + string(line) + " s " + s);
        rc = stat( line, &statbuf ); 
        if( rc == 0 ) {
          if( ((statbuf.st_mode & S_IFMT) == S_IFREG) && (s.substr(0,1) != ".") ) {
            string m;
            for( int i = 0; i < level + 3; i++ ) m += " ";
            char buf[80 + s.size() * 2];
            memset(&buf, 0, sizeof(buf));
            dpsencode( s.c_str(), buf, s.size() );
            out += m + "<entry><n>" + string(buf) + "</n>";
            out += "<md>" + to_string(statbuf.st_mtime) + "</md>";
            out += "<cd>" + to_string(statbuf.st_ctime) + "</cd>";
            out += "<fs>" + to_string(statbuf.st_size) + "</fs>";
            if( makeFilesPublic ) {
              // make all file open to the public using encoded URLs
              out += "<ac>world</ac>";
            }
            out += "<t>f</t></entry>\n";
          }
        } else {
          if( g_Debugging )
            logger.error("error stat: " + itoa(rc) + " " + s);
        }
      }
    }
    if( out.size() >= maxResp ) {
      done = true;
      end = line;
      moreData = true;
      if( g_Debugging )
        logger.error("MOREDATA set to TRUE end " + end);
    }
  }
  map<int, string> currmap;
  mapdirs( currdir, currmap );
  typedef map<int, string>::const_iterator I;
  for( int i = currmap.size(); i > 0; i-- ) {
    string m;
    for( int j = 0; j <  i + 3; j++ ) m += " ";
    //out += m + "</entry> /* " + dpsaddslashes(currmap[i - 1].c_str(), currmap[i - 1].size()) + " */ \n";
    out += m + "</entry>\n";
  } 
  if( infile.is_open() )
    infile.close();

  if( g_Debugging )
    logger.error("get_dirents DONE " + string(line) + " end " + end + " more " + itoa(moreData) + " len " + itoa(out.size()));

  return out;
}

int 
Dsp_share::getFileStatus( string &resp, string &respheader, string req, string sharedir, string strval ) {

  logger.error("getFileStatus " + sharedir + "/" + req); 
  resp = "";
  respheader = "";
  int rc = 0;

  string filename = sharedir + "/" + req;
  struct stat fs;

  rc = stat( filename.c_str(), &fs );

  if( rc == 0 ) {
    string op = "getfilestatus";
    resp = "[{\"op\":\"" + op + "\",\"which\":\"" + filename + "\",\"size\":" + to_string(fs.st_size) + ",\"modified\":" + to_string(fs.st_mtime) + "}]";
    respheader = resp;
  }

  return rc;
}

int 
Dsp_share::getFile( string &resp, string &respheader, string req, int maxResp, string sharedir, bool& moreData, string strval ) {

  //if( g_Debugging )
    logger.error("getFile " + sharedir + "/" + req + " strval " + strval + " max " + to_string(maxResp));

  resp = "";
  respheader = "";
  string type, op, which, localid, token, reqid, authcode; 
  int offset = 0, maxlen = 0, stan = 0;

#ifdef _USEJSON
  char* endptr = NULL;
  JsonValue value;
  JsonAllocator allocator;

  int rc = jsonParse((char*)strval.c_str(), &endptr, &value, allocator);
  if( rc != 0 ) { 
    logger.error("ERROR: can't parse input");
    return SENDACK;
  }
  JsonIterator i = begin(value);
  JsonIterator j = end(value);

  int tag = 0;

  while( i.p != j.p && ! g_quit ) {
    tag  = i.p->value.getTag();

    //logger.error("tag: " + to_string(tag));

    if( tag == JSON_ARRAY || tag == JSON_OBJECT ) {   
      JsonIterator i2 = begin(i.p->value);
      JsonIterator j2 = end(i.p->value);

      while( i2.p != j2.p && ! g_quit ) {
        tag = i2.p->value.getTag();
        type = string(i2.p->key);
        //logger.error("tag " + to_string(tag) + " " + type);
        if( tag == JSON_NUMBER ) {
          long n = i2.p->value.toNumber();
          //logger.error("number " + type + " " + to_string(n));
          if( type == "offset" )
            offset = n;
          else
          if( type == "stan" )
            stan = n;
          else
          if( type == "len" )
            maxlen = n;
        }
        else
        if( tag == JSON_STRING ) {
          char *p = i2.p->value.toString();
          string n(p);
          //logger.error("string " + type + " " + n );
          if( type == "op" )
            op = n;
          else
          if( type == "which" )
            which = n;
          else
          if( type == "offset" )
            offset = atoi(p);
          else
          if( type == "len" )
            maxlen = atoi(p);
        }
        i2.p = i2.p->next;
      }
    } 
    i.p = i.p->next;
  }
#endif

  // add in check of authorization level etc
  
  if( maxlen > 0 && maxlen < maxResp )
    maxResp = maxlen;

  string filename = sharedir + "/" + req;
  struct stat fs;
  //char buf[maxResp];
  //memset(buf, 0, sizeof(buf));
  char* buf = (char*)malloc(maxResp);
  memset( buf, 0, maxResp );

  rc = stat( filename.c_str(), &fs );
  unsigned long s = fs.st_size;

  if( rc == 0 ) {
    int fd = open( filename.c_str(), O_RDONLY );
    if( g_Debugging )
      logger.error("open " + filename + " " + to_string(fd));
    if( fd > 0 ) {
      int rc2 = 0;
      if( offset != 0 ) {
        unsigned long off = offset;
        rc2 = lseek( fd, off, SEEK_SET );
        if( g_Debugging )
          logger.error("seek " + filename + " " + to_string(rc2) + " max " + itoa(maxlen) + " maxResp " + itoa(maxResp));
        if( rc2 == off ) {
          s -= off;
        } else {
          logger.error("seek failed!");
          rc2 = -1;
        }
      }
      if( s > maxResp ) s = maxResp;
      if( rc2 >= 0 )
        //rc2 = read( fd, &buf, s );
        rc2 = read( fd, buf, s );
      //if( g_Debugging )
        logger.error("read " + filename + " " + to_string(rc2) + " " + to_string(s));
      if( rc2 >= 0 && rc2 == s ) {
        resp = string(buf, rc2);
        rc = 0;
      } else {
        logger.error("read failed!");
        rc = -1;
      }
      close(fd); fd = 0;
    }
  }
  respheader = "[{\"op\":\"" + op + "\",\"which\":\"" + filename + "\",\"offset\":" + to_string(offset) + ",\"len\":" + ltoa(s) + "}]";

  if( buf )
    free(buf);

  return rc;
}

int 
Dsp_share::getDirectory( string &resp, string &respheader, string& headin, string& tailin, string req, int maxResp, string sharedir, bool& moreData, string& dir_end, int start_index, int& index, stack<string>& dirs, bool initialize, DssObject& o, string logfilename ) {

  if( g_Debugging )
    logger.error("getDirectory " + sharedir + " start " + itoa(start_index) + " index " + itoa(index) + " headin " + headin);

  string xmldir;
  if( ! initialize ) 
    xmldir  = "\n <table name=\"files\" type=\"dssObject\" localid=\"" + o.localid + "\">\n";
  else
    xmldir  = "\n <table name=\"files\" type=\"dssObject\" initialize=\"true\" localid=\"" + o.localid + "\">\n";

  string extra;
  if( makeFilesPublic )
    extra = "<ac>world</ac>";

  string s = "/";
  char buf[80 + s.size() * 2];
  memset(&buf, 0, sizeof(buf));
  dpsencode( s.c_str(), buf, s.size() );
 
  xmldir += "\
  <files>\n\
   <entry> <n>" + string(buf) + "</n> <md>0</md>";

  xmldir += extra + "<t>d</t>\n";

  int inlen = xmldir.size() + 25;

  if( g_Debugging )
    logger.error("using heading " + headin);

  // get_dirents( string infilename, int maxResp, bool& moreData, string& end, int start_index, int& index, stack<string>& dirs 
  xmldir += headin + get_dirents( logfilename, sharedir, maxResp, moreData, dir_end, start_index, index, dirs ) + tailin;
  
  headin = tailin = "";

  if( g_Debugging )
    logger.error("have dir_end " + dir_end + " start " + itoa(start_index) + " ind " + to_string(index) + " more " + itoa(moreData) );

  xmldir += "\
   </entry>\n\
  </files>\n\
 </table>\n";

  //if( g_Debugging )
    //logger.error("have xmldir " + xmldir);

  logger.error("response size " + to_string(xmldir.size()));

  resp = xmldir;
  string op = "getDirectory";

  respheader = "[{\"op\":\"" + op + "\",\"which\":\"" + sharedir + "\",\"index\":" + ltoa(index) + "\",\"dir_end\":\"" + dir_end + "\"}]";

  return 0;
}
