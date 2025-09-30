#ifndef __Dsp
#define __Dsp

#include "Mech.h"

class Dsp : public Mech {

  public:
    Dsp( void ) : Mech() { 
    }
    ~Dsp( void ) {
    };

    virtual int  main( DssObject& o );

    static const  key_t  INITKEY2;

  private:
};
#endif
