#ifndef __Upnp
#define __Upnp

#include "Mech.h"

class Upnp : public Mech {

  public:
    Upnp( void ) : Mech() { 
    }
    virtual ~Upnp( void ) {
    };

    virtual int  main( DssObject& o ) override;

    static const  key_t  INITKEY2;

  private:
};
#endif
