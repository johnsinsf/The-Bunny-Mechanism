include ../../make.inc

DSS_SRC = MechLJ.cc

LIBS = -lm -llog4cpp  -L$(DIR)/lib -lmydss3 -ldpsio3 -licomm3 -llog4cpp -lcrypto -lssl -lcrypt -ldl -Wall -lm $(MYSQLLIBS) $(XMLLIBS) -lpthread ../Mech.o ../../baseServer/BaseServer.o

ifdef labjacku3
  DSS_SRC += MechLJU3.cc dpslibLJU3.cc u3.c
  LIBS += -llabjackusb
endif

ifdef labjackt4
  DSS_SRC += MechLJT4.cc 
  LIBS += -lLabJackM
endif

DSS_OBJ=$(DSS_SRC:.cc=.o)

SRCS=$(wildcard *.c)
HDRS=$(wildcard *.h)

CXX = g++ $(CCFLAGS) -I ../ -I../../include  -I../../icomm -I../../baseServer -I ../../classes  -I ../../dpsio $(XMLINC) -I/usr/include/mysql -DINSTALLDIR=\"$(DIR)\"

all :	libdsslj.so

.SUFFIXES : .a .o .c .cc

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

libdsslj.so : $(DSS_OBJ)
	g++ -rdynamic -o libdsslj.so -fPIC -shared  $(DSS_OBJ) $(LIBS)

clean:
	rm -f *.o *.a

install:
	@cp -f libdsslj.so $(DIR)/lib/

MechLJ.o:\
	../../include/dpsframework.h \
	../Mech.h \
	../Mech.cc \
	../dssClient1.cc  \
	MechLJ.cc \
	MechLJ.h

MechLJT4.o:\
	../../include/dpsframework.h \
	../Mech.h \
	../Mech.cc \
	../dssClient1.cc  \
	../../classes/Channel_types.h \
	../../classes/Channel_types.cc \
	MechLJT4.cc \
	MechLJT4.h

MechLJU3.o:\
	../../include/dpsframework.h \
	../Mech.h \
	../Mech.cc \
	../dssClient1.cc  \
	../../classes/Channel_types.h \
	../../classes/Channel_types.cc \
	u3.h \
	u3.c \
	dpslibLJU3.cc \
	dpslibLJU3.h \
	MechLJU3.cc \
	MechLJU3.h

dpslibLJU3.o:\
	../../include/dpsframework.h \
	../Mech.h \
	../Mech.cc \
	../dssClient1.cc  \
	dpslibLJU3.cc \
	dpslibLJU3.h 

