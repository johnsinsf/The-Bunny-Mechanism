include ../../../make.inc

ifdef system_macos
  LIBS = -L$(DIR)/lib $(XMLLIBS) ../../Mech.o ../../../baseServer/BaseServer.o  ../../../classes/libmydss3.a
else
  LIBS = -L$(DIR)/lib $(XMLLIBS) ../../Mech.o ../../../baseServer/BaseServer.o  
endif

DSS_SRC = Upnp.cc Upnp_ushare.cc 

DSS_OBJ=$(DSS_SRC:.cc=.o)

CXX = g++ $(CCFLAGS) -I ../../ -I../../../include  -I../../../icomm -I../../../baseServer -I ../../../classes  -I ../../../dpsio $(XMLINC) 

OBJ :	libdssushare.so

all : $(OBJ)

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

libdssushare.so : $(DSS_OBJ)
ifndef system_macos
	g++ -rdynamic -o libdssushare.so -fPIC -shared  $(DSS_OBJ) $(LIBS) $(DIR)/lib/ushare.so
else
	g++ -undefined suppress -rdynamic -o libdssushare.so -fPIC -shared  $(DSS_OBJ) $(LIBS) $(DIR)/lib/ushare.so
endif

clean:
	rm -f *.o *.a *.so

install:
	@cp -f libdssushare.so $(DIR)/lib/

