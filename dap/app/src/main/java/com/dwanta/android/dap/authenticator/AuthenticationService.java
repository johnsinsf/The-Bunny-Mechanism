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

import static android.os.Build.VERSION_CODES.R;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;

import android.os.RemoteException;
import android.os.Message;
import android.os.Messenger;
import android.os.Handler;
import android.os.IBinder;
import android.os.Binder;

import android.content.ServiceConnection;
import android.content.Context;

import android.widget.Toast;

import java.util.ArrayList;

import android.util.Log;

import android.content.Intent;

import com.dwanta.android.dap.R;

/**
 * Service to handle Account authentication. It instantiates the authenticator
 * and returns its IBinder.
 */
public class AuthenticationService extends Service {

    private static final String TAG = "AuthenticationService";

    private Authenticator mAuthenticator;

    NotificationManager mNM;
    Context ctx;
    private final IBinder mBinder = new LocalBinder();

    @Override
    public void onCreate() {
      Log.v(TAG, "Dap Authentication Service started.");

      mAuthenticator = new Authenticator(this);

      //mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

      //showNotification();
    }

    @Override
    public void onDestroy() {
       Log.v(TAG, "Dap Authentication Service stopped.");
    }

    @Override
    public IBinder onBind(Intent intent) {
       Log.v(TAG, "getBinder()...  returning the AccountAuthenticator binder for intent " + intent);

       int actionCode = 0;

       if( intent.hasExtra("dapMain") ) {
          Log.d(TAG, "returning service binder");
          return mBinder;
       }
       Log.d(TAG, "returning authenticator binder");
       return mAuthenticator.getIBinder();
    }

    /**
     * Show a notification while this service is running.
     */
    private void showNotification() {
        // In this sample, we'll use the same text for the ticker and the expanded notification
/*
        CharSequence text = getText(R.remote_service_started);

        // Set the icon, scrolling text and timestamp
        Notification notification = new Notification(R.icon, text,
                System.currentTimeMillis());

        // The PendingIntent to launch our activity if the user selects this notification
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, Authenticator.class), PendingIntent.FLAG_IMMUTABLE);

        // Set the info for the views that show in the notification panel.
        //notification.setLatestEventInfo(this, getText(R.string.remote_service_label),
          //             text, contentIntent);

        // Send the notification.
        // We use a string id because it is a unique number.  We use it later to cancel.
        mNM.notify(R.string.remote_service_started, notification);
*/
    }

    public AuthenticationService  getService() {
      Log.d(TAG, "get local binder");
      return this;
   }

    public class LocalBinder extends Binder {
        public AuthenticationService getService() {
            Log.d(TAG, "get local binder");
            return AuthenticationService.this;
            //return mAuthenticator.getIBinder();
        }
    }

    public void setContext( Context c ) {
        ctx = c;
        Log.d(TAG, "set context " + ctx);
    }

    public Authenticator getAuthenticator( ) {
      return mAuthenticator;
    }
}
