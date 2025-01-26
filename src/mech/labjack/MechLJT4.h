
#ifndef __MechLJT4
#define __MechLJT4

#include "MechLJ.h"
#include "Channels.h"
#include <LabJackM.h>

class MechLJT4 : public MechLJ {

  public:
    MechLJT4( void ) : MechLJ() { 
    }
    ~MechLJT4( void ) {
    };

    virtual int  main ( DssObject& o );

  private:
    virtual int  readDeviceLoop ( int hDevice, DssObject& o );
    virtual int  dpsLJRead      ( int hDevice, DssObject& o );
    virtual int  dpsLJMotor     ( int hDevice, DssObject& o );
    virtual int  handleCommand  ( int hDevice, DssObject& o );

    Channels::ctstruct init_channels[11] = { 
      { "DAC0",   0, 2, 2 },
      { "DAC1",   1, 2, 2 },
      { "AIN2",   2, 1, 1 },
      { "AIN3",   3, 1, 1 },
      { "AIN0",   4, 1, 1 },
      { "AIN1",   5, 1, 1 },
      { "FIO7",   6, 1, 1 },
      { "FIO6",   7, 1, 1 },
      { "FIO5",   8, 1, 1 },
      { "FIO4",   9, 1, 1 },
      { "END",    -1, 0, 0 },
    };
};
#endif
