include ../../../make.inc

ifdef system_macos
  LIBS = -L$(DIR)/lib $(XMLLIBS) ../../Mech.o ../../../baseServer/BaseServer.o  ../../../classes/libmydss3.a
else
  LIBS = -L$(DIR)/lib $(XMLLIBS) ../../Mech.o ../../../baseServer/BaseServer.o  
endif

ifndef system_macos
  DSS_SRC = System.cc System_netlink.cc System_auditd.cc System_host.cc System_skel.cc
else
  DSS_SRC = System.cc System_auditd_macosx.cc System_host.cc
endif

DSS_OBJ=$(DSS_SRC:.cc=.o)

CXX = g++ $(CCFLAGS) -I ../../ -I../../../include  -I../../../icomm -I../../../baseServer -I ../../../classes  -I ../../../dpsio $(XMLINC) 

OBJ :	libdsssystem.so

all : $(OBJ)

.cc.o :
	echo "Compiling ............. $*.cc"
	$(CXX) $(CCFLAGS) -c $*.cc -o $*.o

libdsssystem.so : $(DSS_OBJ)
ifndef system_macos
	g++ -rdynamic -o libdsssystem.so -fPIC -shared  $(DSS_OBJ) $(LIBS)
else
	#libtool  -undefined suppress -flat_namespace -dynamic -o libdsssystem.so $(DSS_OBJ) $(LIBS)
	#libtool   -dynamic -o libdsssystem.so $(DSS_OBJ) $(LIBS)
	g++ -undefined suppress -rdynamic -o libdsssystem.so -fPIC -shared  $(DSS_OBJ) $(LIBS)
endif

clean:
	rm -f *.o *.a *.so

install:
	@cp -f libdsssystem.so $(DIR)/lib/

System.o:\
	../../../include/dpsframework.h \
	../../../include/dpsglobal.h \
	../../Mech.h \
	../../Mech.cc \
	../../dssClient1.cc  \
	System.cc \
	System.h \
	System_netlink.h \
	System_netlink.cc \
	System_auditd.h \
	System_auditd.cc \
	System_host.h \
	System_host.cc \
	System_macosx.h \
	System_macosx.cc \

System_auditd.o:\
	../../../include/dpsframework.h \
	../../../include/dpsglobal.h \
	../../Mech.h \
	../../Mech.cc \
	../../dssClient1.cc  \
	System.cc \
	System.h \
	System_auditd.h \
	System_auditd.cc

System_netlink.o:\
	../../../include/dpsframework.h \
	../../../include/dpsglobal.h \
	../../Mech.h \
	../../Mech.cc \
	../../dssClient1.cc  \
	System.cc \
	System.h \
	System_netlink.h \
	System_netlink.cc

System_host.o:\
	../../../include/dpsframework.h \
	../../../include/dpsglobal.h \
	../../Mech.h \
	../../Mech.cc \
	../../dssClient1.cc  \
	System.cc \
	System.h \
	System_host.h \
	System_host.cc

System_macosx.o:\
	../../../include/dpsframework.h \
	../../../include/dpsglobal.h \
	../../Mech.h \
	../../Mech.cc \
	../../dssClient1.cc  \
	System.cc \
	System.h \
	System_macosx.h \
	System_macosx.cc

System_skel.o:\
	../../../include/dpsframework.h \
	../../../include/dpsglobal.h \
	../../Mech.h \
	../../Mech.cc \
	../../dssClient1.cc  \
	System.cc \
	System.h \
	System_skel.h \
	System_skel.cc
