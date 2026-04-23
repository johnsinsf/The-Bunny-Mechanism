package com.dwanta.android.dap.client;
    
import android.app.Activity;
import android.app.Notification;
import android.app.AlarmManager;
//import android.app.AlarmManager.AlarmReceiver;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;

//import android.content.res.AssetFileDescriptor;
//import android.content.ContentProvider;
//import android.content.ContentResolver;
import android.os.Binder;
import android.os.IBinder;
import android.os.PowerManager;
import android.util.Log;
import android.widget.Toast;
import android.media.RingtoneManager;
import android.media.Ringtone;

//import android.os.RemoteException;
import android.os.Message;
//import android.os.MessageQueue;
//import android.os.Messenger;
import android.os.Handler;
import android.os.Looper;
//import android.os.Bundle;
import android.os.HandlerThread;
import android.os.Process;
import android.os.SystemClock;
//import android.os.AsyncTask;
import android.net.Uri;


//import android.widget.Toast;
//import com.dwanta.android.dap.client.dpsio;
//import com.dwanta.android.dap.client.dpsmsg;
import com.dwanta.android.dap.authenticator.AuthenticatorActivity;
import com.dwanta.android.dap.activities.DapActivity;
import com.dwanta.android.dap.activities.SiteActivity;
import com.dwanta.android.dap.activities.DioActivity;
import com.dwanta.android.dap.activities.DechoActivity;

import java.io.IOException;
import java.io.FileNotFoundException;
import java.net.InetAddress;
//import java.net.InetSocketAddress;
//import java.net.Socket;
//import java.net.SocketOptions;
//import java.net.SocketImpl;
import java.util.StringTokenizer;
import java.util.HashMap;
//import java.util.Iterator;
//import java.util.Map;
//import java.util.Arrays;

import java.security.KeyStore;

import java.io.File;
import java.io.FileInputStream;
//import java.io.FileOutputStream;

import java.io.InputStream;
//import java.io.OutputStream;
/*
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;

import java.io.BufferedReader;
import java.io.BufferedWriter;
*/
import android.content.Context;
//import android.content.res.AssetManager;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
//import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import android.net.SSLCertificateSocketFactory;

//import javax.net.SocketFactory;
//import javax.net.ssl.X509KeyManager;
import javax.net.ssl.TrustManagerFactory;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
//import android.database.Cursor;

//import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
//import android.telephony.TelephonyManager;
    

import com.dwanta.android.dap.R;

public class ClientService extends Service {
  private NotificationManager mNM;
  private NotificationChannel mChannel;

  //private InetAddress mAddress = null;
  private Context ctx = null;
  private SSLSocket sss = null;
  private SSLSocket sssc = null;
  private SSLCertificateSocketFactory mSF = null;
  private Handler mAuthHandler = null;
  private Handler mDapHandler = null;
  private Handler mSiteHandler = null;
  public  Handler mDioHandler = null;
  public  Handler mDechoHandler = null;
  private Handler tmlHandler = null;
  private volatile Looper mServiceLooper;
  private volatile Looper mServiceLooper2;
  private volatile Looper tmlLooper;
  private volatile ServiceHandler mServiceHandler;
  private volatile ServiceHandler2 mServiceHandler2;
  private boolean quitService = false;
  private boolean signedOn   = false;
  private boolean signedOn2   = false;
  private boolean serviceRunning = false;
  private String mCardDevice = "";
  private String mSiteDevice = "";
  private String mJson  = "";
  private String serverIP    = "";
  private int    portNum = 0;
  private long   controlMsgTime = 0;

  public  String mDevname    = "";
  public  String mUsername   = "";
  public  String mPassword   = "";

  public HashMap<String, String[]> siteProductMap = new HashMap<String, String[]>(0);
  public HashMap<String, String[]> siteProductUnitMap = new HashMap<String, String[]>(0);
  public HashMap<String, String>   cardProductMap = new HashMap<String, String>(0);

  public String siteProductMap_siteid = "";

  public dpsio  chat = null;
  public dpsio  chat2 = null;
  public String mEmboss1 = "";
  public String mCardId  = "";
  public int    mystan   = 1;

  private long testMsgTime = 0;
  private long ackMsgTime = 0;

  //public static final int DPS_AUTH     = 400 + 3;
  //public static final int DPS_PROMPT   = 400 + 4;
  public static final int DPS_PROMPT_RESPONSE = 400 + 5;
  //public static final int DPS_COMPLETION = 400 + 6;
  public static final int DPS_TESTMSG  = 400 + 7;
  //public static final int DPS_READDONE = 400 + 8;
  public static final int DPS_SIGNON   = 400 + 9;
  //public static final int DPS_SIGNONOK = 400 + 10;
  public static final int DPS_READ     = 400 + 11;
  public static final int DPS_START    = 400 + 12;
  public static final int DPS_STOP     = 400 + 13;
  public static final int DPS_RESIGNON = 400 + 14;
  public static final int CARD_MESSAGE = 400 + 15;
  public static final int SITE_MESSAGE = 400 + 16;
  public static final int TRAN_MESSAGE = 400 + 17;
  public static final int WAKEUP       = 400 + 18;
  public static final int PING         = 400 + 19;
  public static final int DPS_SENDACK  = 400 + 20;
  public static final int DIO_MESSAGE  = 400 + 21;
  public static final int DIO_MESSAGE2 = 400 + 22;
  public static final int DECHO_MESSAGE = 400 + 23;
  public static final int DECHO_MESSAGE2 = 400 + 24;

  public static final int MAX_READ_TIME =  660000;
  public static final int TESTMSG_TIME =  1200000; 
  public static final int ACKMSG_TIME =    240000; 
  public static final int MSG_RETRY_TIME =  60000;

  private static final String TAG = "ClientService";
  public final int NOTIFICATION = R.string.dap_service_started;
  public final int TRANNOTIFICATION = 0xFFFFFF;

  private final IBinder mBinder = new LocalBinder();
  private HandlerThread serviceThread = null;
  private HandlerThread serviceThread2 = null;
  private HandlerThread tmlThread = null;
  private DapActivity mDapActivity = null;

  public ClientService getService() {
    return ClientService.this;
 }

  public class LocalBinder extends Binder {
    public ClientService getService() {
      return ClientService.this;
    }
  }

  public interface MessageTarget {
    public Handler getHandler();
  }

  public String returnToken() {
    return chat.returnToken();
  }

  public Handler getHandler() {
    Log.d(TAG, "getHandler " + mServiceHandler);
    return mServiceHandler;
  }

  public boolean getSignedOn() {
    Log.d(TAG, "getSignedOn " + signedOn);
    return signedOn;
  }

  public boolean getSignedOn2() {
    Log.d(TAG, "getSignedOn2 " + signedOn2);
    return signedOn2;
  }

  public String getSiteDevice() {
    Log.d(TAG, "getSiteDev " + mSiteDevice);
    return mSiteDevice;
  }

  public String getCardDevice() {
    Log.d(TAG, "getCardDev " + mCardDevice);
    return mCardDevice;
  }

  public String getJson() {
    Log.d(TAG, "getJson " + mJson);
    return mJson;
  }

  public void setDapActivity(DapActivity a) {
    Log.d(TAG, "setDapActivity");
    mDapActivity = a;
  }

  public void signoff() {
    if( signedOn ) {
      Log.d(TAG, "sending QUIT msg");
      try {
        mServiceHandler.obtainMessage(AuthenticatorActivity.QUIT_SERVICE, this).sendToTarget();
      } catch ( UnsupportedOperationException e ) {
        Log.d(TAG, "unable to send message");
        e.printStackTrace();
        quitService = true;
      }
      try {
        mServiceHandler2.obtainMessage(AuthenticatorActivity.QUIT_SERVICE, this).sendToTarget();
      } catch ( UnsupportedOperationException e ) {
        Log.d(TAG, "unable to send message2");
        e.printStackTrace();
      }
    }
    serviceThread = null;
    serviceThread2 = null;
  }

  public void setSiteHandler( Handler handler ) {
    mSiteHandler = handler;
  }

  public void setDechoHandler( Handler handler ) {
    mDechoHandler = handler;
  }

  public void setDioHandler( Handler handler ) {
    mDioHandler = handler;
  }

  public void setDapHandler( Handler handler ) {
    mDapHandler = handler;
  }

  public void setAuthHandler( Handler handler ) {
    mAuthHandler = handler;
  }

  public void setContext( Context c ) {
    ctx = c;
    Log.d(TAG, "set context " + ctx);
  }

  public final class ServiceHandler extends Handler {
    public ServiceHandler(Looper looper, Context ctx) {
      super(looper);
      Log.d(TAG, "serviceHandler constructor " + ServiceHandler.this);
    }
 
    public void doWake( ) {
      Log.d(TAG, "doWake");
      this.sendEmptyMessage(ClientService.WAKEUP);
    }

    @Override
    public void handleMessage(Message msg) {

      Log.d(TAG, "handle message starting #" + msg.arg1);

      serviceHandleMessage(msg);
 
      super.handleMessage(msg);

      Log.d(TAG, "Done with #" + msg.arg1);

      if( quitService ) {
        Log.d(TAG, "quitService received");
        signedOn = false;
        if( mAuthHandler != null )
          mAuthHandler.obtainMessage(AuthenticatorActivity.QUIT_SERVICE, msg).sendToTarget();
        if( mDapHandler != null )
          mDapHandler.obtainMessage(AuthenticatorActivity.QUIT_SERVICE, msg).sendToTarget();
        if( mServiceHandler2 != null )
          mServiceHandler2.obtainMessage(AuthenticatorActivity.QUIT_SERVICE, msg).sendToTarget();
        Looper l = Looper.myLooper();
        Log.d(TAG, "quitting the serviceHandler now");
        serviceThread = null;
        serviceThread2 = null;
        l.quit();
      }
    }
  };

  public final class ServiceHandler2 extends Handler {
    public ServiceHandler2(Looper looper, Context ctx) {
      super(looper);
      Log.d(TAG, "serviceHandler2 constructor " + ServiceHandler2.this);
    }

    public void doWake( ) {
      Log.d(TAG, "doWake2");
      this.sendEmptyMessage(ClientService.WAKEUP);
    }
 
    @Override
    public void handleMessage(Message msg) {

      Log.d(TAG, "handle message2 starting #" + msg.arg1);

      serviceHandleMessage2(msg);
 
      super.handleMessage(msg);

      Log.d(TAG, "Done2 with #" + msg.arg1);

      //long t = SystemClock.uptimeMillis();
      //if( t >= testMsgTime )
        //mServiceHandler.sendEmptyMessage(ClientService.WAKEUP);

      if( msg.what != ClientService.WAKEUP && msg.what != ClientService.PING )
        mServiceHandler.doWake();

      if( quitService ) {
        Looper l = Looper.myLooper();
        Log.d(TAG, "quitting the serviceHandler2 now");
        serviceThread2 = null;
        l.quit();
      }
    }
  };

  public final class TestMsgLooper extends Handler {
    private Handler myserviceHandler;
    private long sendTime;

    public TestMsgLooper(Looper looper, Handler h) {
      super(looper);
      Log.d(TAG, "testmsglooper constructor");
      myserviceHandler = h;
      sendTime = 0;
     }

    @Override
    public void handleMessage(Message msg) {

      Log.d(TAG, "testmsglooper starting #" + msg.arg1 + " " + sendTime);

      //if( msg.what == ClientService.WAKEUP )
        send();
 
      super.handleMessage(msg);

      Log.d(TAG, "tml with #" + msg.arg1);

      if( ! quitService ) {
        if( msg.what == ClientService.WAKEUP ) {
          Log.d(TAG, "posting TML");
          long t = SystemClock.uptimeMillis();
          tmlHandler.sendEmptyMessageAtTime(ClientService.WAKEUP, t + ACKMSG_TIME);
        }
      }
    }
/*
    void sleepTimer() {
       long start = SystemClock.elapsedRealtime();

       do {
          Log.d(TAG, "TestMsgLooper sleeping " + SystemClock.elapsedRealtime() + " " + start + TESTMSG_TIME);
          try { this.sleep(5000); } catch (InterruptedException e) {}
      } while (SystemClock.elapsedRealtime() < start + TESTMSG_TIME);

      //try { this.sleep(TESTMSG_TIME); } catch (InterruptedException e) {};
    }
*/ 
    public void send() {
      Log.d(TAG, "in tml send");
      try {
        if( signedOn ) {
          long t = SystemClock.uptimeMillis();
          if( t >= sendTime ) {
            myserviceHandler.obtainMessage(ClientService.DPS_TESTMSG, this).sendToTarget();
            sendTime = t + TESTMSG_TIME;
            Log.d(TAG, "posted DPS_TESTMSG");
          } else {
            myserviceHandler.obtainMessage(ClientService.DPS_SENDACK, this).sendToTarget();
            Log.d(TAG, "posted DPS_SENDACK");
          }
        }
        else
        if( ! mServiceHandler.hasMessages(ClientService.DPS_RESIGNON) ) {
          Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
          mServiceHandler.sendMessage( m );
          Log.d(TAG, "posted DPS_RESIGNON");
        }
      } catch ( UnsupportedOperationException e ) {
        Log.d(TAG, "unable to send message");
        e.printStackTrace();
        quitService = true;
      }
    }
/*
    public void run2() {
      while( ! quitService ) {
        Log.d(TAG, "TestMsgLooper timer sleep start");
        sleepTimer();
        Log.d(TAG, "TestMsgLooper timer sleep done");
        if( ! quitService ) {
          try {
            Log.d(TAG, "sending TESTMSG msg " + signedOn + " " + signedOn2);
            if( signedOn )
              myserviceHandler.obtainMessage(ClientService.DPS_TESTMSG, this).sendToTarget();
            else
            if( ! mServiceHandler.hasMessages(ClientService.DPS_RESIGNON) ) {
              Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
              mServiceHandler.sendMessage( m );
              Log.d(TAG, "posted DPS_RESIGNON");
            }
          } catch ( UnsupportedOperationException e ) {
            Log.d(TAG, "unable to send message");
            e.printStackTrace();
            quitService = true;
          } catch ( IllegalStateException e ) {
            Log.d(TAG, "unable to send message");
            e.printStackTrace();
            quitService = true;
          }
        }
      }
      Log.d(TAG, "TestMsgLooper timer quitService");
*/
/*
      try {
        stop();
      } catch ( UnsupportedOperationException e ) {
        e.printStackTrace();
      }
    }
*/
  };

  public void tranComplete(dpsmsg m) {
    Log.d(TAG, "tranComplete " + m.getType() + " " + m.getInt(dpsmsg.actionCode));
    dpsmsg m2 = new dpsmsg();
    m2.setType(m.getType());
    int action = m.getInt(dpsmsg.actionCode);
    byte[] response = m.get(dpsmsg.userData);
    m2.set( dpsmsg.actionCode, action );
    m2.set( dpsmsg.userData, response );
    Log.d(TAG, "have msg " + m2.header + " " + mSiteHandler);

    showNotification(m, TRANNOTIFICATION);

    if( mSiteHandler != null )
      mSiteHandler.obtainMessage(ClientService.TRAN_MESSAGE, m2).sendToTarget();
  }

  public boolean serviceHandleMessage(Message msg) {
    //int state = dpsio.READ_STATE;
    int state = ClientService.DPS_TESTMSG;
    ctx = this;
    Log.d(TAG, "NEW serviceHandler starting msg =" + msg);
/*
    PowerManager.WakeLock wl = null;
  
    if( ctx != null ) {
      Intent intent = new Intent();
      String packageName = ctx.getPackageName();
      PowerManager pm = (PowerManager) ctx.getSystemService(Context.POWER_SERVICE);
      wl = pm.newWakeLock( PowerManager.FULL_WAKE_LOCK, TAG);
      wl.acquire();
      Log.d(TAG, "serviceHandler WAKE_LOCK");
    }
*/
    switch (msg.what) {
      case ClientService.DPS_SIGNON:
          Log.d(TAG, "handleMessage DPS_SIGNON " + msg);
          if( signedOn ) {
            Log.d(TAG, "already signed on");
            mAuthHandler.obtainMessage(AuthenticatorActivity.SIGNON_ALREADY_ON, msg).sendToTarget();
            state = dpsio.READ_STATE;
          } else {
            // mUsername, mPassword, mDevname
            mPassword = ((AuthenticatorActivity.UserLoginTask)(msg.obj)).getPassword();
            mUsername = ((AuthenticatorActivity.UserLoginTask)(msg.obj)).getUsername();
            mDevname = ((AuthenticatorActivity.UserLoginTask)(msg.obj)).getDevname();
            //Log.d(TAG, "got password " + pass + " user " + username + " dev " + devname);
            state = startup();
            Log.d(TAG, "got state " + state);
            if( state == dpsio.SEND_SIGNON_ACK || state == dpsio.SEND_ACK ) {
              signedOn = true;
              Log.d(TAG, "sending auth handler done message ");
              mAuthHandler.obtainMessage(AuthenticatorActivity.SIGNON_DONE, this).sendToTarget();
              mServiceHandler2.obtainMessage(AuthenticatorActivity.SIGNON_DONE, this).sendToTarget();
              state = dpsio.READ_STATE;
              //long t = SystemClock.uptimeMillis();
              long t = SystemClock.elapsedRealtime();
              testMsgTime = t + TESTMSG_TIME;
            } else {
              Log.d(TAG, "sending auth handler fail message " + state);
              if( chat != null && chat.msg != null ) {
                Log.d(TAG, "chat is set");
                if( chat.msg.test(dpsmsg.userData) ) {
                  String t1 = "error: " + new String(chat.msg.get(dpsmsg.userData));
                  int errcode = chat.msg.getInt(dpsmsg.actionCode);
                  Log.d(TAG, "chat is set2 " + t1 + " " + errcode);
/*
                  if( errcode == dpsmsg.certerror || errcode == dpsmsg.signonerror ) {
                    Toast.makeText(this, t1, Toast.LENGTH_LONG).show();
                    Log.d(TAG, "made toast for " + t1);
                  }
*/
                }
                if( mAuthHandler != null )
                  mAuthHandler.obtainMessage(AuthenticatorActivity.SIGNON_FAIL, chat.msg).sendToTarget();
                else
                  signedOn = false;
              }
              else
                signedOn = false;
            }
            if( mDapHandler != null )
              mDapHandler.obtainMessage(AuthenticatorActivity.CHECK_SIGNON, msg).sendToTarget();
          }
          break;

      case ClientService.DPS_RESIGNON:
          Log.d(TAG, "handleMessage DPS_RESIGNON " + msg);
          if( signedOn ) {
            Log.d(TAG, "already signed on");
            state = dpsio.READ_STATE;
          } else {
            state = doSocketReopen(0);
            if( state == dpsio.SEND_ACK ) {
              //chat.setSocket(sss);
              state = chat.writeACK();
              Log.d(TAG, "reauth OK");
              signedOn = true;
              long t = SystemClock.elapsedRealtime();
              testMsgTime = t + TESTMSG_TIME;
            } else {
              Log.d(TAG, "auth handler fail");
              //Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
              //long t2 = SystemClock.elapsedRealtime();
              //mServiceHandler.sendMessageDelayed( m, MSG_RETRY_TIME);
              //Log.d(TAG, "posted DPS_RESIGNON");
            }
          }
          if( mDapHandler != null )
            mDapHandler.obtainMessage(AuthenticatorActivity.CHECK_SIGNON, msg).sendToTarget();
          if( ! signedOn2 ) {
            Message m = mServiceHandler2.obtainMessage(ClientService.DPS_RESIGNON, this);
            mServiceHandler2.sendMessage( m );
          }
          break;

      case ClientService.DPS_TESTMSG:
          Log.d(TAG, "handleMessage TESTMSG " + msg);
          if( signedOn ) {
            if( chat != null ) {
              state = chat.doTestmsg(0);
              if( state == dpsio.SEND_ACK ) {
                state = chat.writeACK();
              }
              long t = SystemClock.elapsedRealtime();
              testMsgTime = t + TESTMSG_TIME;
              ackMsgTime = t + ACKMSG_TIME;
            }
          } else {
            state = ClientService.DPS_TESTMSG;
          }
          break;

      case ClientService.DPS_PROMPT_RESPONSE:
          Log.d(TAG, "handleMessage prompt response " + msg);
          if( chat != null ) {
            dpsmsg m = (dpsmsg)(msg.obj);
            Log.d(TAG, "handleMessage prompt response " + m.header + " " + m.fields);
            state = chat.doSend(m);
            hideNotification(m);
            if( state == dpsio.SEND_ACK ) {
              state = chat.writeACK();
            }
          }
          else
          if( chat == null ) {
            dpsmsg m = (dpsmsg)(msg.obj);
            if( m.getInt( dpsmsg.actionCode ) == 999 )
              hideNotification(m);
            Log.d(TAG, "handleMessage prompt response null handler " + m.header + " " + m.fields);
          }
          break;

      case ClientService.CARD_MESSAGE:
          Log.d(TAG, "handleMessage card message " + msg);
          if( chat != null ) {
            mCardId = (String)(msg.obj);
            state = chat.doCardRequest(mCardId);
            Log.d(TAG, "doCardRequest state " + state);
            if( state == dpsio.SEND_ACK ) {
              state = chat.writeACK();
              String data1 = new String(chat.msg.get(dpsmsg.passedData1));
              String data2 = new String(chat.msg.get(dpsmsg.passedData2));
              String data4 = new String(chat.msg.get(dpsmsg.passedData4));
              Log.d(TAG, "have emboss1 " + data1 + " 2 " + data2 + " 4 " + data4);
              if( chat.msg.test(dpsmsg.passedData4) )
                parseCardProducts(data4);
              if( chat.msg.test(dpsmsg.passedData1) )
                mEmboss1 = data1;
              if( data2.length() > 0 )
                mCardId = data2;
            }
          }
          if( mSiteHandler != null )
            mSiteHandler.obtainMessage(ClientService.CARD_MESSAGE, msg).sendToTarget();
          break;

      case ClientService.TRAN_MESSAGE:
          Log.d(TAG, "handleMessage tran message " + msg);
          if( chat != null ) {
            SiteActivity a = (SiteActivity)msg.obj;
            Log.d(TAG, "emboss1 " + mEmboss1);
            state = chat.doTranRequest(a);
            Log.d(TAG, "doTranRequest done " + mEmboss1);
            if( state == dpsio.SEND_ACK ) {
              state = chat.writeACK();
            }
          }
          break;

      case ClientService.DIO_MESSAGE:
          Log.d(TAG, "handleMessage dio message " + msg);
          DioActivity a = (DioActivity)(msg.obj);
          if( chat != null ) {
            Log.d(TAG, "company is " + a.companyid );
            //if( s != siteProductMap_siteid ) {
              state = chat.doDioRequest(a);
              if( state == dpsio.SEND_ACK ) {
                state = chat.writeACK();
                if( chat.msg.test(dpsmsg.passedData1) ) {
                  String data = new String(chat.msg.get(dpsmsg.passedData1));
                  Log.d(TAG, "have data for company " + a.companyid + " " + data);
                  //parseSiteProducts(data);
                  //data = new String(chat.msg.get(dpsmsg.passedData3));
                  //Log.d(TAG, "have unit data for site " + s.companyid + " " + data);
                  //siteProductMap_siteid = s;
                }
              }
            //}
          }
          if( mDioHandler != null )
            mDioHandler.obtainMessage(ClientService.DIO_MESSAGE, a).sendToTarget();
          break;

      case ClientService.DECHO_MESSAGE:
          Log.d(TAG, "handleMessage decho message " + msg);
          DechoActivity b = (DechoActivity)(msg.obj);
          if( chat != null ) {
            Log.d(TAG, "company is " + b.companyid );
            state = chat.doDechoRequest(b);
            if( state == dpsio.SEND_ACK ) {
              state = chat.writeACK();
              if( chat.msg.test(dpsmsg.passedData1) ) {
                String data = new String(chat.msg.get(dpsmsg.passedData1));
                Log.d(TAG, "have data for company " + b.companyid + " " + data);
              }
            }
          }
          if( mDechoHandler != null )
            mDechoHandler.obtainMessage(ClientService.DECHO_MESSAGE, b).sendToTarget();
          break;

      case ClientService.SITE_MESSAGE:
          Log.d(TAG, "handleMessage site message " + msg);
          if( chat != null ) {
            String s = (String)(msg.obj);
            Log.d(TAG, "site is " + s);
            if( s != siteProductMap_siteid ) {
              state = chat.doSiteRequest(s);
              if( state == dpsio.SEND_ACK ) {
                state = chat.writeACK();
                if( chat.msg.test(dpsmsg.passedData4) ) {
                  String data = new String(chat.msg.get(dpsmsg.passedData4));
                  Log.d(TAG, "have prod data for site " + s + " " + data);
                  parseSiteProducts(data);
                  data = new String(chat.msg.get(dpsmsg.passedData3));
                  Log.d(TAG, "have unit data for site " + s + " " + data);
                  parseSiteProductUnits(data);
                  siteProductMap_siteid = s;
                }
              }
            }
          }
          if( mSiteHandler != null )
            mSiteHandler.obtainMessage(ClientService.SITE_MESSAGE, msg).sendToTarget();
          break;

      case ClientService.DPS_SENDACK:
          Log.d(TAG, "handleMessage SENDACK" + msg);
          if( chat != null ) {
            state = chat.writeACK();
          }
          break;

      case ClientService.DPS_READ:
          Log.d(TAG, "handleMessage READ " + msg);
          if( chat != null ) {
            state = chat.doRead(1000);
            if( state == dpsio.SEND_ACK ) {
              state = chat.writeACK();
            }
          }
          break;

      case AuthenticatorActivity.QUIT_SERVICE:
          Log.d(TAG, "handleMessage QUIT " + msg);
          state = dpsio.END_STATE;
          if( signedOn ) 
            chat.writeEOT();
          signedOn = false;
          try {
            if( sss != null )
              sss.close();
          } catch ( IOException e ) {
            Log.d(TAG, "unable to close socket");
            e.printStackTrace();
          }
          mAuthHandler.obtainMessage(AuthenticatorActivity.INVALIDATE_AUTHTOKEN, msg).sendToTarget();
          break;

      default:
          Log.d(TAG, "handleMessage DEFAULT " + msg);
          break;
    }

    long t = SystemClock.elapsedRealtime();
    //boolean con = isConnectedWifi(ctx); 
    Log.d(TAG, "STATE IS " + state + " signedOn is " + signedOn + " " + t + " " + testMsgTime );

    if( ! signedOn ||  state == dpsio.END_STATE || state == dpsio.IOERROR_STATE ) {
      Log.d(TAG, "state was END_STATE/IOERROR_STATE " + signedOn + " " + state);
      if( state == dpsio.IOERROR_STATE || ! signedOn || msg.what == ClientService.DPS_RESIGNON ) {
        Log.d(TAG, "closing on error, retry in 60 seconds");
        signedOn = false;
        if( mDapHandler != null )
          mDapHandler.obtainMessage(AuthenticatorActivity.CHECK_SIGNON, msg).sendToTarget();
        if( msg.what == ClientService.WAKEUP ) {
          Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
          mServiceHandler.sendMessage( m );
        }
        else 
        if( ! mServiceHandler.hasMessages(ClientService.DPS_RESIGNON) ) {
          Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
          long t2 = SystemClock.uptimeMillis() + MSG_RETRY_TIME;
          mServiceHandler.sendMessageAtTime( m, t2 );
          Log.d(TAG, "posted DPS_RESIGNON");
        }
        return true;
      } else {
        quitService = true;
        mServiceLooper.quit();
        mServiceLooper2.quit();
      }
    }
    else
    if( signedOn && t >= testMsgTime ) {
       Log.d(TAG, "t > testMsgTime");
       if( chat != null ) {
         Log.d(TAG, "calling doTestMsg");
         int tstate = chat.doTestmsg(0);
         if( tstate == dpsio.SEND_ACK ) {
           Log.d(TAG, "writing ack");
           chat.writeACK();
         }
         else
         if( tstate == dpsio.IOERROR_STATE ) {
           Log.d(TAG, "IOERROR");
           state = dpsio.IOERROR_STATE;
         }
       }
       testMsgTime = t + TESTMSG_TIME;
    }
    else
    if( signedOn && t >= ackMsgTime ) {
       Log.d(TAG, "t > ackMsgTime");
       if( chat != null ) {
         state = chat.writeACK();
       }
       ackMsgTime = t + ACKMSG_TIME;
    }
    //if( ! signedOn2 ) {
      //mServiceHandler2.obtainMessage(ClientService.DPS_RESIGNON, this).sendToTarget();
    //}
/*
    if( chat != null && state == dpsio.READ_STATE ) {
      if( ! mServiceHandler.hasMessages(ClientService.DPS_READ) ) {
        Log.d(TAG, "posting READ message");
        mServiceHandler.obtainMessage(ClientService.DPS_READ, this).sendToTarget();
      }
    }
*/


    if( state == dpsio.IOERROR_STATE ) {
      Log.d(TAG, "state was IOERROR_STATE " + signedOn + " " + state);
      signedOn = false;
      if( ! mServiceHandler.hasMessages( ClientService.DPS_RESIGNON ) ) {
        Log.d(TAG, "closing on error, retry in 60 seconds");
        Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
        //long t2 = SystemClock.elapsedRealtime();
        //mServiceHandler.sendMessageDelayed( m, t2 + MSG_RETRY_TIME);
        long t2 = SystemClock.uptimeMillis() + MSG_RETRY_TIME;
        mServiceHandler.sendMessageAtTime( m, t2 );
        Log.d(TAG, "posted DPS_RESIGNON");
      }
    }


    if( state == dpsio.SEND_STATE ) {

      Log.d(TAG, "send message prompt " + chat.msg);
      // need to add progress, text details to message
      showNotification(chat.msg, NOTIFICATION);
/*
      if( ! mServiceHandler.hasMessages(ClientService.DPS_READ) ) {
        Log.d(TAG, "posting READ message");
        mServiceHandler.obtainMessage(ClientService.DPS_READ, this).sendToTarget();
      }
*/
    }
    Log.d(TAG, "service message handler done " + state + " quit " + quitService);

    return true;
  }

  public boolean serviceHandleMessage2(Message msg) {
    int state = dpsio.READ_STATE;
    ctx = this;
    Log.d(TAG, "serviceHandler2 starting ctx =" + ctx + " what " + msg.what);

    switch (msg.what) {

      case ClientService.DPS_RESIGNON:
          Log.d(TAG, "handleMessage2 DPS_RESIGNON " + msg);
          if( signedOn2 ) {
            Log.d(TAG, "already signed on");
            state = dpsio.READ_STATE;
          } else {
            controlMsgTime = SystemClock.elapsedRealtime();
            state = doSocketReopen(1);
            if( state == dpsio.SEND_ACK || state == dpsio.READ_STATE ) {
              //chat2.setSocket(sssc);
              if( state == dpsio.SEND_ACK )
                state = chat2.writeACK();
              Log.d(TAG, "reauth2 OK");
              signedOn2 = true;
              state = dpsio.READ_STATE;
            } else {
              Log.d(TAG, "auth handler2 fail");
              if( ! mServiceHandler2.hasMessages(ClientService.DPS_RESIGNON) ) {
                Message m = mServiceHandler2.obtainMessage(ClientService.DPS_RESIGNON, this);
                long t = SystemClock.uptimeMillis() + MSG_RETRY_TIME;
                mServiceHandler2.sendMessageAtTime( m, t);
                Log.d(TAG, "posted2 DPS_RESIGNON");
              }
              //long t2 = SystemClock.elapsedRealtime();
              //mServiceHandler2.sendMessageDelayed( m, t2 + MSG_RETRY_TIME);
            }
          }
          break;

      case ClientService.DPS_TESTMSG:
          Log.d(TAG, "handleMessage2 TESTMSG " + msg + " signedOn " + signedOn);
          controlMsgTime = SystemClock.elapsedRealtime();
          if( chat2 != null && signedOn ) {
            state = chat2.processHostMsg();
            if( state == dpsio.SEND_ACK ) {
              state = chat2.writeACK();
            }
          }
          else
            state = ClientService.DPS_READ;
          break;

      case ClientService.DPS_PROMPT_RESPONSE:
          Log.d(TAG, "handleMessage2 prompt response " + msg);
          if( chat2 != null ) {
            dpsmsg m = (dpsmsg)(msg.obj);
            Log.d(TAG, "handleMessage prompt response " + m.header + " " + m.fields);
            state = chat2.doSend(m);
            hideNotification(m);
            if( state == dpsio.SEND_ACK ) {
              state = chat2.writeACK();
              controlMsgTime = SystemClock.elapsedRealtime();
            }
          }
          else
          if( chat2 == null ) {
            dpsmsg m = (dpsmsg)(msg.obj);
            if( m.getInt( dpsmsg.actionCode ) == 999 )
              hideNotification(m);
            Log.d(TAG, "handleMessage2 prompt response null handler " + m.header + " " + m.fields);
          }
          break;

      case ClientService.CARD_MESSAGE:
          Log.d(TAG, "handleMessage2 card message " + msg);
          controlMsgTime = SystemClock.elapsedRealtime();
          if( chat2 != null ) {
            mCardId = (String)(msg.obj);
            state = chat2.doCardRequest(mCardId);
            Log.d(TAG, "doCardRequest state " + state);
            if( state == dpsio.SEND_ACK ) {
              state = chat2.writeACK();
              String data1 = new String(chat2.msg.get(dpsmsg.passedData1));
              String data2 = new String(chat2.msg.get(dpsmsg.passedData2));
              String data4 = new String(chat2.msg.get(dpsmsg.passedData4));
              Log.d(TAG, "have emboss1 " + data1 + " 2 " + data2 + " 4 " + data4);
              if( chat2.msg.test(dpsmsg.passedData4) )
                parseCardProducts(data4);
              if( chat2.msg.test(dpsmsg.passedData1) )
                mEmboss1 = data1;
              if( data2.length() > 0 )
                mCardId = data2;
            }
          }
          if( mSiteHandler != null )
            mSiteHandler.obtainMessage(ClientService.CARD_MESSAGE, msg).sendToTarget();
          break;

      case ClientService.TRAN_MESSAGE:
          Log.d(TAG, "handleMessage2 tran message " + msg);
          controlMsgTime = SystemClock.elapsedRealtime();
          if( chat2 != null ) {
            SiteActivity a = (SiteActivity)msg.obj;
            Log.d(TAG, "emboss1 " + mEmboss1);
            state = chat2.doTranRequest(a);
            Log.d(TAG, "doTranRequest2 done " + mEmboss1);
            if( state == dpsio.SEND_ACK ) {
              state = chat2.writeACK();
            }
          }
          break;

      case ClientService.SITE_MESSAGE:
          Log.d(TAG, "handleMessage2 site message " + msg);
          controlMsgTime = SystemClock.elapsedRealtime();
          if( chat2 != null ) {
            String s = (String)(msg.obj);
            Log.d(TAG, "site is " + s);
            if( s != siteProductMap_siteid ) {
              state = chat2.doSiteRequest(s);
              if( state == dpsio.SEND_ACK ) {
                state = chat2.writeACK();
                if( chat2.msg.test(dpsmsg.passedData4) ) {
                  String data = new String(chat.msg.get(dpsmsg.passedData4));
                  Log.d(TAG, "have prod data for site " + s + " " + data);
                  parseSiteProducts(data);
                  data = new String(chat2.msg.get(dpsmsg.passedData3));
                  Log.d(TAG, "have unit data for site " + s + " " + data);
                  parseSiteProductUnits(data);
                  siteProductMap_siteid = s;
                }
              }
            }
          }
          if( mSiteHandler != null )
            mSiteHandler.obtainMessage(ClientService.SITE_MESSAGE, msg).sendToTarget();
          break;

      case ClientService.DPS_READ:
          Log.d(TAG, "handleMessage2 READ " + msg);
          if( chat2 != null && signedOn2 ) {
            state = chat2.doRead(MAX_READ_TIME);
            Log.d(TAG, "handleMessage2 doRead " + state);
            if( state == dpsio.SEND_ACK ) {
              state = chat2.writeACK();
            }
            if( state != dpsio.IOERROR_STATE ) {
              controlMsgTime = SystemClock.elapsedRealtime();
            }
          }
          break;

      case AuthenticatorActivity.SIGNON_DONE:
          Log.d(TAG, "handleMessage2 SIGNON DONE " + msg);
          state = dpsio.READ_STATE;
          signedOn = true;
          break;

      case AuthenticatorActivity.QUIT_SERVICE:
          Log.d(TAG, "handleMessage2 QUIT " + msg);
          state = dpsio.END_STATE;
          chat2.writeEOT();
          try {
            if( sssc != null )
              sssc.close();
          } catch ( IOException e ) {
            Log.d(TAG, "unable to close socket");
            e.printStackTrace();
          }
          break;

      default:
          Log.d(TAG, "handleMessage2 DEFAULT " + msg);
          break;
    }

    long t = SystemClock.elapsedRealtime();
    boolean con = isConnectedWifi(ctx); 
    Log.d(TAG, "STATE2 IS " + state + " signedOn is " + signedOn2 + " " + t + " " + testMsgTime );

    if( ! signedOn2 || state == dpsio.IOERROR_STATE ) {
      Log.d(TAG, "state2 was IOERROR_STATE " + signedOn2 + " " + state);
      if( ! signedOn2 || state == dpsio.IOERROR_STATE ) {
        Log.d(TAG, "closing on error, retry in 60 seconds");
        signedOn2 = false;
        if( ! mServiceHandler2.hasMessages(ClientService.DPS_RESIGNON) ) {
          Message m = mServiceHandler2.obtainMessage(ClientService.DPS_RESIGNON, this);
          long t2 = SystemClock.uptimeMillis() + MSG_RETRY_TIME;
          mServiceHandler2.sendMessageAtTime( m, t2 );
          Log.d(TAG, "posted2 DPS_RESIGNON");
        }
        //long t2 = SystemClock.elapsedRealtime();
        //mServiceHandler2.sendMessageDelayed( m, t2 + MSG_RETRY_TIME);

        return true;
      }
    }
/*
    if( state == dpsio.END_STATE ) {
       quitService = true;
       mServiceLooper2.quit();
    }
*/


    if( signedOn2 && chat2 != null && state == dpsio.READ_STATE ) {
      if( ! mServiceHandler2.hasMessages(ClientService.DPS_READ) ) {
        Log.d(TAG, "posting READ message2");
        mServiceHandler2.obtainMessage(ClientService.DPS_READ, this).sendToTarget();
      }
    }

    if( chat2 != null &&  state == dpsio.SEND_STATE ) {
      Log.d(TAG, "service message2 writing SEND_STATE");

      chat2.doWrite();

      if( ! mServiceHandler2.hasMessages(ClientService.DPS_READ) ) {
        Log.d(TAG, "posting READ message2");
        mServiceHandler2.obtainMessage(ClientService.DPS_READ, this).sendToTarget();
      }
    }
    Log.d(TAG, "service message2 handler done " + state);
    return true;
  }

  @Override
  public void onCreate() {
    Log.d(TAG, "onCreate");
    mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

    String id = "dps_channel_01";

    // The user-visible name of the channel.
    CharSequence name = getString(R.string.channel_name);

    // The user-visible description of the channel.
    String description = getString(R.string.channel_description);

    int importance = NotificationManager.IMPORTANCE_LOW;

    mChannel = new NotificationChannel(id, name,importance);

    // Configure the notification channel.
    mChannel.setDescription(description);

    mChannel.enableLights(true);
    // Sets the notification light color for notifications posted to this
    // channel, if the device supports this feature.
    //mChannel.setLightColor(Color.RED);

    mChannel.enableVibration(true);
    mChannel.setVibrationPattern(new long[]{100, 200, 300, 400, 500, 400, 300, 200, 400});

    mNM.createNotificationChannel(mChannel);

    showMainNotification("dap started");
    quitService = false;

    if( serviceThread == null )
      createServiceThread();
    if( serviceThread2 == null )
      createServiceThread2();

    long win = 10 * 60 * 1000;
    ctx = this;

    AlarmManager.OnAlarmListener listener 
      = new AlarmManager.OnAlarmListener () {

      @Override
      public void onAlarm() {
        Log.d(TAG, "alarmed!");
        if( ! signedOn ) {
          if( ! mServiceHandler.hasMessages(ClientService.DPS_RESIGNON) ) {
            Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
            mServiceHandler.sendMessage( m );
            Log.d(TAG, "posted DPS_RESIGNON");
          }
        }
        if( ! signedOn2 ) {
          if( ! mServiceHandler2.hasMessages(ClientService.DPS_RESIGNON) ) {
            Message m = mServiceHandler2.obtainMessage(ClientService.DPS_RESIGNON, this);
            mServiceHandler2.sendMessage( m );
            Log.d(TAG, "posted2 DPS_RESIGNON");
          }
        }
      }
    };
    AlarmManager am = (AlarmManager) this.getSystemService(Context.ALARM_SERVICE);
    //am.setWindow( AlarmManager.RTC, SystemClock.elapsedRealtime(), win, "DapAlarm", listener, null );

    PendingIntent alarmIntent;
    Intent intent = new Intent("DAPWAKE");
    intent.setAction("DAPWAKE");
    alarmIntent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_IMMUTABLE );

    am.setInexactRepeating(AlarmManager.RTC_WAKEUP, SystemClock.elapsedRealtime(),
        1000 * 60 * 10, alarmIntent);

    registerBroadcastReceiver();

    return;
  }

  private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
      String action = intent.getAction();
      String wake = "DAPWAKE";
      Log.d(TAG, "in broadcast onReceive " + action);

      if( ! serviceRunning )
        return;

      if (action.equals(wake)) {
        Log.d(TAG, "received the wake broadcast");
        if( mServiceHandler != null ) {
          mServiceHandler.doWake();
          if( ! signedOn ) {
            //if( ! mServiceHandler.hasMessages(ClientService.DPS_RESIGNON) ) {
              Message m = mServiceHandler.obtainMessage(ClientService.DPS_RESIGNON, this);
              mServiceHandler.sendMessage( m );
              Log.d(TAG, "posted DPS_RESIGNON");
            //}
          }
        }
        if( mServiceHandler2 != null ) {
          mServiceHandler2.doWake();
          long t = SystemClock.elapsedRealtime();
          if( serviceRunning && (controlMsgTime > 0) && ((controlMsgTime + MAX_READ_TIME) < t) ) {
            Log.d(TAG, "control message not received in 10 minutes! " + controlMsgTime + " " + MAX_READ_TIME + t);
            signedOn2 = false;
          }
          if( ! signedOn2 ) {
            //if( ! mServiceHandler2.hasMessages(ClientService.DPS_RESIGNON) ) {
              Message m = mServiceHandler2.obtainMessage(ClientService.DPS_RESIGNON, this);
              mServiceHandler2.sendMessage( m );
              Log.d(TAG, "posted2 DPS_RESIGNON");
            //}
          }
        } 
      } 
    }
  };

  private void registerBroadcastReceiver() {
    // Create a filter with the broadcast intents we are interested in.
    IntentFilter filter = new IntentFilter();
    //filter.addAction(AccountManager.LOGIN_ACCOUNTS_CHANGED_ACTION);
    filter.addAction("DAPWAKE");
    registerReceiver(mBroadcastReceiver, filter, null, null);
    //registerReceiver(mBroadcastReceiver, null, null, null);
  }

  private void unregisterBroadcastReceiver() {
    try {
      unregisterReceiver(mBroadcastReceiver);
    } catch (IllegalArgumentException e) {
      Log.d(TAG, "bad getResult ");
      e.printStackTrace();
    }
  }

  public void onReceive(Context context, Intent intent) {
    StringBuilder sb = new StringBuilder();
    sb.append("Action: " + intent.getAction() + "\n");
    sb.append("URI: " + intent.toUri(Intent.URI_INTENT_SCHEME).toString() + "\n");
    String log = sb.toString();
    Log.d(TAG, log);
    Toast.makeText(context, log, Toast.LENGTH_LONG).show();
  }

  private void createServiceThread() {
    Log.d(TAG, "createServiceThread");
    //testMsgTime  = SystemClock.elapsedRealtime() + TESTMSG_TIME;
    // testMsgTime  = SystemClock.elapsedRealtime() + 10;
    serviceThread = new HandlerThread("ClientService",
              //Process.THREAD_PRIORITY_BACKGROUND);
              Process.THREAD_PRIORITY_FOREGROUND);
    serviceThread.start();

    mServiceLooper = serviceThread.getLooper();

    mServiceHandler = new ServiceHandler(mServiceLooper, ctx);
 
    //Thread t = new TestMsgLooper(mServiceHandler);
    //t.start();

    //Handler t = new TestMsgLooper(mServiceHandler);

    long t = SystemClock.uptimeMillis();
    testMsgTime  = t + TESTMSG_TIME;
    ackMsgTime  = t + ACKMSG_TIME;

    tmlThread = new HandlerThread("TestMsgLooper", Process.THREAD_PRIORITY_FOREGROUND);
    tmlThread.start();

    tmlLooper = tmlThread.getLooper();

    tmlHandler = new TestMsgLooper( tmlLooper, mServiceHandler );

    //tmlHandler.sendEmptyMessage(ClientService.WAKEUP);
    tmlHandler.sendEmptyMessageAtTime(ClientService.WAKEUP, ackMsgTime );
  }

  private void createServiceThread2() {
    Log.d(TAG, "createServiceThread2");
    //long testMsgTime  = SystemClock.elapsedRealtime() + 10000;
    //testMsgTime  = SystemClock.uptimeMillis() + ACKMSG_TIME;
    serviceThread2 = new HandlerThread("ClientService",
              //Process.THREAD_PRIORITY_BACKGROUND);
              Process.THREAD_PRIORITY_FOREGROUND);
    serviceThread2.start();

    mServiceLooper2 = serviceThread2.getLooper();

    mServiceHandler2 = new ServiceHandler2(mServiceLooper2, ctx);

  }

  public int startup() {
    Log.d(TAG, "opening socket");
    boolean init_complete = false;
    //mUsername = username;
    //mPassword = password;
    //mDevname = devname;
    try {
      FileInputStream fis = openFileInput(dpsio.CERTFILENAME);
      Log.d(TAG, "opened " + dpsio.CERTFILENAME);
      init_complete = true;
      try {
        fis.close();
      } catch ( IOException e ) {
       Log.d(TAG, "mycert error close");
      }
    } catch ( FileNotFoundException e ) {
      Log.d(TAG, "mycert not found, will initialize");
      init_complete = false;
    }
    int option = 0;
    if( ! init_complete ) {
      option = 1;
    }

    boolean socketrc = openSocket(option);

    if( ! socketrc ) 
      return -1;

    Log.d(TAG, "Launching the I/O handler " + option);

    mSiteDevice = "";
    mCardDevice = "";
    mJson = "";

    if( option == 0 ) {
      chat = new dpsio(sss, mServiceHandler, mAuthHandler, ctx, this);
      chat.setDapActivity(mDapActivity);
      int rc = chat.doSignon(option, mUsername, mPassword, mDevname);

      if( rc == dpsio.SEND_ACK && chat.msg.test( dpsmsg.passedData ) ) {
        mCardDevice = new String(chat.msg.get( dpsmsg.passedData ));
        Log.d(TAG, "have card device " + mCardDevice);
      }
      if( rc == dpsio.SEND_ACK && chat.msg.test( dpsmsg.passedData1 ) ) {
        mSiteDevice = new String(chat.msg.get( dpsmsg.passedData1 ));
        Log.d(TAG, "have site device " + mSiteDevice);
      }
      if( rc == dpsio.SEND_ACK && chat.msg.test( dpsmsg.passedData4 ) ) {
        mJson = new String(chat.msg.get( dpsmsg.passedData4 ));
        Log.d(TAG, "have json " + mJson);
      }
      if( rc == dpsio.END_STATE && chat.msg.test( dpsmsg.passedData ) ) {
        Toast.makeText(this, new String(chat.msg.get(dpsmsg.passedData)), Toast.LENGTH_LONG).show();
        return rc;
      }
      //if( rc == dpsio.CERTERROR_STATE ) {
        //if( chat.msg.test( dpsmsg.userData ) ) {
          //Toast.makeText(this, new String(chat.msg.get(dpsmsg.userData)), Toast.LENGTH_LONG).show();
          //return rc;
        //}
      //}

      if( rc == dpsio.SEND_ACK || rc == dpsio.SEND_SIGNON_ACK ) {
        signedOn = true;
        chat2 = new dpsio(sssc, mServiceHandler2, mAuthHandler, ctx, this);
        chat2.setDapActivity(mDapActivity);
        int rc2 = chat2.doSignon2( mUsername, mPassword, mDevname);
        Log.d(TAG, "control signon result " + rc2);
        if( rc2 == dpsio.READ_STATE ) {
          signedOn2 = true;
          mServiceHandler2.obtainMessage(ClientService.DPS_READ, this).sendToTarget();
          serviceRunning = true;
        } else {
          rc = dpsio.IOERROR_STATE;
        }
        return rc;
      }

      Log.d(TAG, "need to reinitialize the certificate");

      File f  = new File(dpsio.CERTPASSNAME);
      File f2 = new File(dpsio.CERTFILENAME);
      boolean rc2 = f.delete();
      if( ! rc2 ) 
        Log.d(TAG, "unable to delete passfile");
      rc2 = f2.delete();
      if( ! rc2 ) 
        Log.d(TAG, "unable to delete certfile");
      try {
        sss.close();
      } catch ( IOException e ) {
        Log.d(TAG, "error closing socket ");
        e.printStackTrace();
      }
      option = 1;
      socketrc = openSocket(option);
      if( ! socketrc )
        return -1;
    }
    chat = new dpsio(sss, mServiceHandler, mAuthHandler, ctx, this);
    chat.setDapActivity(mDapActivity);
    int rc = chat.doSignon(option, mUsername, mPassword, mDevname);

    if( rc == dpsio.END_STATE && chat.msg != null && chat.msg.test( dpsmsg.passedData ) ) {
      Toast.makeText(this, new String(chat.msg.get(dpsmsg.passedData)), Toast.LENGTH_LONG).show();
    }

    if( rc == dpsio.SEND_SIGNON_ACK ) {
      Log.d(TAG, "got signon OK");
      chat.doSignonACK();     
      chat.writeEOT();     
      try {
        sss.close();
      } catch ( IOException e ) {
        Log.d(TAG, "error closing socket ");
        e.printStackTrace();
      }
      Log.d(TAG, "reopening socket");

      socketrc = openSocket(0);
      if( ! socketrc ) 
        return -1;

      chat = new dpsio(sss, mServiceHandler, mAuthHandler, ctx, this);
      chat.setDapActivity(mDapActivity);
      rc = chat.doSignon(0, mUsername, mPassword, mDevname);
      Log.d(TAG, "signon result " + rc);

      if( rc == dpsio.SEND_ACK && chat.msg.test( dpsmsg.passedData ) ) {
        mCardDevice = new String(chat.msg.get( dpsmsg.passedData ));
        Log.d(TAG, "have card device " + mCardDevice);
      }
      if( rc == dpsio.SEND_ACK && chat.msg.test( dpsmsg.passedData1 ) ) {
        mSiteDevice = new String(chat.msg.get( dpsmsg.passedData1 ));
        Log.d(TAG, "have site device " + mSiteDevice);
      }
      if( rc == dpsio.SEND_ACK ) {
        chat2 = new dpsio(sssc, mServiceHandler2, mAuthHandler, ctx, this);
        chat2.setDapActivity(mDapActivity);
        int rc2 = chat2.doSignon2( mUsername, mPassword, mDevname);
        Log.d(TAG, "control signon result " + rc2);
        if( rc2 == dpsio.READ_STATE )
          mServiceHandler2.obtainMessage(ClientService.DPS_READ, this).sendToTarget();
        else
          rc = dpsio.IOERROR_STATE;
      }
    }
    // open control socket
    return rc;
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    Log.d(TAG, "Received start id " + startId + ": " + intent );

    quitService = false;

    if( serviceThread == null )
      createServiceThread();

    if( serviceThread2 == null )
      createServiceThread2();

    return START_STICKY;
  }

  @Override
  public void onDestroy() {
    // Cancel the persistent notification.
    Log.d(TAG, "onDestroy");

    //mNM.cancel(NOTIFICATION);
    hideMainNotification();

    // Tell the user we stopped.
    Toast.makeText(this, R.string.dap_service_stopped, Toast.LENGTH_SHORT).show();

    unregisterBroadcastReceiver();
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "on Bind");
    return mBinder;
  }

  private void ringBell() {
    try {
      Uri notification = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
      Ringtone r = RingtoneManager.getRingtone(getApplicationContext(), notification);
      r.play();
    } catch (Exception e) {
      Log.d(TAG, "no sound");
    }
  }

  public void showNotification(dpsmsg msg, int notifyid) {

    ringBell();
      
    Intent intent = new Intent(this, ClientServiceActivities.Controller.class);

    intent.putExtra("stan",   msg.getInt(dpsmsg.stan));
    intent.putExtra("date",   msg.getInt(dpsmsg.date));
    intent.putExtra("time",   msg.getInt(dpsmsg.time));
    intent.putExtra("siteid", msg.getInt(dpsmsg.siteid));

    if( msg.test(dpsmsg.userData) )
      intent.putExtra("userData",   new String(msg.get(dpsmsg.userData)));
    if( msg.test(dpsmsg.actionCode) )
      intent.putExtra("actionCode", msg.getInt(dpsmsg.actionCode));
    if( msg.test(dpsmsg.additionalData) )
      intent.putExtra("additionalData", new String(msg.get(dpsmsg.additionalData)));
    if( msg.test(dpsmsg.passedData) )
      intent.putExtra("passedData", new String(msg.get(dpsmsg.passedData)));

    intent.addFlags(
      Intent.FLAG_ACTIVITY_NEW_TASK | 
      Intent.FLAG_ACTIVITY_CLEAR_TASK | 
      Intent.FLAG_ACTIVITY_SINGLE_TOP 
    );

    PendingIntent contentIntent = PendingIntent.getActivity(
      this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
            
    Log.d(TAG, "message header siteid " + msg.getInt(dpsmsg.siteid) + " " + msg.header + " fields " + msg.fields );

    CharSequence text = "Pin Request";
    boolean onGoing = false;
    if( notifyid == NOTIFICATION ) {
      if( msg.getInt(dpsmsg.actionCode) == dpsmsg.confirmRequired )
        text = "Dps Confirm Request"; 
      else
      if( msg.getInt(dpsmsg.actionCode) == dpsmsg.dataSend )
        text = "Command Send"; 
      else
      if( msg.getInt(dpsmsg.actionCode) != dpsmsg.pinRequired )
        text = "Dps Prompt Request"; 
    } else {
      if( msg.getInt(dpsmsg.actionCode) == 0 )
        text = "Dps Tran complete"; 
      else {
        text = "Dps Tran failed"; 
        if( msg.test(dpsmsg.userData) )
          text = text +  ": " + new String(msg.get(dpsmsg.userData));
      }
      contentIntent = null;
    }
    Uri soundUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);

    String t = "Store " + msg.getInt(dpsmsg.siteid);
    if( notifyid == NOTIFICATION ) {
      if( msg.test(dpsmsg.userData) )
        if( msg.getInt(dpsmsg.actionCode) != dpsmsg.dataSend)
          t += " card " + new String(msg.get(dpsmsg.userData));
        else
          t = "Command " + new String(msg.get(dpsmsg.userData));
    } else {
      if( msg.test(dpsmsg.amount) ) {
        try {
          float m = new Float(new String(msg.get(dpsmsg.amount))).floatValue();
          t += " amt $" + m;
        } catch (Exception e) {
          Log.d(TAG, "bad amount " + new String(msg.get(dpsmsg.amount)));
          t += " amt $" + new String(msg.get(dpsmsg.amount));
        }
      }
      if( msg.test(dpsmsg.passedData) )
        t += " card " + new String(msg.get(dpsmsg.passedData));
    }

    Log.d(TAG, "text " + text + " t " + t);

    String channelid = "dps_channel_01";

    Notification notification = new Notification.Builder(this).
            setContentTitle(text).
            setContentText(t).
            setSmallIcon(R.drawable.icon).
            setTicker("Dps " + text).
            setOngoing(onGoing).
            setSound(soundUri).
            setChannelId(channelid).
            setContentIntent(contentIntent).
            getNotification();

    int id = notifyid;
    if( msg.getInt(dpsmsg.stan) > 0 && id == NOTIFICATION )
      id = msg.getInt(dpsmsg.stan);

    if( msg.getInt(dpsmsg.actionCode) == dpsmsg.dataSend)
      id = 1;

    mNM.notify(id, notification);
    Log.d(TAG, "show Notification " + id + " stan " + msg.getInt(dpsmsg.stan));
  }

  private void hideNotification(dpsmsg msg) {
    int id = NOTIFICATION;
    if( msg.getInt(dpsmsg.stan) > 0 )
      id = msg.getInt(dpsmsg.stan);

    Log.d(TAG, "hide Notification " + id  + " stan " + msg.getInt(dpsmsg.stan));
    mNM.cancel(id);
  }

  private void showMainNotification(String text) {
      
    Intent intent = new Intent(this, DapActivity.class);

    intent.addFlags(
      Intent.FLAG_ACTIVITY_NEW_TASK | 
      Intent.FLAG_ACTIVITY_CLEAR_TASK | 
      Intent.FLAG_ACTIVITY_SINGLE_TOP 
    );

    PendingIntent contentIntent = PendingIntent.getActivity(
      this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE );
            
    text = "DAP Service";
    String channelid = "dps_channel_01";

    Notification notification = new Notification.Builder(this).
            setContentTitle(text).
            setContentText("Touch to manage DAP service.").
            setSmallIcon(R.drawable.icon).
            setTicker("DAP started").
            setOngoing(true).
            setChannelId(channelid).
            setContentIntent(contentIntent).
            getNotification();

    int id = NOTIFICATION;
    mNM.notify(id, notification);
    Log.d(TAG, "show main Notification " + id);
  }

  private void hideMainNotification() {
    int id = NOTIFICATION;

    Log.d(TAG, "hide Notification " + id);
    mNM.cancel(id);
  }

  public boolean openSocket(int option) {
    boolean rc = doOpenSocket( option, 0 );
    if( ! rc ) {
      rc = doOpenSocket( option, 1 );
    }
    return rc;
  }

  private boolean doOpenSocket(int option, int thistry) {
    SSLSocketFactory sssf;

      Intent intent = new Intent();
      String packageName = ctx.getPackageName();
      PowerManager pm = (PowerManager) ctx.getSystemService(Context.POWER_SERVICE);
      PowerManager.WakeLock wl = null;
      wl = pm.newWakeLock( PowerManager.FULL_WAKE_LOCK, "ClientService:FullWakeTag");
      wl.acquire();
      Log.d(TAG, "doOpenSocket WAKE_LOCK");


    try {
      if( sss != null )
        sss.close( );
    } catch (IOException e) {
      Log.d(TAG, "unable to close socket");
    }
    try {
      if( sssc != null )
        sssc.close( );
    } catch (IOException e) {
      Log.d(TAG, "unable to close control socket");
    }

    portNum = 22913; 
    if( option == 0 )
      portNum = 22914;

    serverIP = "173.8.141.1";
    //serverIP = "64.62.178.197";

    if( ! isConnected(ctx) ) {
      Log.d(TAG, "not connected, no service");
      return false;
    }

    boolean connectedWifi = isConnectedWifi(ctx);
    boolean connectedMobile = false;

    if( ! connectedWifi ) 
      connectedMobile = isConnectedMobile(ctx);

    //serverIP = "64.151.110.133"; 
    //if( thistry > 0 )
      //serverIP = "64.151.110.139";

    //if( connectedWifi )
      //serverIP = "10.0.2.27";

    Log.d(TAG, "running socket bind " + serverIP + " " + portNum);

    try {
      CertificateFactory cf = CertificateFactory.getInstance("X.509");

      Log.d(TAG, "loading keychain " + option);

      String tmfAlgorithm = TrustManagerFactory.getDefaultAlgorithm();
      TrustManagerFactory tmf = TrustManagerFactory.getInstance(tmfAlgorithm);
/*
      try {
          InputStream cis = ctx.getAssets().open("load-dps.crt");
          Certificate ca;
          // Create a KeyStore containing our trusted CAs
          String keyStoreType = KeyStore.getDefaultType();
          KeyStore keyStore = KeyStore.getInstance(keyStoreType);
          keyStore.load(null, null);
          // Create a TrustManager that trusts the CAs in our KeyStore
          tmf.init(keyStore);

          try {
              ca = cf.generateCertificate(cis);
              keyStore.setCertificateEntry("ca", ca);
              Log.d(TAG, "ca=" + ((X509Certificate) ca).getSubjectDN());
          } finally {
              cis.close();
          }
      } catch ( FileNotFoundException e ) {
          Log.d(TAG, "mycert not found");
      }
*/

      SSLContext sslContext = SSLContext.getInstance("TLS");
      KeyManagerFactory kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());

      // Load the PKCS12 key chain
      KeyStore ks = KeyStore.getInstance("PKCS12");

      boolean certfound = false;
      if( option == 0 ) {
        FileInputStream fis2, fip;

        try {
          fis2 = openFileInput(dpsio.CERTFILENAME);
          fip = openFileInput(dpsio.CERTPASSNAME);

          byte[] pass = new byte[64];

          int len = fip.read(pass, 0, 64);
          String p = new String(pass, 0, len);
          Log.d(TAG, "GOT PASS " + p + " len " + len);

          ks.load(fis2, p.toCharArray());
          kmf.init(ks, p.toCharArray());

          String keyStoreType = KeyStore.getDefaultType();
          ks.load(null, null);
          // Create a TrustManager that trusts the CAs in our KeyStore
          tmf.init(ks);

          certfound = true;
          Log.d(TAG, "certfound = true");
          fis2.close();
          fip.close();
        } catch ( FileNotFoundException e ) {
          Log.d(TAG, "mycert not found");
        }
      }
      // Initialize the SSL context
      if( certfound )
        sslContext.init(kmf.getKeyManagers(), tmf.getTrustManagers(), null);

      SSLCertificateSocketFactory sf = 
        (SSLCertificateSocketFactory)SSLCertificateSocketFactory.getInsecure(10000, null);

      Log.d(TAG, "after sf");

      //mSF = (SSLCertificateSocketFactory)SSLCertificateSocketFactory.getInsecure(10000, null);

      if( certfound )
        sf.setKeyManagers(kmf.getKeyManagers());

      if( certfound )
        sf.setTrustManagers(tmf.getTrustManagers());

      sss = (SSLSocket)sf.createSocket( serverIP, portNum );
      if( option == 0 )
        sssc = (SSLSocket)sf.createSocket( serverIP, portNum );

      //HostnameVerifier hv = HttpsURLConnection.getDefaultHostnameVerifier();

      sss.setSoTimeout(0);
      sss.setKeepAlive(true);
 
      if( option == 0 ) {
        sssc.setSoTimeout(0);
        sssc.setKeepAlive(true);
      }

      return true;

    } catch (Exception e) {
        e.printStackTrace();
        return false;
    }
  }

  private int doSocketReopen( int option ) {
    boolean rc = false;

    if( serverIP == "" || portNum == 0 ) {
      Log.d(TAG, "bad server/port");
      SystemClock.sleep(5000);
      return startup();
      //return dpsio.END_STATE;
    }

    try {
      if( option == 0 )  {
        if( sss != null )
          sss.close();
      } else {
        if( sssc != null )
          sssc.close();
      }
    } catch (IOException e) {
      Log.d(TAG, "bad close");
    }

    try {
      CertificateFactory cf = CertificateFactory.getInstance("X.509");

      String tmfAlgorithm = TrustManagerFactory.getDefaultAlgorithm();
      TrustManagerFactory tmf = TrustManagerFactory.getInstance(tmfAlgorithm);
      SSLContext sslContext = SSLContext.getInstance("TLS");
      KeyManagerFactory kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());

      // Load the PKCS12 key chain
      KeyStore ks = KeyStore.getInstance("PKCS12");

      boolean certfound = false;
      {
        FileInputStream fis2, fip;

        try {
          fis2 = openFileInput(dpsio.CERTFILENAME);
          fip = openFileInput(dpsio.CERTPASSNAME);

          byte[] pass = new byte[64];

          int len = fip.read(pass, 0, 64);
          String p = new String(pass, 0, len);
          Log.d(TAG, "GOT PASS " + p + " len " + len);

          ks.load(fis2, p.toCharArray());
          kmf.init(ks, p.toCharArray());

          String keyStoreType = KeyStore.getDefaultType();
          ks.load(null, null);
          // Create a TrustManager that trusts the CAs in our KeyStore
          tmf.init(ks);

          certfound = true;
          Log.d(TAG, "certfound = true");
          fis2.close();
          fip.close();
        } catch ( FileNotFoundException e ) {
          Log.d(TAG, "mycert not found");
        }
      }
      // Initialize the SSL context
      if( certfound )
        sslContext.init(kmf.getKeyManagers(), tmf.getTrustManagers(), null);

      SSLCertificateSocketFactory sf = 
        (SSLCertificateSocketFactory)SSLCertificateSocketFactory.getInsecure(10000, null);
      Log.d(TAG, "after sf");

      //sf = (SSLCertificateSocketFactory)SSLCertificateSocketFactory.getInsecure(10000, null);

      if( certfound )
        sf.setKeyManagers(kmf.getKeyManagers());

      if( certfound )
        sf.setTrustManagers(tmf.getTrustManagers());
      if( option == 0 ) {
        //sss.close();
        sss = (SSLSocket)sf.createSocket( serverIP, portNum );
        sss.setSoTimeout(0);
        sss.setKeepAlive(true);
        rc = true;
      } else {
        //sssc.close();
        sssc = (SSLSocket)sf.createSocket( serverIP, portNum );
        sssc.setSoTimeout(0);
        sssc.setKeepAlive(true);
        rc = true;
      }
    } catch (Exception e) {
        e.printStackTrace();
        rc = false;
    }
    if( rc ) {
      int state = 0;
      if( option == 0 && sss != null ) {
        chat.setSocket(sss);
        state = chat.doSignon(option, mUsername, mPassword, mDevname);
      } 
      else 
      if( sssc != null ) {
        chat2.setSocket(sssc);
        state = chat2.doSignon2(mUsername, mPassword, mDevname);
      }
      if( state != 0 ) 
        return state;
    }
    return dpsio.END_STATE;
  }

  /**
   * Get the network info
   * @param context
   * @return
   */
  public static NetworkInfo getNetworkInfo(Context context){
    ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    return cm.getActiveNetworkInfo();
  }
  
  /**
   * Check if there is any connectivity
   * @param context
   * @return
   */
  public static boolean isConnected(Context context){
    NetworkInfo info = ClientService.getNetworkInfo(context);
    return (info != null && info.isConnected());
  }
  
  /**
   * Check if there is any connectivity to a Wifi network
   * @param context
   * @return
   */
  public static boolean isConnectedWifi(Context context){
    NetworkInfo info = ClientService.getNetworkInfo(context);
    return (info != null && info.isConnected() && info.getType() == ConnectivityManager.TYPE_WIFI);
  }
  
  /**
   * Check if there is any connectivity to a mobile network
   * @param context
   * @return
   */
  public static boolean isConnectedMobile(Context context){
    NetworkInfo info = ClientService.getNetworkInfo(context);
    return (info != null && info.isConnected() && info.getType() == ConnectivityManager.TYPE_MOBILE);
  }

  public int parseSiteProducts(String siteProds) {
    // 011,REG UNL,Gas,3.299,0,1,test
    int numProds = 0;
    StringTokenizer tokens = new StringTokenizer(siteProds, ":");
    int numTokens = tokens.countTokens();
    int x = 0;
    numProds = (numTokens - 1) / 6;

    Log.d(TAG, "parseSiteProducts have " + numTokens + " tokens, = " + numProds);

    String productid;

    siteProductMap = new HashMap<String, String[]>(numProds);

    if( numTokens > 0 ) {
      boolean done = false;
      while( ! done ) {
        productid  = tokens.nextToken();
        String t[] = new String[6];
        t[0] = tokens.nextToken();
        t[1] = tokens.nextToken();
        t[2] = tokens.nextToken();
        t[3] = tokens.nextToken();
        t[4] = tokens.nextToken();
        t[5] = tokens.nextToken();

        siteProductMap.put(productid, t);

        Log.d(TAG, "have product " + productid + " sub " + t[5]);

        x++;
        if( ! tokens.hasMoreTokens() )
          done = true;
      }
    }
    numProds = x;
    return x;
  }

  public int parseCardProducts(String cardProds) {
    // 0,099,
    int numProds = 0;
    StringTokenizer tokens = new StringTokenizer(cardProds, ",");
    int numTokens = tokens.countTokens();
    int x = 0;
    numProds = (numTokens + 1) / 2;
    Log.d(TAG, "have " + numTokens + " tokens, = " + numProds + " " + cardProds);
    String cardProduct;
    String prodLimit;

    cardProductMap = new HashMap<String, String>(numProds);

    if( numTokens > 0 ) {
      boolean done = false;
      while( ! done ) {
        try {
          cardProduct = tokens.nextToken();
          prodLimit   = tokens.nextToken();
          Log.d(TAG, "have product " + cardProduct + " " + prodLimit);
          cardProductMap.put(cardProduct, prodLimit);
          x++;
        } catch (Exception e) {
          Log.d(TAG, "bad read in parseCardProducts " + cardProds);
          done = true;
        }
        if( ! tokens.hasMoreTokens() )
          done = true;
      }
    }
    numProds = x;
    return x;
  }

  public int parseSiteProductUnits(String siteProdUnits) {
    // 0,Gallons,3
    int numProds = 0;
    StringTokenizer tokens = new StringTokenizer(siteProdUnits, ":");
    int numTokens = tokens.countTokens();
    int x = 0;
    numProds = (numTokens + 1) / 3;
    Log.d(TAG, "have " + numTokens + " tokens, = " + numProds);

    siteProductUnitMap = new HashMap<String, String[]>(numProds);

    if( numTokens > 0 ) {
      boolean done = false;
      while( ! done ) {
        String t[] = new String[2];
        String unitid = tokens.nextToken();
        t[0] = tokens.nextToken();
        t[1] = tokens.nextToken();

        siteProductUnitMap.put(unitid, t);

        Log.d(TAG, "have product unit " + unitid);
        x++;
        if( ! tokens.hasMoreTokens() )
          done = true;
      }
    }
    return x;
  }
  
  public void doWakeup( ) {
    Log.d(TAG, "Wakeup");
    if( mServiceHandler != null )
      mServiceHandler.sendEmptyMessage(ClientService.PING);
    if( mServiceHandler2 != null )
      mServiceHandler2.sendEmptyMessage(ClientService.PING);
    //if( tmlHandler != null )
      //tmlHandler.sendEmptyMessage(ClientService.PING);
  }
}
