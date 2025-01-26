#ifndef __Upnp
#define __Upnp

#include "Mech.h"

class Upnp : public Mech {

  public:
    Upnp( void ) : Mech() { 
    }
    ~Upnp( void ) {
    };

    virtual int  main( DssObject& o );

    static const  key_t  INITKEY2;

  private:
};
#endif
