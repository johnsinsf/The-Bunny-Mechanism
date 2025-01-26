include ../make.inc

CCFLAGS +=  -I. -I../include  -I../icomm $(XMLINC) -I/usr/local/include/openssl -D_USEOLDSSL
ifndef system_macos
  CCFLAGS += -shared
endif

CXX = g++ $(CXX_WARN_FLAGS) $(CXX_INCLUDE)
CC = gcc $(CXX_WARN_FLAGS) $(CXX_INCLUDE)

SRCFILES =\
	SocketIO.cc \
	Lib.cc \
	DssSaxParser.cc \
	DssSems.cc \
	Init.cc 
ifdef use_json
  SRCFILES += Json.cc
endif

OBJFILES =\
	SocketIO.o \
	Lib.o \
	DssSaxParser.o \
	DssSems.o \
	Init.o 
ifdef use_json
  OBJFILES += Json.o
endif

ifdef system_macos
OBJ = libdpsio3.a
else
OBJ = libdpsio3.so
endif

all : $(OBJ)

.SUFFIXES : .a .o .c .cc

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

ifdef system_macos
libdpsio3.a : $(OBJFILES)
	libtool   -static -o libdpsio3.a $(OBJFILES)
else
libdpsio3.so : $(OBJFILES)
	g++ -o libdpsio3.so -fPIC -shared  $(OBJFILES)
endif

clean:
	rm -f *.o *.a *.so

install:
	 @cp $(OBJ) $(DIR)/lib/


DssSaxParser.o:\
	../include/dpsframework.h \
	DssSaxParser.cc \
	DssSaxParser.h

DssSems.o:\
	../include/dpsframework.h \
	DssSems.cc \
	DssSems.h

Lib.o:\
	../include/dpsframework.h \
	Lib.cc

Init.o:\
	../include/dpsframework.h \
	Init.cc

Json.o:\
	../include/dpsframework.h \
	Json.cc \
	Json.h

SocketIO.o:\
	../include/dpsframework.h \
	SocketIO.cc \
	SocketIO.h
