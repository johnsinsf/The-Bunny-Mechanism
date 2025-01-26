#ifndef __System
#define __System

#include "Mech.h"

class System : public Mech {

  public:
    System( void ) : Mech() { 
    }
    ~System( void ) {
    };

    virtual int  main( DssObject& o );
    virtual int  doPoll ( int fd, int t );

    static const  key_t  INITKEY2;

  private:
};
#endif
