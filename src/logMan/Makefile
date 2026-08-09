DIR = /bunnymech

#CXX_FLAGS = -g -I. -I../baseServer -I../icomm -I../dss -I../dpsio -I ../shm -I../include -I/usr/include/mysql -I/usr/include/mysql/server -Wall

#CXX_LIBS = -L$(DIR)/lib  -lz -lmydss -ldpsio2 -ldssshm -licomm2 -llog4cpp -lmysqlclient_r -lcrypto -lpthread -lcrypt -lssl -ldl -lxml2 -lxml++-3.0 -lglibmm-2.4 -L/usr/local/lib ../baseServer/BaseServer2.o

CXX_LIBS = -L$(DIR)/lib  -lz -ldpsio3  -licomm3 -llog4cpp -lcrypto -lpthread -lcrypt -lssl -ldl ../baseServer/BaseServer.o

ifndef system_macos
CXX_FLAGS += -L$(DIR)/lib -lz -I../dpsio -I../include -I../icomm -I../baseServer -I ../classes -I. $(XMLINC) -lmydss3 -ldpsio3 -licomm3 -llog4cpp -lcrypto -lssl -lcrypt -ldl -lm $(MYSQLLIBS) $(XMLLIBS) -lpthread -D_USEFILES
else
CXX_FLAGS += -L$(DIR)/lib -lz -I../dpsio -I../include -I../icomm -I../baseServer -I ../classes -I. -D_USEFILES
endif


CXX = g++ $(CXX_WARN_FLAGS)  $(CXX_INCLUDE) -DINSTALLDIR=\"$(DIR)\"
CC = gcc $(CXX_WARN_FLAGS)  $(CXX_INCLUDE) -DINSTALLDIR=\"$(DIR)\"

SRCFILES =\
	LogMan.cc \
	logMan.cc 

OBJFILES =\
	LogMan.o \
	logMan.o 

all : logMan

.SUFFIXES : .a .o .c .cc

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CXX_FLAGS) -c $*.cc -o $*.o


logMan : $(OBJFILES)
	g++ -o logMan $(OBJFILES) $(CXX_FLAGS) $(CXX_LIBS)

clean:
	rm -f *.o *.a logMan

install:
	 @cp logMan $(DIR)/bin/

depend:
	makedepend -- $(CXX_FLAGS) -- $(SRCFILES)


