
#ifndef __MechLJU3
#define __MechLJU3

#include "MechLJ.h"
#include "u3.h"
#include "dpslibLJU3.h"

class MechLJU3 : public MechLJ {

  public:
    MechLJU3( void ) : MechLJ() { 
    }
    ~MechLJU3( void ) {
    };

    virtual int  main ( DssObject& o );

  private:
    virtual int  readDeviceLoop ( HANDLE hDevice, DssObject& o );
};
#endif
