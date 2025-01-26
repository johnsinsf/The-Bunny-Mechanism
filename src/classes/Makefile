include ../make.inc

CCFLAGS +=  -I../include -I. -I ../dpsio -I../baseServer -I../icomm $(XMLINC) 
ifndef system_macos
  CCFLAGS += -shared
endif

CXX = g++ $(CXX_WARN_FLAGS) $(CXX_INCLUDE)
CC = gcc $(CXX_WARN_FLAGS) $(CXX_INCLUDE)

SRCFILES =\
	Alerts.cc \
	SBase.cc \
	Channel_types.cc \
	Channels_digital.cc \
	Channels_analog.cc \
	Channels.cc \
	Interfaces.cc \
	Locations.cc \
	Hosts.cc \
	Logs.cc  

OBJFILES =\
	Alerts.o \
	SBase.o \
	Channel_types.o \
	Channels_digital.o \
	Channels_analog.o \
	Channels.o \
	Interfaces.o \
	Locations.o \
	Hosts.o \
	Logs.o  

ifdef system_macos
all : libmydss3.a
else
all : libmydss3.so
endif

.SUFFIXES : .a .o .c .cc

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

ifdef system_macos
libmydss3.a : $(OBJFILES)
	libtool -static -o libmydss3.a $(OBJFILES)
else
libmydss3.so : $(OBJFILES)
	g++ -rdynamic -o libmydss3.so -fPIC -shared  $(OBJFILES) 
endif

clean:
	rm -f *.o *.a *.so

install:
ifdef system_macos
	@cp -f libmydss3.a $(DIR)/lib/
else
	@cp -f libmydss3.so $(DIR)/lib/
endif

 
Alerts.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Alerts.cc \
	Alerts.h

Channel_types.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Channel_types.h

Channels.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Interfaces.h \
	Interfaces.cc \
	Channels_digital.h \
	Channels_digital.cc \
	Channels_analog.h \
	Channels_analog.cc \
	Channels.cc \
	Channels.h

Channels_analog.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Interfaces.h \
	Interfaces.cc \
	Channels_analog.cc \
	Channels_analog.h 

Channels_digital.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Interfaces.h \
	Interfaces.cc \
	Channels_digital.cc \
	Channels_digital.h 

Hosts.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Hosts.cc \
	Hosts.h 

Interfaces.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Interfaces.cc \
	Interfaces.h 

Locations.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Locations.cc \
	Locations.h 

Logs.o:	\
	../include/dpsframework.h \
	SBase.cc \
	SBase.h \
	Logs.cc \
	Logs.h 

SBase.o:	\
	../include/dpsframework.h \
	SBase_mysql.cc \
	SBase_mysql.h \
	SBase_files.cc \
	SBase_files.h \
	SBase.cc \
	SBase.h 

SBase_files.o:	\
	../include/dpsframework.h \
	../dpsio/DssSaxParser.h \
	../dpsio/DssSaxParser.cc \
	../baseServer/BaseServer.cc \
	../baseServer/BaseServer.h \
	SBase_files.cc \
	SBase_files.h \
	SBase.cc \
	SBase.h 

SBase_mysql.o:	\
	../include/dpsframework.h \
	../dpsio/DssSaxParser.h \
	../dpsio/DssSaxParser.cc \
	../baseServer/BaseServer.cc \
	../baseServer/BaseServer.h \
	SBase_mysql.cc \
	SBase_mysql.h \
	SBase.cc \
	SBase.h 

