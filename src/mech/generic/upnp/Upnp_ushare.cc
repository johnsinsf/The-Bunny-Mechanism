#include "Upnp_ushare.h"

extern "C" {
  Upnp_ushare* makeUpnp_ushare() {
    return new Upnp_ushare;
  }
}

extern int ushare_main( DssObject& o );

int 
Upnp_ushare::main( DssObject& o ) {

  semRelease( o.semInitID );

  ushare_main(o);

  return 0;
}

void
Upnp_ushare::send_data( DssObject& o, string& msg ) {

  dataType d;
  d.structtype = 0;
  d.datatype   = 5;
  d.localid    = o.localID;
  d.channel    = "NOT SET";
  d.synced     = 0;
  d.tries      = 0;
  d.datetime_sent = 0;
  d.data       = msg;
  d.msgid      = ltoa(msgid++);
  d.msgtime    = ltoa(time(NULL));
 
  o.server->add_client_data( o, d );

  return;
}
