include ../make.inc

SRCS = dssClient1.cc Mech.cc 
OBJS = dssClient1.o Mech.o

ifndef system_macos
CCFLAGS += -L$(DIR)/lib -lz -I../dpsio -I../include -I../icomm -I../baseServer -I ../classes -I. $(XMLINC) -lmydss3 -ldpsio3 -licomm3 -llog4cpp -lcrypto -lssl -lcrypt -ldl -lm $(MYSQLLIBS) $(XMLLIBS) -lpthread 
else
#CCFLAGS += -L$(DIR)/lib -lz -I../dpsio -I../include -I../icomm -I../baseServer -I ../classes -I. -ldl -lm -lpthread /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a /usr/local/lib/liblog4cpp.a
CCFLAGS += -L$(DIR)/lib -lz -I../dpsio -I../include -I../icomm -I../baseServer -I ../classes -I. 
endif

.SUFFIXES : .a .o .c .cc

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

all: $(OBJS)
ifndef system_macos
	g++ $(CCFLAGS) $(SRCS) -o bunnymech  ../baseServer/BaseServer.o
else
	g++ $(CCFLAGS) dssClient1.cc Mech.o ../baseServer/BaseServer.o $(DIR)/lib/libdpsio3.a $(DIR)/lib/libmydss3.a $(DIR)/lib/libicomm3.a -ldl -lm -lpthread /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a /usr/local/lib/liblog4cpp.a -o bunnymech -Wl,-stack_size -Wl,3f00000
endif

Mech.o:
	g++ $(CCFLAGS) -I/usr/local/include/openssl -fPIC -c Mech.cc

clean:
	rm -f *.o *.so labjack/*.o labjack/*.so generic/system/*.o generic/system/*.so generic/upnp/*[.o,.so] generic/upnp/ushare/src/*[.o,.so] bunnymech 

install:
	@cp -f bunnymech $(DIR)/bin


Mech.o:\
	../include/dpsframework.h \
	../baseServer/BaseServer.h \
	../baseServer/BaseServer.cc \
	../icomm/Icomm.cc \
	../icomm/Icomm.h \
	../classes/Logs.h \
	../classes/Logs.cc \
	../classes/Locations.h \
	../classes/Locations.cc \
	../classes/Interfaces.h \
	../classes/Interfaces.cc \
	../classes/Hosts.h \
	../classes/Hosts.cc \
	../classes/Channels.h \
	../classes/Channels.cc \
	../classes/Channels_analog.h \
	../classes/Channels_analog.cc \
	../classes/Channels_digital.h \
	../classes/Channels_digital.cc \
	../dpsio/DssSems.cc \
	../dpsio/DssSems.h \
	Mech.h \
	dssClient1.cc

dssClient1.o:\
	../include/dpsframework.h \
	../include/dpsglobal.h \
	../include/dpsinit.h \
	../baseServer/BaseServer.h \
	../baseServer/BaseServer.cc \
	Mech.h \
	dssClient1.cc
