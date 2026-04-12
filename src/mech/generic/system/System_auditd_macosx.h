#ifndef __System_auditd_macosx
#define __System_auditd_macosx

#include "System.h"

class System_auditd_macosx : public System {

  public:
    System_auditd_macosx( void ) : System() { 
      currentmsgid = 0;
    }
    ~System_auditd_macosx( void ) {
    };

    int  main   ( DssObject& o ) override;

  private:
    int  run       ( int, DssObject& o ) override;
    void parse_buf ( string b, DssObject& o );
    int  parse_msgid( const char* bufptr, int len, DssObject& o, unsigned int& msgid, unsigned int& msgtime, int& cnt);
    int  parse_buf2( const char* bufptr, int len, int msgid, bool& eoeFound, int& cnt );
    void send_data ( DssObject& o, unsigned int msgid, unsigned int msgtime );
    void load_defines( void );
    void load_procs( void );

    int currentmsgid;
    map<string, string> vals;
    map<int, map<string, string> > datavals;
    map<string, string> callvals;
    map<unsigned int, string> procvals;
};
#endif
