#ifndef __Dsp_share
#define __Dsp_share

#include "Dsp.h"
#include "Mech.h"
#include "LObj.h"

class Dsp_share : public Dsp {

  public:
    Dsp_share( void ) : Dsp() { 
      msgid     = 1;
      semRespID = 0;
      mylogctr  = 1;
      logfd     = -1;
      makeFilesPublic = false;
    }
    virtual ~Dsp_share( void ) {
    };

    virtual int main ( DssObject& o ) override;
    virtual int share_main( DssObject& o );
    virtual int doControl( int& threadID ) override;
    virtual int handleCommand( string localId, int localidsem, string sharedir );

    virtual int create_dirents_file( string s, int fd );
    virtual string get_dirents( string infile, string req, int maxResp, bool& moreData, string& end, int start_index, int& index, stack<string>& dirs );
    virtual int getDirectory( string &resp, string &respheader, string& headin, string& tailin, string req, int maxResp, string sharedir, bool& moreData, string& dir_end, int start_index, int& index, stack<string>& dirs, bool initialize, DssObject& o, string logfilename );
    virtual int getFile( string &resp, string &respheader, string req, int maxResp, string sharedir, bool& moreData, string strval );
    virtual int getFileStatus( string &resp, string &respheader, string req, string sharedir, string strval );
    virtual void mapdirs( string s, map<int, string> &m );

  private:
    virtual void send_data( DssObject& o, string& msg );

    int msgid;
    int mylogctr;
    int logfd;
    bool makeFilesPublic;

#ifndef _HAVEMACOS
    int semRespID;
#else
    dispatch_semaphore_t semRespID;
#endif
};
#endif
