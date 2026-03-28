INSTALLDIR = /bunnymech

SRCS = test.cc Test.cc ../baseServer/BaseServer.cc

CFLAGS = -std=gnu++11 -g -L/bunnymech/lib -lz -I/usr/include/mysql -I../classes -I../shm -I../dpsio -I../include -I../icomm -I../baseServer -I../classes -licomm3 -llog4cpp -Wall  -ldpsio3 -lssl -lmydss3 -lmysqlclient_r -fuse-cxa-atexit -I/usr/include/libxml++-3.0 -I/usr/include/glibmm-2.4/ -I/usr/include/glib-2.0/ -I/usr//lib/glib-2.0/include/ -I /usr//lib/libxml++-3.0/include/ -I /usr/lib/glibmm-2.4/include/ -lxml2 -lxml++-3.0 -lglibmm-2.4 -DINSTALLDIR=\"/bunnymech\" -D_USELIBXML  -D_USEFILES

#CFLAGS2 = -std=gnu++11 -g -L/dps3/lib -lz -I/usr/include/mysql -I../shm -I../dpsio -I../include -I../icomm -I../baseServer -I../classes -licomm2 -llog4cpp -Wall  -ldpsio2 -lssl -lmydss -lmysqlclient_r -ldssshm -fuse-cxa-atexit  -I/usr/include/libxml++-3.0 -I/usr/include/glibmm-2.4/ -I/usr/include/glib-2.0/ -I/usr//lib/glib-2.0/include/ -I /usr//lib/libxml++-3.0/include/ -I /usr/lib/glibmm-2.4/include/ -lxml2 -lxml++-3.0 -lglibmm-2.4 

#CXX_FLAGS = -O3 -g -I. -I../dss -I../dpsio -I../icomm -I../baseServer -I ../shm -I../include -I/usr/local/mysql/include/mysql -I/usr/local/mysql/include/mysql/server -Wall  -L/dps3/lib  -lz -lmydss -ldpsio2 -ldssshm -licomm2 -llog4cpp /usr/local/mysql/lib/libmysqlclient_r.so -lcrypto -lpthread -lcrypt -lssl -ldl -lxml2 -lxml++-3.0 -lglibmm-2.4

CXX_FLAGS = -std=gnu++11 -g -L/bunnymech/lib -lz -I/usr/include/mysql -I/usr/include/mysql/server -I../dpsio -I../shm -I../include -I../icomm -I../baseServer -I ../classes -I. -I/usr/include/libxml++-3.0 -I/usr/include/glibmm-2.4/ -I/usr/include/glib-2.0/ -I/usr//lib/glib-2.0/include/ -I /usr//lib/libxml++-3.0/include/ -I /usr/lib/glibmm-2.4/include/ -lmydss3 -ldpsio3 -licomm3 -llog4cpp -lcrypto -lssl -lcrypt -ldl -Wall -lm  -D_USELIBXML -L/usr/local/mysql/lib -lmysqlclient_r  -lxml2 -lxml++-3.0 -lglibmm-2.4   -D_USEFILES

all:
	g++ $(CFLAGS) -lpthread $(SRCS) -o test

test2:
	g++ $(CXX_FLAGS) -lpthread $(SRCS) -I/usr/include/mysql -DINSTALLDIR=\"dps3\" -o test2

test3:
	g++ $(CXX_FLAGS) -lpthread test3.cc ../dss/Climate.o -o test3

test10:
	g++ -lpthread test10.cc -o test10

test12:
	g++ $(CXX_FLAGS) -lpthread test12.cc -o test12

test13:
	g++ $(CXX_FLAGS) -lpthread test13.cc -o test13

test17:
	g++ $(CXX_FLAGS) -lpthread test17.cc -o test17

test21:
	g++ $(CXX_FLAGS) -lpthread test21.cc -o test21

test26:
	g++ $(CXX_FLAGS) -lpthread /bunnymech/lib/BaseServer.o test26.cc -o test26 

testsrv:
	g++ $(CXX_FLAGS) testsrv.cc -o testsrv

testcli:
	g++ $(CXX_FLAGS) testcli.cc -o testcli

testlog:
	g++ $(CXX_FLAGS) testlog.cc -o testlog

memc:
	g++ $(CXX_FLAGS) memc.cc -o memc

