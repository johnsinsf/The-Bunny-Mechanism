/*
  File:    dpsio/SocketIO.h
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2005-2007 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#ifndef __SocketIO
#define __SocketIO

#include <stack>
#include <sys/poll.h>

#include <openssl/x509_vfy.h>
#include <openssl/x509.h>
#include <openssl/lhash.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "dpsframework.h"

extern log4cpp::Category& logger;

extern int dps_client_hello( SSL *s, int *al, void *arg );

class SocketIO {
  public:

    SocketIO( void ) { 
      useSSL    = false;
      doInitSSL = true;
      _certRequired = true;
      controlfd = 0;
      ssl   = NULL;
      //s_ctx = NULL;
      isConnected = false ;
      fd = 0;
      //lock_cs = NULL;
      //lock_count = NULL;
    }

    SocketIO( int infd ) { 
      SocketIO();
      fd = infd;
    }
    virtual ~SocketIO( void ) {
    };

    virtual int    openClient  ( string server, int port, bool blocking = false, bool reopen = false );
    virtual int    openClient  ( string server, string af_socket );
    virtual int    openServer  ( int port, int euid = 0, int egid = 0 );
    virtual int    initServerSSL( void );
    virtual int    startUpSSL  ( int opt = 0 );
    virtual void   doClose     ( void );
    virtual void   doFinish    ( void );

    virtual void   freeCTX     ( void ) {
      if( g_useSSL ) {
        if( s_ctx )
          SSL_CTX_free( s_ctx );
        s_ctx = NULL;
      }
    };

    virtual int   accept        ( int fd );
    virtual int   getFD         ( void ) { return fd; }
    virtual string getIpAddress ( void ) { return ipaddress; }
    virtual string getMyIpAddress ( void ) { return myipaddress; }
    virtual string initPeerName ( void );
    virtual string initSockName ( void );
    virtual bool  isOpen        ( void ) { return isConnected; }
    virtual SSL*  getSSL        ( void ) { return ssl; }
    virtual void  setCertName   ( string s ) { CertFile = s; }
    virtual void  setCertPass   ( string s ) { CertPass = s; }
    virtual void  setCertCAName ( string s ) { CertCAFile = s; }
    virtual bool  write         ( int c );
    virtual bool  write         ( const char* c );
    virtual bool  writePacket   ( const char* c );
    virtual bool  write         ( const char* c, long len );
    virtual bool  write         ( const unsigned char* c, long len );
    virtual bool  writePacket   ( const char* c, long len, bool useLRC = true, bool useLen = false );
    virtual long   doReadPacket  ( char* buf, long len, int timeOut = READTIMEOUT, bool useLRC = true );
    virtual long   doReadPacket  ( string& buf, long len, int timeOut = READTIMEOUT, bool useLRC = true );
    virtual long  doReadLen     ( string& buf, long len, int timeOut = READTIMEOUT );
    virtual long  doRead         ( char* buf, long len, int timeOut = READTIMEOUT, int checkInput = 0 );
    virtual int  doReadLine     ( char* buf, long len, int timeOut = READTIMEOUT );
    virtual void setControlfd( int t ) { controlfd = t; };
    virtual int  doPoll    ( int timeOut, int mask[] );
    virtual int  doClear   ( char* buf );
    virtual void doDump    ( char* buf, int len );
    virtual void bcdToStr  ( unsigned char* buf, int len, char* obuf );
    virtual void strToBcd  ( char* buf, int len, unsigned char* obuf );
    virtual char hexToChar ( char x );
    virtual unsigned char charsToHex( char x, char y );
    virtual unsigned int  crc16( char* pa );
    virtual void   ascii_con(char* pa, unsigned int crc);
    virtual void   pthreads_locking_callback(
          void* b, 
          int mode, int type,
          const char *file, int line );
    typedef struct {
      string certfile;
      string certpass;
      string certcafile;
    } certtype;
    virtual int addServerName( string n, string cert, string pass, string certca ) {
      certtype c;
      c.certfile = cert;
      c.certpass = pass;
      c.certcafile = certca;
      serverNames[n] = c; 
      return 1;
    }
    //static int dps_client_hello( SSL *s, int *al, void *arg );

    //static pthread_mutex_t* lock_cs;
    //static long*   lock_count;

    bool useSSL;
    bool doInitSSL;
    bool _certRequired;
    bool isConnected;
    int  controlfd;
    static SSL_CTX   *s_ctx;
    SSL* ssl;
    BIO* debug_out;
    int fd;
    string ipaddress;
    string myipaddress;
    string CertFile;
    string CertPass;
    string CertCAFile;
 
    static map<string, certtype> serverNames;
    static map<string, SSL_CTX*> serverCerts;

    //map<string, certtype> serverNames;
    //map<string, SSL_CTX*> serverCerts;
};
  
#endif
