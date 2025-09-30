#ifndef __dspframework
#define __dspframework

#ifdef _HAVEMACOS
#include <i386/limits.h>
#endif

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <stack>
#include <bitset>
#include <map>
#include <queue>
#include <list>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/syscall.h>
#include <sys/poll.h>

#include <dlfcn.h>
#include <fstream>
#include <algorithm>
#include <cctype>


#include <openssl/sha.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>
#include <openssl/lhash.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>


#include "Log4.h"

typedef struct {
    int id;
    int interface;
    int fio;
    std::string channel;
    int type;
    int direction;
    int icurrent_state;
    int inew_state;
    int ignore_changes;
    double dcurrent_state;
    double dnew_state;
    time_t last_open;
    time_t last_close;
    std::string name;
} dpsID;

#define 	READTIMEOUT	20
#define 	_MAXINT	2147483647
//#define 	SEP	28
#define 	SEP	','
#define	STX  2
#define	ETX  3
#define	EOT  4
#define	ENQ  5
#define	ACK  6
#define	NAK	21
#define	CR	13
#define	LF	10
#define	ETB	0x17
#define   ETXP 0x83

using namespace std;

std::string itoa( int i, int len = 0 );
//std::string ltoa( long i );
std::string ltoa( long i, int len = 0 );
std::string ftoa( float f );
std::string ftoa( double f );
std::string ftoa3( double f );
std::string ftoa2( double f );
std::string dps_upper( string &s );
std::string itoa( void* i );
std::string dpsaddslashes( const char* buf, int len );
std::string dpsaddslashes2( string s);
std::string dpsaddslashes3( string s);
void replaceAll(std::string& str, const std::string& from, const std::string& to);
std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v");
std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v");
std::string& trim(std::string& s, const char* t = " \t\n\r\f\v");
std::string get_hostname( void );
unsigned long hex2dec(string hex);
void hex2ascii(const string& in, string& out);
int has_binary_data(const char* in, int len);
int remove_binary_data(char* in, int len);

bool check_plain( string& s );
void dpsencode( const char *input, char* output, int insize );
void dpsdecode( const char *input, char* output, int insize );
void* resetHandles ( void *h1, void* h2 );
unsigned long pthreads_thread_id(void);
void pthreads_locking_callback(int mode, int type, char *file, int line);

std::string getTime( void );
std::string getDate( void );
std::string getWord( std::string& s, int& pos );

extern int  	mySeed;
extern int  	g_facility;
extern int  	g_logLevel;
extern bool 	g_quit;
extern volatile sig_atomic_t g_timer;
extern volatile sig_atomic_t g_int;

//extern map<pid_t, int> intMap;

extern bool 	g_testMode;
extern bool 	g_debugging;
extern bool 	g_ping;
extern bool 	g_reconnect;
extern bool 	g_notimeout;
extern bool 	g_useSSL;
extern bool 	g_certRequired;
extern bool 	g_reload_lib;
extern bool 	g_isDss;
extern bool 	g_isInit;
extern bool 	g_isFileServer;
extern string   authHost;
extern string   configFile;
//extern char	dpsMailHost[40];
extern int	numThreads;
extern int	authPort;
extern int	mailPort;
extern int  	_DpsServerNumber;
extern int  	_DPSHOST_TIMEOFFSET;

//jls
extern  pthread_mutex_t* lock_cs;
extern  long*   lock_count;


#define TEST_SERVER_CERT "/etc/pki/tls/certs/server.pem"

#define BLOCKSIZE 4096
#define NUMBLOCKS 16384
#define MCBLOCKSIZE 128
#define MCNUMBLOCKS 100000
#define MAX_CHANNELS_PER_INTERFACE 16

extern pthread_mutex_t  dapIntMutex;

#endif
