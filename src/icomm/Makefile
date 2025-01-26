include ../make.inc

CCFLAGS += -shared -I../include -I../dpsio -I ../baseServer -I. 

CXX = g++ $(CXX_WARN_FLAGS)  $(CXX_INCLUDE)
CC = gcc $(CXX_WARN_FLAGS)  $(CXX_INCLUDE)

SRCFILES =\
	Icomm.cc

OBJFILES =\
	Icomm.o 

ifdef system_macos
OBJ = libicomm3.a
else
OBJ = libicomm3.so
endif

all : $(OBJ)

.SUFFIXES : .a .o .c .cc

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

libicomm3.so : $(OBJFILES)
	g++ -o libicomm3.so -fPIC -shared  $(OBJFILES)

libicomm3.a : $(OBJFILES)
	libtool -static -o libicomm3.a $(OBJFILES)

clean:
	rm -f *.o *.a *.so

install:
	@cp -f $(OBJ) $(DIR)/lib/

Icomm.o:\
	../include/dpsframework.h \
	../dpsio/SocketIO.cc \
	../dpsio/SocketIO.h \
	Icomm.cc \
	Icomm.h

