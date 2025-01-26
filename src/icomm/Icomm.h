/*
  File:    icomm/Icomm.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __ICOMM
#define __ICOMM

//#include <map>
//#include <string>
//#include <bitset>
//#include <iostream>
//#include <sstream>

#include "dpsframework.h"
#include "SocketIO.h"

using namespace std;

extern log4cpp::Category& logger;

class Icomm : public SocketIO {
  public:

    struct Field_def {
      int type;
      int size;
      int alpha;
    };

    // type of fields
    enum {
      fixed_length,
      short_length,
      long_length,
      vlong_length
    };

    // names of fields
    enum {
      processCode  = 3,
      stan         = 11,
      time         = 12,
      date         = 13,
      actionCode   = 39,
      siteid       = 42,
      responseData = 44,
      authData     = 47, // authtoken/signon info
      userData     = 48,
      additionalData = 61,
      dataLength    = 62, 
      messageNumber = 71,
      passedData    = 115,
      passedData1    = 123,
      passedOpt      = 124,
      passedData2    = 125,
      passedData3    = 126,
      passedData4    = 127
    };

    // types of return codes
    enum {
      success           = 0,
      formatError         = 904,
      systemMalfunction   = 909,
      duplicateTransmission= 913,
      signonError         = 916,
      certError           = 917,
      certInit            = 919,
      certReInit          = 920,
      tokenReInit         = 921,
      getProducts         = 922,
      dapTran             = 923,
      confirmRequired     = 924,
      dataSend            = 925
    };

    // message types
    enum {
      fileUpdateRequest    = 302,
      fileUpdateResponse   = 312,
      siteResponse         = 982,
      siteRequest          = 983,
      signonAck            = 986,
      promptRequest        = 987,
      promptResponse       = 988,
      signonRequest        = 989,
      signonResponse       = 990,
      logRequest           = 991,
      logResponse          = 992,
      codeRequest          = 993,
      codeResponse         = 994,
      testRequest          = 998,
      testResponse         = 999
    };

    enum {
      NormalThread = 0,
      ControlThread = 1
    };

    static const Field_def field_def[128];

    Icomm() {
      lrc       = 0;
      inlrc     = 0;
      messageType= 0;
      passChar   = '\0';
      bytesRead  = 0;
      useBCD     = false;
      for( int i = 0; i < 128; i++ )
        header[i] = 0;
    };

    virtual ~Icomm() {};

    int  read   ( SocketIO* socket, int timeOut = READTIMEOUT );
    int  read2  ( SocketIO* socket, int timeOut = READTIMEOUT );

    int  read   ( const char* inbuf, int len );
    int  read   ( string& str );

    bool set    ( int id, int value );
    bool set    ( int id, const char* value );
    bool set    ( int id, string value )
      { return set( id, value.c_str() ); };

    // appends data to field
    bool add    ( int id, const char* value );
    bool add    ( int id, string value )
      { return add( id, value.c_str() ); };

    bool test   ( int id )   { return header[id - 1]; };
    void clear  ( void )     { fields.clear(); header.reset(); messageType = 0; };
    void clear  ( int id )   { header.reset( id ); };

    int           getInt ( int id );
    string        get    ( int id );
    unsigned int  getUInt( int id );

    int  getType ( void )   { return messageType; };
    int  get_lrc ( void )   { return lrc; };

    void setType ( int i )    { messageType = i; };
    bool setHeader ( char* in, int which );
    bool setHeaderBCD ( unsigned char* in, int which );

    int  getResponse  ( void );
    char getPassChar  ( void )   { return passChar; };
    int  getMessageLen( void )   { return bytesRead; };

    void print  ( ostringstream& o );
    void printField( ostringstream& o, int id );
    bool  useBCD;

  private:
    unsigned char lrc;
    unsigned char inlrc;
    unsigned char passChar;
    int messageType;
    int bytesRead;

    map<int,string> fields;
    bitset<128> header;

    char bit_to_hex  ( char* buf );
    void addToLRC    ( char* buf, int len );
    void printHeader ( void );
    bool readField   ( SocketIO* socket, int id );
    int  readField   ( string inbuf, int pos, int id );
};

#endif
