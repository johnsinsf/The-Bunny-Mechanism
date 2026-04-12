#ifndef __Channel_types
#define __Channel_types

#include "dpsframework.h"
#include "SBase.h"

class Channel_types : public SBase {
  public:
    void _dss_init( void ) {
      fio       = 0;
      type      = 0;
      direction = 0;
      interfacetype = 0;
      memset(buf1, 0, sizeof(buf1));
    }
    Channel_types( void ) : SBase() { 
      _dss_init();
    }
    Channel_types( unsigned int s ) : SBase() { 
      _dss_init();
    }
    ~Channel_types( void ) {
    }
    void setSearch  ( int by ) override;
    void setStrings ( void ) override;

    void setInterfacetype( int i ) { interfacetype = i; }
    string getChannel( void ) { return channel; }
    unsigned int getFio ( void ) { return fio; }
    unsigned int getType( void ) { return type; }
    unsigned int getDirection( void ) { return direction; }

  private:
    unsigned int  fio;
    unsigned int  interfacetype;
    unsigned int  type;
    unsigned int  direction;
    string channel;
    char buf1[80];
};
  
#endif
