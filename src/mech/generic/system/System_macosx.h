#ifndef __System_macosx
#define __System_macosx

#define MAXFILENAME 1024

#include "System.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define TCPLOGGER_HANDLE_IP4 0xBABABABA    /* Temp hack to identify this filter */
#define TCPLOGGER_HANDLE_IP6 0xABABABAB    /* Temp hack to identify this filter */
            /*
              Used a registered creator type here - to register for one - go to the
              Apple Developer Connection Datatype Registration page
              <http://developer.apple.com/datatype/>
            */
#define MYBUNDLEID    "com.apple.dts.kext.tcplognke"

struct TCPLogInfo {
  size_t      tli_len;      /* size of structure */
  uint32_t    tli_state;      /* connection state - TLS_CONNECT_OUT or TLS_CONNECT_IN */
  long      tli_genid;      /* one up id for this record */
  union {
    struct sockaddr_in  addr4;    /* ipv4 local addr */
    struct sockaddr_in6  addr6;    /* ipv6 local addr */
  } tli_local;
  union {
    struct sockaddr_in  addr4;    /* ipv4 remote addr */
    struct sockaddr_in6  addr6;    /* ipv6 remote addr */
  } tli_remote;
  uint32_t    tli_bytes_in;
  uint32_t    tli_pkts_in;
  uint32_t    tli_bytes_out;
  uint32_t    tli_pkts_out;
  struct timeval  tli_create;      /* socreate timestamp */
  struct timeval  tli_start;      /* connection complete timestamp */
  struct timeval  tli_stop;      /* connection termination timestamp */
  pid_t      tli_pid;      /* pid that created the socket */
  pid_t      tli_uid;      /* used id that created the socket */
  int        tli_protocol;    /* ipv4 or ipv6 */
};

struct tl_stats {
  int tls_done_count;
  int tls_done_max;
  int tls_qmax; /* Maximum number of info structures for be logged */
  int tls_overflow;
  int tls_active;
  int tls_active_max;
  int tls_inuse; /* Currently in use (attached and not free) */
  int tls_info; /* Number of currently allocated info structures */
  long tls_attached;  /* Number of attachment to sockets - used to set one up value of tli_genid */
  long tls_freed;  /* Number of calls to duplicate calls to sofree */
  long tls_cannotfree;  /* Number of calls to duplicate calls to sofree */
  long tls_dupfree;  /* Number of calls to duplicate calls to sofree */
  long tls_ctl_connected; /* Number of control sockets in use */
  int tls_log;
  int tls_enabled;
  bool tls_initted;
};

#define TLS_CONNECT_OUT  0x1
#define TLS_CONNECT_IN  0x2
#define TLS_LISTENING  0x4
#define TLS_KIND (TLS_CONNECT_OUT | TLS_CONNECT_IN | TLS_LISTENING)

#define TCPLOGGER_STATS 1   /* get tl_stats*/
#define TCPLOGGER_QMAX  2   /* get or set tls_qmax */
#define TCPLOGGER_ENABLED  3   /* get or set tls_enabled */
#define TCPLOGGER_FLUSH 4
#define TCPLOGGER_ADDUNIT 5
#define TCPLOGGER_DELUNIT 6
#define TCPLOGGER_LOG 7

class System_macosx : public System {

  public:
    System_macosx( void ) : System() { 
      msgid = 1;
    }
    ~System_macosx( void ) {
    };

    virtual int  main ( DssObject& o );

  private:
    virtual int  run      ( int sock, DssObject& o );
    virtual void send_data( DssObject& o, string& msg );
    virtual int  socketfilter_connect();
    virtual string parse_buf( void );

    virtual void   get_hostname   ( void );

    string hostname;
    int    msgid;
    struct sockaddr_ctl sc;
    struct TCPLogInfo tlp;
    struct ctl_info ctl_info;
};
#endif
