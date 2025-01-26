
#ifndef __MechLJ
#define __MechLJ

#include "Mech.h"
#include <LabJackM.h>

class MechLJ : public Mech {

  public:
    MechLJ( void ) : Mech() { 
    }
    ~MechLJ( void ) {
    };

    virtual void processDeviceData( DssObject& o, bool opt = true );
    virtual void doDataChanged    ( int i, DssObject& o );

  private:
};
#endif
