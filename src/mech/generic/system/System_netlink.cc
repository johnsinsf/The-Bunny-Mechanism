/*
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.
*/

#include "System_netlink.h"

#define _XOPEN_SOURCE 700

// set this to divide exit code by 256
#define __EXITCODEFIX 1

#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/fsuid.h>
#include <sys/socket.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


int 
System_netlink::main( DssObject& o ) {
  int nl_sock;
  int rc = -1;
  
  if( o.hostname.size() > 0 )
    hostname = o.hostname;
  else
    get_hostname();

  nl_sock = nl_connect();

  if( nl_sock > -1 ) {
    logger.error("System_netlink running on " + hostname);
    rc = set_proc_ev_listen( nl_sock, true );
    logger.error("System_netlink listen " + itoa(rc));
  } else {
    logger.error("System_netlink failed on " + hostname);
  }

  logger.error("System_netlink releasing semaphore");
  // signal we are ready 
  semRelease( o.semInitID );

  if( o.semRunID > 0 )
    semGet( o.semRunID );

  logger.error("System_netlink got semaphore");

  if( rc > -1 ) {
    // loop on events
    logger.error("System_netlink run");
    rc = run( nl_sock, o );

    // release socket listener
    set_proc_ev_listen( nl_sock, false );
  }
  if( nl_sock > -1 )
    close( nl_sock );

  return rc;
}


int 
System_netlink::nl_connect() {
  int rc;
  int nl_sock;
  struct sockaddr_nl sa_nl;

  nl_sock = socket( PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR );

  if( nl_sock == -1 ) {
    logger.error("nl_connect socket " + itoa(errno));
    return -1;
  }
  sa_nl.nl_family = AF_NETLINK;
  sa_nl.nl_groups = CN_IDX_PROC;
  sa_nl.nl_pid    = getpid();

  rc = bind( nl_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl) );

  if( rc == -1 ) {
    logger.error("nl_connect bind " + itoa(errno));
    close( nl_sock );
    return -1;
  }
  return nl_sock;
}

/*
* subscribe on proc events (process notifications)
*/

int 
System_netlink::set_proc_ev_listen(int nl_sock, bool enable) {
    int rc;
    struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
        struct nlmsghdr nl_hdr;
        struct __attribute__ ((__packed__)) {
            struct cn_msg cn_msg;
            //enum proc_cn_mcast_op cn_mcast;
        };
    } nlcn_msg;

    memset(&nlcn_msg, 0, sizeof(nlcn_msg));
    nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
    nlcn_msg.nl_hdr.nlmsg_pid = getpid();
    nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;

    nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
    nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
    nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);
    nlcn_msg.cn_msg.seq = seq++;
    nlcn_msg.cn_msg.ack = 0;

    enum proc_cn_mcast_op cn_mcast;
    cn_mcast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

    rc = ::send(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);
    logger.error("set_proc_ev_listen " + itoa(rc));
    rc += ::send(nl_sock, &cn_mcast, sizeof(cn_mcast), 0);
    logger.error("set_proc_ev_listen2 " + itoa(rc));

    if (rc == -1) {
        perror("netlink send");
        return -1;
    }
    return rc;
}
/*

int 
System_netlink::set_proc_ev_listen(int nl_sock, bool enable) {

  struct cn_msg *m;

  void* m2 = malloc( sizeof(*m) + sizeof(PROC_CN_MCAST_LISTEN) );

  m = (cn_msg*)m2;

  m->id.idx = CN_IDX_PROC;
  m->id.val = CN_VAL_PROC;
  m->seq = seq++;
  m->ack = 0;
  m->len = sizeof(PROC_CN_MCAST_LISTEN);

  m->data[0] = 1;

  int len = netlink_send(nl_sock, m);
  
  return len;
}
*/

int 
System_netlink::run( int nl_sock, DssObject& o ) {
  int rc = -1;
  bool done = false;
  bool sendData = true;
  string msg, procvalmsg;
  map<int, procvaltype>::iterator I;
  char buf[1024];
  struct nlmsghdr *reply = NULL;
  struct cn_msg *mydata = NULL;
  memset(buf, 0, sizeof(buf));

  struct dirent **namelist;
  int n;

  n = scandir("/proc/", &namelist, NULL, alphasort);
  if( n != -1 ) {
    while( n-- ) {
      if( isdigit(*namelist[n]->d_name) ) {
        int i = atoi(namelist[n]->d_name);
        if( i >= 0 && i <= 9999999 ) {
          read_proc_entry(i);
        }
      }
      free(namelist[n]);
    }
    free(namelist);
  }
  while( ! done && ! g_quit ) {
    memset(buf, 0, sizeof(buf));
    logger.error("System_netlink recv");
    rc = recv(nl_sock, buf, sizeof(buf), 0);

    if (rc == 0) {
      logger.error("received shutdown");
      done = true;
    } else if (rc == -1) {
      if (errno == EINTR) continue;
      logger.error("netlink recv " + to_string(errno));
      return -1;
    }

    if( rc > 0 ) {
      reply = (struct nlmsghdr *)buf;

      switch (reply->nlmsg_type) {
        case NLMSG_ERROR:
           logger.error( "Error message received.");
	   break;
        case NLMSG_DONE:
          mydata = (struct cn_msg *)NLMSG_DATA(reply);
          break;
      }

      struct proc_event *proc_ev2;
      proc_ev2 = (proc_event*)mydata->data;

      sendData = true;
      procvalmsg = "";

      switch (proc_ev2->what) {
        case /*proc_ev2->PROC_EVENT_NONE*/ 0:
          if( g_logLevel > 0 )
            logger.error("set mcast listen ok");
          break;

        case /*proc_ev2->PROC_EVENT_FORK */ 1:
          get_proc_vals(proc_ev2->event_data.fork.parent_tgid, proc_ev2->event_data.fork.child_tgid, procvalmsg, sendData);
          msg = "fork: parent tid=" + itoa(proc_ev2->event_data.fork.parent_pid) 
              + " pid=" + itoa(proc_ev2->event_data.fork.parent_tgid) 
              + " -> child tid=" + itoa(proc_ev2->event_data.fork.child_pid) 
              + " pid=" + itoa(proc_ev2->event_data.fork.child_tgid) 
              + procvalmsg;
          break;

        case /*proc_ev2->PROC_EVENT_EXEC*/ 2:
          get_proc_vals(0,proc_ev2->event_data.exec.process_tgid, procvalmsg, sendData);
          msg = "exec: tid=" + itoa(proc_ev2->event_data.exec.process_pid) 
              + " pid=" + itoa(proc_ev2->event_data.exec.process_tgid)
              + procvalmsg;
          break;

        case /*proc_ev2->PROC_EVENT_UID */ 4:
          msg = "uid change: tid=" + itoa(proc_ev2->event_data.id.process_pid) 
              + " pid=" + itoa(proc_ev2->event_data.id.process_tgid)
              + " from " + itoa(proc_ev2->event_data.id.r.ruid) +
              + " to " + itoa(proc_ev2->event_data.id.r.ruid);
          break;

        case /*proc_ev2->PROC_EVENT_GID*/ 0x00000040:
          msg = "gid change: tid=" + itoa(proc_ev2->event_data.id.process_pid) 
              + " pid=" + itoa(proc_ev2->event_data.id.process_tgid)
              + " from " + itoa(proc_ev2->event_data.id.r.rgid) +
              + " to " + itoa(proc_ev2->event_data.id.r.rgid);
          break;

        case /*proc_ev2->PROC_EVENT_EXIT */ 0x80000000:
#ifdef _HAVEPARENTTGID
          get_proc_vals(proc_ev2->event_data.exit.parent_tgid,proc_ev2->event_data.exit.process_tgid, procvalmsg, sendData);
#else
          get_proc_vals(0,proc_ev2->event_data.exit.process_tgid, procvalmsg, sendData);
#endif
          msg = "exit: tid=" + itoa(proc_ev2->event_data.exit.process_pid) 
              + " pid=" + itoa(proc_ev2->event_data.exit.process_tgid)
              + procvalmsg
#ifdef __EXITCODEFIX
              + " exit code=" + itoa(proc_ev2->event_data.exit.exit_code / 256);
#else
              + " exit code=" + itoa(proc_ev2->event_data.exit.exit_code );
#endif
              + " sig=" + itoa(proc_ev2->event_data.exit.exit_signal);
          break;

        default:
          if( g_logLevel > 0 ) {
            msg =  "unhandled proc event " + itoa(proc_ev2->what);
            logger.error( msg );
          }
          break;
      }
      if( sendData && msg.size() ) {
        if( g_logLevel > 0 )
          logger.error( msg );
        send_data( o, msg );
      }
    }
    else
    if( rc == -1 ) {
      if( errno != EINTR ) {
        logger.error("netlink recv error " + itoa(errno));
        done = true;
      }
      else {
        logger.error("netlink interrupted");
        sleep(1);
      }
    }
    else {
      logger.error("netlink recv eof");
      done = true;
    } 
  }
  return 0;
}

// read the /proc/pid entry for cwd and exe values

void
System_netlink::get_proc_vals( int ppid, int pid, string& msg, bool& sendData ) {
  map<int, procvaltype>::iterator I;

  if( ppid == 1 || ppid == 2 ) {
    sendData = false;
    msg = "";
    return;
  }

  if( ppid != 0 && ppid != pid ) {
    I = procvals.find(ppid);
    if( I != procvals.end() ) {
      if( I->second.exe != "/usr/bin/syslog-ng" ) {
        msg = " parent=" + I->second.exe;
      } else {
        sendData = false;
        msg = "";
        return;
      }
    }
  }
  if( pid != 0 ) {
    I = procvals.find(pid);
    if( I != procvals.end() ) {
      if( I->second.exe != "/usr/bin/syslog-ng" ) {
        sendData = true;
        msg += " exe=" + I->second.exe + " cwd=" + I->second.cwd + " user=" + itoa(I->second.uid);
        return;
      } else {
        sendData = false;
        msg = "";
        return;
      }
    }
  }

  procvaltype pvt, pvt2;

  if( ppid != 0 && ppid != pid ) {
    pvt = read_proc_entry( ppid );

    msg = " parent=" + pvt.exe;
    procvals[ppid] = pvt;
  }
  if( pid != 0 ) {
    pvt2 = read_proc_entry( pid );

    if( pvt2.exe != "/usr/bin/syslog-ng" ) {
      sendData = true;
      msg += " exe=" + pvt2.exe + " cwd=" + pvt2.cwd + " user=" + itoa(pvt2.uid);
      procvals[pid] = pvt2;
      return;
    }
  }
  sendData = false;
  msg = "";
  return;
}

System_netlink::procvaltype
System_netlink::read_proc_entry(int pid) {
  string s1 = "/proc/" + itoa(pid) + "/";
  string s2 = s1 + "exe";
  string s3 = s1 + "cwd";
  string s4 = s1 + "status";

  memset(fbuf2, 0, sizeof(fbuf2));
  memset(fbuf3, 0, sizeof(fbuf3));

  int i = 0;
  i = readlink(s2.c_str(), fbuf2, MAXFILENAME-1);
  if( ! i )
    logger.error("error reading " + s2);

  i = readlink(s3.c_str(), fbuf3, MAXFILENAME-1);
  if( ! i )
    logger.error("error reading " + s3);

  procvaltype pvt;
  pvt.exe = string(fbuf2);
  pvt.cwd = string(fbuf3);

  int uid = -1;
  int fd = open(s4.c_str(), O_RDONLY );
  if( fd ) {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    i = read(fd, buf, sizeof(buf));
    if( i > 0 ) {
      string s(buf);
      int j = s.find("Uid:");
      if( j > 0 ) {
        int k = s.find("\n", j);
        if( k > 0 ) {
          uid = atoi(s.substr(j+5,k-j-5).c_str());
        }
      }
      if( pvt.exe.size() == 0 ) {
        j = s.find("Name:");
        if( j >= 0 ) {
          int k = s.find("\n", j);
          if( k > 0 ) {
            pvt.exe = s.substr(j+6,k-j-6);
          }
        }
      }
    }
    close(fd); fd = -1;
  }
  pvt.uid = uid;

  procvals[pid] = pvt;

  return pvt;
}

void
System_netlink::send_data( DssObject& o, string& msg ) {
  dataType d;
  d.structtype = 0;
  d.datatype   = 4;
  d.localid    = o.localID;
  d.channel    = "NETLINK";
  d.synced     = 0;
  d.tries      = 0;
  d.datetime_sent = 0;
  d.hostname   = hostname;
  d.data       = msg;
  d.msgid      = ltoa(msgid++);
  d.msgtime    = ltoa(time(NULL));
 
  if( has_binary_data( msg.c_str(), msg.size() ) )
    logger.error("skipping bad data " + msg);
  else
    o.server->add_client_data( o, d );

  return;
}

int 
System_netlink::netlink_send(int s, struct cn_msg *msg) {

  struct nlmsghdr *nlh;
  unsigned int size;
  int err;
  char buf[128];
  struct cn_msg *m;

  size = NLMSG_SPACE(sizeof(struct cn_msg) + msg->len);

  nlh = (struct nlmsghdr *)buf;
  nlh->nlmsg_seq = seq++;
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_type = NLMSG_DONE;
  nlh->nlmsg_len = size;
  nlh->nlmsg_flags = 0;

  m = (cn_msg*)NLMSG_DATA(nlh);

  memcpy(m, msg, sizeof(*m) + msg->len);

  err = ::send(s, nlh, size, 0);

  if (err == -1)
    logger.error( "Failed to send: " + string(strerror(errno)) + " " + to_string(errno) );

  return err;
}

