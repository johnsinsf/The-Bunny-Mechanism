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

import android.widget.Button;
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


public class SiteActivity extends Activity  implements
    Handler.Callback, MessageTarget, Parcelable {

  public final static String TAG = "SiteActivity";
  private Context ctx = this;
  private Intent mIntent = null;

  private int mTestInt = 0;
  private int mTestState = 0;
  private String mSelectedProduct = "";
  private String mSelectedSubProduct = "";
  private String mSelectedQty   = "";
  private String mSelectedPrice = "";
  private String mPriceText = "";
  private String mQtyText = "";
  private String mCardText = "";
  private int    mSelectedItem = 0;
  
  public  String mSiteNumber = "";
  public  String mSiteName   = "";
  public  String mCardNumber = "";

  public  ArrayList<String[]> selectedProducts = new ArrayList<String[]>();

  private ArrayList<String> mProducts;
  private ProductAdapter mProductAdapter;
  private Spinner mProductSpinner;

  private Handler mHandler = new Handler(this);  

  public ClientService mBoundService;
 
  IBinder mBinder = new LocalBinder();
  static final Pattern CODE_PATTERN = Pattern.compile("([0-9]{0,4})|([0-9]{4}-)+|([0-9]{4}-[0-9]{0,4})+");
  static final Pattern QTY_PATTERN = Pattern.compile("(^[0-9,-]*\\.[0-9]{0,3}$)|(^[0-9,-]*$)");

  public SiteActivity() {
  }

  protected SiteActivity(Parcel in) {
    mIntent = in.readParcelable(Intent.class.getClassLoader());
    mTestInt = in.readInt();
    mTestState = in.readInt();
    mSelectedProduct = in.readString();
    mSelectedSubProduct = in.readString();
    mSelectedQty = in.readString();
    mSelectedPrice = in.readString();
    mPriceText = in.readString();
    mQtyText = in.readString();
    mCardText = in.readString();
    mSelectedItem = in.readInt();
    mSiteNumber = in.readString();
    mSiteName = in.readString();
    mCardNumber = in.readString();
    mProducts = in.createStringArrayList();
    mBinder = in.readStrongBinder();
  }

  public static final Creator<SiteActivity> CREATOR = new Creator<SiteActivity>() {
    @Override
    public SiteActivity createFromParcel(Parcel in) {
      return new SiteActivity(in);
    }

    @Override
    public SiteActivity[] newArray(int size) {
      return new SiteActivity[size];
    }
  };

  @Override
  public void onCreate(Bundle savedInstanceState) { 
    super.onCreate(savedInstanceState);

    mTestInt = 1;

    mIntent = getIntent();

    Bundle extras = mIntent.getExtras();

    if( mIntent.hasExtra("siteid") ) {
      String[] s = extras.getString("siteid").split(",");
      if( s[0] != null )
        mSiteNumber = s[0];
      if( s[1] != null )
        mSiteName   = s[1];
    }
    if( mIntent.hasExtra("mTestInt") ) {
      mTestInt = extras.getInt("mTestInt");
      //Log.d(TAG, "have mTestInt " + mTestInt);
    }
    Log.d(TAG, "siteid is " + mSiteNumber + " name " + mSiteName + " testint " + mTestInt);
  }
    
  @Override
  public void onStart() {
    //Log.d(TAG, "onStart " + mConnection);
    super.onStart();
    return;
  }
 
  @Override
  public void onResume() {
    //Log.d(TAG, "onResume " + mTestInt + " " + mBoundService);
    if( mBoundService == null ) {
      Intent mStartIntent = new Intent(this, ClientService.class);
      startService(mStartIntent);
      mTestState = 1;
      doBindService();
    } else {
      // Log.d(TAG, "setting up view");
      setView();
    }
    super.onResume();
    return;
  }
 
  public void setView() {
    //Log.d(TAG, "setView " + mTestInt + " " + mPriceText);

    switch( mTestInt ) {
      case 7: setViewSubTot(); break;
      case 6: setViewPrice(); break;
      case 5: setViewQty(); break;
      case 4: subProductView(); break;
      case 3: productView(); break;
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
    outState.putString("mSelectedProduct", mSelectedProduct);
    outState.putString("mSelectedSubProduct", mSelectedSubProduct);
    outState.putString("mSelectedQty",     mSelectedQty);
    outState.putString("mSelectedPrice",   mSelectedPrice);
    outState.putString("mSiteNumber",      mSiteNumber);
    outState.putString("mSiteName",        mSiteName);
    outState.putString("mCardNumber",      mCardNumber);
    outState.putString("mCardText",        mCardText);
    outState.putString("mPriceText",       mPriceText);
    outState.putString("mQtyText",         mQtyText);
    outState.putInt   ("mSelectedItem",    mSelectedItem);
    outState.putInt   ("mTestInt",         mTestInt);
    
    int count = selectedProducts.size();
    String prods = "";
    for( int i = 0; i < count; i++ ) {
      if( prods.length() > 0 ) prods += "|";
      prods += selectedProducts.get(i)[0] + "|";
      prods += selectedProducts.get(i)[1] + "|";
      prods += selectedProducts.get(i)[2];
      //Log.d(TAG, "have prods " + prods);
    }
    outState.putString("selectedProducts", prods);
    //Log.d(TAG, "saveMyState " + mCardText + " " + mSelectedProduct + " " + mSelectedQty);
  }

  @Override
  protected void onRestoreInstanceState(Bundle savedInstanceState) {
    super.onRestoreInstanceState(savedInstanceState);

    //Log.d(TAG, "onRestoreInstanceState " + mTestInt);

    String prods = savedInstanceState.getString("selectedProducts");
    StringTokenizer tokens = new StringTokenizer(prods, "|");
    int numTokens = tokens.countTokens();
    if( numTokens > 0 ) {
      for( int i = 0; i < numTokens; i+=3 ) {
        String[] t = new String[3];
        t[0] = new String(tokens.nextToken());
        t[1] = new String(tokens.nextToken());
        t[2] = new String(tokens.nextToken());
        selectedProducts.add(t);
        //Log.d(TAG, "added product " + t[0]);
      }
    }
    mSelectedProduct = savedInstanceState.getString("mSelectedProduct");
    mSelectedSubProduct = savedInstanceState.getString("mSelectedSubProduct");
    mSelectedQty     = savedInstanceState.getString("mSelectedQty");
    mSelectedPrice   = savedInstanceState.getString("mSelectedPrice");
    mSiteNumber      = savedInstanceState.getString("mSiteNumber");
    mSiteName        = savedInstanceState.getString("mSiteName");
    mCardNumber      = savedInstanceState.getString("mCardNumber");
    mCardText        = savedInstanceState.getString("mCardText");
    mPriceText       = savedInstanceState.getString("mPriceText");
    mQtyText         = savedInstanceState.getString("mQtyText");
    mSelectedItem    = savedInstanceState.getInt("mSelectedItem");
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

  public void addProduct(View view) {
    //Log.d(TAG, "addproductView");
    productView();
  }

  private void productView() {
    //Log.d(TAG, "productView");
    setContentView(R.layout.siteprod);
    mTestInt = 3;
    if( mBoundService.mCardId.length() > 0 && mBoundService.mCardId != mCardNumber ) {
      //Log.d(TAG, "new card number " + mBoundService.mCardId);
      mCardNumber = mBoundService.mCardId;
    }
    TextView textview =  (TextView)findViewById(R.id.embosstext);
    textview.setText("Card: " + mBoundService.mCardId + " " + mBoundService.mEmboss1);
    mProductSpinner = (Spinner) findViewById(R.id.productSpinner);
    mProducts = new ArrayList<String>();

    int cardProdCount = mBoundService.cardProductMap.size();
    int siteProdCount = mBoundService.siteProductMap.size();

    String t[];
    int prodCount = 0;
    if( mBoundService.cardProductMap.get("0") == null ) {
      t = new String[cardProdCount];
      prodCount = cardProdCount;
      Iterator it = mBoundService.cardProductMap.entrySet().iterator();
      int x = 0;
      while (it.hasNext()) {
        Map.Entry pairs = (Map.Entry)it.next();
        String prodid = pairs.getKey().toString();
        String prodVals = (String)(pairs.getValue());
        //Log.d(TAG, "card map is " + pairs.getKey() + " = " + pairs.getValue() + " prodid " + prodid );
        String[] sp = mBoundService.siteProductMap.get(prodid);
        if( sp != null ) {
          //Log.d(TAG, "sp " + sp[0] + " " + sp[1]);     
          t[x++] = prodid + ", " + sp[0] + " " + sp[1];
        }
      }
    } else {
      t = new String[siteProdCount];
      prodCount = siteProdCount;
      Iterator it = mBoundService.siteProductMap.entrySet().iterator();
      int x = 0;
      while (it.hasNext()) {
        Map.Entry pairs = (Map.Entry)it.next();
        String prodid = pairs.getKey().toString();
        String[] prodVals = (String[])(pairs.getValue());
        String[] sp = mBoundService.siteProductMap.get(prodid);
        //Log.d(TAG, "site map is " + pairs.getKey() + " = " + prodVals[0] + " prodid " + prodid );
        t[x++] = prodid + ", " + sp[0] + " " + sp[1];
      }
    }
    Arrays.sort(t);
    for( int i = 0; i < prodCount; i++ ) {
      mProducts.add(t[i]);
    }
    mProductAdapter = new ProductAdapter(this, mProducts);
    //mProductAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    mProductSpinner.setAdapter(mProductAdapter);
    if( mSelectedItem > 0 )
      mProductSpinner.setSelection(mSelectedItem);

    mProductSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        Object item = parent.getItemAtPosition(pos);
        String selected = (String) item;
        //Log.d(TAG, "Have item " + selected + " pos " + pos);
        mSelectedItem = pos;
        //((TextView) parent.getChildAt(0)).setTextColor(Color.BLUE);
        //((TextView) parent.getChildAt(0)).setTextSize(5);
      }
      @Override
      public void onNothingSelected(AdapterView<?> parent) {
      }
    });
  }

  private void subProductView() {
    //Log.d(TAG, "subproductView");
    setContentView(R.layout.sitesubprod);
    mTestInt = 4;
    TextView textview =  (TextView)findViewById(R.id.embosstext);
    textview.setText("Card: " + mBoundService.mCardId + " " + mBoundService.mEmboss1);
    mProductSpinner = (Spinner) findViewById(R.id.productSpinner);
    mProducts = new ArrayList<String>();
    int siteProdCount = mBoundService.siteProductMap.size();

    String t[] = new String[siteProdCount + 1];

    t[0] = "none";
    for( int i = 1; i < siteProdCount + 1; i++ ) 
      t[i] = "yum";

    Iterator it = mBoundService.siteProductMap.entrySet().iterator();
    int x = 1;
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      String prodid = pairs.getKey().toString();
      String[] prodVals = (String[])(pairs.getValue());
      //Log.d(TAG, "testing " + prodid + " " + prodVals[5]);
      if( prodid.equals(mSelectedProduct) && prodVals[5].length() > 0 && ! prodVals[5].equals(" ") ) {
        t[x] = prodVals[5];
        //Log.d(TAG, "found " + t[x]);
        x++;
      }
    }
    //Log.d(TAG, "array is " + t);
    Arrays.sort(t);
    for( int i = 0; i < x; i++ ) {
      mProducts.add(t[i]);
    }
    mProductAdapter = new ProductAdapter(this, mProducts);
    mProductSpinner.setAdapter(mProductAdapter);
    if( mSelectedItem > 0 )
      mProductSpinner.setSelection(mSelectedItem);

    mProductSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        Object item = parent.getItemAtPosition(pos);
        String selected = (String) item;
        //Log.d(TAG, "Have item " + selected + " pos " + pos);
        mSelectedItem = pos;
      }
      @Override
      public void onNothingSelected(AdapterView<?> parent) {
      }
    });
  }
  private void handleTranView() {
    //Log.d(TAG, "handleTranView");
    setContentView(R.layout.sitecard);
    mTestInt = 2;
    mCardNumber = "";
    EditText card = (EditText)findViewById(R.id.carddpsid_id);
    //Log.d(TAG, "mCardText " + mCardText);
    if( mCardText != "" )
      card.setText(mCardText);

    card.addTextChangedListener( new TextWatcher() {
      public void afterTextChanged(Editable s) {
        mCardText = s.toString();
        //Log.d(TAG, "card text has been changed " + mCardText);
      }
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
      public void onTextChanged(CharSequence s, int start, int before, int count) {}
    });
  }

  public void checkCard(View view) {
    //Log.d(TAG, "checkCard");
    EditText carddpsid = (EditText) findViewById(R.id.carddpsid_id);
    selectedProducts = new ArrayList<String[]>();
    if( mBoundService != null ) {
      Log.d(TAG, "checking card " + carddpsid.getText().toString());
      mBoundService.getHandler().obtainMessage(ClientService.CARD_MESSAGE, carddpsid.getText().toString()).sendToTarget();
    }
  }

  public void checkProduct(View view) {
    String selected = (String) mProductSpinner.getSelectedItem();
    Log.d(TAG, "checkProduct " + selected);
    if( selected == null || selected == "" )
      return;
    StringTokenizer tokens = new StringTokenizer(selected, ",");
    int numTokens = tokens.countTokens();
    int x = 0;
    if( numTokens > 0 ) {
      mSelectedProduct  = tokens.nextToken();
    } else {
      mSelectedProduct = "";
    }
    //Log.d(TAG, "selected " + mSelectedProduct + " qty " + mQtyText);
    if( mBoundService.siteProductMap.get(mSelectedProduct) != null ) {

      Iterator it = mBoundService.siteProductMap.entrySet().iterator();
      int subprods = 0;
      while (it.hasNext()) {
        Map.Entry pairs = (Map.Entry)it.next();
        String prodid = pairs.getKey().toString();
        String[] prodVals = (String[])(pairs.getValue());
        Log.d(TAG, "testing subproduct " + prodVals[5] + " prodid " + prodid + " sel " + mSelectedProduct);
        if( prodid.equals( mSelectedProduct ) ) {
          if( prodVals[5].length() > 0 && ! prodVals[5].equals(" ") ) {
            subprods++;
            Log.d(TAG, "found subproduct " + prodVals[5] + " " + prodVals[5].length());
          }
        }
      }
      Log.d(TAG, "subprods " + subprods);
      if( subprods == 0 )
        setViewQty();
      else
        subProductView();
    }
  }

  public void checkSubProduct(View view) {
    String selected = (String) mProductSpinner.getSelectedItem();
    //Log.d(TAG, "checkSubProduct " + selected);
    StringTokenizer tokens = new StringTokenizer(selected, ",");
    int numTokens = tokens.countTokens();
    int x = 0;
    if( numTokens > 0 ) {
      mSelectedSubProduct  = tokens.nextToken();
    } else {
      mSelectedSubProduct = "";
    }
    //Log.d(TAG, "selected " + mSelectedSubProduct);
    if( mBoundService.siteProductMap.get(mSelectedProduct) != null ) {

      setViewQty();
    }
  }

  public void setViewQty() {
    setContentView(R.layout.siteqty);
    String[] t = mBoundService.siteProductMap.get(mSelectedProduct);
    mTestInt = 5;
    TextView textview = (TextView)findViewById(R.id.embosstext);
    //textview.setText(mBoundService.mEmboss1);
    textview.setText("Card: " + mBoundService.mCardId + " " + mBoundService.mEmboss1);
    textview = (TextView)findViewById(R.id.producttext);
    textview.setText(t[0]);
    String unitid = t[3];
    if( mBoundService.siteProductUnitMap.get(unitid) != null ) {
      t = mBoundService.siteProductUnitMap.get(unitid);
      textview = (TextView)findViewById(R.id.addqtyunit);
      textview.setText(t[0]);
      //Log.d(TAG, "found unit " + t[0] + " " + t[1]);
    }

    final EditText editText = (EditText) findViewById(R.id.productqty);
    if( mQtyText != "" )
      editText.setText(mQtyText);

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
        } else {
          mQtyText = s.toString();
        }
        //Log.d(TAG, "new qty " + mQtyText);
      }
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
      }
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }
    });
  }

  public void postTran(View view) {
    Log.d(TAG, "postTran");
    if( mBoundService != null ) {
      mBoundService.getHandler().obtainMessage(ClientService.TRAN_MESSAGE, this).sendToTarget();
    }
  }

  public void cancelTran(View view) {
    //Log.d(TAG, "cancelTran");
    mTestInt = 0;
    selectedProducts.clear();
    finish();
  }

  public void checkPrice(View view) {
    EditText editprice = (EditText)findViewById(R.id.productprice);
    //Log.d(TAG, "checkPrice " + editprice.getText().toString() );

    mSelectedPrice = editprice.getText().toString();
    if( mSelectedPrice  == "" ) {
      Log.d(TAG, "bad price ");
      return;
    }
    float m = new Float(mSelectedQty).floatValue();
    if( m == 0 ) {
      Log.d(TAG, "bad price " + m);
      return;
    }

    String[] t = new String[3];
    t[0] = new String(mSelectedProduct);
    t[1] = new String(mSelectedQty);
    t[2] = new String(mSelectedPrice);
    selectedProducts.add(t);

    setViewSubTot();
  }

  public void setViewSubTot() {
    String unitid = "";
    String unit = "";
    String product = "";
    String prods = "";
    String prodname = "";
    setContentView(R.layout.sitesubtot);
    mTestInt = 7;
    TextView textview =  (TextView)findViewById(R.id.embosstext);
    textview.setText("Card: " + mBoundService.mCardId + " " + mBoundService.mEmboss1);
    int count = selectedProducts.size();
    //Log.d(TAG, "setViewSubTot " + count);

    LinearLayout ll2 = new LinearLayout(this);
    ll2.setOrientation(LinearLayout.VERTICAL);

    for( int i = 0; i < count; i++ ) {
      String prod = selectedProducts.get(i)[0];
      String qty = selectedProducts.get(i)[1];
      String price = selectedProducts.get(i)[2];

      if( mBoundService.siteProductMap.get(prod) != null ) {
        String[] t = mBoundService.siteProductMap.get(prod);
        unitid = t[3];
        prodname = t[0];
      }
      if( mBoundService.siteProductUnitMap.get(unitid) != null ) {
        String[] t2 = mBoundService.siteProductUnitMap.get(unitid);
        unit = t2[0];
      }
      prods = "Item: " + prodname + " " + qty + " " + unit + " x $" + price + " ";
      TextView tv = new TextView(this);
      tv.setText(prods);
      ll2.addView(tv);
    }

    LinearLayout mAppWidgetFrame2 = (LinearLayout)findViewById(R.id.prodframe);

    mAppWidgetFrame2.addView(ll2, new LinearLayout.LayoutParams(
                      LinearLayout.LayoutParams.MATCH_PARENT,
                      LinearLayout.LayoutParams.WRAP_CONTENT));
  }

  public void checkQty(View view) {
    EditText editqty = (EditText)findViewById(R.id.productqty);
    //Log.d(TAG, "checkQty " + editqty.getText().toString() + " selectedProduct " + mSelectedProduct + " " + mBoundService);

    mSelectedQty = editqty.getText().toString();
    if( mSelectedQty.length() == 0 )
      return;

    float m = new Float(mSelectedQty).floatValue();

    if( mBoundService.siteProductMap.get(mSelectedProduct) != null && m != 0 ) {
      setViewPrice();
    }
  }

  public void setViewPrice() {
    String[] t = mBoundService.siteProductMap.get(mSelectedProduct);
    setContentView(R.layout.siteprice);
    mTestInt = 6;
    TextView textview = (TextView)findViewById(R.id.embosstext);
    textview.setText("Card: " + mBoundService.mCardId + " " + mBoundService.mEmboss1);

    String unitid = t[3];
    String unit = "";
    if( mBoundService.siteProductUnitMap.get(unitid) != null ) {
      String[] t2 = mBoundService.siteProductUnitMap.get(unitid);
      unit = t2[0];
    }
    textview = (TextView)findViewById(R.id.producttext);
    textview.setText("Item: " + t[0] + " " + mSelectedQty + " " + unit);

    final EditText editText = (EditText) findViewById(R.id.productprice);

    if( mPriceText != "" )
      editText.setText(mPriceText);
    else
      editText.setText(t[2]);

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
          mPriceText = newstring;
        } else {
          mPriceText = s.toString();
        }
        //Log.d(TAG, "have price " + mPriceText);
      }
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
      }
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }
    });
  }

  public void handleTran(View view) {
    //Log.d(TAG, "handleTran");
    mTestInt = 2;
    mCardNumber = "";
    handleTranView();
  }

  public Handler getHandler() {
    return mHandler;
  }
 
  @Override
  public boolean handleMessage(Message msg) {
    switch (msg.what) {

      case ClientService.CARD_MESSAGE:
        int actionCode = mBoundService.chat.msg.getInt(dpsmsg.actionCode);
        Log.d(TAG, "handleMessage CARD MESSAGE " + actionCode);
        if( actionCode == 0 ) {
          productView();     
        } else {
          mBoundService.mEmboss1 = "";
          mBoundService.mCardId  = "";
          Toast.makeText(this, R.string.card_invalid, Toast.LENGTH_SHORT).show();
        }
        break;

      case ClientService.SITE_MESSAGE:
        Log.d(TAG, "handleMessage SITE MESSAGE");
        break;

      case ClientService.TRAN_MESSAGE:

        mTestInt = 0;
        Bundle b = new Bundle();
        saveMyState(b);
        
        mIntent.putExtras(b);
        startActivity(mIntent);

        Log.d(TAG, "handleMessage TRAN MESSAGE " + msg + " " + msg.obj);
        dpsmsg m = (dpsmsg)(msg.obj);
        Log.d(TAG, "have type " + m.getType() + " " + m.header );
        int action = m.getInt(dpsmsg.actionCode);
        if( action == 0 ) {
          Toast.makeText(this, "transaction successful!", Toast.LENGTH_SHORT).show();
          selectedProducts = new ArrayList<String[]>();
        } else {
          String response = new String(m.get(dpsmsg.userData));
          //Log.d(TAG, "response " + response + " " + selectedProducts);
          Toast.makeText(this, "transaction failed " + response + ", code " + action, Toast.LENGTH_LONG).show();
          selectedProducts = new ArrayList<String[]>();
        }
        break;
    }
    return true;
  }

  private ServiceConnection mConnection = new ServiceConnection() {
     public void onServiceConnected(ComponentName className, IBinder service) {
       //Log.d(TAG, "new service connection " + className + " service " + service);
       mBoundService = ((ClientService.LocalBinder)service).getService();

       //Log.d(TAG, "new service connection " + mBoundService);
       mBoundService.setSiteHandler(mHandler);
       //Log.d(TAG, "getting site products " + mSiteNumber + " " + mTestState);
       mBoundService.getHandler().obtainMessage(ClientService.SITE_MESSAGE, mSiteNumber).sendToTarget();
       if( mTestState == 1 ) {
         mTestState = 0;
         setView();
       }
     }
     public void onServiceDisconnected(ComponentName className) {
       //Log.d(TAG, "service disconnection " + mBoundService);
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
    public SiteActivity getService() {
      //Log.d(TAG, "get local binder");
      return SiteActivity.this;
    }
  }

    /**
     * Custom adapter used to display account icons and descriptions in the account spinner.
     */
  private class ProductAdapter extends ArrayAdapter<String> {
    public ProductAdapter(Context context, ArrayList<String> productData) {
      //super(context, android.R.layout.simple_spinner_item, productData);
      super(context, R.layout.myspinner, productData);
      setDropDownViewResource(R.layout.product_entry);
    }

    public View getDropDownView(int position, View convertView, ViewGroup parent) {
      // Inflate a view template
      if (convertView == null) {
        LayoutInflater layoutInflater = getLayoutInflater();
        convertView = layoutInflater.inflate(R.layout.product_entry, parent, false);
      }
      TextView firstProductLine = (TextView) convertView.findViewById(R.id.firstProductLine);

      // Populate template
      String data = getItem(position);
      firstProductLine.setText(data);
      return convertView;
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
