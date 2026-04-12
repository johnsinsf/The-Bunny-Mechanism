#ifndef __System_netlink
#define __System_netlink

#define MAXFILENAME 1024

#include "System.h"

class System_netlink : public System {

  public:
    System_netlink( void ) : System() { 
      memset(fbuf2, 0, sizeof(fbuf2));
      memset(fbuf3, 0, sizeof(fbuf3));
      msgid = 1;
      seq = 1;
    }
    ~System_netlink( void ) {
    };

    int  main ( DssObject& o ) override;

    typedef struct {
      string exe;
      string cwd;
      string msg;
      int    uid;
    } procvaltype;

  private:
    int  run      ( int sock, DssObject& o ) override;
    int  nl_connect();
    void send_data( DssObject& o, string& msg );
    procvaltype read_proc_entry( int pid );

    int set_proc_ev_listen( int sock, bool flag );
    void get_proc_vals  ( int ppid, int pid, string& msg, bool& sendData );
    int netlink_send( int s, struct cn_msg *msg );

    char fbuf2[MAXFILENAME];
    char fbuf3[MAXFILENAME];

    int    msgid;
    map<int, procvaltype> procvals;
    //__u32 seq;
    __uint32_t seq;
};
#endif
