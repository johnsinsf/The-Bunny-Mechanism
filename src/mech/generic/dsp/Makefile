include ../../../make.inc

ifdef system_macos
  LIBS = -L$(DIR)/lib $(XMLLIBS) ../../Mech.o ../../../baseServer/BaseServer.o  ../../../classes/libmydss3.a
else
  LIBS = -L$(DIR)/lib $(XMLLIBS) ../../Mech.o ../../../baseServer/BaseServer.o  
endif

DSS_SRC = Dsp.cc Dsp_share.cc 

DSS_OBJ=$(DSS_SRC:.cc=.o)

CXX = g++ $(CCFLAGS) -I ../../ -I../../../include  -I../../../icomm -I../../../baseServer -I ../../../classes  -I ../../../dpsio $(XMLINC) 

OBJ :	libdspshare.so

all : $(OBJ)

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

libdspshare.so : $(DSS_OBJ)
ifndef system_macos
	g++ -rdynamic -o libdspshare.so -fPIC -shared  $(DSS_OBJ) $(LIBS) 
else
	g++ -undefined suppress -rdynamic -o libdspshare.so -fPIC -shared  $(DSS_OBJ) $(LIBS)
endif

clean:
	rm -f *.o *.a *.so

install:
	@cp -f libdspshare.so $(DIR)/lib/

