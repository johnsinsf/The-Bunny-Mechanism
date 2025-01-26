/*
  File:    dpsio/Lib.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

//#include <iostream>
//#include <sstream>
//#include <string>
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>

#include <openssl/lhash.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/blowfish.h>

#include "dpsframework.h"
#include "SocketIO.h"

//#include "../baseServer/BaseServer.h"


string getWord( string& s, int& pos ) {
  int len = s.size();
  if( pos >= len ) {
    pos = -1;
    return string("");
  }

  while( s.substr( pos, 1 ) == " " )
    pos++;

  if( s.substr(pos, 1) == "\"" ) {
    int i = s.find("\"", pos + 1);
    if( i > 0 ) {
      string n = s.substr(pos + 1, i - pos - 1);
      pos = i + 1;
      return n;
    }
    pos = -1;
    return "";
  }
  int npos = s.find(' ', pos);
  if( npos == -1 ) {
    string n = s.substr(pos, len - pos);
    pos = -1;
    return n;
  }
  string n = s.substr(pos, npos - pos); 
  pos = npos + 1;
  return n;
}
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
  if(from.empty())
    return;
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

inline std::string& sanitize(std::string& s) {
#ifdef _HAVETRANSFORM
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { if(c == '\\' || c == '\'') c = ' '; return c; });
#else
  replaceAll(s, "\'", " ");
  replaceAll(s, "\\", " ");
#endif
  return s;
}

struct equal_is_space : std::ctype<char> {
  equal_is_space() : std::ctype<char>(get_table()) {}
  static mask const* get_table() {
    static mask rc[table_size];
    rc['='] = std::ctype_base::space;
    rc['\n'] = std::ctype_base::space;
    return &rc[0];
  }
};

//inline std::string& ltrim(std::string& s, const char* t  = " \t\n\r\f\v")
std::string& ltrim(std::string& s, const char* t )
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

//inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v" )
std::string& rtrim(std::string& s, const char* t )
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

//inline std::string& trim(std::string& s, const char* t = " \t\n\r\f\v" )
std::string& trim(std::string& s, const char* t )
{
    return ltrim(rtrim(s, t), t);
}

std::string 
itoa(int i, int len) {
	char s[24];
	char t[24];
	if( ( len > 0 ) && ( len < 10 ) )
	{
		sprintf( &t[1], ".%di", len );
		t[0] = '%';
	}
	else
	{
		memcpy( t, "%i", 2 );
		t[2] = 0;
	}
	sprintf( s, t, i );
	return std::string(s);
}

std::string 
ltoa(long i) {
    char s[24];
    sprintf( s, "%ld", i );
    return std::string(s);
}

std::string 
itoa(void* i) {
    char s[24];
    sprintf( s, "%ld", long(i) );
    return std::string(s);
}

std::string 
ftoa(float f) {
    char s[24];
    sprintf( s, "%f", f );
    return std::string(s);
}

std::string 
ftoa(double f) {
    char s[24];
    //sprintf( s, "%5.4f", f );
    sprintf( s, "%f", f );
    return std::string(s);
}

std::string 
ftoa3(double f) {
    char s[24];
    sprintf( s, "%5.3f", f );
    return std::string(s);
}

std::string 
ftoa2(double f) {
    char s[24];
    sprintf( s, "%5.2f", f );
    return std::string(s);
}

std::string 
dps_upper(std::string &s) {
	int len = s.length();
	if( len > 100 )
		len = 100;
	char buf[len];
	for( int i = 0; i < len; i++ )
		buf[i] = toupper( s[i] );
 
	return std::string(buf);
}

std::string 
getDate( void )
{
	struct tm t;
	time_t now = time(NULL);
	localtime_r( &now, &t );

	char buf[128];
	strftime ( buf, sizeof(buf), "%y%m%d", &t );

	return std::string( buf );
}


std::string 
getTime( void )
{
	struct tm t;
	time_t now = time(NULL);
	localtime_r( &now, &t );

	char buf[128];
	strftime ( buf, sizeof(buf), "%H%M%S", &t );

	return std::string( buf );
}


bool
check_plain( std::string& s ) {

  const char* p = s.c_str();
  int len = s.size();
  for( int i = 0; i < len; i++ ) {
    if( ! isalnum(*(p+i)) && *(p+i) != '_' && *(p+i) != '-' && *(p+i) != '.' ) 
      return false;
  }
  return true;
}

std::string
dpsaddslashes2( string str ) {
  string o;
  int i = 0;
  int size = str.size();
  while( str[i] ) {
    if( str[i] != '/' && str[i] != '%' && str[i] != '+' ) {
      o += str[i]; 
    } 
    else 
    if( str[i] == '/' ) {
      o += "\\/";
    }
    else 
    if( str[i] == '+' ) {
      o += " ";
    }
    else
    if( str[i] == '%' ) {
      if( i <= size - 3 ) {
        if( str[i+1] == '2' && str[i+2] == '6' ) 
          o += "&";
        else
        if( str[i+1] == '2' && str[i+2] == 'F' ) 
          o += "\\/";
        else
        if( str[i+1] == '3' && str[i+2] == 'A' ) 
          o += ":";
        else
        if( str[i+1] == '3' && str[i+2] == 'D' ) 
          o += "=";
        else
        if( str[i+1] == '3' && str[i+2] == 'F' ) 
          o += "?";
        else
        if( str[i+1] == '4' && str[i+2] == '0' ) 
          o += "@";
        else
        if( str[i+1] == '5' && str[i+2] == 'B' ) 
          o += "[";
        else
        if( str[i+1] == '5' && str[i+2] == 'D' ) 
          o += "]";
        else
          o += '%';
        i += 2;
      } else { 
        o += '%';
      }
    }
    i++;
  }
  return o;
}

std::string
dpsaddslashes3( string str ) {
  string o;
  int i = 0;
  while( str[i] ) {
    if( str[i] != '"' ) {
      o += str[i]; 
    } 
    else { 
      o += "\\";
      o += "\"";
    }
    i++;
  }
  return o;
}


std::string
dpsaddslashes( const char* buf, int len ) {
  unsigned char buf3[len*2+1];
  memset(buf3, '\0', sizeof(buf3));
  int bad_found = 0;

  for( int i=0,j=0;i < len;i++ ) {
    if( buf[i] == 34 ) {
      buf3[j++] = '\\';buf3[j++]='"';
    }
    else
    if( buf[i] == 39 ) {
      buf3[j++] = '\\';buf3[j++]='\'';
    }
    else
    if( buf[i] == 92 ) {
      buf3[j++] = '\\';buf3[j++]='\\';
    }
    else
    if( buf[i] == '\0' ) {
      buf3[j++] = '\\';buf3[j++]='0';
    }
    else
    if( (buf[i] < 32 || buf[i] > 126) && (buf[i] != 9 && buf[i] != 10 && buf[i] != 13 ) ) {
      bad_found++;
    }
    else
      buf3[j++] = buf[i];
  }
  if( bad_found && g_logLevel > 0 )
    logger.error("stripped bad data " + itoa(bad_found));

  return std::string((char*)&buf3);
}

int
remove_binary_data( char* buf, int len ) {
  int bad_found = 0;
  for( int i=0;i < len;i++ ) {
    if( (buf[i] < 32 || buf[i] > 126) && (buf[i] != 9 && buf[i] != 10 && buf[i] != 13 ) ) {
      buf[i] = ' ';
      bad_found++;
    }
  }
  if( bad_found && g_logLevel > 0 )
    logger.error("stripped bad data " + itoa(bad_found));

  return bad_found;
}

int
has_binary_data( const char* buf, int len ) {
  int bad_found = 0;
  for( int i=0;i < len;i++ ) {
    if( (buf[i] < 32 || buf[i] > 126) && (buf[i] != 9 && buf[i] != 10 && buf[i] != 13 ) ) {
      bad_found++;
    }
  }
  return bad_found;
}

std::string
get_hostname( void ) {
  string hostname;
  char buf[64];
  memset(buf, 0, sizeof(buf));
  int i = gethostname( buf, sizeof(buf) );
  if( i == 0 )
    hostname = buf;

  return hostname;
}


/*
** Translation Table as described in RFC1113
*/
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
** encodeblock
**
** encode 3 8-bit binary bytes as 4 '6-bit' characters
*/
void dpsencodeblock( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

/*
** encode
*/
void dpsencode( const char *input, char* output, int insize )
{
    unsigned char in[3], out[4];
    int i, len = 0;
	
    int charsRead = 0;	
    while( charsRead < insize ) {
        len = 0;
        for( i = 0; i < 3; i++ ) {
            in[i] = (unsigned char) *input++;
            charsRead++;
            if( charsRead <= insize )
                len++;
            else 
                in[i] = 0;
        }
        if( len ) {
            dpsencodeblock( in, out, len );
            for( i = 0; i < 4; i++ ) {
                *output++ = out[i];
            }
        }
    }
}


void dpsdecodeblock( unsigned char in[4], unsigned char out[3] )
{   
    out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
    out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/*
** decode
**
** decode a base64 encoded stream discarding padding, line breaks and noise
*/
void dpsdecode( const char *input, char *output, int insize )
{
	unsigned char in[4], out[3], v;
	int i, len;
	int charsRead = 0;	

	int inlen =  strlen( input );
	if( inlen == 0 )
		return;
	if( inlen < insize )
		insize = inlen;

	while( charsRead < insize ) {
		for( len = 0, i = 0; i < 4; i++ ) {
			v = 0;
			charsRead++;
			while( v == 0 ) {
				v = (unsigned char) *input++;
				v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
				if( v ) {
					v = (unsigned char) ((v == '$') ? 0 : v - 61);
				}
			}
			len++;
			if( v ) {
				in[i] = (unsigned char) (v - 1);
			}
			else {
				in[i] = 0;
			}
		}
		if( len ) {
			dpsdecodeblock( in, out );
            	for( i = 0; i < 3; i++ ) {
                	*output++ = out[i];
			}
		}
	}
}

//#include <iostream>
//#include <string>
#include "math.h"
using namespace std;
unsigned long hex2dec(string hex)
{
    unsigned long result = 0;
    for (long unsigned int i=0; i<hex.length(); i++) {
        if (hex[i]>=48 && hex[i]<=57)
        {
            result += (hex[i]-48)*pow(16,hex.length()-i-1);
        } else if (hex[i]>=65 && hex[i]<=70) {
            result += (hex[i]-55)*pow(16,hex.length( )-i-1);
        } else if (hex[i]>=97 && hex[i]<=102) {
            result += (hex[i]-87)*pow(16,hex.length()-i-1);
        }
    }
    return result;
}

unsigned char hexval(unsigned char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    else
        return 0;
    //else abort();
}

void hex2ascii(const string& in, string& out)
{
    out.clear();
    out.reserve(in.length() / 2);
    for (string::const_iterator p = in.begin(); p != in.end(); p++)
    {
       unsigned char c = hexval(*p);
       if( c == 0 ) break;
       p++;
       c = (c << 4) + hexval(*p);
       out.push_back(c);
    }
}

/*
** returnable errors
**
** Error codes returned to the operating system.
**
*/
#define B64_SYNTAX_ERROR        1
#define B64_FILE_ERROR          2
#define B64_FILE_IO_ERROR       3
#define B64_ERROR_OUT_CLOSE     4
#define B64_LINE_SIZE_TO_MIN    5
#define B64_SYNTAX_TOOMANYARGS  6

/*
** b64_message
**
** Gather text messages in one place.
**
*/
/*
char *b64_message( int errcode )
{
    #define B64_MAX_MESSAGES 7
    char *msgs[ B64_MAX_MESSAGES ] = {
            "b64:000:Invalid Message Code.",
            "b64:001:Syntax Error -- check help for usage.",
            "b64:002:File Error Opening/Creating Files.",
            "b64:003:File I/O Error -- Note: output file not removed.",
            "b64:004:Error on output file close.",
            "b64:005:linesize set to minimum.",
            "b64:006:Syntax: Too many arguments."
    };
    char *msg = msgs[ 0 ];

    if( errcode > 0 && errcode < B64_MAX_MESSAGES ) {
        msg = msgs[ errcode ];
    }

    return( msg );
}
*/

extern log4cpp::Category& logger;

//extern pthread_mutex_t *lock_cs;
//extern long *lock_count;

unsigned long
pthreads_thread_id(void)
{
	unsigned long ret;

	ret=(unsigned long)pthread_self();
	//logger.info( "SSL pthreads thread id " + itoa(ret) );
	return(ret);
}


void
pthreads_locking_callback(int mode, int type, 
					char *file, int line )
{
	//static long*   lock_count;
	//static pthread_mutex_t* lock_cs;

	if (mode & CRYPTO_LOCK)
	{
		pthread_mutex_lock(&(lock_cs[type]));
		lock_count[type]++;
	}
	else
	{
		pthread_mutex_unlock(&(lock_cs[type]));
	}

}

int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	char    buf[256];
	X509   *err_cert;
	int     err, depth;

	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

	logger.error( "depth " + itoa(depth) + " " + buf );

	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
		//X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
		//printf("issuer= %s\n", buf);
		//logger.info( "issuer certificate failure " + string(buf) );
		logger.error( "issuer certificate failure ");
	}
	// always verify
	return 1;
}
