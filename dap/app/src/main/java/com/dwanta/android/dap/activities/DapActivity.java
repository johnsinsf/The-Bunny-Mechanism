package com.dwanta.android.dap.activities;

import android.app.Activity;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.os.Parcelable;
import android.os.Parcel;
import android.os.PowerManager;
import android.os.SystemClock;
import android.net.Uri;
import android.provider.Settings;
import android.content.ComponentName;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.app.PendingIntent;
import android.content.Intent;
import android.media.RingtoneManager;
import android.media.Ringtone;

import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;

import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.content.Intent;
import android.widget.EditText;
import android.app.Service;
import android.content.Context;
import android.content.ServiceConnection;
import android.util.Log;
import android.accounts.Account;
import android.accounts.AccountAuthenticatorResponse;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AccountManagerCallback;
import android.accounts.AbstractAccountAuthenticator;
import android.accounts.NetworkErrorException;
import android.accounts.OperationCanceledException;
import android.accounts.AuthenticatorException;
import java.util.concurrent.TimeUnit;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.widget.Button;
import android.widget.ToggleButton;

import android.widget.LinearLayout;
import android.widget.FrameLayout;
import android.widget.Toast;

import java.io.IOException;

import com.dwanta.android.dap.client.ClientService.MessageTarget;
import com.dwanta.android.dap.authenticator.AuthenticatorActivity;
import com.dwanta.android.dap.authenticator.AuthenticationService;
import com.dwanta.android.dap.authenticator.Authenticator;
import com.dwanta.android.dap.client.ClientService;
import com.dwanta.android.dap.client.dpscamera;
import com.dwanta.android.dap.activities.SiteActivity;
import com.dwanta.android.dap.activities.DioActivity;
import com.dwanta.android.dap.activities.DechoActivity;
import com.dwanta.android.dap.R;
import com.dwanta.android.dap.Constants;

public class DapActivity extends Activity  
  implements Handler.Callback, MessageTarget {

  public final static String TAG = "DapActivity";
  public  String mUsername = "";
  public  String mDevname  = "";
  public  String mPassword = "";

  private Context ctx = this;
  
  private Handler mHandler = new Handler(this);  
  private DapActivity mDapActivity = this;

  ClientService mBoundService;
 
  private final int START_AUTH = 0;
  private boolean toggleState = false;
  private Button deleteButton = null;
  private String mSiteDevice = "";
  private String mCardDevice = "";
  private String mJson = "";
  private boolean mExpired = false;
  private int LOOP_TIME = 300000;

  private IBinder mBinder = new LocalBinder();

  private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
      String action = intent.getAction();

      Log.d(TAG, "in broadcast onReceive " + action);
      if (AccountManager.LOGIN_ACCOUNTS_CHANGED_ACTION.equals(action)) {
        Log.d(TAG, "received the login broadcast");
        setSvcButton();      
      } 
    }
  };


  @Override
  public void onCreate(Bundle savedInstanceState) { 
    super.onCreate(savedInstanceState);

    Intent mIntent = getIntent();

    Bundle extras = mIntent.getExtras();

    if( mIntent.hasExtra("username") )  {
      mUsername = extras.getString("username");
      Log.d(TAG, "username was sent " + mUsername);
    }
    if( mIntent.hasExtra("devname") ) 
      mDevname = extras.getString("devname");
    if( mIntent.hasExtra("password") ) 
      mPassword = extras.getString("password");

/*
    long timeNow = System.currentTimeMillis();
    long expiretime = 1425322736000L + ( 365 * 60 * 60 * 24 * 1000 );
    Log.d(TAG, "XXXXXX starting client service " + timeNow + " expires " + expiretime);
    
    if( timeNow > expiretime ) {
      Toast.makeText(this, "Dap expired, please install latest version.", Toast.LENGTH_LONG).show();
      mExpired = true;
    } else {
*/

/*
   Intent intent = new Intent();
   String packageName = ctx.getPackageName();
   //PowerManager pm = ctx.getSystemService(PowerManager.class);
   PowerManager pm = (PowerManager) ctx.getSystemService(Context.POWER_SERVICE);
   PowerManager.WakeLock wl = pm.newWakeLock(
                                      PowerManager.FULL_WAKE_LOCK,
                                      TAG);
   //wl.acquire();

   if (pm.isIgnoringBatteryOptimizations(packageName))
     intent.setAction(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS);
   else {
     intent.setAction(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
     intent.setData(Uri.parse("package:" + packageName));
   }
   ctx.startActivity(intent);
*/

   mDapActivity = this;

   Intent mStartIntent = new Intent(this, ClientService.class);
   startService(mStartIntent);
   doBindService();

   //long t = SystemClock.uptimeMillis();
   //mHandler.sendEmptyMessageAtTime(ClientService.WAKEUP, t + LOOP_TIME);
  }
  
  @Override
  public void onStart() {
    Log.d(TAG, "onStart " + mConnection);
    super.onStart();
  }
 
  @Override
  public void onResume() {
    Log.d(TAG, "onResume");
    if( ! mExpired ) {
      if( mBoundService == null ) {
        Intent mStartIntent = new Intent(this, ClientService.class);
        startService(mStartIntent);
        doBindService();
      }

      if( mBoundService != null && mBoundService.getSignedOn() )
        toggleState = true;
      else
        toggleState = false;

      setContentView(R.layout.dapmain);
      ToggleButton button2 = (ToggleButton) findViewById(R.id.svctogglebutton);
      if( button2 != null )
        button2.setChecked( toggleState );
      else
        Log.d(TAG, "button2 is null");

      deleteButton = (Button) findViewById(R.id.delete_button);
      if( deleteButton != null ) {
        deleteButton.setOnClickListener(new OnClickListener() {
          public void onClick(View v) {
            Log.d(TAG, "clicked delete");
            showDialog(1);
          }
        });
      }
      Log.d(TAG, "delete button " + deleteButton);
  
      checkSiteAdmin(0);
    }
    super.onResume();
    return;
  }
 
  @Override
  public void onStop() {
    Log.d(TAG, "onStop");
    super.onStop();
    return;
  }
 
  @Override
  public void onDestroy() {
    Log.d(TAG, "onDestroy");
    doUnbindService();
    super.onDestroy();
    return;
  }

  public void doCamera() {
    Log.d(TAG, "doCamera");
    dpscamera c = new dpscamera();
   
    Intent intent = new Intent(this, DapActivity.class);
 
    intent.addFlags(
      Intent.FLAG_ACTIVITY_NEW_TASK | 
      Intent.FLAG_ACTIVITY_CLEAR_TASK | 
      Intent.FLAG_ACTIVITY_SINGLE_TOP 
    );

    PendingIntent contentIntent = PendingIntent.getActivity(
      this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
            
    CharSequence text = "Video";
    boolean onGoing = false;
    Uri soundUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);

    String t = "video";
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
  
    //startForeground( 1, notification );
    super.onResume();

    c.setActivity(mDapActivity);
    c.setContext(ctx);
    c.startBackgroundThread();
    //c.openCamera(640, 480);
    //c.openCamera(1024, 768);
    c.openCamera(1920, 1024);
    c.takePicture();
    //c.captureStillPicture();
    //c.closeCamera();
    //c.stopBackgroundThread();
  }

  public void doCameraStream() {
    Log.d(TAG, "doCameraStream");
    dpscamera c = new dpscamera();
   
    Intent intent = new Intent(this, DapActivity.class);
 
    intent.addFlags(
      Intent.FLAG_ACTIVITY_NEW_TASK | 
      Intent.FLAG_ACTIVITY_CLEAR_TASK | 
      Intent.FLAG_ACTIVITY_SINGLE_TOP 
    );

    PendingIntent contentIntent = PendingIntent.getActivity(
      this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
            
    CharSequence text = "Video";
    boolean onGoing = false;
    Uri soundUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);

    String t = "video";
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
  
    super.onResume();

    c.setActivity(mDapActivity);
    c.setContext(ctx);
    c.startBackgroundThread();
    //c.openCamera(640, 480);
    //c.openCamera(1024, 768);
    c.openCamera(1920, 1024);
    c.startStream();
  }

  private void checkSiteAdmin(int opt) {
    Log.d(TAG, "checkSiteAdmin " + opt + " " + mBoundService);
    if( mBoundService != null ) {
      mSiteDevice = mBoundService.getSiteDevice();
      mCardDevice = mBoundService.getCardDevice();
      mJson = mBoundService.getJson();
    }
    Log.d(TAG, "mSiteDevice " + mSiteDevice + " mCardDevice " + mCardDevice + " json " + mJson);

    if( mSiteDevice.length() == 0  && mJson.length() ==  0 )
      return;

    if( opt == 1 )
      setContentView(R.layout.dapmain);

    LinearLayout ll2 = new LinearLayout(this);
    ll2.setOrientation(LinearLayout.VERTICAL);

    if( mSiteDevice.length() > 0 ) { 

      Button sitebutton = new Button(this);
      sitebutton.setOnClickListener(new OnClickListener() {
        public void onClick(View v) {
          Log.d(TAG, "clicked admin site");
          siteAdmin(mSiteDevice);
        }
      });
      sitebutton.setText("run store #" + mSiteDevice);
      ll2.addView(sitebutton);

    }

    if( mJson.length() > 0 ) { 
      Log.d(TAG, "check json: " + mJson);
      try {
        JSONObject oneObject = new JSONObject(mJson);

        Log.d(TAG, "object size " + oneObject.length());

        JSONObject cObject = oneObject.getJSONObject("companies");

        JSONArray companies = cObject.names();

        Log.d(TAG, "companies size " + companies.length());

        Button jsonbuttons[] = new Button[companies.length()];

        for (int j=0; j < companies.length(); j++) {
          try {
            String companyid = companies.getString(j);
            Log.d(TAG, "have id " + companyid );
            JSONObject company = cObject.getJSONObject(companyid);
            String name = company.get("name").toString();
            Log.d(TAG, "have name " + name );
            jsonbuttons[j] = new Button(this);
            jsonbuttons[j].setOnClickListener(new OnClickListener() {
              public void onClick(View v) {
                Log.d(TAG, "clicked json button " + name + " id " + companyid);
                companySelect1(companyid);
              }
            });
            jsonbuttons[j].setText(name);
            jsonbuttons[j].setTag(companyid);
            ll2.addView(jsonbuttons[j]);
          } catch (JSONException e) {
            Log.d(TAG, "bad json2 " + e);
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

    Log.d(TAG, "checkSiteAdmin done");
  }

  public Handler getHandler() {
    return mHandler;
  }
 
  @Override
  public boolean handleMessage(Message msg) {
    switch (msg.what) {
      case AuthenticatorActivity.QUIT_SERVICE:
        Log.d(TAG, "handleMessage QUIT SERVICE");
        setSvcButton();
        checkSiteAdmin(0);
        break;
      case AuthenticatorActivity.CHECK_SIGNON:
        Log.d(TAG, "handleMessage CHECK SIGNON");
        setSvcButton();
        checkSiteAdmin(0);
        if( mBoundService != null ) {
          mUsername = mBoundService.mUsername;
          mDevname = mBoundService.mDevname;
          mPassword = mBoundService.mPassword;
          Log.d(TAG, "username is " + mUsername);
        }
        break;
      case ClientService.WAKEUP:
        Log.d(TAG, "DapWakeup");
        long t = SystemClock.uptimeMillis();
        mHandler.sendEmptyMessageAtTime(ClientService.WAKEUP, t + LOOP_TIME);
        if( mBoundService != null ) {
          mBoundService.doWakeup();
        }
        break;
    }
    return true;
  }

  public void deleteClicked(View view) {
    showDialog(1);
  } 

  public void companySelect1(String companyid) {
    Log.d(TAG, "companySelect1 " + companyid);
    LinearLayout ll2 = new LinearLayout(this);
    ll2.setOrientation(LinearLayout.VERTICAL);

    setContentView(R.layout.dapmain);

    setSvcButton();

    if( mJson.length() > 0 ) { 
      Log.d(TAG, "check json: " + mJson);
      try {
        JSONObject oneObject = new JSONObject(mJson);

        Log.d(TAG, "object size " + oneObject.length());

        JSONObject cObject = oneObject.getJSONObject("bunsc");

        JSONObject company = cObject.getJSONObject(companyid);

        JSONArray buns = company.names();

        Log.d(TAG, "have id " + companyid + " len " + buns.length());

        Button jsonbuttons[] = new Button[buns.length()];

        for( int j = 0; j < buns.length(); j++ ) {
          String name = buns.getString(j);
          Log.d(TAG, "have name " + name );
          jsonbuttons[j] = new Button(this);
          jsonbuttons[j].setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
              Log.d(TAG, "clicked json button " + name + " id " + companyid);
              companySelect2(companyid, name);
            }
          });
          jsonbuttons[j].setText(name);
          jsonbuttons[j].setTag(companyid);
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

  public void siteAdmin(String siteid) {
    Log.d(TAG, "start site button");
    Intent intentnew = new Intent( this, SiteActivity.class );
    intentnew.putExtra("siteid", siteid);

    startActivity(intentnew);
  } 

  public void companySelect2(String companyid, String bun) {
    Log.d(TAG, "start company button " + companyid + " " + bun);
    if( bun.equals( "dfn" ) ) {
      Intent intentnew = new Intent( this, SiteActivity.class );
      intentnew.putExtra("companyid", companyid);
      intentnew.putExtra("bun", bun);

      startActivity(intentnew);
    } 
    else
    if( bun.equals("dio") ) {
      Intent intentnew = new Intent( this, DioActivity.class );
      intentnew.putExtra("companyid", companyid);
      intentnew.putExtra("bun", bun);

      startActivity(intentnew);
    } 
    else
    if( bun.equals("decho") ) {
      Intent intentnew = new Intent( this, DechoActivity.class );
      intentnew.putExtra("companyid", companyid);
      intentnew.putExtra("bun", bun);

      startActivity(intentnew);
    } 
  } 

  public void onSvcToggleClicked(View view) {
    // Is the toggle on?
    boolean on = ((ToggleButton) view).isChecked();
      
    if (on) {
      Log.d(TAG, "its on");
      startAuthService();
    } else {
      Log.d(TAG, "its off");
      stopAuthService();
    }
  }

  @Override
  protected Dialog onCreateDialog(int id) {
    Log.d(TAG, "onCreateDialog " + id);
    switch (id) {
      case 1:
          return new AlertDialog.Builder(DapActivity.this)
              .setIconAttribute(android.R.attr.alertDialogIcon)
              .setTitle(R.string.delete_button_title)
              .setPositiveButton(R.string.delete_dialog_ok, new DialogInterface.OnClickListener() {
                  public void onClick(DialogInterface dialog, int whichButton) {

                      Log.d(TAG, "clicked OK");
                      deleteAccount();
                  }
              })
              .setNegativeButton(R.string.delete_dialog_cancel, new DialogInterface.OnClickListener() {
                  public void onClick(DialogInterface dialog, int whichButton) {

                      Log.d(TAG, "clicked not OK");
                  }
              })
              .create();
    }
    return null;
  }

  protected void deleteAccount() {
    stopAuthService();

    AccountManager am = AccountManager.get(this);

    Account accounts[] = am.getAccountsByType(Constants.ACCOUNT_TYPE);

    if( accounts.length > 0 ) {
      AccountManagerFuture amf = null;

      am.removeAccount(accounts[0], null, null);

      Log.d(TAG, "account removed");
    }
    toggleState = false;
    setContentView(R.layout.dapmain);
    ToggleButton button = (ToggleButton) findViewById(R.id.svctogglebutton);
    button.setChecked(false);
  }

  public void stopAuthService() {
    Log.d(TAG, "stopping Auth Service");
    if( mBoundService != null && mBoundService.getSignedOn() )
      mBoundService.signoff();
  }

  public void startAuthService() {
    Log.d(TAG, "starting Auth Service " + mUsername );

        /* requestWindowFeature(Window.FEATURE_LEFT_ICON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

        WindowManager mWindowManager = (WindowManager)getSystemService(Context.WINDOW_SERVICE);

        getWindow().setFeatureDrawableResource(
                Window.FEATURE_LEFT_ICON, android.R.drawable.ic_dialog_alert);

        setContentView(R.layout.login_activity);

        EditText mUsernameEdit = (EditText) findViewById(R.id.username_edit);
        if( mUsername != null && mUsername.length() > 0 )
          mUsernameEdit.setText(mUsername);
        EditText mDevnameEdit = (EditText) findViewById(R.id.devname_edit);
        if( mDevname != null && mDevname.length() > 0 )
          mDevnameEdit.setText(mDevname);
        EditText mPasswordEdit = (EditText) findViewById(R.id.password_edit);
        if( mPassword != null && mPassword.length() > 0 )
          mPasswordEdit.setText(mPassword);
        else
        if( mBoundService != null && mBoundService.mPassword != null )
          mPasswordEdit.setText(mBoundService.mPassword);

*/
    Intent intent = new Intent(this, AuthenticationService.class);

    intent.putExtra("dapMain", 1);
    intent.addFlags(
        Intent.FLAG_ACTIVITY_NEW_TASK |
        Intent.FLAG_ACTIVITY_CLEAR_TASK |
        Intent.FLAG_ACTIVITY_SINGLE_TOP
    );

    registerBroadcastReceiver();

    AccountManager am = AccountManager.get(this);

    Account accounts[] = am.getAccountsByType(Constants.ACCOUNT_TYPE);

    String authTokenType = new String(Constants.AUTHTOKEN_TYPE);
    if( accounts.length > 0 ) {
      Log.d(TAG, "GOT accounts " + accounts.length + " " + accounts[0]);

      //am.invalidateAuthToken(Constants.ACCOUNT_TYPE, "hi there");
    } else {
      Log.d(TAG, "starting authenicator to create new account");

      Intent intentnew = new Intent( this, AuthenticatorActivity.class );
      intentnew.putExtra(AuthenticatorActivity.PARAM_NEW_ACCOUNT, true);
      if( mUsername.length() > 0 )
        intentnew.putExtra(AuthenticatorActivity.PARAM_USERNAME, mUsername);
      if( mDevname.length() > 0 )
        intentnew.putExtra(AuthenticatorActivity.PARAM_DEVNAME, mDevname);
      if( mPassword.length() > 0 )
        intentnew.putExtra(AuthenticatorActivity.PARAM_PASSWORD, mPassword);

      startActivityForResult(intentnew, 0);

      return;
    }
    AccountManagerFuture<Bundle> amf = null;

    AccountManagerCallback<Bundle> cb = new AccountManagerCallback<Bundle>() {
      @Override public void run(final AccountManagerFuture<Bundle> arg0) {
        Log.d(TAG, "in callback");
        try {
          Bundle b = new Bundle();
          //String authToken = arg0.getResult().get(AccountManager.KEY_AUTHTOKEN); // this is your auth token
          b = arg0.getResult();
          Log.d(TAG, "got result " + b);
          Intent t = null;
          t = b.getParcelable(AccountManager.KEY_INTENT);
          t.putExtra(AuthenticatorActivity.PARAM_NEW_ACCOUNT, false);
          if( t != null )
            startActivityForResult(t, START_AUTH);
        } catch (Exception e) {
          // handle error
          Log.d(TAG, "bad getResult ");
          e.printStackTrace();
        }
      }
    };

    amf = am.getAuthToken(accounts[0], authTokenType, null, true, cb, null);
    
    Log.d(TAG, "getAuthToken() received bundle " + amf + " isdone " + amf.isDone());
  }

  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    Log.d(TAG, "onActivityResult " + resultCode);
    //if (requestCode == START_AUTH ) {
      setSvcButton();
    //}
    unregisterBroadcastReceiver();
    super.onActivityResult(requestCode, resultCode, data);
  }

  private ServiceConnection mConnection = new ServiceConnection() {
     public void onServiceConnected(ComponentName className, IBinder service) {
       Log.d(TAG, "new service connection " + className + " service " + service);
       mBoundService = ((ClientService.LocalBinder)service).getService();
       mBoundService.setContext( ctx );
       mBoundService.setDapHandler( mHandler );
       mBoundService.setDapActivity( mDapActivity );

       Log.d(TAG, "new service connection " + mBoundService);
       setSvcButton();
       checkSiteAdmin(0);
     }
     public void onServiceDisconnected(ComponentName className) {
       Log.d(TAG, "service disconnection " + mBoundService);
       setSvcButton();
       checkSiteAdmin(0);
     }
  };

  void doUnbindService() {
    Log.d(TAG, "unbinding client service");
    if( mBoundService != null ) {
      unbindService(mConnection);
      mBoundService = null;
    }
  }

  void doBindService() {
    Log.d(TAG, "binding client service");
    bindService(new Intent(this, ClientService.class), mConnection, Context.BIND_AUTO_CREATE);
    Log.d(TAG, "binding client service is " + mConnection);
  }

  public class LocalBinder extends Binder {
    public DapActivity getService() {
      Log.d(TAG, "get local binder");
      return DapActivity.this;
    }
  }

  private void setSvcButton() {
    Log.d(TAG, "setSvcButton " + mBoundService);
    setContentView(R.layout.dapmain);
    ToggleButton button = (ToggleButton) findViewById(R.id.svctogglebutton);
    if( mBoundService == null ) {
      Log.d(TAG, "starting service");
      doBindService();
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

  private void registerBroadcastReceiver() {
    // Create a filter with the broadcast intents we are interested in.
    IntentFilter filter = new IntentFilter();
    filter.addAction(AccountManager.LOGIN_ACCOUNTS_CHANGED_ACTION);
    registerReceiver(mBroadcastReceiver, filter, null, null);
  }

  private void unregisterBroadcastReceiver() {
    try {
      unregisterReceiver(mBroadcastReceiver);
    } catch (IllegalArgumentException e) {
      Log.d(TAG, "bad getResult ");
      e.printStackTrace();
    }
  }

 @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
    saveMyState(outState);
  }

  public void saveMyState(Bundle outState) {
    outState.putString("username", mUsername);
    outState.putString("devname", mDevname);
    outState.putString("password", mPassword);
  }
}

