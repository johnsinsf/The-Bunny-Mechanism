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

    virtual int  main   ( DssObject& o );

  private:
    virtual int  audisp_connect();
    virtual int  run       ( int, DssObject& o );
    virtual void parse_buf ( string b, DssObject& o );
    virtual void parse_buf_binary ( string b, DssObject& o );
    virtual int  parse_msgid( string b, DssObject& o, int& msgid, bool& eoeFound );
    virtual int  parse_buf2( string b, int msgid );
    virtual void send_data ( DssObject& o, int msgid );
    virtual void load_defines( void );

    int currentmsgid;
    map<string, string> vals;
    map<int, map<string, string> > datavals;
    map<string, string> callvals64;
    map<string, string> callvals86;
};
#endif
