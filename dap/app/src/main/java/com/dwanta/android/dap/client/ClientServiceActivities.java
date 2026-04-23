package com.dwanta.android.dap.client;

import com.dwanta.android.dap.R;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.IBinder;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.FrameLayout;
import android.widget.EditText;
import android.view.ViewGroup;

import android.util.Log;
import android.widget.Toast;
import android.widget.TextView;
import android.net.Uri;

import com.dwanta.android.dap.client.dpsmsg;

public class ClientServiceActivities {
    final private String TAG = "ClientServiceActivities";

    public static class Controller extends Activity {

        final private String TAG = "ClientServiceActivities Controller";
        private boolean mIsBound;
        private ClientService mBoundService;
        private dpsmsg msg = new dpsmsg();
        private int actionCode = 0;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            Intent intent = getIntent();
            Uri uri = intent.getData();

            Bundle extras = intent.getExtras();

            Log.d(TAG, "on create uri " + uri + " extras " + extras);

            bindService(new Intent(this, ClientService.class), mConnection, Context.BIND_AUTO_CREATE);

            int actionCode = 0;
            if( intent.hasExtra("actionCode") ) {
              actionCode = extras.getInt("actionCode");
            }
            Log.d(TAG, "actionCode is " + actionCode);
            msg.set(dpsmsg.actionCode, actionCode);

            int date = 0;
            if( intent.hasExtra("date") ) {
              date = extras.getInt("date");
            }
            Log.d(TAG, "date is " + date);
            msg.set(dpsmsg.date, date);

            int time = 0;
            if( intent.hasExtra("time") ) {
              time = extras.getInt("time");
            }
            Log.d(TAG, "time is " + time);
            msg.set(dpsmsg.time, time);

            int stan = 0;
            if( intent.hasExtra("stan") ) {
              stan = extras.getInt("stan");
            }
            Log.d(TAG, "stan is " + stan);
            msg.set(dpsmsg.stan, stan);

            int siteid = 0;
            if( intent.hasExtra("siteid") ) {
              siteid = extras.getInt("siteid");
            }
            Log.d(TAG, "siteid is " + siteid);
            msg.set(dpsmsg.siteid, siteid);

            String userData = "";
            if( intent.hasExtra("userData") ) {
              userData = extras.getString("userData");
            }
            Log.d(TAG, "userData is " + userData);
            msg.set(dpsmsg.userData, userData);

            String additionalData = "";
            if( intent.hasExtra("additionalData") ) {
              additionalData = extras.getString("additionalData");
              Log.d(TAG, "additionalData is " + additionalData);
              msg.set(dpsmsg.additionalData, additionalData);
            }
        }

        @Override
        public void onNewIntent(Intent intent){
          Bundle extras = intent.getExtras();
          Log.d(TAG, "new intent " + extras);

          if(extras != null) {
            if(extras.containsKey("actionCode")) {
              Log.d(TAG, "new actioncode" + extras.getInt("actionCode") ); 
              msg.set(dpsmsg.actionCode, extras.getInt("actionCode"));
            }
          }
        }

        @Override
        public void onDestroy() {
            Log.d(TAG, "onDestroy");
            super.onDestroy();

            Log.d(TAG, "unbinding client service");
            if( mBoundService != null ) {
              unbindService(mConnection);
              mBoundService = null;
            }
        }

        private OnClickListener mStartListener = new OnClickListener() {
            public void onClick(View v) {
                promptRequest();
            }
        };

        private OnClickListener mStopListener = new OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "stop listener");
                cancelRequest();
            }
        };

        private void formatPromptView() {
          msg.parsePrompts();
          int numPrompts = msg.getNumPrompts();
          Log.d(TAG, "have " + numPrompts + " prompts");

          for( int i = 0; i < numPrompts; i++ ) {
            LinearLayout ll2 = new LinearLayout(this);
            ll2.setOrientation(LinearLayout.VERTICAL);
            TextView tv2 = new TextView(this);
            tv2.setText(msg.getPromptText(i));
            int p = new Integer(msg.getPromptId(i)).intValue();
            int len = new Integer(msg.getPromptLen(i)).intValue();
            if( len == 0 ) len = 8;

            ll2.addView(tv2);

            Log.d(TAG, "adding " + p + " prompt " + msg.getPromptText(i));

            EditText c2 = new EditText(this);
            c2.setText("");
            c2.setId(p);
            c2.setWidth(len);
            ll2.addView(c2);

            LinearLayout mAppWidgetFrame2 = (LinearLayout)findViewById(R.id.prompt_frame);
      
            mAppWidgetFrame2.addView(ll2, new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT));
          }
        }

        private ServiceConnection mConnection = new ServiceConnection() {
            public void onServiceConnected(ComponentName className, IBinder service) {
                mBoundService = ((ClientService.LocalBinder)service).getService();
                
                actionCode = msg.getInt(dpsmsg.actionCode);

                Log.d(TAG, "service connection " + actionCode + " " + mBoundService);

                if( actionCode == dpsmsg.pinRequired ) {
                  setContentView(R.layout.pin_activity);
                  Log.d(TAG, "set to pin activity layout");
                } 
                else 
                if( actionCode == dpsmsg.confirmRequired ) {
                  setContentView(R.layout.confirm_activity);
                  Log.d(TAG, "set to confirm activity layout2");
                } else {
                  setContentView(R.layout.prompt_activity);
                  Log.d(TAG, "set to prompt activity layout2");
                  formatPromptView();
                }
                Button button = (Button)findViewById(R.id.ok_button);
                Button cancelbutton = (Button)findViewById(R.id.cancel_button);

                if( button != null )
                  button.setOnClickListener(mStartListener);
                if( cancelbutton != null )
                  cancelbutton.setOnClickListener(mStopListener);
                //Toast.makeText(Controller.this, R.string.dap_service_connected,
                        //Toast.LENGTH_SHORT).show();
            }

            public void onServiceDisconnected(ComponentName className) {
                mBoundService = null;
            }
        };

        private void promptRequest() {
          msg.setType(msg.promptResponse);

          actionCode = msg.getInt(dpsmsg.actionCode);

          if( actionCode == dpsmsg.pinRequired ) {
            Log.d(TAG, "set to pin activity layout");
            EditText pinEdit = (EditText)findViewById(R.id.pin_edit);
            if( pinEdit != null ) {
              String pin = pinEdit.getText().toString();
              Log.d(TAG, "have PIN " + pin);
              msg.set(dpsmsg.pin, pin);
            } else {
              Log.d(TAG, "pin is NULL");
            }
          } else {
            Log.d(TAG, "set to prompt activity layout1");
            int numPrompts = msg.getNumPrompts();
            String promptKey = msg.getPromptKey();
            String resp = "";
            if( ( numPrompts > 0 ) && ( promptKey != "0" ) ) {
              resp = promptKey + ",";
              for( int i = 0; i < numPrompts; i++ ) {
                int p = new Integer(msg.getPromptId(i)).intValue();
                Log.d(TAG, "looking for prompt " + p);
                EditText promptEdit = (EditText)findViewById(p);
                String text = "";
                if( promptEdit != null )
                  text = promptEdit.getText().toString();
                else
                  Log.d(TAG, "prompt was null");
                resp += msg.getPromptId(i) + "," + text + ",";
              } 
            } 
            Log.d(TAG, "setting response " + resp);
            msg.set(dpsmsg.additionalData, resp);
          } 

          Message m = mBoundService.getHandler().obtainMessage(ClientService.DPS_PROMPT_RESPONSE, msg);
          mBoundService.getHandler().sendMessageAtFrontOfQueue( m );
          
          Log.d(TAG, "prompt response siteid " + msg.getInt(dpsmsg.siteid) + " " + mBoundService.getHandler());

          finish();
        }

        private void cancelRequest() {
          msg.setType(dpsmsg.promptResponse);
          msg.set( dpsmsg.actionCode, 999);
          msg.set( dpsmsg.pin, "0" );

          Log.d(TAG, "stan is " + msg.getInt( dpsmsg.stan ));

          mBoundService.getHandler().obtainMessage(ClientService.DPS_PROMPT_RESPONSE, msg).sendToTarget();
          
          Log.d(TAG, "cancel response sent " + mBoundService.getHandler());

          finish();
        }

        public void confirmRequest(View v) {
          msg.setType(dpsmsg.promptResponse);
          msg.set( dpsmsg.actionCode, 0);
          msg.set( dpsmsg.origActionCode, 924);

          Log.d(TAG, "stan is " + msg.getInt( dpsmsg.stan ));

          mBoundService.getHandler().obtainMessage(ClientService.DPS_PROMPT_RESPONSE, msg).sendToTarget();
          
          Log.d(TAG, "confirm response sent " + mBoundService.getHandler());

          finish();
        }
    }

    public static class SvcController extends Activity {

        final private String TAG = "ClientServiceActivities SvcController";
        private boolean mIsBound;
        private ClientService mBoundService;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            bindService(new Intent(this, ClientService.class), mConnection, Context.BIND_AUTO_CREATE);
        }

        @Override
        public void onDestroy() {
            Log.d(TAG, "onDestroy");
            super.onDestroy();

            Log.d(TAG, "unbinding client service");

            if( mBoundService != null ) {
              unbindService(mConnection);
              mBoundService = null;
            }
        }

        private OnClickListener mStartListener = new OnClickListener() {
            public void onClick(View v) {
                stopRequest();
            }
        };

        private OnClickListener mStopListener = new OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "stop listener");
                stopRequest();
            }
        };

        private ServiceConnection mConnection = new ServiceConnection() {
            public void onServiceConnected(ComponentName className, IBinder service) {
                mBoundService = ((ClientService.LocalBinder)service).getService();
                
                Log.d(TAG, "service connection " + mBoundService);

                setContentView(R.layout.prompt_activity);

                Button button = (Button)findViewById(R.id.ok_button);
                Button cancelbutton = (Button)findViewById(R.id.cancel_button);

                button.setOnClickListener(mStartListener);
                if( cancelbutton != null )
                  cancelbutton.setOnClickListener(mStopListener);
            }

            public void onServiceDisconnected(ComponentName className) {
                mBoundService = null;
            }
        };

        private void startRequest() {
          Message m = mBoundService.getHandler().obtainMessage(ClientService.DPS_START, this);
          mBoundService.getHandler().sendMessageAtFrontOfQueue( m );
          
          Log.d(TAG, "start request sent " + mBoundService.getHandler());

          finish();
        }

        private void stopRequest() {
          Message m = mBoundService.getHandler().obtainMessage(ClientService.DPS_STOP, this);
          mBoundService.getHandler().sendMessageAtFrontOfQueue( m );
          
          Log.d(TAG, "stop request sent " + mBoundService.getHandler());

          finish();
        }
    }
}
