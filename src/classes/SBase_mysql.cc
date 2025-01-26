/*
  File:    classes/SBase.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.

  This is a simple mysql interface. Each object will get it's own connection, it was designed that way since this is multi-threaded 
  and I wanted it safe to use and also very simple.

  If you have hundreds of interfaces then I would suggest making the mysql and result vars statics and use a semaphore. 
*/


#include <time.h>
#include "dpsframework.h"
#include "SBase_mysql.h"

extern "C" {
  string getDpsVersion() {
    return string("dpsversion 0.001");
  }
}

void
SBase::dps_init(void) {
  _dps_initialized = true;
  map<string,string>::iterator I = _dpsServer->getConfigMap().find("disable_database");
  if( I != _dpsServer->getConfigMap().end() ) {
    logger.error("disable_database set");
    disable_database = true;
  }
  if( ! isOpen ) {
    open();
  }
}

int 
SBase::open( bool reOpen, string in_db ) {
  if( disable_database )
    return 0;

  if( isOpen ) {
    if( reOpen ) {
      mysql_close(mysql);
      isOpen = false;
    } else {
      return 0;
    }
  }
  if( _dpsServer ) {
    map<string,string>::iterator I = _dpsServer->getConfigMap().find("database");
  
    if( I != _dpsServer->getConfigMap().end() ) {
      database = _dpsServer->getConfigMap()["database"];
    }
    I = _dpsServer->getConfigMap().find("database_user");
    if( I != _dpsServer->getConfigMap().end() ) {
      username = _dpsServer->getConfigMap()["database_user"];
    }
    I = _dpsServer->getConfigMap().find("database_password");
    if( I != _dpsServer->getConfigMap().end() ) {
      password = _dpsServer->getConfigMap()["database_password"];
    }
    I = _dpsServer->getConfigMap().find("database_host");
    if( I != _dpsServer->getConfigMap().end() ) {
      hostname = _dpsServer->getConfigMap()["database_host"];
    }
  }

  if( hostname == "" )
    hostname = "localhost";

  mysql = mysql_init( NULL );

  if( ! mysql_real_connect( mysql, hostname.c_str(), username.c_str(), password.c_str(), database.c_str(), 0, NULL, 0 ) ) {
    logger.error("Open Failed, error " + string( mysql_error(mysql) ) );
    logger.error("host " + hostname + " user " + username + " pass " + password );
    return -1;
  } else {
    int reconnect = 1;
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);
    isOpen = true;
    return 0;
  }
}

long 
SBase::last_insert_id( void ) {
  if( disable_database )
    return 0;

  return ::mysql_insert_id(mysql);
}

int 
SBase::close( void ) {
  if( disable_database )
    return 0;

  isOpen = false;
  mysql_close(mysql);

  return 0;
}

int
SBase::update( const string &c ) {
  int rc = -1;

  logger.error("update " + c);

  if( ! _dps_initialized ) dps_init();

  if( disable_database )
    return 0;

  if( ! isOpen ) {
    logger.error( "read: database not opened: " + database );
    return 0;
  }
  if( mysql_real_query( mysql, c.c_str(), c.length() ) != 0 ) {
    logger.error( "ERROR: update failed: " + c );
    logger.error( mysql_error( mysql ) );
    rc = 0;
  } else {
    rc = 1;
  }
  return rc;
}

int
SBase::insert( const string &c ) {
  return SBase::update( c );
}

int
SBase::read( int by ) {
  setSearch( by );
  return read( search );
}

string
SBase::getDBname( string table) {
  string db;
  for( int i = 0, done = 0; ! done; i++  ) {
    if( tables[i].table == table ) {
      db = tables[i].db;
      done = 1;
    }
    else
    if( tables[i].table == "END" ) {
      db = tables[i - 1].db;
      done = 1;
    }
  }
  return db;
}

SBase::dstruct
SBase::getDBstruct( string& db ) {
  struct dstruct d;
  for( int i = 0, done = 0; ! done; i++  ) {
    if( databases[i].db == db) {
      d.db   = databases[i].db;
      d.host = databases[i].host;
      d.pass = databases[i].pass;
      d.user = databases[i].user;
      d.port = databases[i].port;
      done = 1;
    }
    else
    if( tables[i].table == "END" )
      done = 1;
  }
  return d;
}

string
SBase::getDBname( void ) {
  string d = database;
  return d;
}

int 
SBase::read( const string &c, int mode ) {
  int rc = 0;
  MYSQL_ROW row;

  logger.error("read " + c);

  if( ! _dps_initialized ) dps_init();

  if( disable_database )
    return 0;

  if( ! isOpen ) {
    logger.error( "read: database not opened: " + database );
    return 0;
  }
  if( mysql_real_query( mysql, c.c_str(), c.length() ) != 0 ) {
    logger.error( "ERROR: read failed: " + c );
    logger.error( mysql_error( mysql ) );
    rc = 0;
  } else {
    result = mysql_store_result( mysql );

    if( result )  {
      row = mysql_fetch_row( result );

      if( row ) {
        MYSQL_FIELD* f         = mysql_fetch_fields ( result );
        unsigned long* lengths = mysql_fetch_lengths( result );
        int num_fields         = mysql_field_count  ( mysql );

        getResults(num_fields, lengths, row, f);

        setStrings();
        rc = 1;
      } else {
        rc = 0;
      }
    }
    if( ! rc || readMode != READSEQ ) {
      mysql_free_result( result );
      result = NULL;
    }
  }
  return rc;
}

int 
SBase::readmo( void ) {
  int rc = 0;
  MYSQL_ROW row;

  if( disable_database )
    return 0;

  if( ! isOpen ) {
    logger.error( "readmo: database not opened: " + database );
    return 0;
  }

  if( result )  {
    row = mysql_fetch_row( result );

    if( row ) {
      MYSQL_FIELD* f         = mysql_fetch_fields ( result );
      unsigned long* lengths = mysql_fetch_lengths( result );
      int num_fields         = mysql_field_count  ( mysql );

      getResults(num_fields, lengths, row, f);
      setStrings();
      rc = 1;
    } else {
      rc = 0;
      mysql_free_result( result );
      result = NULL;
    }
  }
  return rc;
}

void
SBase::read_end( void ) {
  if( result ) {
    mysql_free_result( result );
    result = NULL;
  }
}

void
SBase::addResultVar(void* p, int& i) {
  if( i >= MAXRESULTVARS ) {
    logger.error("too many result vars, change the limit in SBase.h and recompile.");
    return;
  }
  result_var[i++] = p;
}

void
SBase::getResults(int num_fields, unsigned long* lengths, MYSQL_ROW& r, MYSQL_FIELD* f) {
  if( disable_database )
    return;

  for( int i = 0; i < num_fields; i++ ) {
    if( IS_NUM( f[i].type ) ) {
      if( r[i] == 0 ) {
        if( f[i].decimals > 0 )
          *(double*)result_var[i] = (double)0.0;
        else
        if( f[i].length < 10 )
          *(int*)result_var[i] = (int)0;
        else
        if( f[i].length == 10 )
          *(unsigned int*)result_var[i] = (unsigned int)0;
        else
          *(time_t*)result_var[i] = (time_t)0;
      } 
      else
      if( f[i].decimals > 0 ) {
        double fl = strtod( r[i], NULL );
        *(double*)result_var[i] = fl;
      }
      else 
      if( f[i].length < 10 ) {
        *(int*)result_var[i] = atoi(r[i]);
      }
      else 
      if( f[i].length == 10 ) {
        *(unsigned int*)result_var[i] = (unsigned int)strtoul(r[i], NULL, 10);
      }
      else 
        *(long*)result_var[i] = atol(r[i]);
    } else {
      if( f[i].charsetnr != 63 ) {
        if( r[i] != 0 ) {
          strcpy((char*)result_var[i], r[i]);
        } else {
          char tbuf[2];
          tbuf[0] = ' '; tbuf[1] = 0;
          strcpy((char*)result_var[i], tbuf);
        }
      } else {
        bindata *b = new bindata;
        b->ptr = new unsigned char [lengths[i]];
        b->length = lengths[i];
        memcpy( (unsigned char*)result_var[i], r[i], lengths[i] );
      }
    }
  }
}
