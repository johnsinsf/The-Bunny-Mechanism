include ../make.inc

CCFLAGS += -shared -I. -I../include -I../dpsio -I../icomm -I ../shm 

CXX = g++ $(CXX_WARN_FLAGS) $(CXX_INCLUDE)
CC = gcc $(CXX_WARN_FLAGS)  $(CXX_INCLUDE)

SRCFILES =\
	BaseServer.cc

OBJFILES =\
	BaseServer.o 

all : BaseServer.o

.SUFFIXES : .a .o .c .cc

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

clean:
	rm -f *.o *.a

install:
	@cp -f BaseServer.o $(DIR)/lib/

BaseServer.o: ../include/dpsframework.h 
BaseServer.o: ../icomm/Icomm.h ../dpsio/SocketIO.h
BaseServer.o: ../include/dpsinit.h 
