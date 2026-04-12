#ifndef __System_auditd
#define __System_auditd

#include "System.h"

class System_auditd : public System {

  public:
    System_auditd( void ) : System() { 
      currentmsgid = 0;
    }

    ~System_auditd( void ) {
    };

    int  main   ( DssObject& o ) override;

  private:
    int  run       ( int, DssObject& o ) override;
    void send_data ( DssObject& o, int msgid );

    int  audisp_connect();

    void parse_buf ( string b, DssObject& o );
    int  parse_msgid( string b, DssObject& o, int& msgid, bool& eoeFound );
    int  parse_buf2( string b, int msgid );
    void load_defines( DssObject& o );

    int currentmsgid;
    map<string, string> vals;
    map<int, map<string, string> > datavals;
    map<string, string> callvals64;
    map<string, string> callvals86;
};
#endif
