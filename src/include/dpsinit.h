
#ifndef __DPSINIT
#define __DPSINIT

extern void dpsSigHandler( int sig );
extern void dpsSigInit( void );
extern void dpsInit( int argc, char **argv );
extern void* resetHandles( void** h1, void** h2 );
extern void* getdpsdlsym( void* handle, string s, pthread_mutex_t& mutex );
extern void doDetach( void );

#endif
