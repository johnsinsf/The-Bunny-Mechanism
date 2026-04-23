/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.dwanta.android.dap.authenticator;

import android.app.Activity;

import android.os.Messenger;
import android.os.RemoteException;
import android.content.ServiceConnection;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.FrameLayout;
import android.widget.ViewFlipper;
import android.view.ViewGroup;
import android.graphics.Color;
import android.widget.Toast;
import android.os.IBinder;
import android.content.ComponentName;

import com.dwanta.android.dap.Constants;
import com.dwanta.android.dap.R;
import com.dwanta.android.dap.client.ClientService;
import com.dwanta.android.dap.client.dpsmsg;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.provider.ContactsContract;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.TextView;
import java.net.InetAddress;
import javax.net.ssl.SSLSocket;

import android.content.Context;

import com.dwanta.android.dap.client.ClientService.MessageTarget;
import com.dwanta.android.dap.client.ClientServiceActivities.Controller;

/**
 * Activity which displays login screen to the user.
 */
public class AuthenticatorActivity extends AccountAuthenticatorActivity implements
    Handler.Callback, MessageTarget {
   
    public static final String PARAM_NEW_ACCOUNT = "newAccount";
    public static final String PARAM_USERNAME    = "username";
    public static final String PARAM_DEVNAME     = "devname";
    public static final String PARAM_PASSWORD    = "password";
    public static final String PARAM_AUTHTOKEN_TYPE = "authtokenType";
    public static final String PARAM_CONFIRM_CREDENTIALS = "confirmCredentials";

    public static final String PKCS12_PASSWORD = "changeit";
    public static final String PKCS12_FILENAME = "keychain.p12";

    public static final int MESSAGE_READ      = 0x400 + 1;
    public static final int MY_HANDLE         = 0x400 + 2;
    public static final int SIGNON_DONE       = 0x400 + 3;
    public static final int SIGNON_FAIL       = 0x400 + 4;
    public static final int PROMPT_REQUEST    = 0x400 + 5;
    public static final int QUIT_SERVICE      = 0x400 + 6;
    public static final int SIGNON_ALREADY_ON = 0x400 + 7;
    public static final int CHECK_SIGNON      = 0x400 + 8;
    public static final int INVALIDATE_AUTHTOKEN = 0x400 + 9;

    private static final String TAG = "AuthenticatorActivity";
    private AccountManager mAccountManager;

    /** Keep track of the login task so can cancel it if requested */
    private UserLoginTask mAuthTask = null;

    /** Keep track of the progress dialog so we can dismiss it */
    private ProgressDialog mProgressDialog = null;

    /**
     * If set we are just checking that the user knows their credentials; this
     * doesn't cause the user's password or authToken to be changed on the
     * device.
     */
    private Boolean mConfirmCredentials = false;

    /** for posting authentication attempts back to UI thread */

    private Handler   mHandler = new Handler(this);
    private TextView  mMessage;
    private String    mPassword;
    private EditText  mPasswordEdit;
    private EditText  mDevnameEdit;
    private EditText  mUsernameEdit;
    private String    mUsername;
    private String    mDevname;
    private String    mAuthToken;
    protected boolean mRequestNewAccount = false;
    private TextView  statusTxtView;
    private WindowManager mWindowManager;
    private ClientService mBoundService;
    private Context   ctx = this;
    private Boolean   quitService = false;

    public Handler getHandler() {
        return mHandler;
    }

    public TextView getStatusTxtView() {
        return statusTxtView;
    }

    public void setHandler(Handler handler) {
        this.mHandler = handler;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        mAccountManager = AccountManager.get(this);

        final Intent intent = getIntent();

        mUsername = intent.getStringExtra(PARAM_USERNAME);
        mPassword = intent.getStringExtra(PARAM_PASSWORD);
        mDevname = intent.getStringExtra(PARAM_DEVNAME);
        mRequestNewAccount = intent.getBooleanExtra(PARAM_NEW_ACCOUNT, false);

        mConfirmCredentials = intent.getBooleanExtra(PARAM_CONFIRM_CREDENTIALS, false);

        Log.d(TAG, "request new: " + mRequestNewAccount + " " + mUsername + " " + mDevname + " " + mPassword);

        requestWindowFeature(Window.FEATURE_LEFT_ICON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

        mWindowManager = (WindowManager)getSystemService(Context.WINDOW_SERVICE);

        getWindow().setFeatureDrawableResource(
                Window.FEATURE_LEFT_ICON, android.R.drawable.ic_dialog_alert);

        setContentView(R.layout.login_activity);

        mUsernameEdit = (EditText) findViewById(R.id.username_edit);
        if( mUsername != null && mUsername.length() > 0 )
          mUsernameEdit.setText(mUsername);
        mDevnameEdit = (EditText) findViewById(R.id.devname_edit);
        if( mDevname != null && mDevname.length() > 0 )
          mDevnameEdit.setText(mDevname);
        mPasswordEdit = (EditText) findViewById(R.id.password_edit);
        if( mPassword != null && mPassword.length() > 0 )
          mPasswordEdit.setText(mPassword);
        else 
        if( mBoundService != null && mBoundService.mPassword != null )
          mPasswordEdit.setText(mBoundService.mPassword);

        if( mBoundService != null ) {
          Log.d(TAG, "shutting service");
          doUnbindService();
          mBoundService = null;
        }
        Intent mStartIntent = new Intent(this, ClientService.class);
        startService(mStartIntent);
        doBindService();
    }

    @Override
    public void onResume() {
      Log.d(TAG, "onResume");
      if( mBoundService == null ) {
        Intent mStartIntent = new Intent(this, ClientService.class);
        startService(mStartIntent);
        doBindService();
      }
      super.onResume();
    }

    private ServiceConnection mConnection = new ServiceConnection() {
       public void onServiceConnected(ComponentName className, IBinder service) {
         Log.d(TAG, "new service connection " + className + " service " + service);
         mBoundService = ((ClientService.LocalBinder)service).getService();
         mBoundService.setContext( ctx ); 
         Log.d(TAG, "new service connection " + mBoundService);
       }
       public void onServiceDisconnected(ComponentName className) {
         Log.d(TAG, "service disconnection " + mBoundService);
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

    @Override
    protected void onStop() {
      Log.d(TAG, "onStop");
      doUnbindService();
      super.onStop();
    }

    @Override
    protected void onDestroy() {
      Log.d(TAG, "onDestroy");
      hideProgress();
      super.onDestroy();
      getWindow().closeAllPanels();
      //mWindowManager.removeView(mDialogText);
      //if( mAuthToken != "" )
        //mAccountManager.invalidateAuthToken(Constants.ACCOUNT_TYPE, mAuthToken);
    }

    @Override
    protected Dialog onCreateDialog(int id, Bundle args) {
      Log.d(TAG, "onCreateDialog " + id);
      final ProgressDialog dialog = new ProgressDialog(this);
      dialog.setMessage(getText(R.string.ui_activity_authenticating));
      dialog.setIndeterminate(true);
      dialog.setCancelable(true);
      dialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
        public void onCancel(DialogInterface dialog) {
          Log.i(TAG, "user cancelling authentication");
          if (mAuthTask != null) {
            mAuthTask.cancel(true);
          }
        }
      });
      mProgressDialog = dialog;
      return dialog;
    }

    public void cancelLogin(View view) {
      Log.d(TAG, "user clicked cancel");
      doUnbindService();
      super.finish();
    }

    public void handleLogin(View view) {
        Log.d(TAG, "handleLogin " + mPassword);

        //if( mPassword == null || mPassword.length() == 0 ) {
        {
          mPasswordEdit = (EditText) findViewById(R.id.password_edit);
          mPassword = mPasswordEdit.getText().toString();
          Log.d(TAG, "pass " + mPassword);
        }
        //if( mUsername == null || mUsername.length() == 0 ) {
        {
          mUsernameEdit = (EditText) findViewById(R.id.username_edit);
          mUsername = mUsernameEdit.getText().toString();
          Log.d(TAG, "name " + mUsername);
        }
        //if( mDevname == null || mDevname.length() == 0 ) {
        {
          mDevnameEdit = (EditText) findViewById(R.id.devname_edit);
          mDevname = mDevnameEdit.getText().toString();
          Log.d(TAG, "dev name " + mDevname);
        }

        Log.d(TAG, "user " + mUsername + " " + mPassword + " " + mDevname);

        if (TextUtils.isEmpty(mUsername) || TextUtils.isEmpty(mPassword) || TextUtils.isEmpty(mDevname)) {
            setContentView(R.layout.login_activity);
            mMessage = (TextView) findViewById(R.id.message);
            mMessage.setText("please enter user/pass and device.");
            Log.d(TAG, "message displayed");

            mUsernameEdit = (EditText) findViewById(R.id.username_edit);
            if( mUsername != null && mUsername.length() > 0 )
              mUsernameEdit.setText(mUsername);
            mDevnameEdit = (EditText) findViewById(R.id.devname_edit);
            if( mDevname != null && mDevname.length() > 0 )
              mDevnameEdit.setText(mDevname);
            if( mPassword != null && mPassword.length() > 0 )
              mPasswordEdit.setText(mPassword);
            else 
            if( mBoundService != null && mBoundService.mPassword != null )
              mPasswordEdit.setText(mBoundService.mPassword);
        } else {
            // Show a progress dialog, and kick off a background task to perform
            // the user login attempt.
            //showProgress();
            mAuthTask = new UserLoginTask();
            Log.d(TAG, "service is " + mBoundService);
            //mBoundService.setContext(this);
            mAuthTask.addStatusTxtView(statusTxtView);
            mAuthTask.execute();
            Log.d(TAG, "executed mAuthTask");
        }
    }

    private void finishConfirmCredentials(boolean result) {
      Log.i(TAG, "finishConfirmCredentials()");

      final Account account = new Account(mUsername, Constants.ACCOUNT_TYPE);

      mAccountManager.setPassword(account, mPassword);

      final Intent intent = new Intent();

      intent.putExtra(AccountManager.KEY_BOOLEAN_RESULT, result);

      setAccountAuthenticatorResult(intent.getExtras());

      setResult(RESULT_OK, intent);

      doUnbindService();
    }

    private void finishLogin(String authToken) {
        mAuthToken = authToken;

        final Account account = new Account(mUsername, Constants.ACCOUNT_TYPE);

        if (mRequestNewAccount) {
          Log.d(TAG, "adding new account");
          mAccountManager.addAccountExplicitly(account, "", null);
        } else {
          Log.d(TAG, "updating account");
          //mAccountManager.setPassword(account, "");
          //mAccountManager.setAuthToken(account, PARM_AUTHTOKEN_TYPE, authToken);
        }
        mAccountManager.setUserData(account, AuthenticatorActivity.PARAM_DEVNAME, mDevname);
        mAccountManager.setAuthToken(account, PARAM_AUTHTOKEN_TYPE, authToken);
 
        Log.d(TAG, "done updating account " + mDevname);

        final Intent intent = new Intent();
        intent.putExtra(AccountManager.KEY_ACCOUNT_NAME, mUsername);
        intent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNT_TYPE);
        setAccountAuthenticatorResult(intent.getExtras());
        setResult(RESULT_OK, intent);

        Log.d(TAG, "update done");

        doUnbindService();

        if( ! isFinishing() )
          finish();
    }

    public void onAuthenticationResult(String authToken) {
        boolean success = ((authToken != null) && (authToken.length() > 0));

        Log.d(TAG, "onAuthenticationResult(" + success + ")");

        // Our task is complete, so clear it out
        mAuthTask = null;

        // Hide the progress dialog
        hideProgress();

        if (success) {
          if (!mConfirmCredentials) {
            finishLogin(authToken);
          } else {
            finishConfirmCredentials(success);
          }
        } else {
          Log.e(TAG, "onAuthenticationResult: failed to authenticate " + mUsername + " " + mDevname + " " + mPassword);
/*
          setContentView(R.layout.login_activity);

          mUsernameEdit = (EditText) findViewById(R.id.username_edit);
          if( mUsername != null && mUsername.length() > 0 )
            mUsernameEdit.setText(mUsername);
          mDevnameEdit = (EditText) findViewById(R.id.devname_edit);
          if( mDevname != null && mDevname.length() > 0 )
            mDevnameEdit.setText(mDevname);
          mPasswordEdit = (EditText) findViewById(R.id.password_edit);
          if( mPassword != null && mPassword.length() > 0 )
            mPasswordEdit.setText(mPassword);
          else 
          if( mBoundService != null && mBoundService.mPassword != null )
            mPasswordEdit.setText(mBoundService.mPassword);
          mMessage = (TextView) findViewById(R.id.message);
          Toast.makeText(this, "sign on failed", Toast.LENGTH_SHORT).show();
          if (mRequestNewAccount) {
            mMessage.setText(getText(R.string.login_activity_loginfail_text_both));
          } else {
            if( mMessage != null )
              mMessage.setText(getText(R.string.login_activity_loginfail_text_pwonly));
          }
*/
        }
        doUnbindService();
     
        final Intent intent = new Intent();
        intent.putExtra(AccountManager.KEY_ACCOUNT_NAME, mUsername);
        intent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNT_TYPE);
        setAccountAuthenticatorResult(intent.getExtras());
        setResult(RESULT_CANCELED, intent);

        Log.d(TAG, "onAuthenticationResult done");
        if( ! isFinishing() )
          finish();
    }

    public void onAuthenticationCancel() {
        Log.i(TAG, "onAuthenticationCancel()");

        mAuthTask = null;

        hideProgress();

        doUnbindService();

        final Intent intent = new Intent();
        intent.putExtra(AccountManager.KEY_ACCOUNT_NAME, mUsername);
        intent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNT_TYPE);
        setAccountAuthenticatorResult(intent.getExtras());
        setResult(RESULT_CANCELED, intent);

        finish();
    }

    /**
     * Returns the message to be displayed at the top of the login dialog box.
     */
    private CharSequence getMessage() {
        Log.d(TAG, "getMessage");
        getString(R.string.label);
        if (TextUtils.isEmpty(mUsername)) {
            // If no username, then we ask the user to log in using an
            // appropriate service.
            final CharSequence msg = getText(R.string.login_activity_newaccount_text);
            return msg;
        }
        if (TextUtils.isEmpty(mPassword)) {
            // We have an account but no password
            return getText(R.string.login_activity_loginfail_text_pwmissing);
        }
        return null;
    }

    /**
     * Shows the progress UI for a lengthy operation.
     */
    private void showProgress() {
        Log.d(TAG, "showProgress");
        showDialog(0);
    }

    /**
     * Hides the progress UI for a lengthy operation.
     */
    private void hideProgress() {
        Log.d(TAG, "hideProgress");
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
    }

    private void promptRequest(Message msg) {
      dpsmsg m = (dpsmsg)(msg.obj);
      m.setType(dpsmsg.promptResponse);
      //m.set( dpsmsg.pin, "1234" );
      mBoundService.getHandler().obtainMessage(ClientService.DPS_PROMPT_RESPONSE, m).sendToTarget();
      Log.d(TAG, "prompt response sent");
    }

    @Override
    public boolean handleMessage(Message msg) {
          switch (msg.what) {
            case SIGNON_DONE:
                String authtoken = ((ClientService)(msg.obj)).returnToken();
                Log.d(TAG, "handleMessage SIGNON DONE " + authtoken);
                onAuthenticationResult(authtoken);
        
                break;

            case SIGNON_ALREADY_ON:
                Log.d(TAG, "handleMessage SIGNON FAIL ");
                onAuthenticationCancel();
                break;

            case SIGNON_FAIL:
                Log.d(TAG, "handleMessage SIGNON FAIL ");
                onAuthenticationResult("");
                dpsmsg m = (dpsmsg)(msg.obj);
                if( m != null ) {
                  String t1 = "error: " + new String(m.get(dpsmsg.userData));
                  int errcode = m.getInt(dpsmsg.actionCode);
                  if( errcode == dpsmsg.certerror || errcode == dpsmsg.signonerror ) {
                    Toast.makeText(this, t1, Toast.LENGTH_LONG).show();
                    Log.d(TAG, "made toast for " + t1);
                  }
                }
                break;

            case PROMPT_REQUEST:
                Log.d(TAG, "handleMessage prompt " );
                promptRequest(msg);
                break;

            case INVALIDATE_AUTHTOKEN:
                Log.d(TAG, "resetting authtoken");
                mAccountManager.invalidateAuthToken(Constants.ACCOUNT_TYPE, mAuthToken);
                break;

            case QUIT_SERVICE:
                Log.d(TAG, "quitting client service");
                Intent mStopIntent = new Intent(this, ClientService.class);
                //stopService(mStopIntent);
                //doUnbindService();
                break;

            default:
                Log.d(TAG, "handleMessage DEFAULT " + msg);
                break;
        }
        Log.d(TAG, "message done");

        return true;
    }

    /**
     * Represents an asynchronous task used to authenticate a user against the
     * SampleSync Service
     */
    public class UserLoginTask extends AsyncTask<Void, Void, String> {

        private Context ctx;
        private TextView statusTxtView;

        @Override
        protected String doInBackground(Void... params) {

            Log.d(TAG, "Connecting to client service " + mHandler);

            mBoundService.setAuthHandler( mHandler ); 

            mBoundService.getHandler().obtainMessage(ClientService.DPS_SIGNON, this).sendToTarget();

            return "doInBackground done";
        }

        @Override
        protected void onPostExecute(final String authToken) {

            Log.d(TAG, "UserLoginTask.onPostExecute running now " + authToken);
        }

        @Override
        protected void onCancelled() {
            onAuthenticationCancel();
        }

        protected void addContext( Context ctx ) {
          this.ctx = ctx;
        }

        protected void addStatusTxtView( TextView view ) {
          this.statusTxtView = view;
          Log.d(TAG, "adding text view " + this.statusTxtView);
        }

        public String getUsername() {
            return mUsername;
        }

        public String getDevname() {
            return mDevname;
        }

        public String getPassword() {
            return mPassword;
        }
    }
}
