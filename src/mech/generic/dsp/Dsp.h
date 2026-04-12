#ifndef __Dsp
#define __Dsp

#include "Mech.h"

class Dsp : public Mech {

  public:
    Dsp( void ) : Mech() { 
    }
    virtual ~Dsp( void ) {
    };

    virtual int  main( DssObject& o ) override;

    static const  key_t  INITKEY2;

  private:
};
#endif
