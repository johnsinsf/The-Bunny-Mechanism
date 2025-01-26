#include "System_skel.h"

int 
System_skel::main( DssObject& o ) {

  // put your code here
  return 0;
}

void
System_skel::send_data( DssObject& o, string& msg ) {

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
