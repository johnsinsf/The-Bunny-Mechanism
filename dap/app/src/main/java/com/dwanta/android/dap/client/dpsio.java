
package com.dwanta.android.dap.client;

import android.app.Activity;
import android.os.Handler;
import android.util.Log;
import android.os.Message;
import android.os.SystemClock;
import android.os.Build;

import android.content.Intent;
import android.provider.Settings.Secure;
import android.telephony.TelephonyManager;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.StringTokenizer;

import android.content.res.AssetFileDescriptor;
import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;

import android.net.Uri;

import android.content.Context;
import android.content.res.AssetManager;

import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;

import javax.net.ssl.SSLSocket;
import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;

import com.dwanta.android.dap.authenticator.AuthenticatorActivity;
import com.dwanta.android.dap.client.ClientService.MessageTarget;
import com.dwanta.android.dap.client.ClientService;
import com.dwanta.android.dap.client.dpscamera;
import com.dwanta.android.dap.activities.SiteActivity;
import com.dwanta.android.dap.activities.DapActivity;
import com.dwanta.android.dap.activities.DioActivity;
import com.dwanta.android.dap.activities.DechoActivity;
//import com.dwanta.android.dap.activities.CameraActivity;

/**
 * Handles reading and writing of messages with ssl sockets 
 */
public class dpsio {

  private SSLSocket socket = null;
  private Handler handler;
  private Handler mAuthHandler;
  private ClientService mService;
  private DapActivity mDapActivity = null;

  private static final String TAG = "dpsio";
  public  static final String CERTFILENAME = "mycertm.p12";
  public  static final String CERTPASSNAME = "mycertm.pass";

  public  static final int SIGNON_STATE = 1;
  public  static final int END_STATE    = 2;
  public  static final int READ_STATE   = 3;
  public  static final int SEND_ACK     = 4;
  public  static final int SEND_STATE   = 5;
  public  static final int DPS_SIGNONOK = 6;
  public  static final int SEND_SIGNON_ACK= 7;
  public  static final int IOERROR_STATE = 8;
  public  static final int CERTERROR_STATE = 9;

  private static final int EOT          = 4;
  private static final int ENQ          = 5;
  private static final int ACK          = 6;
  private static final int NACK         = 21;
  private static final int READTIMEOUT  = 600500;

  private int state = SIGNON_STATE;
  public dpsmsg msg;
  private Context ctx = null;
  private String mAuthToken = "";
  private String mDevname   = "";
  private String mUsername  = "";
  private String mPassword  = "";

  public dpsio(SSLSocket socket, Handler handler, Handler handler2, Context ctx, ClientService service) {
    this.socket = socket;
    this.handler = handler;
    this.mAuthHandler = handler2;
    this.ctx = ctx;
    this.mService = service;
    try {
        msg = new dpsmsg(this.socket);
      } catch (Exception e) {
        Log.d(TAG, "bad dpsmsg constructor ");
        e.printStackTrace();
    }
  }

  public void setDapActivity(DapActivity a) {
    mDapActivity = a;
  }
  public void setSocket(SSLSocket newsocket) {
    socket = newsocket;
    msg.setSocket(newsocket);
  }
 
  public int doSignon(int option, String username, String password, String devname) {
    mDevname = devname;
    mUsername = username;
    mPassword = password;

    Log.d(TAG, "doSignon " + option);

    if( msg == null )
      return END_STATE;

    msg.clear();
    msg.setType(dpsmsg.signonRequest);

    msg.set(dpsmsg.authData, username + "/" + password);
    msg.set(dpsmsg.additionalData, devname);

    //msg.set(dpsmsg.siteid, devname);
    //msg.set(dpsmsg.userData, username);
    //msg.set(dpsmsg.additionalData, password);

    msg.set(dpsmsg.mystan, mService.mystan++);

    int state = SEND_SIGNON_ACK;
    if( option == 0 )
      state = SEND_ACK;

    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      //Log.d(TAG, "signon state read " + rc);

      if( rc != 1 ) {
        if( rc == 2 ) {
          //if( msg.getPasschar() == NACK )
            //state = SIGNON_STATE;
          //else
            state = IOERROR_STATE;
        }
      }
      else
      if( msg.getType() != dpsmsg.signonResponse ) {
        Log.d(TAG, "bad response type " + msg.getType());
        return IOERROR_STATE;
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad signon write");
      e.printStackTrace();
    }
    msg.setState(state);
    if( msg.getType() != dpsmsg.signonResponse ) {
      Log.d(TAG, "wrong return type " + msg.getType());
      return IOERROR_STATE;
    }
    int actionCode = msg.getInt(dpsmsg.actionCode);
    if( actionCode == dpsmsg.signonerror ) {
      Log.d(TAG, "signon error");
      return IOERROR_STATE;
    }
    mAuthToken = new String(msg.get(dpsmsg.additionalData));
    //Log.d(TAG, "got authToken " + mAuthToken); 

    if( option == 0 ) {
      Log.d(TAG, "actioncode " + actionCode);
      if( actionCode == dpsmsg.certreinit ) {
        Log.d(TAG, "need to reinitialize");
        return CERTERROR_STATE;
      }
    }
    else
    if( option == 1 ) {
      Log.d(TAG, "initialize actioncode " + actionCode);
      if( actionCode == dpsmsg.certerror) {
        Log.d(TAG, "need to reinitialize, cert error");
        //return -3;
        return CERTERROR_STATE;
      } 
      else if( actionCode == dpsmsg.certinit ) {
        int len = msg.getInt(dpsmsg.recordNumber);
        byte[] pass = msg.get(dpsmsg.userData);
        //Log.d(TAG, "initialize certificate " + len + " " + new String(pass) + " len " + pass.length);
        boolean rc = false;
        try {
          rc = msg.readExtendedData(len);
        } catch (Exception e) {
          Log.d(TAG, "readExtended failed " + rc);
          e.printStackTrace();
        }
        //Log.d(TAG, "readExtended " + rc);
        if( rc ) {
          try {
            FileOutputStream fos = ctx.openFileOutput(CERTFILENAME, Context.MODE_PRIVATE);
            try {
              byte[] b = msg.getOutbuf();
              //Log.d(TAG, "writing outbuf size " + b.length + " " + b[0] + " " + b[1] + " " + b[2] + " " + b[3]);
              fos.write(b, 0, b.length);
              fos.close();
            } catch( IOException e ) {
              Log.d(TAG, "error writing file");
              e.printStackTrace();
              rc = false;
            }
          } catch ( FileNotFoundException e ) {
            Log.d(TAG, "mycert not found for writing");
            rc = false;
          }
          if( rc ) {
            try {
              FileOutputStream fos = ctx.openFileOutput(CERTPASSNAME, Context.MODE_PRIVATE);
              try {
                fos.write(pass, 0, pass.length);
                fos.close();
              } catch( IOException e ) {
                Log.d(TAG, "error writing pass file");
                e.printStackTrace();
                rc = false;
              }
            } catch ( FileNotFoundException e ) {
              Log.d(TAG, "mycert pass file not found for writing");
              rc = false;
            }
          }
        }
        if( ! rc )
          state = IOERROR_STATE;
        else {
/*
          ContentValues contentValues = new ContentValues();
          contentValues.put("certpass", msg.get(dpsmsg.userData));
          Log.d(TAG, "ContentValues " + contentValues);
          Uri mBaseUri = Uri.parse("content://com.dwanta.android.dap.client/main");
          ContentResolver mContentResolver = ctx.getContentResolver();
          mContentResolver.insert(mBaseUri, contentValues);
*/
        }
      }
    }
    Log.d(TAG, "signon returning state " + state);
    return state;
  }
 
  public int doSignon2(String username, String password, String devname) {
    mDevname = devname;
    mUsername = username;
    mPassword = password;

    Log.d(TAG, "doSignon2 ");

    if( msg == null )
      return END_STATE;

    msg.clear();
    msg.setType(dpsmsg.signonRequest);

    msg.set(dpsmsg.authData, username + "/" + password);
    msg.set(dpsmsg.additionalData, devname);

    //msg.set(dpsmsg.siteid, devname);
    //msg.set(dpsmsg.userData, username);
    //msg.set(dpsmsg.additionalData, password);

    msg.set(dpsmsg.passedOpt, 1);

    msg.set(dpsmsg.mystan, mService.mystan++);

    state = READ_STATE;

    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      //Log.d(TAG, "signon state read " + rc);

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = IOERROR_STATE;
          else
            state = READ_STATE;
        }
      }
      else
      if( msg.getType() != dpsmsg.signonResponse ) {
        Log.d(TAG, "bad response type " + msg.getType());
        return IOERROR_STATE;
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad signon write");
      e.printStackTrace();
    }
    msg.setState(state);
    if( msg.getType() != dpsmsg.signonResponse ) {
      Log.d(TAG, "wrong return type " + msg.getType());
      return IOERROR_STATE;
    }
    int actionCode = msg.getInt(dpsmsg.actionCode);
    if( actionCode == dpsmsg.signonerror ) {
      Log.d(TAG, "signon error");
      return IOERROR_STATE;
    }
    mAuthToken = new String(msg.get(dpsmsg.additionalData));
    Log.d(TAG, "signon2 got authToken " + mAuthToken); 

    Log.d(TAG, "actioncode " + actionCode);
    if( actionCode == dpsmsg.certreinit ) {
      Log.d(TAG, "need to reinitialize");
      return CERTERROR_STATE;
    }
    Log.d(TAG, "signon2 returning state " + state);

    return state;
  }
 
  public int doTestmsg(int option) {
    //Log.d(TAG, "doTestmsg");
    msg.clear();
    msg.setType(dpsmsg.testRequest);
    msg.set(dpsmsg.siteid, mDevname);
    msg.set(dpsmsg.authData, mAuthToken);
    int mystan = mService.mystan++;
    msg.set(dpsmsg.mystan, mystan);
    //Log.d(TAG, "mystan is " + mystan);
    int state = SEND_ACK;
    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      //Log.d(TAG, "testmsg read " + rc);

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = SIGNON_STATE;
          else
          if( msg.getPasschar() == ACK || msg.getPasschar() == ENQ )
            state = READ_STATE;
          else
            state = IOERROR_STATE;
        }
      } 
      else
      if( msg.getType() != dpsmsg.testResponse  || ( msg.getInt(dpsmsg.mystan) != mystan )) {
        Log.d(TAG, "bad response type " + msg.getType());
        return IOERROR_STATE;
      }
      if( rc == 1 ) {
        int actionCode = msg.getInt(dpsmsg.actionCode);
        if( actionCode == dpsmsg.tokenreinit ) {
          Log.d(TAG, "need new auth token");
          if( option == 0 )
            state = doSignon(0, mUsername, mPassword, mDevname);
          else
            state = doSignon2(mUsername, mPassword, mDevname);
        }
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad testmsg write");
      e.printStackTrace();
    }
    if( state != IOERROR_STATE && msg.getType() != dpsmsg.testResponse )
      state = processHostMsg();

    return state;
  }
 
  public int doSignonACK() {
    Log.d(TAG, "doSignonACK");
    msg.clear();
    msg.setType(dpsmsg.signonAck);
    msg.set(dpsmsg.authData, mAuthToken);
    String devid = getDeviceID();
    Log.d(TAG, "device ID " + devid);
    msg.set(dpsmsg.additionalData, devid);
    msg.set(dpsmsg.mystan, mService.mystan++);
    int state = SEND_ACK;
    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      //Log.d(TAG, "signonack read " + rc);

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = SIGNON_STATE;
          else
            state = IOERROR_STATE;
        }
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad signonack write");
      e.printStackTrace();
    }
    return state;
  }

  public int doWrite() {
    Log.d(TAG, "doWrite");
    state = READ_STATE;
    try {
      msg.write();
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad dowrite");
      e.printStackTrace();
    }
    return state;
  }

  public int doCardRequest(String carddpsid) {
    Log.d(TAG, "doCardRequest " + carddpsid);
    msg.clear();
    msg.setType(dpsmsg.cardRequest);
    msg.set(dpsmsg.authData, mAuthToken);
    msg.set(dpsmsg.additionalData, carddpsid);
    //msg.set(dpsmsg.additionalData, "6282445241710");
    mService.mCardId = carddpsid;
    msg.set(dpsmsg.actionCode, dpsmsg.getProducts);
    int mystan = mService.mystan++;
    //Log.d(TAG, "mystan is " + mystan);
    msg.set(dpsmsg.mystan, mystan);
    int state = SEND_ACK;
    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      Log.d(TAG, "cardrequest read " + rc + " type " + msg.getType());

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = SIGNON_STATE;
          else
            state = IOERROR_STATE;
        }
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad cardrequest write");
      e.printStackTrace();
    }
    //if( state != IOERROR_STATE && ( msg.getType() != dpsmsg.cardResponse || msg.getInt(dpsmsg.mystan) != mystan) )
    if( state != IOERROR_STATE && ( msg.getType() != dpsmsg.cardResponse ) )
      state = processHostMsg();

    return state;
  }

  public int doTranRequest(SiteActivity a) {
    Log.d(TAG, "doTranRequest");
    msg.clear();
    msg.setType(dpsmsg.tranRequest);
    msg.set(dpsmsg.authData, mAuthToken);
    //msg.set(dpsmsg.additionalData, carddpsid);
    //msg.set(dpsmsg.additionalData, "6282445241710");
    msg.set(dpsmsg.actionCode, dpsmsg.dapTran);
    msg.set(dpsmsg.siteid, a.mSiteNumber);
    msg.set(dpsmsg.additionalData, a.mCardNumber);
    int mystan = mService.mystan++;
    //Log.d(TAG, "mystan is " + mystan);
    msg.set(dpsmsg.mystan, mystan);

    int count = a.selectedProducts.size();
    String prods = "0";
    for( int i = 0; i < count; i++ ) {
      if( prods.length() > 0 ) prods += ",";
      prods += a.selectedProducts.get(i)[0] + ",";
      prods += a.selectedProducts.get(i)[1] + ",";
      prods += a.selectedProducts.get(i)[2];
      Log.d(TAG, "have prods " + prods);
    }
    msg.set(dpsmsg.userData, prods);
    int state = SEND_ACK;
    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      Log.d(TAG, "tranrequest read " + rc + " state " + state);

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = SIGNON_STATE;
          else
            state = IOERROR_STATE;
        }
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad tranrequest write");
      e.printStackTrace();
    }
/*
    if( state != IOERROR_STATE && 
      ( msg.getType() != dpsmsg.tranResponse || msg.getInt(dpsmsg.mystan) != mystan) ) {
      Log.d(TAG, "tranrequest processHostMsg");
      state = processHostMsg();
    }
*/
    state = processHostMsg();

    Log.d(TAG, "msg state " + state);
  
    return state;
  }

  public int doSiteRequest(String siteid) {
    //Log.d(TAG, "doSiteRequest");
    msg.clear();
    msg.setType(dpsmsg.siteRequest);
    msg.set(dpsmsg.authData, mAuthToken);
    //msg.set(dpsmsg.additionalData, siteid);
    msg.set(dpsmsg.siteid, siteid);
    msg.set(dpsmsg.actionCode, dpsmsg.getProducts);
    int mystan = mService.mystan++;
    //Log.d(TAG, "mystan is " + mystan);
    msg.set(dpsmsg.mystan, mystan);
    int state = SEND_ACK;
    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      //Log.d(TAG, "siterequest read " + rc);

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = SIGNON_STATE;
          else
            state = IOERROR_STATE;
        }
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad siterequest write");
      e.printStackTrace();
    }
    //if( state != IOERROR_STATE && ( msg.getType() != dpsmsg.siteResponse || msg.getInt(dpsmsg.mystan) != mystan) )
    if( state != IOERROR_STATE && ( msg.getType() != dpsmsg.siteResponse ) )
      state = processHostMsg();

    return state;
  }

  public int doDioRequest(DioActivity a) {
    Log.d(TAG, "doDioRequest company " + a.companyid);
    msg.clear();
    msg.setType(dpsmsg.dioRequest);
    msg.set(dpsmsg.authData, mAuthToken);
    msg.set(dpsmsg.actionCode, dpsmsg.getData);
    int mystan = mService.mystan++;
    //Log.d(TAG, "mystan is " + mystan);
    msg.set(dpsmsg.mystan, mystan);
    if( a.requestType == 0 ) {
      msg.set(dpsmsg.additionalData, a.companyid);
      msg.set(dpsmsg.passedData1, "get interfaces");
    }
    else
    if( a.requestType == 1 ) {
      msg.set(dpsmsg.additionalData, a.companyid + "," + a.interfaceid);
      msg.set(dpsmsg.passedData1, "get interface");
    }
    else
    if( a.requestType == 2 ) {
      msg.set(dpsmsg.additionalData, a.companyid + "," + a.interfaceid + "," + a.channelid);
      msg.set(dpsmsg.passedData1, "get channel");
    }
    else
    if( a.requestType == 3 ) {
      msg.set(dpsmsg.additionalData, a.companyid + "," + a.interfaceid + "," + a.channelname);
      msg.set(dpsmsg.passedData1, "set channel");
      msg.set(dpsmsg.passedData2, a.channel_val);
    }
    int state = SEND_ACK;
    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      Log.d(TAG, "diorequest read " + rc);

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = SIGNON_STATE;
          else
            state = IOERROR_STATE;
        }
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad diorequest write");
      e.printStackTrace();
    }

    if( state != IOERROR_STATE && ( msg.getType() != dpsmsg.dioResponse ) )
      state = processHostMsg();

    return state;
  }

  public int doDechoRequest(DechoActivity a) {
    Log.d(TAG, "doDechoRequest company " + a.companyid);
    msg.clear();
    msg.setType(dpsmsg.dechoRequest);
    msg.set(dpsmsg.authData, mAuthToken);
    msg.set(dpsmsg.actionCode, dpsmsg.getData);
    int mystan = mService.mystan++;
    //Log.d(TAG, "mystan is " + mystan);
    msg.set(dpsmsg.mystan, mystan);
    if( a.requestType == 0 ) {
      msg.set(dpsmsg.additionalData, a.companyid);
      msg.set(dpsmsg.passedData1, "get devices");
    }
    else
    if( a.requestType == 1 ) {
      msg.set(dpsmsg.additionalData, a.companyid + "," + a.dpsid + "," + a.localid);
      msg.set(dpsmsg.passedData1, "get config");
    }
    else
    if( a.requestType == 2 ) {
      msg.set(dpsmsg.additionalData, a.companyid + "," + a.dpsid + "," + a.localid);
      msg.set(dpsmsg.passedData1, "power on");
    }
    else
    if( a.requestType == 3 ) {
      msg.set(dpsmsg.additionalData, a.companyid + "," + a.interfaceid);
      msg.set(dpsmsg.passedData1, "power off");
    }
    else
    if( a.requestType == 4 ) {
      msg.set(dpsmsg.additionalData, a.companyid + "," + a.interfaceid); 
      msg.set(dpsmsg.passedData1, "set volume");
      msg.set(dpsmsg.passedData2, a.volume_val);
    }
    int state = SEND_ACK;
    try {
      msg.write();
      int timeout = READTIMEOUT;
      int rc = msg.read(timeout);
      Log.d(TAG, "dechorequest read " + rc);

      if( rc != 1 ) {
        if( rc == 2 ) {
          if( msg.getPasschar() == NACK )
            state = SIGNON_STATE;
          else
            state = IOERROR_STATE;
        }
      }
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad dechorequest write");
      e.printStackTrace();
    }

    if( state != IOERROR_STATE && ( msg.getType() != dpsmsg.dechoResponse ) )
      state = processHostMsg();

    return state;
  }

  private void processNotifyMsg( JSONObject obj ) {
    Log.d(TAG, "processNotifyMsg"); 
    try {
      String which = obj.getString("which");
      String command = obj.getString("command");
      Log.d(TAG, "processNotifyMsg " + command);
      String when = obj.getString("when");
      if( command.equals( "send alert" ) ) {
        Log.d(TAG, "send alert");

        msg.set(dpsmsg.userData, which);
        //mService.showNotification( msg, mService.NOTIFICATION );
      } 
    } catch ( JSONException e ) {
      Log.d(TAG, "no which/command");
    }
  }

  private void processCameraMsg( JSONObject obj ) {
    Log.d(TAG, "processCameraMsg"); 
    try {
      String which = obj.getString("which");
      String command = obj.getString("command");
      Log.d(TAG, "processCameraMsg " + command);
      //String quality = obj.getString("quality");
      if( command.equals( "take picture" ) ) {
        Log.d(TAG, "take picture");

        mDapActivity.doCamera();
      } 
      else
      if( command.equals( "start stream" ) ) {
        Log.d(TAG, "start stream");

        mDapActivity.doCameraStream();
      } 
      else
      if( command.equals( "download picture" ) ) {
        Log.d(TAG, "download picture");
      } 
    } catch ( JSONException e ) {
      Log.d(TAG, "no which/command");
    }
  }
  private void processVideoMsg( JSONObject obj ) {
    try {
      String which = obj.getString("which");
      String command = obj.getString("command");
      //String quality = obj.getString("quality");
      if( command.equals("start_recording" ) ) {
        Log.d(TAG, "start video recording");
      } 
      else
      if( command.equals( "stop_recording" ) ) {
        Log.d(TAG, "stop video recording");
      } 
    } catch ( JSONException e ) {
      Log.d(TAG, "no which/command");
    }
  }

  private void processStreamMsg( JSONObject obj ) {
    try {
      String which = obj.getString("which");
      String command = obj.getString("command");
      if( command.equals( "start_sending" ) ) {
        Log.d(TAG, "start video stream");
      } 
      else
      if( command.equals( "stop_sending" ) ) {
        Log.d(TAG, "stop video stream");
      } 
    } catch ( JSONException e ) {
      Log.d(TAG, "no which/command");
    }
  }

  private void processChannelMsg( JSONObject obj ) {
    Log.d(TAG, "processChannelMsg");
    try {
      String which = obj.getString("which");
      String val = obj.getString("value");
      String command = obj.getString("command");
      Log.d(TAG, "processChannelMsg " + command);
      if( command.equals( "change value" ) ) {
        Log.d(TAG, "change channel value");
        mService.mDioHandler.obtainMessage(ClientService.DIO_MESSAGE2, obj).sendToTarget();
      } 
    } catch ( JSONException e ) {
      Log.d(TAG, "no which/command");
    }
  }

  private void processPlayListMsg( JSONObject obj ) {
    Log.d(TAG, "processPlayListMsg");
    try {
      String which = obj.getString("which");
      String val = obj.getString("value");
      String command = obj.getString("command");
      Log.d(TAG, "processPlayListMsg " + command);
    } catch ( JSONException e ) {
      Log.d(TAG, "no which/command");
    }
  }

  public int processHostMsg() {
    int msgType = msg.getType();
    int actionCode = msg.getInt(dpsmsg.actionCode);
    Log.d(TAG, "processHostMsg msg type " + msgType + " " + actionCode);

    if( msg.getType() == dpsmsg.fileUpdateRequest && actionCode == dpsmsg.dataSend ) {
      Log.d(TAG, "need to process command buffer");

      try {
        String n = new String(msg.getOutbuf(), "UTF-8");
        int i = n.indexOf("\n");
        Log.d(TAG, "message: " + n + " i " + i);
        if( i > 0 ) {
          String s = n.substring(i + 1, n.length());
          Log.d(TAG, "json: " + s);
          try {
            JSONArray jArray = new JSONArray(s);
            for (int j=0; j < jArray.length(); j++) {
              try {
                  JSONObject oneObject = jArray.getJSONObject(j);
                  // Pulling items from the array
                  String op = oneObject.getString("op");
                  String which = oneObject.getString("which");
                  Log.d(TAG, "op " + op + " which " + which);
                  msg.set(dpsmsg.userData, "op " + op + " which " + which);
                  mService.showNotification( msg, mService.NOTIFICATION + j );
                  Log.d(TAG, "op2check " + op + " which " + which);
                  if( op.equals( "camera" ) ) {
                    Log.d(TAG, "processCameraMsg");
                    processCameraMsg( oneObject );
                  }
                  else
                  if( op.equals( "video" ) ) {
                    processVideoMsg( oneObject );
                  }
                  else
                  if( op.equals( "stream" ) ) {
                    processStreamMsg( oneObject );
                  }
                  else
                  if( op.equals( "notify" ) ) {
                    processNotifyMsg( oneObject );
                  }
                  else
                  if( op.equals( "channel" ) ) {
                    processChannelMsg( oneObject );
                  }
                  else
                  if( op.equals( "playlist" ) ) {
                    processPlayListMsg( oneObject );
                  }
              } catch (JSONException e) {
                  Log.d(TAG, "bad json2");
              }
            }
          } catch (JSONException e) {
              Log.d(TAG, "bad json");
          }
        }
      } catch (Exception e) {
        Log.d(TAG, "bad conversion");
        e.printStackTrace();
      }
      msg.clear();
      return SEND_ACK;
    }
    else
    if( msgType == dpsmsg.promptRequest ) {
      msg.setType( dpsmsg.promptResponse );
      if( actionCode == dpsmsg.pinRequired ) {
        //Log.d(TAG, "pin request");
      } 
      else 
      if( actionCode == dpsmsg.confirmRequired ) {
        //Log.d(TAG, "confirm request");
      } else {
        //Log.d(TAG, "prompt request " + msg.get(dpsmsg.additionalData));
      }
      return SEND_STATE;
    } 
    else 
    if( msgType == dpsmsg.tranResponse || msgType == dpsmsg.dapTran ) {
      Log.d(TAG, "tran message complete");
      mService.tranComplete(msg);
    }
    else 
    if( msgType == dpsmsg.testRequest ) {
      Log.d(TAG, "test message");
      msg.setType( dpsmsg.testResponse );

      return SEND_STATE;
    } else {
      Log.d(TAG, "undefined message " + msgType);
    }
    return READ_STATE;
  }

  public int writeACK() {
    return writePasschar(ACK);
  }

  public int doRead(int timeout) {
    msg.clear();
    //SystemClock.sleep(timeout);

    Log.d(TAG, "set socket timeout " + timeout);

    try {
      socket.setSoTimeout(timeout);
    } catch( java.net.SocketException e ) {
        Log.d(TAG, "socket sotimeout error");
    }

    int rc = 0;
    int state = SEND_ACK;

    //rc = 1;

    if( timeout <= 0 )
      timeout = READTIMEOUT;
    try {
        Log.d(TAG, "calling msg read");
        rc = msg.read(timeout);
        Log.d(TAG, "doRead rc " + rc);
        if( rc < 0 )
          state = IOERROR_STATE;
    } catch (java.net.SocketTimeoutException e) {
        Log.d(TAG, "socket timeout");
        state = IOERROR_STATE;
    } catch (Exception e) {
        Log.d(TAG, "bad doRead");
        e.printStackTrace();
        rc = -1;
        state = IOERROR_STATE;
    }

    if( state != IOERROR_STATE ) {
      int msgType = msg.getType();
      int actionCode = msg.getInt(dpsmsg.actionCode);

      Log.d(TAG, "read: " + msgType + " " + actionCode);

      if( msg.getType() == dpsmsg.fileUpdateRequest && actionCode == dpsmsg.dataSend ) {
        int len = msg.getInt(dpsmsg.recordNumber);
        Log.d(TAG, "readextended " + len);
        boolean rc2 = false;
        try {
          rc2 = msg.readExtendedData(len);
        } catch (Exception e) {
          Log.d(TAG, "readExtended failed " + rc2);
          e.printStackTrace();
        }
        Log.d(TAG, "readExtended " + rc2);
        if( rc2 ) {
          byte[] b = msg.getOutbuf();
          try {
            String n = new String(b, "UTF-8");
            Log.d(TAG, "message: " + n);
          } catch (Exception e) {
            Log.d(TAG, "bad conversion");
          }
        }
      }
    }

    if( state != IOERROR_STATE && rc > 0 )
      state = processHostMsg();
    else
    if( state != IOERROR_STATE )
      state = READ_STATE;

    Log.d(TAG, "doRead done " + state);

    try {
      socket.setSoTimeout(0);
    } catch( java.net.SocketException e ) {
        Log.d(TAG, "socket sotimeout error");
    }

    return state;
  }

  public int doSend(dpsmsg msgin) {
    msg.clear();
    msg.header = msgin.header;
    msg.fields = msgin.fields;
    msg.setType( msgin.getType());

    msg.set(dpsmsg.authData, mAuthToken);

    return doSend();
  }

  public int doSend() {
    int state = READ_STATE;
    try {
      msg.write();
    } catch (Exception e) {
      state = IOERROR_STATE;
      Log.d(TAG, "bad msg write");
      e.printStackTrace();
    }
    return state;
  }

  public int writeEOT() {
    state = END_STATE;

    try {
      msg.writePasschar((byte)EOT);
    } catch (Exception e) {
      Log.e(TAG, "Exception during write", e);
      state = END_STATE;
    }
    return state;
  }

  public int writePasschar(int b) {
    int state = READ_STATE;

    if( msg == null )
      return IOERROR_STATE;

    try {
      msg.writePasschar((byte)b);
    } catch (Exception e) {
      Log.e(TAG, "Exception during write", e);
      state = IOERROR_STATE;
    }
    return state;
  }

  public void write(byte[] buffer) {
    //Log.e(TAG, "dpsio write" + buffer);
/*
        try {
            oStream.write(buffer);
        } catch (IOException e) {
            Log.e(TAG, "Exception during write", e);
        }
*/
  }

  public String returnToken() {
    //Log.d(TAG, "returnToken " + mAuthToken);
    return mAuthToken;
  }

  public String getDeviceID() {
    String serial = null;

    if(Build.VERSION.SDK_INT > Build.VERSION_CODES.FROYO) 
      serial = Build.SERIAL;

    if( serial != null )
      return "03" + serial;

    String androidId = Secure.getString(ctx.getContentResolver(), Secure.ANDROID_ID);

    if(androidId != null) 
      return "02" + androidId;

    //TelephonyManager tm = (TelephonyManager) ctx.getSystemService(Context.TELEPHONY_SERVICE);
    //String tmDevice = tm.getDeviceId();

    //if(tmDevice != null) 
      //return "01" + tmDevice;

    return "unknown";
  }
}
