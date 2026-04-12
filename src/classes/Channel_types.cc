
#include "dpsframework.h"
#include "Channel_types.h"

void
Channel_types::setSearch( int by ) {
  switch( by ) {
    default:
    case byKey:
      search = "SELECT channel, fio FROM channel_types WHERE interfacetype = " + itoa(interfacetype);
      readMode = READSEQ;
      break;
  }
  int i = 0;
  addResultVar(&buf1, i);
  addResultVar(&fio, i);
}

void
Channel_types::setStrings( void ) {
  channel = buf1;
}
