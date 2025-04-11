#ifndef __Upnp_ushare
#define __Upnp_ushare

#include "Upnp.h"
#include "bunny.h"

class Upnp_ushare : public Upnp {

  public:
    Upnp_ushare( void ) : Upnp() { 
      msgid = 1;
      semRespID = 0;
    }
    ~Upnp_ushare( void ) {
    };

    virtual int  main ( DssObject& o );
    virtual int doControl( int& threadID );
    virtual string sendCommand( string json, string ip );
    virtual int handleCommand( string localId, int localidsem, string ip );

  private:
    virtual void send_data( DssObject& o, string& msg );

    int msgid;
#ifndef _HAVEMACOS
    int semRespID;
#else
    dispatch_semaphore_t semRespID;
#endif
};
#endif
