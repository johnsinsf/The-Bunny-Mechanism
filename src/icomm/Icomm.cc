/*
  File:    icomm/Icomm.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "Icomm.h"

// definitions of the fields
const Icomm::Field_def Icomm::field_def[128]  = {

  { Icomm::fixed_length, 16, 0 },   /* 0 */
  { Icomm::fixed_length, 16, 0 },   /* 1 */
  { Icomm::short_length, 0, 0 },    /* 2 */
  { Icomm::fixed_length, 6, 0 },    /* 3 */
  { Icomm::fixed_length, 12, 0 },   /* 4 */
  { Icomm::fixed_length, 12, 0 },   /* 5 */
  { Icomm::fixed_length, 12, 0 },   /* 6 */
  { Icomm::fixed_length, 10, 0 },   /* 7 */
  { Icomm::fixed_length, 8, 0 },    /* 8 */
  { Icomm::fixed_length, 8, 0 },    /* 9 */
  { Icomm::fixed_length, 8, 0 },    /* 10 */
  { Icomm::fixed_length, 6, 0 },    /* 11 */
  { Icomm::fixed_length, 6, 0 },    /* 12 */
  { Icomm::fixed_length, 6, 0 },    /* 13 */
  { Icomm::fixed_length, 4, 0 },    /* 14 */
  { Icomm::fixed_length, 4, 0 },    /* 15 */
  { Icomm::fixed_length, 4, 0 },    /* 16 */
  { Icomm::fixed_length, 4, 0 },    /* 17 */
  { Icomm::fixed_length, 4, 0 },    /* 18 */
  { Icomm::fixed_length, 3, 0 },    /* 19 */
  { Icomm::fixed_length, 3, 0 },    /* 20 */
  { Icomm::fixed_length, 3, 0 },    /* 21 */
  { Icomm::fixed_length, 12, 0 },   /* 22 */
  { Icomm::fixed_length, 3, 0 },    /* 23 */
  { Icomm::fixed_length, 3, 0 },    /* 24 */
  { Icomm::fixed_length, 4, 0 },    /* 25 */
  { Icomm::fixed_length, 4, 0 },    /* 26 */
  { Icomm::fixed_length, 1, 0 },    /* 27 */
  { Icomm::fixed_length, 6, 0 },    /* 28 */
  { Icomm::fixed_length, 3, 0 },    /* 29 */
  { Icomm::fixed_length, 24, 0 },   /* 30 */
  { Icomm::short_length, 0, 0 },    /* 31 */
  { Icomm::short_length, 0, 0 },    /* 32 */
  { Icomm::short_length, 0, 0 },    /* 33 */
  { Icomm::short_length, 0, 0 },    /* 34 */
  { Icomm::short_length, 0, 0 },    /* 35 */
  { Icomm::long_length, 0, 0 },     /* 36 */
  { Icomm::fixed_length, 12, 0 },   /* 37 */
  { Icomm::fixed_length, 6, 0 },    /* 38 */
  { Icomm::fixed_length, 3, 0 },    /* 39 */
  { Icomm::fixed_length, 3, 0 },    /* 40 */
  { Icomm::fixed_length, 8, 0 },    /* 41 */
  { Icomm::short_length, 6, 0 },    /* 42 */
  { Icomm::short_length, 0, 0 },    /* 43 */
  { Icomm::fixed_length, 25, 1 },   /* 44 */
  { Icomm::short_length, 0, 0 },    /* 45 */
  { Icomm::long_length, 0, 0 },     /* 46 */
  { Icomm::long_length, 0, 0 },     /* 47 */
  { Icomm::long_length, 0, 0 },     /* 48 */
  { Icomm::fixed_length, 3, 0 },    /* 49 */
  { Icomm::fixed_length, 3, 0 },    /* 50 */
  { Icomm::fixed_length, 3, 0 },    /* 51 */
  { Icomm::fixed_length, 16, 0 },   /* 52 */
  { Icomm::short_length, 0, 0 },    /* 53 */
  { Icomm::long_length, 0, 0 },     /* 54 */
  { Icomm::long_length, 0, 0 },     /* 55 */
  { Icomm::short_length, 0, 0 },    /* 56 */
  { Icomm::fixed_length, 11, 0 },   /* 57 */
  { Icomm::short_length, 0, 0 },    /* 58 */
  { Icomm::long_length, 0, 0 },     /* 59 */
  { Icomm::long_length, 0, 0 },     /* 60 */
  { Icomm::long_length, 0, 0 },     /* 61 */
  { Icomm::long_length, 0, 0 },     /* 62 */
  { Icomm::long_length, 0, 0 },     /* 63 */
  { Icomm::fixed_length, 8, 0 },    /* 64 */
  { Icomm::fixed_length, 8, 0 },    /* 65 */
  { Icomm::long_length, 0, 0 },     /* 66 */
  { Icomm::fixed_length, 2, 0 },    /* 67 */
  { Icomm::fixed_length, 3, 0 },    /* 68 */
  { Icomm::fixed_length, 3, 0 },    /* 69 */
  { Icomm::fixed_length, 3, 0 },    /* 70 */
  { Icomm::fixed_length, 4, 0 },    /* 71 */
  { Icomm::long_length, 0, 0 },     /* 72 */
  { Icomm::fixed_length, 6, 0 },    /* 73 */
  { Icomm::fixed_length, 10, 0 },   /* 74 */
  { Icomm::fixed_length, 10, 0 },   /* 75 */
  { Icomm::fixed_length, 10, 0 },   /* 76 */
  { Icomm::fixed_length, 10, 0 },   /* 77 */
  { Icomm::fixed_length, 10, 0 },   /* 78 */
  { Icomm::fixed_length, 10, 0 },   /* 79 */
  { Icomm::fixed_length, 10, 0 },   /* 80 */
  { Icomm::fixed_length, 10, 0 },   /* 81 */
  { Icomm::fixed_length, 10, 0 },   /* 82 */
  { Icomm::fixed_length, 10, 0 },   /* 83 */
  { Icomm::fixed_length, 10, 0 },   /* 84 */
  { Icomm::fixed_length, 10, 0 },   /* 85 */
  { Icomm::fixed_length, 16, 0 },   /* 86 */
  { Icomm::fixed_length, 16, 0 },   /* 87 */
  { Icomm::fixed_length, 15, 0 },   /* 88 */
  { Icomm::fixed_length, 16, 0 },   /* 89 */
  { Icomm::fixed_length, 10, 0 },   /* 90 */
  { Icomm::fixed_length, 3, 0 },    /* 91 */
  { Icomm::fixed_length, 3, 0 },    /* 92 */
  { Icomm::short_length, 0, 0 },    /* 93 */
  { Icomm::short_length, 0, 0 },    /* 94 */
  { Icomm::short_length, 0, 0 },    /* 95 */
  { Icomm::long_length, 0, 0 },     /* 96 */
  { Icomm::fixed_length, 17, 0 },   /* 97 */
  { Icomm::fixed_length, 25, 0 },   /* 98 */
  { Icomm::short_length, 0, 0 },    /* 99 */
  { Icomm::short_length, 0, 0 },   /* 100 */
  { Icomm::short_length, 0, 0 },   /* 101 */
  { Icomm::short_length, 0, 0 },   /* 102 */
  { Icomm::short_length, 0, 0 },   /* 103 */
  { Icomm::long_length, 0, 0 },    /* 104 */
  { Icomm::fixed_length, 16, 0 },  /* 105 */
  { Icomm::fixed_length, 16, 0 },  /* 106 */
  { Icomm::fixed_length, 10, 0 },  /* 107 */
  { Icomm::fixed_length, 10, 0 },  /* 108 */
  { Icomm::short_length, 0, 0 },   /* 109 */
  { Icomm::short_length, 0, 0 },   /* 110 */
  { Icomm::long_length, 0, 0 },    /* 111 */
  { Icomm::long_length, 0, 0 },    /* 112 */
  { Icomm::long_length, 0, 0 },    /* 113 */
  { Icomm::long_length, 0, 0 },    /* 114 */
  { Icomm::long_length, 0, 0 },    /* 115 */
  { Icomm::long_length, 0, 0 },    /* 116 */
  { Icomm::long_length, 0, 0 },    /* 117 */
  { Icomm::long_length, 0, 0 },    /* 118 */
  { Icomm::long_length, 0, 0 },    /* 119 */
  { Icomm::long_length, 0, 0 },    /* 120 */
  { Icomm::long_length, 0, 0 },    /* 121 */
  { Icomm::long_length, 0, 0 },    /* 122 */
  { Icomm::long_length, 0, 0 },    /* 123 */
  { Icomm::long_length, 0, 0 },    /* 124 */
  { Icomm::long_length, 0, 0 },    /* 125 */
  { Icomm::long_length, 0, 0 },    /* 126 */
  { Icomm::vlong_length, 0, 0 }     /* 127 */
};

bool
Icomm::set( int id, int value ) {
  return set( id, itoa(value) );
}

bool
Icomm::set( int id, const char* value ) {
  if( (id < 1) || (id > 127) ) {
    logger.error( "bad id in set: " + itoa(id) );

    return false;
  }
  int len = strlen(value);

  if( len == 0 ) {
    logger.error( "bad length in set: 0 for " + itoa(id) );

    return false;
  }

  fields[id] = value;
  header[id - 1] = 1;

  if( id > 64 )
    header[0] = 1;

  return true;
}

bool
Icomm::add( int id, const char* value ) {
  if( (id < 1) || (id > 127) ) {
    logger.error( "bad id in set: " + itoa(id) );
    return false;
  }

  int len = strlen(value);

  if( len == 0 ) {
    logger.error( "bad length in set: 0 for " + itoa(id) );
    return false;
  }
  fields[id] = fields[id] + " " + value;
  header[id - 1] = 1;

  if( id > 64 )
    header[0] = 1;

  return true;
}
    
string
Icomm::get( int id ) {
  if( header[id - 1] != 1 ) {
    logger.error( "get called, not present: " + itoa(id) );
    return string("");
  }
  return fields[id];
}

int
Icomm::getInt( int id ) {
  if( header[id - 1] != 1 ) {
    logger.error("getInt called, not present: " + itoa(id) );
    return -1;
  }
  return atoi( fields[id].c_str() );
}

unsigned int
Icomm::getUInt( int id ) {
  if( header[id - 1] != 1 ) {
    logger.error("getUInt called, not present: " + itoa(id) );
    return -1;
  }
  return (unsigned int)strtoul( fields[id].c_str(), NULL, 10 );
}

bool
Icomm::readField( SocketIO* socket, int id ) {
  if( field_def[id].type == fixed_length ) {
    int s = field_def[id].size;
    char buf[s + 1];
    memset( buf, '\0', sizeof( buf ) );

    int rc = socket->doRead( buf, s, 20, 1 );

    if( rc != s ) {
      logger.error("bad read in readField for: " + itoa(id) );
      return false;
    }
    addToLRC( buf, s );
    return set( id, buf );
  }
  else
  if( field_def[id].type == short_length ) {
    char lbuf[3];
    memset( lbuf, '\0', sizeof( lbuf ) );

    int readlen = 2;
    if( useBCD )
      readlen = 1;
    // read in first length
    int rc = socket->doRead( lbuf, readlen );
    if( rc != readlen ) {
      logger.error("bad read in readField for: " + itoa(id) );
      return false;
    }
    addToLRC( lbuf, readlen );

    if( useBCD ) {
      char buf2[16];
      bcdToStr( (unsigned char*)lbuf, 1, buf2 );
      memcpy( lbuf, buf2, sizeof( lbuf ) );
      char buf3[65];
      snprintf(buf3, 65, "read %x %s", lbuf[0], buf2);
    }
    int s = atoi( lbuf );
    char buf[s + 1];

    memset( buf, '\0', sizeof( buf ) );

    rc = socket->doRead( buf, s, 20, 1);

    if( rc != s ) {
      logger.error( "bad read in readField for: " + itoa(id) );
      return false;
    }
    addToLRC( buf, s );
    return set( id, buf );
  }
  else
  if( field_def[id].type == long_length ) {
    char lbuf[6];
    memset( lbuf, '\0', sizeof( lbuf ) );

    int readlen = 3;
    if( useBCD )
      readlen = 2;
    // read in first length
    int rc = socket->doRead( lbuf, readlen );
    if( rc != readlen ) {
      logger.error( "bad read in readField for: " + itoa(id) );
      return false;
    }

    addToLRC( lbuf, readlen );

    if( useBCD ) {
      char buf2[16];
      bcdToStr( (unsigned char*)lbuf, 4, buf2 );
      buf2[4] = 0;
      memcpy( lbuf, buf2, sizeof( lbuf ) );
    }
    int s = atoi( lbuf );

    if( s > 999 )
      s = 999;

    char buf[s + 1];

    memset( buf, '\0', sizeof( buf ) );

    rc = socket->doRead( buf, s, 20, 1 );

    if( rc != s ) {
      logger.error( "bad read in readField for: " + 
            itoa(id) );
      return false;
    }
    addToLRC( buf, s );
    return set( id, buf );
  }
  else
  if( field_def[id].type == vlong_length ) {
    char lbuf[8];
    memset( lbuf, '\0', sizeof( lbuf ) );

    int readlen = 6;
    if( useBCD )
      readlen = 3;
    // read in first length
    int rc = socket->doRead( lbuf, readlen );
    if( rc != readlen ) {
      logger.error( "bad read in readField for: " + itoa(id) );
      return false;
    }
    addToLRC( lbuf, readlen );

    if( useBCD ) {
      char buf2[16];
      bcdToStr( (unsigned char*)lbuf, 6, buf2 );
      buf2[5] = 0;
      memcpy( lbuf, buf2, sizeof( lbuf ) );
    }
    int s = atoi( lbuf );

    if( s > 999999 )
      s = 999999;

    char buf[s + 1];

    memset( buf, '\0', sizeof( buf ) );

    rc = socket->doRead( buf, s, 20, 1 );

    if( rc != s ) {
      logger.error( "bad read in readField for: " + itoa(id) );
      return false;
    }
    addToLRC( buf, s );
    return set( id, buf );
  }
  return false;
}

int
Icomm::readField( string inbuf, int pos, int id ) {
  if( field_def[id].type == fixed_length ) {
    int s = field_def[id].size;
    char buf[s + 1];

    memset( buf, '\0', sizeof( buf ) );
    memcpy( buf, inbuf.substr(pos, s).c_str(), s );

    set( id, buf );
    return s;
  }
  else
  if( field_def[id].type == short_length ) {
    char lbuf[3];
    memset( lbuf, '\0', sizeof( lbuf ) );

    int readlen = 2;
    if( useBCD )
      readlen = 1;
    // read in first length
    memcpy( lbuf, inbuf.substr(pos, readlen).c_str(), readlen);

    if( useBCD ) {
      char buf2[16];
      bcdToStr( (unsigned char*)lbuf, 1, buf2 );
      memcpy( lbuf, buf2, sizeof( lbuf ) );
    }
    int s = atoi( lbuf );
    char buf[s + 1];

    memset( buf, '\0', sizeof( buf ) );
    memcpy( buf, inbuf.substr(pos + 2, s ).c_str(), s );

    set( id, buf );
    return s + readlen;
  }
  else
  if( field_def[id].type == long_length ) {
    char lbuf[6];
    memset( lbuf, '\0', sizeof( lbuf ) );

    int readlen = 3;
    if( useBCD )
      readlen = 2;
    // read in first length

    memcpy( lbuf, inbuf.substr(pos,readlen).c_str(), readlen );

    if( useBCD ) {
      char buf2[16];
      bcdToStr( (unsigned char*)lbuf, 4, buf2 );
      buf2[4] = 0;
      memcpy( lbuf, buf2, sizeof( lbuf ) );
    }
    int s = atoi( lbuf );

    if( s > 999 )
      s = 999;

    char buf[s + 1];

    memset( buf, '\0', sizeof( buf ) );
    memcpy( buf, inbuf.substr(pos + 3, s).c_str(), s);

    set( id, buf );
    return s + readlen;
  }
  else
  if( field_def[id].type == vlong_length ) {
    char lbuf[6];
    memset( lbuf, '\0', sizeof( lbuf ) );

    int readlen = 6;
    if( useBCD )
      readlen = 3;
    // read in first length

    memcpy( lbuf, inbuf.substr(pos,readlen).c_str(), readlen );

    if( useBCD ) {
      char buf2[16];
      bcdToStr( (unsigned char*)lbuf, 4, buf2 );
      buf2[4] = 0;
      memcpy( lbuf, buf2, sizeof( lbuf ) );
    }
    int s = atoi( lbuf );

    if( s > 999999 )
      s = 999999;

    char buf[s + 1];

    memset( buf, '\0', sizeof( buf ) );
    memcpy( buf, inbuf.substr(pos + 3, s).c_str(), s);

    set( id, buf );
    return s + readlen;
  }
  return 0;
}

void
Icomm::addToLRC( char* buf, int len ) {
  for( int i = 0; i < len; i++ ) {
    lrc^=*buf;
    buf++;
  }
}

int
Icomm::read( SocketIO* socket, int timeOut ) {
  lrc   = 0;
  inlrc = 0;
  passChar = '\0';
  char buf[17];
  memset(buf, '\0', sizeof( buf ) );
/*
  if( g_isDss ) {
    //logger.debug("unblocking sigint");
    sigset_t iset;
    sigemptyset(&iset);
    sigaddset(&iset, SIGINT);
    pthread_sigmask( SIG_UNBLOCK, &iset, NULL );
    pthread_sigmask( SIG_SETMASK, &iset, NULL );
  }
*/
  int len1 = socket->doRead( buf, 1, timeOut );

  if( len1 != 1 ) {
    logger.error( "Icomm bad STX read: " + string(buf) );
    if( len1 == -2 )
      return -2;
    return len1;
  }
  if( g_isDss ) {
    sigset_t iset;
    sigemptyset(&iset);
    sigaddset(&iset, SIGINT);
    pthread_sigmask( SIG_BLOCK, &iset, NULL );
    pthread_sigmask( SIG_SETMASK, &iset, NULL );
  }
  bytesRead += len1;
  if( buf[0] != STX ) {
    passChar = buf[0];
    if( useBCD )
      socket->doClear( buf );
    return 2;
  }
  // read the message
  read2( socket, timeOut );

  // get the etx/lrc
  memset(buf, '\0', sizeof( buf ) );
  //logger.debug( "icomm ETX read" );
  len1 = socket->doRead( buf, 1 );
  bytesRead += len1;
  if( ( len1 != 1 ) || ( buf[0] != ETX ) ) {
    logger.error( "icomm bad ETX read: " + itoa(buf[0]) );
    return 0;
  }
  addToLRC( buf, 1 );
  if( ! useBCD ) {
    len1 = socket->doRead( (char*)&inlrc, 1 );
    if( len1 != 1 ) {
      logger.error( "icomm bad lrc read: " );
      return 0;
    }
    if( inlrc != lrc ) {
      logger.error( "lrc failed inlrc: " + itoa(inlrc) + ", " + itoa(lrc) );
      return -1;
    }
  }
  return 1;
}

int
Icomm::read2( SocketIO* socket, int timeOut ) {
  char buf[17];
  char buf2[17];
  char tbuf[17];
  char lenbuf[2];
  int len1 = 0;
  len1 = 0;
  lrc   = 0;
  inlrc = 0;
  passChar = '\0';
  memset(buf, '\0', sizeof( buf ) );
  memset(buf2, '\0', sizeof( buf2 ) );

  if( useBCD ) {
    // get the two byte length
    len1 = socket->doRead( lenbuf, 2 );
    bytesRead += len1;
    if( len1 != 2 ) {
      logger.error( "bad read: " + itoa(len1) );
      return 0;
    }
    addToLRC( lenbuf, len1 );
    bcdToStr( (unsigned char*)lenbuf, 2, buf2 );
    memcpy( buf, buf2, sizeof( buf2 ) );
  }
  int readlen = 4;
  if( useBCD )
    readlen = 2;

  len1 = socket->doRead( buf, readlen, 20 );

  bytesRead += len1;
  if( len1 != readlen ) {
    logger.error( "bad read: " + itoa(len1) );
    return 0;
  }
  snprintf( tbuf, 17, "%d %d", buf[0], buf[1] );

  addToLRC( buf, len1 );
  if( useBCD ) {
    bcdToStr( (unsigned char*)buf, 2, buf2 );
    memcpy( buf, buf2, sizeof( buf2 ) );
  }
  messageType = atoi( buf );

  memset(buf, '\0', sizeof( buf ) );
  if( useBCD )
    readlen = 8;
  else
    readlen = 16;
  len1 = socket->doRead( buf, readlen, 1 );
  bytesRead += len1;
  if( len1 != readlen ) {
    logger.error( "bad read: " + itoa(len1) );
    return 0;
  }
  addToLRC( buf, len1 );

  if( useBCD )
    setHeaderBCD( (unsigned char*)&buf, 1 );
  else
    setHeader( buf, 1 );

  if( header[0] == 1 ) {
    memset( buf, '\0', sizeof( buf ) );
    len1 = socket->doRead( buf, readlen, 1 );
    bytesRead += len1;
    if( len1 != readlen ) {
      logger.error( "bad read: " + itoa(len1) );
      return 0;
    }
    addToLRC( buf, len1 );
    if( useBCD )
      setHeaderBCD( (unsigned char*)&buf, 2 );
    else
      setHeader( buf, 2 );
  }
  bool rc = true;
  for( int i = 2;( (i < 127) && rc ); i++ ) {
    if( header[i - 1] )
      rc = readField( socket, i );
  }
  return 1;
}

int
Icomm::read( string& str ) {
  return read( str.c_str(), str.length() );
}

int
Icomm::read( const char* incbuf, int len ) {
  string inbuf( incbuf, len);

  char buf[17];
  char buf2[17];
  char lenbuf[2];
  lrc   = 0;
  inlrc = 0;
  passChar = '\0';
  memset(buf, '\0', sizeof( buf ) );
  memset(buf2, '\0', sizeof( buf2 ) );
  int pos = 0;

  if( useBCD ) {
    // get the two byte length
    memcpy( lenbuf, inbuf.substr( 0, 2 ).c_str(), 2 );
    pos += 2;
    bcdToStr( (unsigned char*)lenbuf, 2, buf2 );
    memcpy( buf, buf2, sizeof( buf2 ) );
  }
  int readlen = 4;
  if( useBCD )
    readlen = 2;

  memcpy( buf, inbuf.substr( pos, readlen ).c_str(), readlen );

  pos += readlen;

  if( useBCD ) {
    bcdToStr( (unsigned char*)buf, 2, buf2 );
    memcpy( buf, buf2, sizeof( buf2 ) );
  }
  messageType = atoi( buf );

  memset(buf, '\0', sizeof( buf ) );
  if( useBCD )
    readlen = 8;
  else
    readlen = 16;

  memcpy( buf, inbuf.substr( pos, readlen ).c_str(), readlen );
  pos += readlen;

  if( useBCD )
    setHeaderBCD( (unsigned char*)&buf, 1 );
  else
    setHeader( buf, 1 );

  if( header[0] == 1 ) {
    memset( buf, '\0', sizeof( buf ) );
    memcpy( buf, inbuf.substr( pos, readlen ).c_str(), readlen );
    pos += readlen;

    if( useBCD )
      setHeaderBCD( (unsigned char*)&buf, 2 );
    else
      setHeader( buf, 2 );
  }
  bool rc = true;
  for( int i = 2;( (i < 127) && rc ); i++ ) {
    if( header[i - 1] )
      pos += readField( inbuf, pos, i );
  }
  return 1;
}

// in should be 8 bytes
bool
Icomm::setHeaderBCD( unsigned char* in, int which ) {
  unsigned char tbuf;

  char buf[64];
  char buf2[256];
  memset(buf, 0, sizeof(buf) );

  bcdToStr( (unsigned char*)in, 8, buf );

  snprintf( buf2, 256, "header %s", buf);

  if( which == 1 )
    header.reset();

  if( which == 2 )
    header[0] = 1;

  int j = 0;
  if( which == 2 )
    j = 64;

  for( int k = 0; k < 8; k++ ) {
    memcpy( (unsigned char*)&tbuf, in+k, 1 );
    for( int i = 0; i < 8; i++ ) {
      if( ( 128 >> i) & tbuf ) {
        header[i + j] = 1;
      }
    }
    j+=8;
  }
  return true;
}

bool
Icomm::setHeader( char* in, int which ) {
  if( which == 1 )
    header.reset();

  if( which == 2 )
    header[0] = 1;

  if( strlen( in ) != 16 ) {
    logger.error( "bad length in setHeader1: " + itoa(strlen(in)) );
    return false;
  }
  int j = 0;
  if( which == 2 )
    j = 64;

  for( int i = 0; i < 16; i++, j+=4 ) {
    if( in[i] == 'F' || in[i] == 'f' )  {
      header[0+j] = 1;
      header[1+j] = 1;
      header[2+j] = 1;
      header[3+j] = 1;
    }
    else
    if( in[i] == 'E' || in[i] == 'e' ) {
      header[0+j] = 1;
      header[1+j] = 1;
      header[2+j] = 1;
    }
    else
    if( in[i] == 'D' || in[i] == 'd' ) {
      header[0+j] = 1;
      header[1+j] = 1;
      header[3+j] = 1;
    }
    else
    if( in[i] == 'C' || in[i] == 'c' ) {
      header[0+j] = 1;
      header[1+j] = 1;
    }
    else
    if( in[i] == 'B' || in[i] == 'b' ) {
      header[0+j] = 1;
      header[2+j] = 1;
      header[3+j] = 1;
    }
    else
    if( in[i] == 'A' || in[i] == 'a' ) {
      header[0+j] = 1;
      header[2+j] = 1;
    }
    else
    if( in[i] == '9' ) {
      header[0+j] = 1;
      header[3+j] = 1;
    }
    else
    if( in[i] == '8' ) {
      header[0+j] = 1;
    }
    else
    if( in[i] == '7' ) {
      header[1+j] = 1;
      header[2+j] = 1;
      header[3+j] = 1;
    }
    else
    if( in[i] == '6' ) {
      header[1+j] = 1;
      header[2+j] = 1;
    }
    else
    if( in[i] == '5' ) {
      header[1+j] = 1;
      header[3+j] = 1;
    }
    else
    if( in[i] == '4' ) {
      header[1+j] = 1;
    }
    else
    if( in[i] == '3' ) {
      header[2+j] = 1;
      header[3+j] = 1;
    }
    else
    if( in[i] == '2' ) {
      header[2+j] = 1;
    }
    else
    if( in[i] == '1' ) {
      header[3+j] = 1;
    }
    else
    if( in[i] != '0' ) {
      logger.error("setHeader bad char: " + itoa(in[i]) );
      return false;
    }
  }
  return true;
}

void
Icomm::printHeader( void ) {
  cout << header << endl;
}

char
Icomm::bit_to_hex( char* buf ) {
  if( strcmp( buf, "0000" ) == 0 )
    return '0';
  else
  if( strcmp( buf, "0001" ) == 0 )
    return '1';
  else
  if( strcmp( buf, "0010" ) == 0 )
    return '2';
  else
  if( strcmp( buf, "0011" ) == 0 )
    return '3';
  else
  if( strcmp( buf, "0100" ) == 0 )
    return '4';
  else
  if( strcmp( buf, "0101" ) == 0 )
    return '5';
  else
  if( strcmp( buf, "0110" ) == 0 )
    return '6';
  else
  if( strcmp( buf, "0111" ) == 0 )
    return '7';
  else
  if( strcmp( buf, "1000" ) == 0 )
    return '8';
  else
  if( strcmp( buf, "1001" ) == 0 )
    return '9';
  else
  if( strcmp( buf, "1010" ) == 0 )
    return 'A';
  else
  if( strcmp( buf, "1011" ) == 0 )
    return 'B';
  else
  if( strcmp( buf, "1100" ) == 0 )
    return 'C';
  else
  if( strcmp( buf, "1101" ) == 0 )
    return 'D';
  else
  if( strcmp( buf, "1110" ) == 0 )
    return 'E';
  else
  if( strcmp( buf, "1111" ) == 0 )
    return 'F';
  else {
    logger.error( "bad conversion bit_to_hex: " + string(buf) );
    return '0';
  }
  logger.error( "bad return in bit_to_hex: " + string(buf) );

  return '0';
}

void
Icomm::printField( ostringstream& o, int id ) {
  if( id > 127 )
    return;

  if( field_def[id].type == fixed_length ) {
    o.width(field_def[id].size);
    if( field_def[id].alpha == 0 ) {
      o.fill('0');
      o << fields[id];
      o.width(0);
    } else {
      o << std::left << fields[id];
      o.fill(' ');
      o.width(0);
    }
  }
  else
  if( field_def[id].type == short_length ) {
    char buf[4];
    int i = fields[id].length();
    snprintf(buf, 4, "%.2d", i);
    if(  useBCD ) {
      //logger.debug( "exporting bcd length " + string(buf) );
      unsigned char buf2[4];
      strToBcd( buf, 2, buf2 );
      buf2[1] = 0;
      o << buf2 << fields[id];
    }
    else
      o << buf << fields[id];
  }
  else
  if( field_def[id].type == long_length ) {
    char buf[5];
    if( ! useBCD )
      snprintf(buf, 5, "%.3d", (int)(fields[id].length()) );
    else
      snprintf(buf, 5, "%.4d", (int)(fields[id].length()) );
    if(  useBCD ) {
      logger.debug( "exporting bcd long length " + string(buf) );
      unsigned char buf2[5];
      strToBcd( buf, 4, buf2 );
      buf2[2] = 0;
      o << buf2[0] << buf2[1] << fields[id];
    }
    else
      o << buf << fields[id];
  }
  else
  if( field_def[id].type == vlong_length ) {
    char buf[7];
    if( ! useBCD )
      snprintf(buf, 7, "%.6d", (int)(fields[id].length()) );
    else
      snprintf(buf, 7, "%.4d", (int)(fields[id].length()) );
    if(  useBCD ) {
      //logger.debug( "exporting bcd long length " + string(buf) );
      unsigned char buf2[5];
      strToBcd( buf, 4, buf2 );
      buf2[2] = 0;
      o << buf2[0] << buf2[1] << fields[id];
    }
    else
      o << buf << fields[id];
  } else {
    logger.error("wrong field type for: " + itoa(id) );
  }
}

void
Icomm::print( ostringstream& o ) {
  ostringstream t;
  char buf[17];
  unsigned char buf2[17];
  unsigned char buf3[80];

  // start with message type
  if( messageType > 999 )
    return;

  snprintf( buf, 17, "%.4d", messageType );

  if( ! useBCD )
    o << buf;
  else {
    logger.debug( "Using bcd print for message type " + itoa(messageType) );
    memset( buf2, 0, sizeof(buf2) );
    strToBcd( buf, 4, buf2 );
    o << buf2;
    // force to single bitmap
    header[0] = 0;
  }
  // header, output in hex
  t << header;
  
  memset( buf, '\0', sizeof(buf) );
  memset( buf2, '\0', sizeof(buf2) );
  memset( buf3, '\0', sizeof(buf3) );
  
  int s = 64;
  if( header[0] == 1 )
    s = 0;

  for( int i = 124, k = 0; i >= s; i -=4, k++ ) {
    t.str().copy( buf, 4, i );
    // need to reverse order
    buf2[0] = buf[3];
    buf2[1] = buf[2];
    buf2[2] = buf[1];
    buf2[3] = buf[0];
    buf2[4] = '\0';
    unsigned char h = 0;
    h = bit_to_hex( (char*)buf2 );
    if( ! useBCD )
      o << h;
    else
      buf3[k] = h;
  }
  if( useBCD ) {
    unsigned char tbuf[9];
    s = 8;
    if( header[0] == 1 )
      s = 16;
    strToBcd( (char*)buf3, s * 2, tbuf );
    for( int i = 0; i < s; i++ )
      o.put(tbuf[i]);
  }

  // now copy each field to output stream
  s = 64;
  if( header[0] ) {
    s = 128;
  }
  //for( int i = 1; i <= s; i++ ) {
  for( int i = 1; i < s; i++ ) {
    if( header[i] )
      printField( o, i + 1 );
  }
  return;
}

int
Icomm::getResponse( void ) {
  switch( messageType ) {

    case fileUpdateRequest:
      return fileUpdateResponse;

    case testRequest:
      return testResponse;

    case codeRequest:
      return codeResponse;

    case signonRequest:
      return signonResponse;
  
    case promptRequest:
      return promptResponse;
  
    case siteRequest:
      return siteResponse;
  
    case logRequest:
      return logResponse;

    default:
      return 0;
  }
}
