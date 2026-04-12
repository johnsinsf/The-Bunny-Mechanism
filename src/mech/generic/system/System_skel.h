#ifndef __System_skel
#define __System_skel

#include "System.h"

class System_skel : public System {

  public:
    System_skel( void ) : System() { 
      msgid = 1;
    }
    ~System_skel( void ) {
    };

    int  main ( DssObject& o ) override;

  private:
    void send_data( DssObject& o, string& msg );

    int msgid;
};
#endif
