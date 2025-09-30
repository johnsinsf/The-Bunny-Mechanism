#include "Dsp_share.h"
#include <dirent.h>
#include <sys/stat.h>

extern "C" {
  Dsp_share* makeDsp_share() {
    return new Dsp_share;
  }
}


int 
Dsp_share::share_main( DssObject& o ) {

  bool done = false;
  string resp, respheader;
  string sharedir = "/var/dspshare";
  string req = "/";
  string dir_end;
  int index = 0;
  bool moreData = false;
  bool initialize = true;

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

  logger.error("share_main " + sharedir + "/" + req);

  while( ! done && ! g_quit ) {
    int rc = getDirectory( resp, respheader, req, _SIZELIMIT1, sharedir, moreData, dir_end, index, initialize, o );

    if( rc == 0 )
      o.server->add_client_data( o, resp );
    else {
      g_quit = true;
      logger.error("error in getDirectory");
    }
    initialize = false;
    if( ! moreData ) 
      done = true;
    else {
      req = dir_end;
      resp = "";
      moreData = false;
    }
  }

  done = false;

  while( ! done && ! g_quit ) {
    sleep(60);
    logger.error("sleeping");
    // todo wait for directory/file changes and send updates
  }
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
      logger.error("dspshare received a signal, doing commands" + localid + " " + to_string(localidsem) + " " + sharedir);
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
        int rc2 = getFile( resp, respheader, req, _SIZELIMIT1, sharedir, moreData, I2->second.strval );
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
        int index;
        bool moreData = false;
        int rc2 = getDirectory( resp, respheader, req, _SIZELIMIT1, sharedir, moreData, dir_end, index, false, o );
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

string
Dsp_share::get_dirents( string dir_in, int level, int maxResp, bool& moreData, string& end, int& in_index ) {

  struct dirent **namelist;
  struct stat statbuf;
  int rc = 0;
  int index = 0;
  int n;

  logger.error("get_dirents " + dir_in + " level " + itoa(level));

  n = scandir(dir_in.c_str(), &namelist, NULL, alphasort);

  string out;
  index = 0;
  bool skip = false;

  if( n != -1 ) {
    while( n-- ) {
      if( out.size() >= maxResp ) {
        end = dir_in;
        in_index = index;
        moreData = true;
        n = 0;
      }
      else
      if( namelist[n]->d_namlen <= 255 && out.size() < maxResp ) {
        index++;
        if( in_index > 0 && index < in_index )
          skip = true; 
        else
          skip = false; 
        string s = string(namelist[n]->d_name, namelist[n]->d_namlen);
        int t = namelist[n]->d_type;
        if( s != "." && s != ".." && ! skip ) {
          logger.error(dir_in + "/" + s + " type is " + to_string(t) + " isdir " + to_string(S_IFDIR));
          rc = stat( (dir_in + "/" + s).c_str(), &statbuf );
          if( rc == 0 ) {
            out += "<entry><n>" + s + "</n>";
            out += "<md>" + to_string(statbuf.st_mtime) + "</md>";
            out += "<cd>" + to_string(statbuf.st_ctime) + "</cd>";
            out += "<fs>" + to_string(statbuf.st_size) + "</fs>";
            if( t == 4 && level++ < 100 && s != "." && s != ".." ) {
              out += "<t>d</t>";
              string x = dir_in + "/" + s;
              out += get_dirents( x, level, maxResp, moreData, end, index );
            }
            else
            if( t == 8 && level++ < 100 ) {
              out += "<t>f</t>";
            }
            out += "</entry>";
          }
        }
      }
      free(namelist[n]);
    }
    free(namelist);
  }
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

  logger.error("getFile " + sharedir + "/" + req + " strval " + strval);

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

  while( i.p != j.p ) {
    tag  = i.p->value.getTag();

    logger.error("tag: " + to_string(tag));

    if( tag == JSON_ARRAY || tag == JSON_OBJECT ) {   
      JsonIterator i2 = begin(i.p->value);
      JsonIterator j2 = end(i.p->value);

      while( i2.p != j2.p ) {
        tag = i2.p->value.getTag();
        type = string(i2.p->key);
        logger.error("tag " + to_string(tag) + " " + type);
        if( tag == JSON_NUMBER ) {
          long n = i2.p->value.toNumber();
          logger.error("number " + type + " " + to_string(n));
          if( type == "offset" )
            offset = n;
          else
          if( type == "stan" )
            stan = n;
          else
          if( type == "maxlen" )
            maxlen = n;
        }
        else
        if( tag == JSON_STRING ) {
          char *p = i2.p->value.toString();
          string n(p);
          logger.error("string " + type + " " + n );
          if( type == "op" )
            op = n;
          else
          if( type == "which" )
            which = n;
          else
          if( type == "offset" )
            offset = atoi(p);
          else
          if( type == "maxlen" )
            maxlen = atoi(p);
        }
        i2.p = i2.p->next;
      }
    } 
    i.p = i.p->next;
  }
#endif

  // add in check of authorization level etc
  
  string filename = sharedir + "/" + req;
  struct stat fs;
  char buf[maxResp];
  memset(buf, 0, sizeof(buf));

  rc = stat( filename.c_str(), &fs );
  unsigned long s = fs.st_size;
  if( rc == 0 ) {
    int fd = open( filename.c_str(), O_RDONLY );
    logger.error("open " + filename + " " + to_string(fd));
    if( fd > 0 ) {
      int rc2 = 0;
      if( offset != 0 ) {
        unsigned long off = offset;
        rc2 = lseek( fd, off, SEEK_SET );
        logger.error("seek " + filename + " " + to_string(rc2));
        if( rc2 == off ) {
          s -= off;
        } else {
          logger.error("seek failed!");
          rc2 = -1;
        }
      }
      if( s > maxResp ) s = maxResp;
      if( rc2 >= 0 )
        rc2 = read( fd, &buf, s );
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

  return rc;
}

int 
Dsp_share::getDirectory( string &resp, string &respheader, string req, int maxResp, string sharedir, bool& moreData, string& dir_end, int& index, bool initialize, DssObject& o ) {

  logger.error("getDirectory " + sharedir);

  string xmldir;
  if( ! initialize ) 
    xmldir  = "<table name=\"files\" type=\"dssObject\" localid=\"" + o.localid + "\">";
  else
    xmldir  = "<table name=\"files\" type=\"dssObject\" initialize=\"true\" localid=\"" + o.localid + "\">";

  xmldir += "\
    <files>\
      <entry>\
        <n>/</n>\
        <md>0</md>\
        <t>d</t>";

  xmldir += get_dirents( sharedir, 0, maxResp, moreData, dir_end, index );

  logger.error("have dir_end " + dir_end + " " + to_string(index));

  xmldir += "\
      </entry>\
    </files>\
  </table>";

  logger.error("have xmldir " + xmldir);

  resp = xmldir;
  string op = "getDirectory";

  respheader = "[{\"op\":\"" + op + "\",\"which\":\"" + sharedir + "\",\"index\":" + ltoa(index) + "\",\"dir_end\":\"" + dir_end + "\"}]";

  return 0;
}
