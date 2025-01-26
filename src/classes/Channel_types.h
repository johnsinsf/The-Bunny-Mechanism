#ifndef __Channel_types
#define __Channel_types

//#include <string>
#include "dpsframework.h"
//#include "Log4.h"
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
    virtual ~Channel_types( void ) {
    }
    virtual void setSearch  ( int by );
    virtual void setInterfacetype( int i ) { interfacetype = i; }
    virtual int  insert     ( void );
    virtual void setStrings ( void );
    virtual string getChannel( void ) { return channel; }
    virtual unsigned int getFio ( void ) { return fio; }
    virtual unsigned int getType( void ) { return type; }
    virtual unsigned int getDirection( void ) { return direction; }

  private:
    unsigned int  fio;
    unsigned int  interfacetype;
    unsigned int  type;
    unsigned int  direction;
    string channel;
    char buf1[80];
};
  
#endif
