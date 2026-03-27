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

    virtual int  main ( DssObject& o );

    typedef struct {
      string exe;
      string cwd;
      string msg;
      int    uid;
    } procvaltype;

  private:
    virtual int  nl_connect();
    virtual int  run      ( int sock, DssObject& o );
    virtual void send_data( DssObject& o, string& msg );
    virtual procvaltype read_proc_entry( int pid );

    virtual int set_proc_ev_listen( int sock, bool flag );
    virtual void get_proc_vals  ( int ppid, int pid, string& msg, bool& sendData );
    virtual int netlink_send( int s, struct cn_msg *msg );

    char fbuf2[MAXFILENAME];
    char fbuf3[MAXFILENAME];

    int    msgid;
    map<int, procvaltype> procvals;
    //__u32 seq;
    __uint32_t seq;
};
#endif
