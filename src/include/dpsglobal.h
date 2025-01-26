#ifndef __DPSGLOBAL
#define __DPSGLOBAL

#include <syslog.h>

// global vars

//map<pid_t, int> intMap;

int  mySeed         = 0;
int  g_facility     = LOG_LOCAL1;
int  g_logLevel     = false;
bool g_quit         = false;
volatile sig_atomic_t g_timer = 0;
volatile sig_atomic_t g_int   = 0;
bool g_testMode     = false;
bool g_debugging    = false;
bool g_ping         = false;
bool g_reconnect    = true;
bool g_notimeout    = false;
bool g_useSSL       = true;
bool g_certRequired = true;
bool g_reload_lib   = false;
bool g_isDss        = false;
bool g_isInit       = false;
bool g_isFileServer = false;
string  authHost;
string  configFile;
int  numThreads  = 1;
int  authPort    = 22925;
int  mailPort    = 25;
int  _DpsServerNumber = 0;
int  _DPSHOST_TIMEOFFSET = -480 * 60;

pthread_mutex_t dapIntMutex   = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t* lock_cs 	= NULL;
long*   lock_count 			= NULL;

#endif
