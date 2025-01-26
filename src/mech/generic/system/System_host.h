#ifndef __System_host
#define __System_host

#include "System.h"
#include "Hosts.h"
#include "Alerts.h"

#define MAX_DIRECTORIES 10
#define MAXFILENAME 1024
#define LOOP_INTERVAL 60

class System_host : public System {

  public:
    System_host( void ) : System() { 
      msgid        = 1;
      warn_percent = 90.0;
      load_warn     = 5;
      loop_interval = LOOP_INTERVAL;
    }
    ~System_host( void ) {
    };

    virtual int  main ( DssObject& o );

  private:
    virtual void load_config( DssObject& o );
    virtual void run        ( DssObject& o );
    virtual void send_data  ( DssObject& o, const string& msg );
    virtual int read_proc_entry(int pid, int opt);
    virtual void load_processes ( void );
    virtual int check_process ( string p );

    int    msgid;
    int    load_warn;
    int    loop_interval;
    double warn_percent;
    map<int, string> directories;
    map<int, string> processes;
    map<int, string> scripts;
    map<string, int> procs;
    map<string, int> cmdlines;

    Hosts host;
    Alerts alert;
};
#endif
