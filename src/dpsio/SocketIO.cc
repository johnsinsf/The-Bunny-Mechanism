/*
  File:    dpsio/SocketIO.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <sys/syscall.h>
#include "SocketIO.h"

SSL_CTX* SocketIO::s_ctx           = NULL;

// TESTING
map<string,SSL_CTX*> SocketIO::serverCerts;
map<string,SocketIO::certtype> SocketIO::serverNames;

//pthread_mutex_t* SocketIO::lock_cs = NULL;
//long*   SocketIO::lock_count       = NULL;

extern int verify_callback(int preverify_ok, X509_STORE_CTX *ctx);

int
SocketIO::initServerSSL( void ) {
  if( g_logLevel > 0 )
    logger.error("initServerSSL");
/*
  map<string,SSL_CTX*>:iterator I = serverCerts.find("default");
  if( I != serverCerts.end() && I->second != NULL )
    s_ctx = I->second;

  if( s_ctx != NULL ) {
*/
  if( serverCerts.size() > 0 ) {
    if( g_logLevel > 0 )
      logger.error("SSL already initialized");
    return 1;
  }

  SSL_METHOD *ssl_method=(SSL_METHOD *)SSLv23_method();
#ifndef _HAVEMACOS
  OPENSSL_INIT_SETTINGS *settings = NULL;
  OPENSSL_init_ssl( OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS | OPENSSL_INIT_ENGINE_OPENSSL, settings);
  SSL_load_error_strings();
  SSL_library_init();
#else
  OpenSSL_add_ssl_algorithms();
  SSL_load_error_strings();
  SSL_library_init();

  s_ctx = SSL_CTX_new(ssl_method);

#endif


  int i;
  int num_locks = CRYPTO_num_locks();
  if( num_locks < 4 ) num_locks = 4;

  if( lock_cs == NULL ) {
    lock_cs=(pthread_mutex_t*)OPENSSL_malloc(num_locks * sizeof(pthread_mutex_t));
    if( lock_count == NULL )
      lock_count=(long*)OPENSSL_malloc(num_locks * sizeof(long));
    for (i=0; i<num_locks; i++) {
      lock_count[i]=0;
      pthread_mutex_init(&(lock_cs[i]),NULL);
    }
  }
  CRYPTO_set_id_callback((unsigned long (*)())::pthreads_thread_id);
  CRYPTO_set_locking_callback((void (*)(int, int, const char*, int))::pthreads_locking_callback);
  if( serverNames.size() == 0 ) {
    certtype c;
    c.certfile = CertFile;
    c.certpass = CertPass;
    c.certcafile = CertCAFile;
    serverNames["default"] = c;
    if( g_logLevel > 0 )
      logger.error("serverNames added default");
  }
  map<string,certtype>::iterator I2;

  if( g_logLevel > 0 )
    logger.error("serverNames size " + itoa(serverNames.size()) );

  for( I2 = serverNames.begin(); I2 != serverNames.end(); ++I2 ) {
    if( g_logLevel > 0 )
      logger.error("creating certficate for " + I2->first);

    s_ctx = SSL_CTX_new(ssl_method);
  
    if ( s_ctx == NULL ) {
      logger.error( "bio error");
      return -1;
    }
    SSL_CTX_set_session_cache_mode(s_ctx,
                  SSL_SESS_CACHE_NO_AUTO_CLEAR|SSL_SESS_CACHE_SERVER);
    SSL_CTX_set_session_cache_mode(s_ctx, SSL_SESS_CACHE_OFF);
    if( !SSL_CTX_set_default_verify_paths(s_ctx) ) {
      logger.error( "SSL_load_verify_locations failed");
      return -1;
    }
    if( I2->first == "default" && CertFile == "" ) {
      CertFile = string(INSTALLDIR) + "/ssl/dss.pem";
      //CertCAFile = string(INSTALLDIR) + "/ssl/dssca.pem";
      CertPass = I2->second.certpass;
      logger.error("default password " + CertPass);
    } else {
      CertFile = I2->second.certfile;
      CertPass = I2->second.certpass;
      CertCAFile= I2->second.certcafile;
    } 
    if( _certRequired ) {
      logger.error( "client cert is required");
      SSL_CTX_set_verify(s_ctx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verify_callback );
    }
    if( CertCAFile != "" ) {
      if( g_logLevel > 0 )
        logger.error("loading CA file " + CertCAFile);
      if( SSL_CTX_load_verify_locations(s_ctx, CertCAFile.c_str(), NULL) <= 0 ) {
        logger.error("CAFile failure");
        return -1;
      }
    }
    if( g_logLevel > 0 )
      logger.error("Cert password " + CertPass);

    if( CertPass != "" ) {
      if( g_logLevel > 0 )
        logger.error("using cert password " + CertPass);
      SSL_CTX_set_default_passwd_cb_userdata( s_ctx, (void*)CertPass.c_str() );
    }
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(s_ctx, CertFile.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
      logger.error( "Certfile failure loading "  + CertFile);
      return -1;
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(s_ctx, CertFile.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
      logger.error( "Private key failure on " + CertFile );
      return -1;
    } else {
      /* verify private key */
      if ( !SSL_CTX_check_private_key(s_ctx) ) {
        logger.error( "Private key does not match the public certificate" );
        return -1;
      }
    }
    serverCerts[I2->first] = s_ctx;
  }
  //if( !debug_out ) {
    //logger.error( "SSL unable to open /var/tmp/ssl_debug" );
  //}
  return 1;
}

int
SocketIO::startUpSSL( int opt ) {
  ssl = NULL;
  initPeerName();
  initSockName();
  if( g_logLevel > 0 )
    logger.error( "startUpSSL " + itoa(fd) + " peer " + ipaddress + " me " + myipaddress );

  if( s_ctx == NULL ) {
    logger.error( "startUpSSL is null" );
    return -1;
  }

  if( opt == 0 ) {
    // wait 5 seconds to see if any input
    struct pollfd fds[1];
    fds[0].fd       = fd;
    fds[0].events   = POLLIN | POLLPRI;
    fds[0].revents  = 0;
    int rc = 0;
#ifndef _HAVEMACOS
    timespec ts;
    ts.tv_sec = 5;
    ts.tv_nsec = 0;
    sigset_t mask;
    sigemptyset(&mask);

    rc = ppoll( fds, 1, &ts, &mask );
#else
    sigset_t mask;
    sigemptyset(&mask);
    sigset_t origmask;
    pthread_sigmask(SIG_SETMASK, &mask, &origmask);
    rc = poll(fds, 1, 5);
    pthread_sigmask(SIG_SETMASK, &origmask, NULL);
#endif
    if( g_logLevel > 0 )
      logger.error("ppoll " + itoa(rc));

    if( rc <= 0 ) {
      logger.error( "startUpSSL no data, bailing out" );
      return -1;
    }
  } else {
    logger.error( "startUpSSL no polling opt set");
  }

  if( ssl == NULL ) {
    if( g_logLevel > 0 )
      logger.error("creating new ssl");
    ssl = SSL_new( s_ctx );

    if( ! ssl ) {
      logger.error( "bad SSL new" );
      return -1;
    }  
/*
  const char* ptr = NULL;
  int x = 0;
  ptr = SSL_get_cipher_list(ssl, x);
  while ( (ptr != NULL) && (x < 10 )) {
    logger.error( "cipher list " + string(ptr));
    ptr = SSL_get_cipher_list(ssl, ++x);
  }
*/
    SSL_set_fd( ssl, fd );
    SSL_set_mode( ssl, SSL_MODE_AUTO_RETRY );
    unsigned char cid[5] = {"DPS3"};
    SSL_set_session_id_context(ssl, cid, 4 );
  }
  //if( debug_out ) {
    //SSL_set_msg_callback(ssl,SSL_trace); 
    //SSL_set_msg_callback_arg(ssl,debug_out);
    //logger.error( "SSL_trace set");
  //}


  if( opt == 0 ) {
    int rc = SSL_accept(ssl);
    if ( rc != 1 ) {
      int t = errno;
      int rc2 = SSL_get_error(ssl, rc);
      logger.error( "SSL accept fail " + itoa(rc) + ", " + itoa(rc2) + ", " + itoa(t) );
      unsigned long e = ERR_get_error();
      char ebuf[240];
      while (e) {
        ERR_error_string_n(e, ebuf, 240);
        if( g_logLevel > 0 )
          logger.error( "accept " + string(ebuf) );
        e = ERR_get_error();
      }
      return -1;
    }
  } else {
    int rc = SSL_connect(ssl);
    if ( rc != 1 ) {
      int t = errno;
      int rc2 = SSL_get_error(ssl, rc);
      logger.error( "SSL connect fail " + itoa(rc) + ", " + itoa(rc2) + ", " + itoa(t) );
      unsigned long e = ERR_get_error();
      char ebuf[240];
      while (e) {
        ERR_error_string_n(e, ebuf, 240);
        logger.error( "connect " + string(ebuf) );
        e = ERR_get_error();
      }
      return -1;
    }
  }
  //logger.error( "SSL handshake done " + itoa(ssl) );
  return 1;
}


int dps_client_hello( SSL *s, int *al, void *arg ) {
  if( g_logLevel > 0 )
    logger.error("in client hello");

#ifndef _USEOLDSSL
  typedef map<string,SSL_CTX*> mytype;
  mytype *my_var;
  my_var  = (mytype*)arg;

  typedef mytype::iterator I;
  for( I i = my_var->begin();i != my_var->end(); ++i ) {
    if( g_logLevel > 0 )
      logger.error("my arg " + i->first);
  }
 
  const char *servername;
  const unsigned char *p;
  size_t len = 0, remaining = 0;

  int rc = SSL_client_hello_get0_ext(s, TLSEXT_TYPE_server_name, &p, &remaining);
  if( g_logLevel > 0 )
    logger.error("rc " + itoa(rc) + " " + itoa(remaining) );

  if( ! rc || remaining <= 2) {
    logger.error("bad host in client hello, using default");
    servername = "default";
  } else {
    /* Extract the length of the supplied list of names. */
    len = (*(p++) << 8);
    len += *(p++);
    if (len + 2 != remaining)
        return 0;
    remaining = len;
    if (remaining == 0 || *p++ != TLSEXT_NAMETYPE_host_name)
        return 0;
    remaining--;
    if (remaining <= 2)
        return 0;
    len = (*(p++) << 8);
    len += *(p++);
    if (len + 2 > remaining)
        return 0;
    remaining = len;
    servername = (const char *)p;
  
    if( g_logLevel > 0 )
      logger.error("servername " + string(servername));
  }
  I i = my_var->find(servername);
  if( i != my_var->end() ) {
    if( g_logLevel > 0 )
      logger.error("loading " + i->first);
    SSL_CTX *new_ctx = i->second;
    SSL_set_SSL_CTX(s, new_ctx);
#ifndef _HAVEMACOS
    SSL_clear_options(s, 0xFFFFFFFFL);
    SSL_set_options(s, SSL_CTX_get_options(new_ctx));
#endif
  }
  if (len == strlen("server2") && strncmp(servername, "server2", len) == 0) {
    return 1;
  } 
  else 
  if (len == strlen("server1") && strncmp(servername, "server1", len) == 0) {
    return 1;
  }
#endif
  return 1;
}

int
SocketIO::openServer( int port, int euid, int egid ) {

  if( doInitSSL && useSSL ) {
    if( initServerSSL() < 1 ) {
      logger.error( "initSSL failed" );
      return -1;
    }
  }

  typedef map<string,SSL_CTX*> mytype;
  mytype my_var;

  if( doInitSSL && useSSL ) {
#ifndef _USEOLDSSL
    logger.error("set clienthello " + itoa(serverCerts.size()));

    void * arg = (void*)&serverCerts;

    SSL_CTX_set_client_hello_cb(s_ctx, ::dps_client_hello, arg);
#endif
  }
  struct  sockaddr_in    addr;
  int _fd = socket( AF_INET, SOCK_STREAM, 0 );
  if( _fd < 0 ) {
    logger.error( "socket failed " + itoa(errno) );
    return -1;
  }
  if( g_logLevel > 0 )
    logger.error( "socket opened " + itoa(_fd) );

  bzero( (char*)&addr, sizeof( addr ) );
  addr.sin_family     = AF_INET;
  addr.sin_addr.s_addr= htonl( INADDR_ANY );
  addr.sin_port       = htons( port );

  int sockOpt = 1;
  setsockopt( _fd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(sockOpt) );
  struct timeval tv;
  tv.tv_sec = 1800;
  tv.tv_usec = 0;
  setsockopt( _fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
  setsockopt( _fd, SOL_SOCKET, SO_KEEPALIVE, &sockOpt, sizeof(sockOpt) );

  if( port < 1024 ) {
    if( g_logLevel > 0 )
      logger.error("setting euid to " + itoa(0));
    if( euid > 0 ) {
      int rc = seteuid(0);
      if( g_logLevel > 0 )
        logger.error("results: " + itoa(rc));
    }
    if( egid > 0 )
      setegid(0);
    int rc = fchmod( _fd, S_IWOTH | S_IXOTH | S_IROTH );
    if( g_logLevel > 0 )
      logger.error("chmod results " + itoa(rc) + " err: " + itoa(errno));
  }

  if( ::bind( _fd, (struct sockaddr *)&addr, sizeof(addr) ) < 0 ) {
    logger.error( "bind failed err: " + itoa(errno) );
    return -1;
  }
  if( port < 1024 ) {
    if( g_logLevel > 0 )
      logger.error("setting euid to " + itoa(euid));
    if( euid > 0 ) {
      int rc = seteuid(euid);
      if( g_logLevel > 0 )
        logger.error("seteuid results: " + itoa(rc));
    }
    if( egid > 0 )
      setegid(egid);
  }
  ::listen( _fd, 1024000 );
  //logger.error( "socket listened" );
  return( _fd );  
}

string
SocketIO::initPeerName( void ) {

  struct  sockaddr_in    addr;
  socklen_t addrLen = sizeof(addr);
  memset((char*)&addr, 0, sizeof(addr));

  int rc = getpeername( fd, (struct sockaddr *)&addr, &addrLen );
  if( rc == 0 )  {
    ipaddress = inet_ntoa( addr.sin_addr );
    if( g_logLevel > 0 )
      logger.error( "peername: " + ipaddress);
  }
  else {
    logger.error( "error getting peer name: "  + itoa(rc) );
    ipaddress = "";
  }
  return ipaddress;
}

string
SocketIO::initSockName( void ) {

  struct  sockaddr_in    addr;
  socklen_t addrLen = sizeof(addr);
  memset((char*)&addr, 0, sizeof(addr));

  int rc = getsockname( fd, (struct sockaddr *)&addr, &addrLen );
  if( rc == 0 )  {
    myipaddress = inet_ntoa( addr.sin_addr );
    if( g_logLevel > 0 )
      logger.error( "sockname: " + myipaddress);
  }
  else {
    logger.error( "error getting sock name: "  + itoa(rc) );
    myipaddress = "";
  }
  return myipaddress;
}

int
SocketIO::accept( int _fd ) {
  struct  sockaddr_in    addr;
  int    newSock;
  socklen_t addrLen;

  addrLen = sizeof( struct sockaddr_in );

  //logger.error( "accept: " + itoa(_fd) );

  newSock = ::accept( _fd, (struct sockaddr *)&addr, &addrLen );

  if( newSock < 0 ) 
    return -1;

  fd = newSock;
  isConnected = true;
  initPeerName();
  initSockName();

  return( newSock );
}

int
SocketIO::openClient( string serverName, int port, bool blocking, bool reopen ) {
  struct  sockaddr_in    addr;
  struct  hostent *hp;
  u_long  in_addr;
  bool    connected = false;
  isConnected = false;
  fd = -1;

  if( g_logLevel > 0 )
    logger.error( "openClient " + serverName + " " + itoa(port) + " blocking " + itoa(blocking) );

  if( useSSL ) {
    if( ! reopen ) {
      if( initServerSSL() < 1 ) {
        logger.error( "initSSL failed" );
        return -1;
      } else {
        if( g_logLevel > 0 )
          logger.error( "openClient initSSL done " + itoa(ssl) );
      }
    }
  }
  while( ! g_quit && ! connected ) {
    fd = socket( AF_INET, SOCK_STREAM, 0 );

    if( fd < 0 ) {
      logger.error( "socket failed " + itoa(errno) );
      return -1;
    }
    int sockOpt = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(sockOpt) );

    bzero( (char*)&addr, sizeof(addr) );

    addr.sin_family = AF_INET;
  
    if( ( in_addr = inet_addr( serverName.c_str() ) ) != INADDR_NONE ) {
      if( g_logLevel > 0 )
        logger.error("using server as IP address " + serverName);
      memcpy( &addr.sin_addr, &in_addr, sizeof(in_addr) );
    } else {
      hp = gethostbyname( serverName.c_str() );
      if( ! hp ) {
        if( fd > 0 )
          close ( fd );
        logger.error("bad gethostbyname");
        return -1;
      }
      memcpy( &addr.sin_addr, hp->h_addr, hp->h_length );
      endhostent();
      //logger.error("converted server to IP address " + string(*hp->h_addr));
      if( g_logLevel > 0 )
        logger.error("converted server to IP address");
    }
    addr.sin_port = htons( (uint16_t)port );
  
    if( g_logLevel > 2 )
      logger.error( "openClient connect");

    if( connect( fd,( struct sockaddr* )&addr, sizeof( addr ) ) < 0 ) {
      int err = errno;
      if( g_logLevel > 2 )
        logger.error( "openClient connect failed " + itoa(err));
      if( fd > 0 )
        ::close( fd );
      fd = -1;
      if( ! blocking ) {
        logger.error( "connect failed: " + serverName + ": " + itoa(port) + " err: " + itoa(err) );
        return -1;
      }
      logger.error( "connect trying again: " + serverName + ": " + itoa(port) + " err: " + itoa(err) );
      int x = 0;
      while( x++ < 6 && ! g_quit )
        sleep( 5 );
    } else {
      connected = true;
      if( g_logLevel > 0 )
        logger.error("connected!");
    }
  }
  if( useSSL ) {
    if( g_logLevel > 0 )
      logger.error("starting ssl");
    int rc = startUpSSL(1);
    if( rc != 1 ) {
      logger.error("SSL failed, closing");
      if( fd > 0 )
        ::close(fd);
      fd = -1;
      connected = false;
    }
  } else {
    if( g_logLevel > 0 )
      logger.error("no ssl " + itoa(fd));
  }
  //logger.error("openClient fd is " + itoa(fd));
  isConnected = connected;

  return( fd );
}


int
SocketIO::openClient( string serverName, string af_socket ) {
  struct  sockaddr_un    addr;
  bool    connected = false;
  isConnected = false;
  fd = -1;

  while( ! g_quit && ! connected ) {
    fd = socket( AF_UNIX, SOCK_STREAM, 0 );

    if( fd < 0 ) {
      logger.error( "socket failed " + itoa(errno) );
      return -1;
    }
    memset(&addr, '\0', sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, af_socket.c_str(), sizeof(addr.sun_path) -1);

    if (::bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
      logger.error( "bind failed " + itoa(errno) );
      return -1;
    } else {
      connected = true;
      if( g_logLevel > 0 )
        logger.error("connected!");
    }
  }
  isConnected = connected;

  return( fd );
}

int
SocketIO::doPoll( int timeOut, int mask[] ) {
  int rc;
  if( useSSL && ssl ) {
    rc = SSL_pending( ssl );  
    //if( g_logLevel > 5 )
      //logger.error("doPoll using ssl " + itoa(rc));
    //return rc;

    if( rc ) {
      return rc;
    }

  }
  // not used?
  mask[0] = 1;
  mask[1] = 1;
  struct pollfd fds[1];

  fds[0].fd       = fd;
  fds[0].events   = POLLIN | POLLPRI;
  fds[0].revents  = 0;

  errno = 0;
  
  int t = -1;

  int retval =  0;

  /* begin critical section, test got_interrupted atomically */
  if( g_isDss ) {
#ifndef _HAVEMACOS
    sigset_t mask;
    sigemptyset(&mask);
    timespec ts;
    ts.tv_sec = timeOut;
    ts.tv_nsec = 0;
    retval = ppoll( fds, 1, &ts, &mask );
#else
    sigset_t mask;
    sigemptyset(&mask);
    sigset_t origmask;
    pthread_sigmask(SIG_SETMASK, &mask, &origmask);
    retval = poll(fds, 1, timeOut);
    pthread_sigmask(SIG_SETMASK, &origmask, NULL);
#endif
    if( retval == -1 ) {
      //pid_t tid;
      //tid = syscall(SYS_gettid);
      //if( intMap[tid] ) {
        //retval = -2;
        //logger.error("ppoll rc : " + itoa(retval) + " " + itoa(tid) );
      //}
    }
  } else {
    // convert to seconds
    if( timeOut )
      t = timeOut * 1000;
    retval = ::poll( fds, 1, t );
    //logger.info( "poll rc : " + itoa(retval) ); 
  }

  if( ( fds[0].revents & POLLERR ) ||
      ( fds[0].revents & POLLNVAL ) ||
      ( fds[0].revents & POLLHUP ) ) {
    logger.error( "doPoll error: invalid " + itoa(errno) + " fd " + itoa(fd) );
    sleep( 1 );
    rc = -1;
    return rc;
  }
  
  if( retval == -1 ) {
    logger.error( "doPoll failed: " + itoa(errno) + " to: " + itoa(fd) );
    sleep( 1 );
  }
  else
  if( retval == 0 ) {
    //logger.error( "doPoll timedout: " + itoa(t) + " to: " + itoa(fd) );
    sleep( 1 ); // should probably take this out now
  }
  else
  if( retval == 1 ) {
    if( !( fds[0].revents & POLLIN ) ) {
      logger.error( "doPoll failed: " + itoa(errno) );
      sleep( 1 );
      rc = -1;
      return rc;
    }
  }
  rc = retval;
  return rc;
}

void
SocketIO::doDump( char* buf, int len ) {
  char buf2[256];
  string tbuf;
  string dbuf;

  for( int i = 0; i < len; i++ ) {
    sprintf( buf2, "%X ", buf[i] & 0xFF );
    tbuf = tbuf + string(buf2);
    if( ( buf[i] > 'z' ) || ( buf[i] < 20 ) )
      dbuf = dbuf + ".";
    else {
      sprintf( buf2, "%c", buf[i] );
      dbuf = dbuf + string(buf2);
    }
    if( tbuf.length() > 200 ) {
      logger.error( tbuf );
      tbuf = "";
    }
    if( dbuf.length() > 200 ) {
      logger.error( dbuf );
      dbuf = "";
    }
  }
  logger.error( "read=" + tbuf );
  logger.error( "read=" + dbuf );
}

int 
SocketIO::doClear( char* buf ) {
  // just read up to 256 bytes for 1 second to clear buffer

  sleep(1);

  int rc = doRead( buf, 256, 1 );

  //logger.error( "read=" + itoa(rc) + ", " + string(buf) );
  if( rc < 0 )
    rc *= -1;

  doDump( buf, rc );

  return rc;
}

long 
SocketIO::doRead( char* buf, long len, int timeOut, int checkInput ) {
  long total  = 0;
  long reads  = 0;
  long logreads  = 0;
  long rc     = 0;
  time_t timeEnd  = time( NULL ) + timeOut;
  *buf = '\0';
  int mask[2];

  if( g_logLevel > 5 && len > 1 )
    logger.error( "doRead fd " + itoa(fd) + " len " + ltoa(len) + " timeout " + itoa(timeOut) );

  // loop waiting to get the full len, expects at least 1 char per read
  while( ! g_quit && ( total != len ) && ( reads < (len + 25) ) ) {
    errno = 0;
    int myerrno = 0;
    reads++;
    rc = 0;

    time_t timeNow = time( NULL );
    time_t t = 0;

    if( ( timeOut ) && ( timeNow < timeEnd ) )
      t = timeEnd - timeNow;
    else
    if( timeOut ) {
      logger.error( "doRead time expired " );
      if( total > 2 )
        return total * -1;
      return -2;
    }
    // see if any data is ready
    //int fileReady = doPoll( t, mask );
    int fileReady = 1;
#ifndef _HAVEMACOS
    fileReady = doPoll( t, mask );
#endif

    //if( g_logLevel > 5 ) 
      //logger.error("fileready: " + itoa(fileReady));

    if( ! g_quit && fileReady > 0 && rc >= 0) {
      if( useSSL && ssl ) {
        if( ssl ) {
          int readlen = 384000;
          if( len-total < readlen ) readlen = len-total;

          rc = SSL_read( ssl, buf+total, readlen );
        } else {
          rc = -1;
        }

        if( g_logLevel > 5 ) {
          //logger.error("read ssl total: " + itoa(total) + " " + itoa(rc) + " " + buf);
          if( logreads++ > 10000 ) {
            logger.error("read ssl total: " + to_string(total) + " " + to_string(len-total) );
            logreads = 0;
          }
          //if( rc == 16384 )
            //usleep(200);
        }

        if( rc <= 0 ) {
          logger.error("ssl sleep");
          rc = 0;
          if( reads < 10 )
            usleep(10000);
          else
            sleep(1);
        }
/*
        if( ssl && rc > 0 && (rc+total) < len ) {
          int rc2 = SSL_pending( ssl );  
          if( rc2 > 0 ) {
            logger.error("ssl read some more");
            rc += SSL_read( ssl, buf+total+rc, rc2 );
          } else {
            logger.error("ssl read some more 1 char");
            //sleep(1);
            rc += SSL_read( ssl, buf+total+rc, 1 ); // try it?
          }
        }
*/
      } else {
        rc = ::read( fd, buf+total, len-total );
        if( rc >= 0 && rc < (len-total) ) {
          //if( g_logLevel > 3 )
            //logger.error("short read " + itoa(rc) + " " + ltoa(len) + " " + ltoa(total));
          //sleep(1);
          //usleep(1000);
        }
      }
      if( rc == -1 ) {
        myerrno = errno;
        logger.error("error set " + itoa(myerrno));
      }
    }
    else    
    if( g_quit ) 
      return -1;
    else
    if( fileReady == 0 ) {
      if( reads < (len + 15) ) {
        usleep(50000);
      } else {
        logger.error( "doRead interrupted: " + itoa(rc) );
        return -1;
      }
    }
    else    
    if( fileReady == -2 ) {
      logger.error( "doPoll interrupted: " + itoa(rc) );
      return -2;
    } else {
      logger.error( "doRead failed " );
      return -1;
    }
    if( rc > 0 )
      total += rc;
    else
    if( rc < 0 ) {
      logger.error( "doRead error: " + itoa(myerrno) );
      sleep( 1 );
      return rc;
    }
  }
  if( total != len ) {
    logger.error( "WARNING: bad read: only " + ltoa(total) + ", of " + ltoa(len) );
    return -1;
  }
  if( checkInput ) {
    for( long i = 0; i <= total; i++ )
      if( ( buf[i] == '\\' ) || ( buf[i] == ';') || ( buf[i] == '"' ) || ( buf[i] == '\'' ) ) 
        buf[i] = ' ';
  }
  //if( g_logLevel > 0 )
    //logger.error("read done " + ltoa(total));

  return total;
}

long 
SocketIO::doReadPacket( char* buf, long len, int timeOut, bool useLRC ) {
  int rc     = 0;
  unsigned char  lrc   = '\0';
  unsigned char   inlrc  = '\0';
  char c;

  rc = doRead( &c, 1, timeOut );
  if( ! rc ) {
    logger.error( "bad read, rc=" + itoa(rc) );
    return false;
  }
  if( c != STX ) {
    if( c == 5 || c == 6 ) {
      buf[0] = c;
      return 1;
    }
    logger.error( "bad STX read, rc=" + c );

    return false;
  }
  bool etxFound = false;
  long i = 0;
  rc = 1;

  while( ( i <= len ) && ( ! etxFound ) && ( rc == 1 ) ) {
    rc = doRead( (char*)&buf[i], 1, 5 );
    if( ( buf[i] == ETX ) || ( buf[i] == ETB ) ) {
      buf[i] = '\0';
      etxFound = true;
      i--;
    }
    else
      i++;
  }
  len = i;

  if( ! etxFound ) {
    logger.error( "no etx found" );
    return false;
  }
  if( rc ) {
    if( useLRC ) {
      for( int i = 0; i <= len; i++ )
        lrc ^= buf[i];

      lrc ^= ETX;

      rc = doRead( (char*)&inlrc, 1 );
      if( ! rc ) {
        logger.error( "bad LRC read" );
        return false;
      }
      if( lrc != inlrc ) {
        logger.error( "LRC check failed for: " + string(buf) +
              ", in " + itoa(inlrc) + ", calc: " + itoa(lrc) );
        return -1;
      }
    }
  }
  else
    return false;

  //logger.error("read " + itoa(len) + " chars");
  return true;
}

long 
SocketIO::doReadPacket( string& buf, long len, int timeOut, bool useLRC ) {
  long rc     = 0;
  unsigned char  lrc   = '\0';
  unsigned char   inlrc  = '\0';
  char c;

  rc = doRead( &c, 1, timeOut );
  if( ! rc ) {
    logger.error( "bad read, rc=" + itoa(rc) );
    return false;
  }
  if( c != STX ) {
    if( c == 5 || c == 6 ) {
      buf = c;
      return 1;
    }
    logger.error( "bad STX read, rc=" + c );

    return false;
  }
  bool etxFound = false;
  long i = 0;
  rc = 1;

  while( ( i <= len ) && ( ! etxFound ) && ( rc == 1 ) ) {
    rc = doRead( &c, 1, 5 );
    if( ( c == ETX ) || ( c == ETB ) ) {
      etxFound = true;
      i--;
    } else {
      buf += c;
      i++;
      if( useLRC )
        lrc ^= c;
    }
  }
  //len = i;

  if( ! etxFound ) {
    logger.error( "no etx found" );
    return false;
  }
  if( rc ) {
    if( useLRC ) {
      lrc ^= ETX;

      rc = doRead( (char*)&inlrc, 1 );
      if( ! rc ) {
        logger.error( "bad LRC read" );
        return false;
      }
      if( lrc != inlrc ) {
        logger.error( "LRC check failed for: " + string(buf) +
              ", in " + itoa(inlrc) + ", calc: " + itoa(lrc) );
        return -1;
      }
    }
  }
  else
    return false;

  return true;
}

long 
SocketIO::doReadLen( string& buf, long len, int timeOut ) {
  long rc     = 0;
  char c = 0;

  rc = doRead( &c, 1, timeOut );
  if( ! rc ) {
    logger.error( "bad read, rc=" + itoa(rc) );
    return false;
  }
  if( c != STX ) {
    if( c == 5 || c == 6 ) {
      buf = c;
      return 1;
    }
    if( c != 0 )
      logger.error( "bad STX read, rc=" + c );
    else
      logger.error( "bad STX read");

    return false;
  }
  bool etxFound = false;
  long i = 0;
  rc = 1;
  string lenbuf;

  while( ( i < 10 ) && ( ! etxFound ) && ( rc == 1 ) ) {
    rc = doRead( &c, 1, 5 );
    if( ( c == ETX ) || ( c == ETB ) ) {
      etxFound = true;
    } else {
      lenbuf += c;
      i++;
    }
  }
  long inlen = atol(lenbuf.c_str());
  if( inlen > len || etxFound || inlen <= 0 ) {
    logger.error("data too large or etx found " + lenbuf + " inlen " + to_string(inlen));
    return false;
  }

  if( inlen > 500000000 || g_logLevel > 0 )
    logger.error("reading for len " + lenbuf);

  char inbuf[inlen + 1];
  //memset(inbuf, 0, inlen + 1);

  //bzero(inbuf, inlen + 1);

  inbuf[inlen] = 0;

  if( inlen > 500000000 || g_logLevel > 0 )
    logger.error("now reading for len " + lenbuf);

  rc = doRead( inbuf, inlen, 5 );

  if( rc == inlen ) {
    //logger.error("getting etx");
    rc = doRead( &c, 1, 5 );
    if( ( c == ETX ) || ( c == ETB ) ) {
      etxFound = true;
    }
  } else {
    logger.error("bad read for " + lenbuf);
    return false;
  }

  if( ! etxFound ) {
    logger.error( "no etx found" );
    return false;
  }
  string out(inbuf, inlen);
  buf = out;

  if( g_logLevel > 0 )
    logger.error( "doReadLen done " + ltoa(inlen) );

  return true;
}

int 
SocketIO::doReadLine( char* buf, long len, int timeOut ) {
  long rc  = 1;
  long i   = 0;
  bool crFound = false;

  while( ( i <= len ) && ( ! crFound ) && ( rc == 1 ) ) {
    rc = doRead( (char*)&buf[i], 1, timeOut );
    //char x = buf[i];
    //logger.error("read " + itoa(x) + " " + x + " rc " + itoa(rc));
    if( i == 0 && buf[i] == ACK ) {
      if( g_logLevel > 0 )
        logger.error("ACK read");
      return 0;
    }
    if( rc == 1 && ( buf[i] == LF || buf[i] == CR ) ) {
      crFound = true;
      if( buf[i] == CR ) {
        char dummy;
        //logger.error("read dummy");
        rc = doRead( &dummy, 1, 1 );
      }
      buf[i] = '\0';
      i--;
    }
    else
      i++;
  }
  len = i;

  if( ! crFound ) {
    logger.error( "no carriage return found" );
    return false;
  }
  return true;
}

bool
SocketIO::write( int c ) {
  //logger.error("writing to client:" + itoa(c) + " fd:" + itoa(fd) );

  errno = 0;
  int rc = 0;
  if( useSSL && ssl ) {

    rc = SSL_write( ssl, &c, 1 );
    int x  = 0;
    while( rc != 1 ) {
      if( x++ > 2 )
        return false;
      rc = SSL_write( ssl, &c, 1 );
    }
  } else
    rc = ::write( fd, &c, 1 );

  if( rc != 1 ) {
    int myerr = errno;
    logger.error( "bad write client:" + itoa(rc) + " err: " + itoa(myerr) + " " + itoa(useSSL));

    return false;
  }
  return true;
}

bool
SocketIO::writePacket( const char* c, long len, bool useLRC, bool useLen ) {

  char buf[len+14];
  unsigned char lrc = 0;
  //memset(buf, 0, sizeof(buf));
  buf[len+13] = 0;

  if( g_logLevel > 0 || len < 0 || len > 10000000 )
   logger.error("writePacket len1: " + ltoa(len));

  long x = 1;
  if( ! useLen ) {
    memcpy( (char*)&buf[1], c, len );
  } else {
    string lenbuf = ltoa(len,10);
    if( g_logLevel > 0 )
      logger.error("writePacket lenbuf1: " + lenbuf);
    memcpy( (char*)&buf[1], lenbuf.c_str(), 10 );

    if( g_logLevel > 0 )
      logger.error("writePacket memcpy1 " + string((char*)&buf[1]) );

    memcpy( (char*)&buf[11], c, len );

    if( g_logLevel > 0 )
      logger.error("writePacket memcpy2 " + string((char*)&buf[1]) );

    x = 11;
  }

  if( useLRC ) {
    for( long i = 1; i < len + x; i++ )
      lrc ^= buf[i];

    lrc ^= ETX;
  } 
  buf[0]    = STX;
  buf[len+x] = ETX;
  buf[len+x+1] = lrc;
  buf[len+x+2] = '\0';

  useLRC ? len += 3 : len += 2;
  if( useLen ) len += 10;

  if( g_logLevel > 0 )
    logger.error( "writingPacket: len: " + ltoa(len) );
 
  return write( buf, len );
}

bool
SocketIO::write( const char* c, long len ) {
  errno = 0;
  
  long rc = 0;
  if( useSSL && ssl ) {

    rc = SSL_write( ssl, c, len );

    if( g_logLevel > 1 )
      logger.error("ssl write rc " + itoa(rc));

    long x  = 0;
    long rc1  = 0;
    while( rc < len ) {
      logger.error("ssl write retry " + itoa(rc) + " " + itoa(fd) + " " + itoa(ssl));
      if( x++ > 5 )
        return false;
      if( rc1 == SSL_ERROR_WANT_READ || rc1 == SSL_ERROR_WANT_WRITE )
        sleep ( 1 );
      else {
        logger.error("ssl write failed" );
        return false;
      }
      rc = SSL_write( ssl, c, len );
    }
  }
  else {
    long numwrites = 0;
    long written = 0;
    while( written != len && numwrites++ < 100 && rc >= 0 ) {
      rc = ::write( fd, c + written, len - written );
      written += rc;
    } 
    rc = written;
  }

  if( rc != len ) {
    int myerr = errno;
    logger.error( "bad write client1:" + ltoa(rc) + " err: " + itoa(myerr) + " " + itoa(useSSL) + " " + ltoa(len) );
    return false;
  }
  if( g_logLevel > 1 )
    logger.error("socket wrote " + itoa(len) + " chars");
  return true;
}
 
// testing
bool
SocketIO::write( const unsigned char* c, long len ) {
  errno = 0;
  long rc = 0;
  
  if( g_logLevel > 1 )
    logger.error("unsigned socket writing " + itoa(len));

  if( useSSL && ssl ) {

    rc = SSL_write( ssl, c, len );

    if( g_logLevel > 1 )
      logger.error("ssl write rc " + itoa(rc));

    long x  = 0;
    long rc1  = 0;

    while( rc < len ) {
      logger.error("ssl write retry " + itoa(rc) );
      if( x++ > 5 )
        return false;
      if( rc1 == SSL_ERROR_WANT_READ || rc1 == SSL_ERROR_WANT_WRITE )
        sleep ( 1 );
      else {
        logger.error("ssl write failed" );
        return false;
      }
      rc = SSL_write( ssl, c, len );
    }
    //logger.info("ssl write ok " + itoa(rc) );
  }
  else
    rc = ::write( fd, c, len );

  if( rc != len ) {
    int myerr = errno;
    logger.error( "bad write client2:" + itoa(rc) + " err: " + itoa(myerr) + " " + itoa(useSSL) + " " + ltoa(len) );

    return false;
  }
  return true;
}

bool
SocketIO::write( const char* c ) {
  int len = strlen( c );
  return write( c, len );
}

bool
SocketIO::writePacket( const char* c ) {
  long len = strlen( c );
  return writePacket( c, len );
}

char
SocketIO::hexToChar( char x ) {
  char buf[8];
  sprintf( buf, "%d", x );

  switch( x )
  {
    case 0: return '0';
    case 1: return '1';
    case 2: return '2';
    case 3: return '3';
    case 4: return '4';
    case 5: return '5';
    case 6: return '6';
    case 7: return '7';
    case 8: return '8';
    case 9: return '9';
    case 10: return 'a';
    case 11: return 'b';
    case 12: return 'c';
    case 13: return 'd';
    case 14: return 'e';
    case 15: return 'f';
  }
  return '0';
}


unsigned char
SocketIO::charsToHex( char x, char y )
{
  unsigned char a = 0, b = 0;
  switch( x ) {
    case '0': a = 0; break;
    case '1': a = 1; break;
    case '2': a = 2; break;
    case '3': a = 3; break;
    case '4': a = 4; break;
    case '5': a = 5; break;
    case '6': a = 6; break;
    case '7': a = 7; break;
    case '8': a = 8; break;
    case '9': a = 9; break;
    case 'A':
    case 'a': a = 10; break;
    case 'B':
    case 'b': a = 11; break;
    case 'C':
    case 'c': a = 12; break;
    case 'D':
    case 'd': a = 13; break;
    case 'E':
    case 'e': a = 14; break;
    case 'F':
    case 'f': a = 15; break;
  }
  switch( y ) {
    case '0': b = 0; break;
    case '1': b = 1; break;
    case '2': b = 2; break;
    case '3': b = 3; break;
    case '4': b = 4; break;
    case '5': b = 5; break;
    case '6': b = 6; break;
    case '7': b = 7; break;
    case '8': b = 8; break;
    case '9': b = 9; break;
    case 'A':
    case 'a': b = 10; break;
    case 'B':
    case 'b': b = 11; break;
    case 'C':
    case 'c': b = 12; break;
    case 'D':
    case 'd': b = 13; break;
    case 'E':
    case 'e': b = 14; break;
    case 'F':
    case 'f': b = 15; break;
  }
  return ( ( a << 4) | b );
}

void
SocketIO::bcdToStr( unsigned char* buf, int len, char* obuf ) {
  unsigned char x = 0;
  char y, z;

  char tbuf[8];
  sprintf( tbuf, "%d %d", buf[0], buf[1] );
  //logger.error( "converting: " + string(tbuf) );

  for( int i = 0; i < len; i++ ) {
    x = buf[i];
    y = hexToChar( ( x >> 4 ) & 0xF );
    z = hexToChar( x & 0xF );
    *obuf++ = y;
    *obuf++ = z;
  }
}

void
SocketIO::strToBcd( char* buf, int len, unsigned char* obuf ) {
  char w = 0;
  char x = 0;
  unsigned char y;

  for( int i = 0; i < len; i+=2 ) {
    w = buf[i];
    x = buf[i+1];
    y = charsToHex( w, x );
    *obuf++ = y;
  }
}

unsigned int
SocketIO::crc16(char* pa) { 
  int i;
  unsigned int crc = 0;
  char c;     
  do  { 
    c = *pa; 
    for (i=0; i<8; i++) {  
      if (c & 1)
        crc ^= 0x8000;
 
      c >>= 1;
 
      if (crc & 0X8000)
        crc = ((crc ^ 0x4002) << 1) + 1; 
      else
        crc <<= 1; 
    }    
  } 
  while (*pa++ != ETB);

  return (crc & 0xffff);
} 

void
SocketIO::ascii_con(char* pa, unsigned int crc) { 
  int i; 
  pa += 4; 
  for (i = 0; i < 4; i++) { 
    pa--;
    *pa = hexToChar( crc & 0xf );
       crc >>= 4;
  } 
  return;
}

void
SocketIO::pthreads_locking_callback(void* b, int mode, int type, const char *file, int line ) {

  if (mode & CRYPTO_LOCK) {
    pthread_mutex_lock(&(lock_cs[type]));
    lock_count[type]++;
  } else {
    pthread_mutex_unlock(&(lock_cs[type]));
  }
  return;
}
 
void
SocketIO::doClose( void ) {

  if( ! fd )
    return;

  if( fd > 0 ) {
    char tbuf[40];
    sprintf(tbuf, "closing syncing socket %d", fd);
    //logger.error( "closing socket " + to_string(fd));
    if( g_logLevel > 0 )
      logger.error( tbuf );
    fsync( fd );
    shutdown( fd, SHUT_RDWR );
    sleep(1);
    if( fd > 0 ) {
      ::close(fd);
    }
    fd = -1;
  }
  if( useSSL ) {
    if( useSSL && ssl ) {
      if( g_logLevel > 0 )
        logger.error( "closing ssl socket ssl " + itoa(ssl) );
      SSL_set_shutdown(ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
      SSL_shutdown(ssl);
      if( ssl )
        SSL_free( ssl );
      ssl = NULL;
    }
  }
  isConnected = false;

  return;
}

void
SocketIO::doFinish( void ) {

  return;
}

