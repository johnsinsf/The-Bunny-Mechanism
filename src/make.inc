#
#  main.inc  -  variables for building the bunny mechanism
#

#
# set DIR to the installation directory
# set SRC to the source directory
#
DIR = /bunnymech
SRC = /home/johns/bunnymech/src

#
# uncomment each interface type you want to build
#
#labjacku3 = yes
#labjackt4 = yes
system_netlink = yes
system_auditd = yes
system_host = yes
#system_macos  = yes

#
# if you want to enable XML support, uncomment the following lines 
# and set the paths correctly, XML is for updating the local database
# from host changes and is not enabled in the first version
#
#libxml    = yes
#XMLINC = -I/usr/include/libxml++-3.0 -I/usr/include/glibmm-2.4/ -I/usr/include/glib-2.0/ -I/usr/lib/glib-2.0/include/ -I /usr/lib/libxml++-3.0/include/ -I /usr/lib/glibmm-2.4/include/
#XMLLIBS = -lxml2 -lxml++-3.0 -lglibmm-2.4 

#
# other specific build options
#
#use_mysql = yes
use_files = yes
use_json = yes
use_transform = yes
use_parenttgid = yes


#
# set the flags to whatever you want to use for the build
#
CCFLAGS = -Wall -g -O2 -fPIC -ansi -pedantic -DINSTALLDIR=\"$(DIR)\" -I/usr/include/mysql

ifdef libxml 
  CCFLAGS += -D_USELIBXML
endif
ifdef use_mysql
  CCFLAGS += -D_USEMYSQL
  MYSQLLIBS = -L/usr/lib64 -L/usr/lib64/mysql -lmysqlclient_r
  CCFLAGS += -std=c++11
endif
ifdef use_files
  CCFLAGS += -D_USEFILES
endif
ifdef use_json
  CCFLAGS += -D_USEJSON
  CCFLAGS += -std=c++11
endif
ifdef use_transform
  CCFLAGS += -D_USETRANSFORM
  CCFLAGS += -std=c++11
endif
ifdef system_macos
  CCFLAGS += -D_HAVEMACOS
endif
ifdef use_parenttgid 
  CCFLAGS += -D_HAVEPARENTTGID
endif
