#ifndef __Upnp_ushare
#define __Upnp_ushare

#include "Upnp.h"

class Upnp_ushare : public Upnp{

  public:
    Upnp_ushare( void ) : Upnp() { 
      msgid = 1;
    }
    ~Upnp_ushare( void ) {
    };

    virtual int  main ( DssObject& o );

  private:
    virtual void send_data( DssObject& o, string& msg );

    int msgid;
};
#endif
