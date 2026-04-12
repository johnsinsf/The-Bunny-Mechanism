#ifndef __System
#define __System

#include "Mech.h"

class System : public Mech {

  public:
    System( void ) : Mech() { 
    }
    virtual ~System( void ) {
    };

    int  main( DssObject& o ) override;

    virtual int  doPoll ( int fd, int t );

    static const  key_t  INITKEY2;

  private:
};
#endif
