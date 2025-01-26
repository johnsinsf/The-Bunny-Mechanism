The Bunny Mechanism is a C++ framework that interfaces to the bunny system.

It is designed to run programs created in a shared library and provides the comm and alert framework so you only have to code your routines and the framework does the rest.

The framework supports data in XML (two versions), JSON, PHP serialize, and ISO8583 (modified version). This version of ISO8583 supports extra large fields and arbitrary length binary data transfers.

I've also included the shared libraries I've created.  They are:

Internet IO using the Labjack devices

System Monitoring

System Auditing

DLNA compliant music streaming


You can create your own library to run, just use the System_skel.cc file as an example.


Internet IO

This gives you the ability to read and write analog and digital values over the internet. For instance, turning on a light or reading the value of a switch. It can also control stepper motors.

labjack bidirectional example:
https://www.youtube.com/watch?v=9CaHMOVGHeE

general purpose data collector example:
https://www.youtube.com/watch?v=icwZB-hHMNk

Be sure to configure your Labjack device before using in the bunny mech.


System Monitoring

This scans the system and reports current system load and how full certain directories are. Alerts are sent if monitored programs or scripts fail, or if disk space exceeds limits.


System Auditing

Uses the AuditD and the Netlink interfaces to look for system activity. You can monitor what files are opened and when sockets are created, for example.

The system collects all the data and analyzes it in realtime looking for anomalies. Reports are created daily of all activity.


MacOs:

On the Mac, first enable auditd. (more directions coming) 

Then edit /etc/security/audit_user and put in the settings you want. I use these:

#root:lo,nt,ex:no
root:lo,ex:no
johns:lo,cl,nt,ex:no

You can also enable root for everything or add other options. Copy your file to a back location because it will be erased during the next OS update. Also, AuditD is deprecated and I don't know how long it will be around. You can also update the audit_control file to increase the limits.

I'm currently using MacOS Sequoia 15.2. When I wrote this software 5 years ago to monitor all my systems, I had figured that 10MB a day would be a sufficient limit. So I had everything planned on that, and then suddenly my Mac decided that it was going to scan all my pictures and videos, including all my emails, looking for nudity. This just happened, and it broke everything because my mac is now sending almost 2GB a day for logging. And the report is what ties the entire room together. It's the reason for this auditing, so now I have to increase my daily limits to 10GB at least! And this now uses about 5 hours of CPU a day to analyze. I'll have to do something to make that more efficient.

To know if Apple is scanning all your files, first enable auditd and make the changes to the control files and reboot. Then use:

praudit /var/audit/current | grep mediaanalysisd

If you see a lot of files being scanned, then I'm not the only one. They have something called the SensitiveContentAnalysisML that looks for nudity.

I plan on finding a way to stop the scanning, but this was a great way to test out my system. I developed it to detect this exact situation. I just never thought it would be Apple.

This is an example from praudit of what I'm seeing in my logs. Apple is scanning all my pictures and emails and I have about 1 million emails:

36,501,501,20,501,20,404,100007,50331650,0.0.0.0
39,0,0
237,1,com.apple.mediaanalysisd,complete,,complete,0xec3187ae4ad2859af1d008c31790f4b54455fdaf
19,160
20,323,11,112,0,1737291740,813
45,2,0x20,fd
35,/Users/johns/oldmac/Library/Mail/V2/IMAP-john@10.0.2.30/INBOX.mbox/FF7691BA-83E3-4CA1-91B3-375F519B82DA/Data/0/2/0/2/Attachments/2020527/12/image012.gif
62,100644,501,20,16777229,1320887,0
36,501,501,20,501,20,404,100007,50331650,0.0.0.0
39,0,0
237,1,com.apple.mediaanalysisd,complete,,complete,0xec3187ae4ad2859af1d008c31790f4b54455fdaf
19,323
20,323,11,112,0,1737291740,813
45,2,0x20,fd
35,/Users/johns/oldmac/Library/Mail/V2/IMAP-john@10.0.2.30/INBOX.mbox/FF7691BA-83E3-4CA1-91B3-375F519B82DA/Data/0/2/0/2/Attachments/2020527/12/image012.gif
62,100644,501,20,16777229,1320887,0
36,501,501,20,501,20,404,100007,50331650,0.0.0.0
39,0,0
237,1,com.apple.mediaanalysisd,complete,,complete,0xec3187ae4ad2859af1d008c31790f4b54455fdaf


On Linux:

On linux, first update /etc/audit/auditd.conf and change:

distribute_network = yes

then cd to plugins.d and edit af_unix.conf to set active to "yes":

# This file controls the configuration of the
# af_unix socket plugin. It simply takes events
# and writes them to a unix domain socket. This
# plugin can take 2 arguments, the path for the
# socket and the socket permissions in octal.

active = yes
direction = out
path = builtin_af_unix
type = builtin 
args = 0640 /var/run/audispd_events
format = string

Update the audit.rules so the rules you want to start are enabled. I use thse:

# auditctl -l
-a never,exit -F arch=b64 -S connect,openat -F exe=/usr/bin/syslog-ng
-a always,exit -F arch=b64 -S connect,openat -F auid!=1 -F euid!=0 -F key=1
-a always,exit -F arch=b32 -S openat,connect -F auid!=1 -F euid!=0 -F key=1
-a always,exit -F arch=b32 -S open -F exit=-EPERM -F euid!=0 -F key=2
-a always,exit -F arch=b64 -S open -F exit=-EPERM -F euid!=0 -F key=2

You'll need to reboot.


DLNA Music Streaming

About 20 years ago, when ushare first came out, I started using that software to build a music library. Over the years I kept maintaining it and eventually figured I was the only one using it. I had just created a new music streaming system, and wanted a DLNA client, so I decided to use that. Funny thing was I didn't use their new version because it had a bug that stopped playing intermittently. 

So when I wanted to create this version, I looked at the new version and then immediately found the bug. They had added a feature that rescanned the music directory but they didn't add locking, so there was a race condition. I emailed them but didn't hear back.

The performance will be dependent on your network and computer. But I'm not seeing any problems, even when compiling on my Mac. If you experience any dropouts, it's probably my network being overwhelmed!

This is running on my Comcast network, so about 20 to 30 simultaneous streams of high-def music is possible. But I also have a production system with a 1GB connection, so maybe 200 users, and I don't have to spend any money. Based on CPU, the system can handle about 10K streams, it's just limited by bandwidth. It would be nice to create a "club" or something to share music.

https://www.youtube.com/watch?v=oCsAipdjOyY

This version also displays the currently playing song in DIR/cache/playlog. It will also send the link to the smartphone.

You can view https://buuna.dwanta.com/echo/bunnyman/zDojuK4qSLOQtugeVbyi7?hop=7

In the DLNA player, these settings in DIR/conf/dss.conf will play that list using these settings:

bunny_user = QC4ZBEayg5UXk77Vk3kjs
bunny_server = buuna.dwanta.com
bunny_server_port = 443

These settings should demo playing about 1000 streams. It should take about 5 seconds to download, and then the data is cached.

bunny_user = e9WBvPedXB2R1teW6qfM0
bunny_server = buuna.dwanta.com
bunny_server_port = 443

The player uses XML for playlists and HTTP to get the music.


Building and installing

The code should work on linux and macos. It should also compile and work on Windows.

To compile, you'll need the development system and C++ compiler, plus the following minimum libraries installed:

engines-1.1		libixml.la		libssl.dylib
libcrypto.1.1.dylib	liblog4cpp.5.dylib	libupnp.17.dylib
libcrypto.a		liblog4cpp.a		libupnp.a
libcrypto.dylib		liblog4cpp.dylib	libupnp.dylib
libixml.11.dylib	liblog4cpp.la		libupnp.la
libixml.a		libssl.1.1.dylib	pkgconfig
libixml.dylib		libssl.a

Plus, if you're compiling for labjack:

xlibLabJackM.so	       libdlna.a	 libixml.a     libixml.so.2.0.8        libmemcached.so.11  libthreadutil.so.6
libLabJackM.so.1       libdlna.so	 libixml.la    liblabjackusb.so        libthreadutil.a	   libthreadutil.so.6.0.4
libLabJackM.so.1.19.0  libdlna.so.0	 libixml.so    liblabjackusb.so.2.6.0  libthreadutil.la    pkgconfig
libLabJackM.so.1.20.1  libdlna.so.0.2.4  libixml.so.2  libmemcached.so	       libthreadutil.so


You should install the latest OpenSSL. I included the log4cpp and libupnp distributions that I used, but you should be able to use more current versions. Sometimes there's changes, especially with upnp. So install each of the updated ones first and if they don't work, try the one's I supplied that are known to work.


Building:

Take all the code from this distribution and place in your source directory ("source");

Edit the make.inc file

cd source

If you are on Macos, copy make.inc.mac to make.inc

vi make.inc and make changes, especially to the DIR and SRC variables:

#
#  main.inc  -  variables for building the bunny mechanism
#

#
# set DIR to the installation directory
# set SRC to the source directory
#
DIR = /bunnymech
SRC = /dssclient/src

#
# uncomment each interface type you want to build
#
#labjacku3 = yes
#labjackt4 = yes
system_netlink = yes
system_auditd = yes
system_host = yes
#system_macos  = yes

Note that these are just defines in the make file, so you can't set the value to "no"! Just having the field defined is what controls the building.

If you are building the labjack Internet IO, you'll also need the Mysql (MariaDB) files installed. Then create the database:

mysql -u root -p
create database your database;

Create the tables:

mysql -u root -p your database < DIR/scripts/sql.sh

Create the destination directory:

drwxr-xr-x 2 johns johns     4096 Jan 25 10:44 bin
drwxr-xr-x 2 johns johns     4096 Jan 25 02:51 cache
drwxr-xr-x 2 johns johns     4096 Jan 25 03:08 conf
drwxr-xr-x 2 johns johns     4096 Jan 25 10:44 lib
drwxr-xr-x 2 johns johns     4096 Jan 25 11:37 scripts
drwxr-xr-x 2 johns johns     4096 Jan 25 03:08 ssl


Copy the source/scripts to DIR/scripts


You don't need to have an account on the bunny server unless you want to use the features of that system.

If you want to create a user, then after you get the signon link, set your password, enable the bundles you want to use, create your company, and then your device. Copy the information from the device screen into
DIR/conf/dss.conf.


Edit dss.conf

Copy the dss.conf file from source/conf/dss.conf.default to DIR/conf/dss.conf. Edit the file to make any changes, especially to the locations of the shared libraries.

Copy the other files from source/conf to DIR/conf.


Build the system

if you're building the upnp code, go to source/mech/generic/upnp/ushare and type sh configure and then make and make install

cd source
sh makeall

That should build the entire system. I included a build log in buildlog.txt.


Edit ld.so.conf

vi /etc/ld.so.conf and add the entries for the bunny mechanism:

# Dynamic linker/loader configuration.
# See ld.so(8) and ldconfig(8) for details.

/bunnymech/lib
/usr/local/lib

then type "ldconfig" as root.


Running

To run,

cd DIR/bin
./bunnymech &

Or use:

DIR/scripts/bunnymech

Right now you have to be in the bin directory where the program is to run it. To change that, you can edit SRC/mech/dssClient1.cc and update the line:

int 
main( int argc, char *argv[] ) {
  std::string initFileName = "log4cpp.properties";

to change the initFileName to include the directory location. 

The bunnymech program can run directly connected to the bunny host, or if you have many servers, use the "local" option and have all the local clients connect to the single node that has a connection to the bunny host.


Stopping

Use ps -ef | grep bunnymech and then use kill.
