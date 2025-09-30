#ifndef __Dsp_share
#define __Dsp_share

#include "Dsp.h"
//#include "bunny.h"
#include "Mech.h"
#include "LObj.h"


class Dsp_share : public Dsp {

  public:
    Dsp_share( void ) : Dsp() { 
      msgid = 1;
      semRespID = 0;
    }
    ~Dsp_share( void ) {
    };

    virtual int main ( DssObject& o );
    virtual int share_main( DssObject& o );
    virtual int doControl( int& threadID );
    virtual int handleCommand( string localId, int localidsem, string sharedir );
    virtual string get_dirents( string s, int level, int maxResp, bool& moreData, string& dir_end, int& index  );
    virtual int getDirectory( string &resp, string &respheader, string req, int maxResp, string sharedir, bool& moreData, string& dir_end, int& index, bool initialize, DssObject& o );
    virtual int getFile( string &resp, string &respheader, string req, int maxResp, string sharedir, bool& moreData, string strval );
    virtual int getFileStatus( string &resp, string &respheader, string req, string sharedir, string strval );

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
