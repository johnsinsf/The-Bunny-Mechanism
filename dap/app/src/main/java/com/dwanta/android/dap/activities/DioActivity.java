package com.dwanta.android.dap.activities;

import android.app.Activity;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Binder;
import android.os.Parcelable;
import android.os.Parcel;
import android.os.Parcelable.Creator;

import android.os.Handler;
import android.os.Message;
import android.content.ComponentName;
import android.content.IntentFilter;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.View.OnClickListener;
import android.content.Intent;
import android.app.Service;
import android.content.Context;
import android.content.ServiceConnection;
import android.util.Log;
import android.text.TextWatcher;
import android.text.Editable;
import android.graphics.Color;

import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;

import android.widget.Button;
import android.widget.ToggleButton;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.widget.EditText;
import android.widget.Toast;
import android.widget.Spinner;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;

import android.widget.AdapterView.OnItemSelectedListener;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;
import java.util.Arrays;
import java.util.StringTokenizer;
import java.util.regex.Pattern;

import java.io.IOException;

import com.dwanta.android.dap.client.dpsmsg;
import com.dwanta.android.dap.client.ClientService.MessageTarget;
import com.dwanta.android.dap.authenticator.AuthenticatorActivity;
import com.dwanta.android.dap.client.ClientService;
import com.dwanta.android.dap.R;
import com.dwanta.android.dap.Constants;


public class DioActivity extends Activity  implements
    Handler.Callback, MessageTarget, Parcelable {

  public final static String TAG = "DioActivity";
  private Context ctx = this;
  private Intent mIntent = null;

  private int mTestInt = 0;
  private int mTestState = 0;

  private String mQtyText = "";

  private Button jsonbuttons[];
  private int    jsonbuttons_size = 0;
  private Handler mHandler = new Handler(this);  

  public  String  companyid = "";
  public  String  interfaceid = "";
  public  String  channelid = "";
  public  String  channelname = "";
  public  String  channeltype = "";
  public  String  channeldir  = "";
  public  String  channel_val = "";
  public  int     requestType = 0;
  private boolean toggleState = false;
  public ClientService mBoundService;
  public DioActivity thisActivity  = this;

  static final Pattern CODE_PATTERN = Pattern.compile("([0-9]{0,4})|([0-9]{4}-)+|([0-9]{4}-[0-9]{0,4})+");
  static final Pattern QTY_PATTERN = Pattern.compile("(^[0-9,-]*\\.[0-9]{0,3}$)|(^[0-9,-]*$)");
 
  IBinder mBinder = new LocalBinder();

  public DioActivity( ) {
  }

  protected DioActivity(Parcel in) {
    mIntent = in.readParcelable(Intent.class.getClassLoader());
    mTestInt = in.readInt();
    mTestState = in.readInt();
    mBinder = in.readStrongBinder();
  }

  public static final Creator<DioActivity> CREATOR = new Creator<DioActivity>() {
    @Override
    public DioActivity createFromParcel(Parcel in) {
      return new DioActivity(in);
    }

    @Override
    public DioActivity[] newArray(int size) {
      return new DioActivity[size];
    }
  };

  @Override
  public void onCreate(Bundle savedInstanceState) { 
    super.onCreate(savedInstanceState);

    mTestInt = 1;

    mIntent = getIntent();

    Bundle extras = mIntent.getExtras();

    String s = extras.getString("companyid");
    String s2 = extras.getString("bun");

    Log.d(TAG, "onCreate " + s + " " + s2);

    companyid = s;
    if( mIntent.hasExtra("mTestInt") ) {
      mTestInt = extras.getInt("mTestInt");
      //Log.d(TAG, "have mTestInt " + mTestInt);
    }
  }
    
  @Override
  public void onStart() {
    //Log.d(TAG, "onStart " + mConnection);
    super.onStart();
    return;
  }
 
  @Override
  public void onResume() {
    Log.d(TAG, "onResume " + mTestInt + " " + mBoundService);
    if( mBoundService == null ) {
      Intent mStartIntent = new Intent(this, ClientService.class);
      startService(mStartIntent);
      mTestState = 1;
      doBindService();
    } else {
      Log.d(TAG, "setting up view");
      setView();
    }
    super.onResume();
    return;
  }
 
  public void setView() {
    Log.d(TAG, "setView " + mTestInt);

    switch( mTestInt ) {
      case 2: handleTranView(); break;
      default: setContentView(R.layout.sitemain); break;
    }
    return;
  }

  @Override
  public void onStop() {
    //Log.d(TAG, "onStop");
    super.onStop();
    return;
  }

  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
    saveMyState(outState);
  }

  public void saveMyState(Bundle outState) {
    outState.putInt   ("mTestInt",         mTestInt);
    
  }

  @Override
  protected void onRestoreInstanceState(Bundle savedInstanceState) {
    super.onRestoreInstanceState(savedInstanceState);

    //Log.d(TAG, "onRestoreInstanceState " + mTestInt);

    mTestInt         = savedInstanceState.getInt("mTestInt");

    //Log.d(TAG, "restored sitenum " + mSiteNumber + " " + mQtyText + " " + mSelectedProduct + " " + mSelectedQty);
  }
 
  @Override
  public void onDestroy() {
    //Log.d(TAG, "onDestroy");
    doUnbindService();
    super.onDestroy();
    return;
  }

  private void handleTranView() {
    Log.d(TAG, "handleTranView");
    setContentView(R.layout.sitecard);
    mTestInt = 2;
  }

  public void handleTran(View view) {
    Log.d(TAG, "handleTran");
    mTestInt = 2;
    handleTranView();
  }

  public void channelVal(String which, String val) {
    Log.d(TAG, "channelVal");
  }

  public Handler getHandler() {
    return mHandler;
  }
 
  @Override
  public boolean handleMessage(Message msg) {
    Log.d(TAG, "handleMessage " + msg.what);
    switch (msg.what) {

      case ClientService.DIO_MESSAGE:
        DioActivity m = (DioActivity)msg.obj;
        if( m.requestType == 0 ) {
          if( m.mBoundService != null && m.mBoundService.chat != null && m.mBoundService.chat.msg != null ) {
            String interfaces = new String(m.mBoundService.chat.msg.get(dpsmsg.passedData4));
            Log.d(TAG, "have interfaces " + interfaces);
            handleInterfaces(interfaces);
          }
        }
        else
        if( m.requestType == 1 ) {
          String interface2 = new String(m.mBoundService.chat.msg.get(dpsmsg.passedData4));
          Log.d(TAG, "have interface " + interface2);
          handleInterface(interface2);
        }
      break;
      case ClientService.DIO_MESSAGE2:
        JSONObject obj = (JSONObject)(msg.obj);
        try {
          String which = obj.getString("which");
          String val = obj.getString("value");
          int i2 = new Integer(which).intValue();
          Log.d(TAG, "MESSAGE2 have which " + which);
          if( mTestInt == 4 ) {
            Log.d(TAG, "was in menu");
            for(int i = 0; i < jsonbuttons_size; i++ ) {
              if( jsonbuttons[i].getId() == i2 ) {
                Log.d(TAG, "found button");
                String tag = jsonbuttons[i].getTag().toString();
                jsonbuttons[i].setText(tag + " UPDATED VALUE " + val);
              }
            }
          }
        } catch ( JSONException e ) {
        }
      break;
    }
    return true;
  }

  public void setViewChannel(String cid, String cname, String cdir, String ctype ) {
    setContentView(R.layout.channel);
    channelid = cid;
    channelname  = cname;
    channeldir = cdir;
    channeltype = ctype;

    mTestInt = 5;
    TextView textview = (TextView)findViewById(R.id.channelId);
    textview.setText("Channel ID: " + cid);
    TextView textview2 = (TextView)findViewById(R.id.channelName);
    textview2.setText("Channel name: " + cname);
    TextView textview3 = (TextView)findViewById(R.id.channelType);
    textview3.setText("Channel type: " + ctype);

    final EditText editText = (EditText) findViewById(R.id.channelStatus);
    if( channel_val != "" )
      editText.setText(channel_val);

    editText.addTextChangedListener(new TextWatcher() {
      @Override
      public void afterTextChanged(Editable s) {
        if (s.length() > 0 && !QTY_PATTERN.matcher(s).matches()) {
          String input = s.toString();
          Log.d(TAG, "bad input " + input);
          int len = input.length();
          String newstring = input.substring(0, len - 1);
          editText.removeTextChangedListener(this);
          editText.setText(newstring);
          editText.addTextChangedListener(this);
          mQtyText = newstring;
          Log.d(TAG, "newstring: " + newstring);
        } else {
          mQtyText = s.toString();
        }
        Log.d(TAG, "new qty " + mQtyText);
      }
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
      }
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }
    });
  }

  private void handleInterface(String i) {
    LinearLayout ll2 = new LinearLayout(this);
    ll2.setOrientation(LinearLayout.VERTICAL);

    setContentView(R.layout.dapmain);
    setSvcButton();

    mTestInt = 4;

    if( i.length() > 0 ) { 
      try {
        JSONObject oneObject = new JSONObject(i);

        Log.d(TAG, "object size " + oneObject.length());

        JSONObject cObject = oneObject.getJSONObject("channels");

        JSONArray channels = cObject.names();

        Log.d(TAG, "have len " + channels.length());

        jsonbuttons = new Button[channels.length()];
        jsonbuttons_size = channels.length();

        for( int j = 0; j < channels.length(); j++ ) {
          String name = channels.getString(j);
          Log.d(TAG, "have name " + name );
          JSONObject interface2 = cObject.getJSONObject(name);
          String cid = name;
          String cname = new String(interface2.get("name").toString());
          String ctype = new String(interface2.get("type").toString());
          String cdir = new String(interface2.get("dir").toString());
          channel_val = new String(interface2.get("val").toString());
          Log.d(TAG, "have cname " + cname );
          jsonbuttons[j] = new Button(this);
          jsonbuttons[j].setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
              Log.d(TAG, "clicked json button " + cname);
              setViewChannel(cid, cname, cdir, ctype);
            }
          });
          jsonbuttons[j].setText(cname + " current value " + channel_val);
          int i2 = new Integer(cid).intValue();
          jsonbuttons[j].setId(i2);
          jsonbuttons[j].setTag(cname);
          ll2.addView(jsonbuttons[j]);
        //} catch (JSONException e) {
            //Log.d(TAG, "bad json2 " + e);
        }
      } catch (JSONException e) {
        Log.d(TAG, "bad json " + e);
      }
    }
    LinearLayout mAppWidgetFrame2 = (LinearLayout)findViewById(R.id.mainframe2);
    mAppWidgetFrame2.addView(ll2, new LinearLayout.LayoutParams(
                      LinearLayout.LayoutParams.MATCH_PARENT,
                      LinearLayout.LayoutParams.WRAP_CONTENT));
  }

  private void companySelect3(String iname, int option) {

    requestType = 1;
    interfaceid = iname;

    mBoundService.getHandler().obtainMessage(ClientService.DIO_MESSAGE, thisActivity).sendToTarget();
  }

  private void companySelect4(String iname, String name, int option) {

    channelid = iname;
    channelname = name;
    requestType = 2;
    mBoundService.getHandler().obtainMessage(ClientService.DIO_MESSAGE, thisActivity).sendToTarget();
  }

  public void checkChannel(View view) {
    EditText editqty = (EditText)findViewById(R.id.channelStatus);
    Log.d(TAG, "checkChannel " + editqty.getText().toString()); 

    channel_val = editqty.getText().toString();
    if( channel_val.length() == 0 )
      return;

    //TextView t1 = (TextView)findViewById(R.id.channelId);
    //TextView t2 = (TextView)findViewById(R.id.channelName);
    //TextView t3 = (TextView)findViewById(R.id.channelType);
    //channelid = t1.getText().toString();
    //channelname = t2.getText().toString();
    //channeltype = t3.getText().toString();

    requestType = 3;
    mBoundService.getHandler().obtainMessage(ClientService.DIO_MESSAGE, thisActivity).sendToTarget();
    //float m = new Float(s).floatValue();

  }

  private void handleInterfaces(String i) {
    LinearLayout ll2 = new LinearLayout(this);
    ll2.setOrientation(LinearLayout.VERTICAL);

    setContentView(R.layout.dapmain);
    setSvcButton();

    if( i.length() > 0 ) { 
      try {
        JSONObject oneObject = new JSONObject(i);

        Log.d(TAG, "object size " + oneObject.length());

        JSONObject cObject = oneObject.getJSONObject("interfaces");

        if( cObject != null ) { 
          JSONArray interfaces = cObject.names();
  
          Log.d(TAG, "have len " + interfaces.length());
  
          jsonbuttons = new Button[interfaces.length()];
          jsonbuttons_size = interfaces.length();
  
          for( int j = 0; j < interfaces.length(); j++ ) {
            String name = interfaces.getString(j);
            Log.d(TAG, "have name " + name );
            JSONObject interface2 = cObject.getJSONObject(name);
            String iname = new String(interface2.get("name").toString());
            Log.d(TAG, "have iname " + iname );
            jsonbuttons[j] = new Button(this);
            jsonbuttons[j].setOnClickListener(new OnClickListener() {
              public void onClick(View v) {
                Log.d(TAG, "clicked json button " + name);
                companySelect3(name, 0);
              }
            });
            jsonbuttons[j].setText(iname);
            ll2.addView(jsonbuttons[j]);
          }
        }
      } catch (JSONException e) {
        Log.d(TAG, "bad json " + e);
      }
    }
    LinearLayout mAppWidgetFrame2 = (LinearLayout)findViewById(R.id.mainframe2);
    mAppWidgetFrame2.addView(ll2, new LinearLayout.LayoutParams(
                      LinearLayout.LayoutParams.MATCH_PARENT,
                      LinearLayout.LayoutParams.WRAP_CONTENT));
  }

  private void setSvcButton() {
    Log.d(TAG, "setSvcButton " + mBoundService);
    setContentView(R.layout.dapmain);
    ToggleButton button = (ToggleButton) findViewById(R.id.svctogglebutton);
    if( mBoundService == null ) {
      return;
    }
    else
    if( mBoundService != null && mBoundService.getSignedOn() ) {
      Log.d(TAG, "setting on");
      //Toast.makeText(this, "Dap on", Toast.LENGTH_SHORT).show();
      if( button != null )
        button.setChecked(true);
      toggleState = true;
    } else {
      Log.d(TAG, "setting off");
      //Toast.makeText(this, "Dap off", Toast.LENGTH_SHORT).show();
      if( button != null )
        button.setChecked(false);
      toggleState = false;
    }
  }

  private ServiceConnection mConnection = new ServiceConnection() {
     public void onServiceConnected(ComponentName className, IBinder service) {
       Log.d(TAG, "new service connection " + className + " service " + service);

       mBoundService = ((ClientService.LocalBinder)service).getService();

       //Log.d(TAG, "new service connection " + mBoundService);
       mBoundService.setDioHandler(mHandler);
       Log.d(TAG, "getting interfaces"); 
       mBoundService.getHandler().obtainMessage(ClientService.DIO_MESSAGE, thisActivity).sendToTarget();

       if( mTestState == 1 ) {
         mTestState = 0;
         setView();
       }
     }
     public void onServiceDisconnected(ComponentName className) {
       Log.d(TAG, "service disconnection " + mBoundService);
     }
  };

  void doUnbindService() {
    //Log.d(TAG, "unbinding client service");
    if( mBoundService != null ) {
      unbindService(mConnection);
      mBoundService = null;
    }
  }

  void doBindService() {
    //Log.d(TAG, "binding client service");
    bindService(new Intent(this, ClientService.class), mConnection, Context.BIND_AUTO_CREATE);
    //Log.d(TAG, "binding client service is " + mConnection);
  }

  public class LocalBinder extends Binder {
    public DioActivity getService() {
      //Log.d(TAG, "get local binder");
      return DioActivity.this;
    }
  }

  @Override
  public void writeToParcel(Parcel dest, int flags) {
    dest.writeInt(1);
    //dest.writeInt(colors[0]);
    //dest.writeInt(colors[1]);
  }

  @Override
  public int describeContents() {
    return 0;
  }
}
