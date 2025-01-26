ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

#dssclient
include ../../../../../make.inc

PROG = ushare

EXTRADIST = ushare.1 \
	cds.h \
	cms.h \
	msr.h \
	http.h \
	bunny.h \
	bunny_meta.h \
	presentation.h \
	metadata.h \
	mime.h \
	services.h \
	buffer.h \
	util_iconv.h \
	content.h \
	cfgparser.h \
	trace.h \
	redblack.h \
	osdep.h \
	ctrl_telnet.h \
	ushare.h \
	gettext.h \
	minmax.h \


SRCS = \
        cds.cc \
        cms.cc \
        msr.cc \
        http.cc \
        bunny.cc \
        bunny_meta.cc \
        presentation.cc \
        metadata.cc \
        mime.cc \
        services.cc \
        buffer.cc \
        util_iconv.cc \
        content.cc \
        cfgparser.cc \
        trace.cc \
        redblack.cc \
        osdep.cc \
        ctrl_telnet.cc \
        ushare.cc

OBJS = $(SRCS:.cc=.o)

.SUFFIXES: .cc .o

all: depend $(PROG)

ifndef system_macos
.cc.o:
	g++ -c $(CFLAGS) -I/dssclient/src -I/dssclient/src/classes -I/dssclient/src/icomm -I/dssclient/src/dpsio -I/dssclient/src/baseServer -I/dssclient/src/include -I/dssclient/src/mech $(OPTFLAGS) -DUSE_BUNNY -D_USEFILES -DINSTALLDIR=\"$(DIR)\" -fPIC -shared  -o $@ $<

$(PROG): $(OBJS)
	g++ $(OBJS) $(LDFLAGS) -I/dssclient/src -I/dssclient/src/classes -I/dssclient/src/icomm -I/dssclient/src/dpsio -I/dssclient/src/baseServer -I/dssclient/src/include -I/dssclient/src/mech $(EXTRALIBS) -DUSE_BUNNY -D_USEFILES -DINSTALLDIR=\"$(DIR)\" -fPIC -shared  -dynamic -o $@.so

else

.cc.o:
	g++ -c $(CFLAGS) -I$(SRC)/ -I$(SRC)/classes -I$(SRC)/icomm -I$(SRC)/dpsio -I$(SRC)/baseServer -I$(SRC)/include -I$(SRC)/mech $(OPTFLAGS) -DUSE_BUNNY -D_HAVEMACOS -D_USEFILES -DINSTALLSRC=\"$(SRC)\" -fPIC -shared  -o $@ $<

$(PROG): $(OBJS)
	g++ $(OBJS) $(LDFLAGS) -I$(SRC)/ -I$(SRC)/classes -I$(SRC)/icomm -I$(SRC)/dpsio -I$(SRC)/baseServer -I$(SRC)/include -I$(SRC)/mech $(EXTRALIBS) -DUSE_BUNNY -D_HAVEMACOS -D_USEFILES -fPIC -shared  -dynamic -o $@.so -Wl, -undefined suppress

endif

clean:
	-$(RM) -f *.o $(PROG)
	-$(RM) -f .depend

distclean:

install: $(PROG)
	$(INSTALL) -d $(DIR)/bin
	$(INSTALL) $(PROG).so $(DIR)/bin
	$(INSTALL) -d $(DIR)/lib
	$(INSTALL) $(PROG).so $(DIR)/lib
	#$(STRIP) $(INSTALLSTRIP) $(DIR)/bin/$(PROG).so
	#$(STRIP) $(INSTALLSTRIP) $(DIR)/lib/$(PROG).so

depend:
ifdef system_macos
	g++ -I.. -MM -I$(SRC)/ -I$(SRC)/classes -I$(SRC)/icomm -I$(SRC)/dpsio -I$(SRC)/baseServer -I$(SRC)/include -I$(SRC)/mech -DUSE_BUNNY -D_HAVEMACOS -D_USEFILES  $(CFLAGS) $(SRCS) 1>.depend
else
	g++ -I.. -MM -I$(SRC)/ -I$(SRC)/classes -I$(SRC)/icomm -I$(SRC)/dpsio -I$(SRC)/baseServer -I$(SRC)/include -I$(SRC)/mech -DUSE_BUNNY -D_USEFILES  $(CFLAGS) $(SRCS) 1>.depend
endif

.PHONY: clean distclean install depend

dist-all:
	cp $(EXTRADIST) $(SRCS) Makefile $(DIST)

.PHONY: dist-all

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
