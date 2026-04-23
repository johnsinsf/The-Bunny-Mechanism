
package com.dwanta.android.dap.client;

import android.util.Log;
import android.os.SystemClock;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;

import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
//import java.io.BufferedWriter;

import java.io.InputStream;
import java.io.OutputStream;

import javax.net.ssl.SSLSocket;

import java.nio.ByteBuffer;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Calendar;
import java.util.StringTokenizer;

public class dpsmsg {

  private static final String TAG = "dpsmsg"; 
  static final int fixed_length   = 0;
  static final int short_length   = 1;
  static final int long_length    = 2;
  static final int vlong_length   = 3;

  public static final int pan            = 2;
  public static final int processCode    = 3;
  public static final int amount         = 4;
  public static final int mystan         = 9;
  public static final int price          = 10;
  public static final int stan           = 11;
  public static final int time           = 12;
  public static final int date           = 13;
  public static final int approvalCode   = 38;
  public static final int actionCode     = 39;
  public static final int siteid         = 42;
  public static final int responseData   = 44;
  public static final int authData       = 47;
  public static final int userData       = 48;
  public static final int currencyCode   = 49;
  public static final int pin            = 52;
  public static final int additionalData = 61;
  public static final int recordNumber   = 62;
  public static final int messageNumber  = 71;
  public static final int origActionCode = 73;
  public static final int passedData     = 115;
  public static final int passedData1    = 123;
  public static final int passedOpt      = 124;
  public static final int passedData2    = 125;
  public static final int passedData3    = 126;
  public static final int passedData4    = 127;

  public static final int authRequest    = 100;
  public static final int authResponse   = 110;
  public static final int tranRequest    = 200;
  public static final int tranResponse   = 210;
  public static final int fileUpdateRequest   = 302;
  public static final int fileUpdateResponse  = 312;
  public static final int dechoResponse    = 978;
  public static final int dechoRequest     = 979;
  public static final int dioResponse    = 980;
  public static final int dioRequest     = 981;
  public static final int siteResponse   = 982;
  public static final int siteRequest    = 983;
  public static final int cardResponse   = 984;
  public static final int cardRequest    = 985;
  public static final int signonAck      = 986;
  public static final int promptRequest  = 987;
  public static final int promptResponse = 988;
  public static final int signonRequest  = 989;
  public static final int signonResponse = 990;
  public static final int logRequest     = 991;
  public static final int logResponse    = 992;
  public static final int codeRequest    = 993;
  public static final int codeResponse   = 994;
  public static final int balanceRequest = 995;
  public static final int balanceResponse= 996;
  public static final int testRequest    = 998;
  public static final int testResponse   = 999;

  public static final int pinRequired    = 112;
  public static final int signonerror    = 916;
  public static final int certerror      = 917;
  public static final int certinit       = 919;
  public static final int certreinit     = 920;
  public static final int tokenreinit    = 921;
  public static final int getProducts    = 922;
  public static final int dapTran        = 923;
  public static final int confirmRequired= 924;
  public static final int dataSend       = 925;
  public static final int getData        = 926;
  public static final int setData        = 927;

  private int  lrc      = 0;
  private int  state    = 0;
  private byte passChar = 0;
  private int  messageType = 0;

  private int numPrompts = 0;
  private String[] promptText;
  private String[] promptType;
  private String[] promptLen;
  private String[] promptId;
  private String   promptKey;

  public HashMap<Integer, Integer> header = new HashMap<Integer, Integer>();
  public HashMap<Integer, byte[]>  fields = new HashMap<Integer, byte[]>();

  private SSLSocket socket;

  private InputStream inputstream;
  private InputStreamReader inputstreamreader;
  //private BufferedReader bufferedreader;

  private OutputStream outputstream;
  private OutputStreamWriter outputstreamwriter;
  //private BufferedWriter bufferedwriter;
  private ByteBuffer outbuf;
  private String lenbuf;

  public final int field_def[][] = {
    { fixed_length, 16, 0 },   /* 0 */
    { fixed_length, 16, 0 },   /* 1 */
    { short_length, 0, 0 },    /* 2 */
    { fixed_length, 6, 0 },    /* 3 */
    { fixed_length, 12, 0 },   /* 4 */
    { fixed_length, 12, 0 },   /* 5 */
    { fixed_length, 12, 0 },   /* 6 */
    { fixed_length, 10, 0 },   /* 7 */
    { fixed_length, 8, 0 },    /* 8 */
    { fixed_length, 8, 0 },    /* 9 */
    { fixed_length, 8, 0 },    /* 10 */
    { fixed_length, 6, 0 },    /* 11 */
    { fixed_length, 6, 0 },    /* 12 */
    { fixed_length, 6, 0 },    /* 13 */
    { fixed_length, 4, 0 },    /* 14 */
    { fixed_length, 4, 0 },    /* 15 */
    { fixed_length, 4, 0 },    /* 16 */
    { fixed_length, 4, 0 },    /* 17 */
    { fixed_length, 4, 0 },    /* 18 */
    { fixed_length, 3, 0 },    /* 19 */
    { fixed_length, 3, 0 },    /* 20 */
    { fixed_length, 3, 0 },    /* 21 */
    { fixed_length, 12, 0 },   /* 22 */
    { fixed_length, 3, 0 },    /* 23 */
    { fixed_length, 3, 0 },    /* 24 */
    { fixed_length, 4, 0 },    /* 25 */
    { fixed_length, 4, 0 },    /* 26 */
    { fixed_length, 1, 0 },    /* 27 */
    { fixed_length, 6, 0 },    /* 28 */
    { fixed_length, 3, 0 },    /* 29 */
    { fixed_length, 24, 0 },   /* 30 */
    { short_length, 0, 0 },    /* 31 */
    { short_length, 0, 0 },    /* 32 */
    { short_length, 0, 0 },    /* 33 */
    { short_length, 0, 0 },    /* 34 */
    { short_length, 0, 0 },    /* 35 */
    { long_length, 0, 0 },     /* 36 */
    { fixed_length, 12, 0 },   /* 37 */
    { fixed_length, 6, 0 },    /* 38 */
    { fixed_length, 3, 0 },    /* 39 */
    { fixed_length, 3, 0 },    /* 40 */
    { fixed_length, 8, 0 },    /* 41 */
    { short_length, 6, 0 },    /* 42 */
    { short_length, 0, 0 },    /* 43 */
    { fixed_length, 25, 1 },   /* 44 */
    { short_length, 0, 0 },    /* 45 */
    { long_length, 0, 0 },     /* 46 */
    { long_length, 0, 0 },     /* 47 */
    { long_length, 0, 0 },     /* 48 */
    { fixed_length, 3, 0 },    /* 49 */
    { fixed_length, 3, 0 },    /* 50 */
    { fixed_length, 3, 0 },    /* 51 */
    { fixed_length, 16, 0 },   /* 52 */
    { short_length, 0, 0 },    /* 53 */
    { long_length, 0, 0 },     /* 54 */
    { long_length, 0, 0 },     /* 55 */
    { short_length, 0, 0 },    /* 56 */
    { fixed_length, 11, 0 },   /* 57 */
    { short_length, 0, 0 },    /* 58 */
    { long_length, 0, 0 },     /* 59 */
    { long_length, 0, 0 },     /* 60 */
    { long_length, 0, 0 },     /* 61 */
    { long_length, 0, 0 },     /* 62 */
    { long_length, 0, 0 },     /* 63 */
    { fixed_length, 8, 0 },    /* 64 */
    { fixed_length, 8, 0 },    /* 65 */
    { long_length, 0, 0 },     /* 66 */
    { fixed_length, 2, 0 },    /* 67 */
    { fixed_length, 3, 0 },    /* 68 */
    { fixed_length, 3, 0 },    /* 69 */
    { fixed_length, 3, 0 },    /* 70 */
    { fixed_length, 4, 0 },    /* 71 */
    { long_length, 0, 0 },     /* 72 */
    { fixed_length, 6, 0 },    /* 73 */
    { fixed_length, 10, 0 },   /* 74 */
    { fixed_length, 10, 0 },   /* 75 */
    { fixed_length, 10, 0 },   /* 76 */
    { fixed_length, 10, 0 },   /* 77 */
    { fixed_length, 10, 0 },   /* 78 */
    { fixed_length, 10, 0 },   /* 79 */
    { fixed_length, 10, 0 },   /* 80 */
    { fixed_length, 10, 0 },   /* 81 */
    { fixed_length, 10, 0 },   /* 82 */
    { fixed_length, 10, 0 },   /* 83 */
    { fixed_length, 10, 0 },   /* 84 */
    { fixed_length, 10, 0 },   /* 85 */
    { fixed_length, 16, 0 },   /* 86 */
    { fixed_length, 16, 0 },   /* 87 */
    { fixed_length, 15, 0 },   /* 88 */
    { fixed_length, 16, 0 },   /* 89 */
    { fixed_length, 10, 0 },   /* 90 */
    { fixed_length, 3, 0 },    /* 91 */
    { fixed_length, 3, 0 },    /* 92 */
    { short_length, 0, 0 },    /* 93 */
    { short_length, 0, 0 },    /* 94 */
    { short_length, 0, 0 },    /* 95 */
    { long_length, 0, 0 },     /* 96 */
    { fixed_length, 17, 0 },   /* 97 */
    { fixed_length, 25, 0 },   /* 98 */
    { short_length, 0, 0 },    /* 99 */
    { short_length, 0, 0 },   /* 100 */
    { short_length, 0, 0 },   /* 101 */
    { short_length, 0, 0 },   /* 102 */
    { short_length, 0, 0 },   /* 103 */
    { long_length, 0, 0 },    /* 104 */
    { fixed_length, 16, 0 },  /* 105 */
    { fixed_length, 16, 0 },  /* 106 */
    { fixed_length, 10, 0 },  /* 107 */
    { fixed_length, 10, 0 },  /* 108 */
    { short_length, 0, 0 },   /* 109 */
    { short_length, 0, 0 },   /* 110 */
    { long_length, 0, 0 },    /* 111 */
    { long_length, 0, 0 },    /* 112 */
    { long_length, 0, 0 },    /* 113 */
    { long_length, 0, 0 },    /* 114 */
    { long_length, 0, 0 },    /* 115 */
    { long_length, 0, 0 },    /* 116 */
    { long_length, 0, 0 },    /* 117 */
    { long_length, 0, 0 },    /* 118 */
    { long_length, 0, 0 },    /* 119 */
    { long_length, 0, 0 },    /* 120 */
    { long_length, 0, 0 },    /* 121 */
    { long_length, 0, 0 },    /* 122 */
    { long_length, 0, 0 },    /* 123 */
    { long_length, 0, 0 },    /* 124 */
    { long_length, 0, 0 },    /* 125 */
    { long_length, 0, 0 },    /* 126 */
    { vlong_length, 0, 0 }     /* 127 */
  };

  public dpsmsg() {
    lrc = 0;
    messageType = 0;
    socket = null;
  }

  public dpsmsg(SSLSocket sock) throws Exception {
    lrc = 0;
    messageType = 0;
    socket = sock;
 
    if( sock != null ) {
      inputstream = socket.getInputStream();
      inputstreamreader = new InputStreamReader(inputstream);
      //bufferedreader = new BufferedReader(inputstreamreader);

      outputstream = socket.getOutputStream();
      outputstreamwriter = new OutputStreamWriter(outputstream);
      //bufferedwriter = new BufferedWriter(outputstreamwriter);
    } else {
      Log.d(TAG, "socket was null");
      throw new java.io.IOException();
    }
  }

  public void setSocket(SSLSocket sock) {
    socket = sock;
    if( sock != null ) {
      try {
        inputstream.close();
        outputstream.close();
        inputstreamreader.close();
        //bufferedreader.close();

        inputstream = socket.getInputStream();
        inputstreamreader = new InputStreamReader(inputstream);
        //bufferedreader = new BufferedReader(inputstreamreader);

        outputstream = socket.getOutputStream();
        outputstreamwriter = new OutputStreamWriter(outputstream);
        //bufferedwriter = new BufferedWriter(outputstreamwriter);
      } catch ( IOException e ) {
        Log.d(TAG, "setSocket err " + e);
      }
    } 
  }

  public boolean set( int id, int value ) {
    byte[] b = new byte[25];
  // String.format(Locale, ...) instead [DefaultLocale]
    String s = String.format("%d", value);
    return this.set( id, s);
  }

  public void setType( int type ) {
    messageType = type;
  }

  public int getType() {
    return messageType;
  }

  public void setState( int s ) {
    state = s;
  }

  public int getState() {
    return state;
  }

  public byte[] getOutbuf() {
    return outbuf.array();
  }

  public byte getPasschar() {
    return passChar;
  }

  public void clear() {
    header.clear();
    fields.clear();
    messageType = 0;
  }

  public boolean set( int id, String value ) {
    //Log.d(TAG, "setting " + id + " to " + value);
    byte[] b = new byte[value.length()];
    strncpy( b, value, value.length(), 0 );
    //Log.d(TAG, "b = " + b);
    return set( id, b );
  }

  public boolean set( int id, byte[] value ) {
    //Log.d(TAG, "setting field " + id + " to " + new String(value));
    if( id < 1 || id > 127 ) {
      Log.d(TAG, "bad id in set " + id);
      return false;
    }
    if( value.length == 0 ) {
      Log.d(TAG, "bad length in set: 0 for " + id);
      return false;
    }
    fields.put(id, value);
    header.put(id - 1, 1);
    if( id > 64 )
      header.put(0, 1);
 
    if( fields.get(id) == value )
      return true;

    Log.d(TAG, "bad set?");
    return false;
  }

  public boolean test( int id ) {
    if( header.get(id - 1) == null ) 
      return false;
    return true;
  }

  public byte[] get( int id ) {
    if( header.get(id - 1) == null ) {
      Log.d(TAG, "get called, not present " + id);
      byte[] b = new byte[1];
      return b;
    }
    return fields.get(id);
  }

  public void setData( dpsmsg m ) {
    for( int i = 3; i < 128; i++ ) {
      if( m.header.get(i - 1) != null && m.header.get(i - 1) == 1 ) {
        fields.put(i, m.fields.get(i));
        header.put(i - 1, 1);
      }
    }
  }

  public int getInt( int id ) {
    if( header.get(id - 1) == null ) {
      Log.d(TAG, "get called, not present " + id);
      return 0;
    }
    byte[] buf = fields.get(id);
    if( buf != null )
      return new Integer(new String(buf, 0, buf.length)).intValue();
    Log.d(TAG, "field " + id + " was null");
    return 0;
  }

  public void addToLRC( byte[] buf, int len ) {
    for( int i = 0; i < len; i++ ) {
      lrc ^= buf[i];
    }
  }

  public boolean readField( int id, int timeOut ) throws Exception {
    //Log.d(TAG, "reading field " + id);
    if( field_def[id][0] == fixed_length ) {
      //Log.d(TAG, "fixed read");
      int len = field_def[id][1];
      byte[] buf = new byte[len];
      int rc = doRead(buf, len, timeOut, false);
      //Log.d(TAG, "fixed read in readField: " + new String(buf));
      if( rc != len ) {
        Log.d(TAG, "bad read in readField on id " + id);
        return false;
      }
      addToLRC(buf, len);
      return set(id, buf);
    }
    else
    if( field_def[id][0] == short_length ) {
      //Log.d(TAG, "short length read");
      byte[] buf1 = new byte[2];
      int rc = doRead(buf1, 2, timeOut, false);
      if( rc != 2 ) {
        Log.d(TAG, "bad len read in readField on id " + id);
        return false;
      }
      addToLRC(buf1, 2);
      int len = new Integer(new String(buf1, 0, 2)).intValue();
      //Log.d(TAG, "short length " + len + " buf1 " + buf1);
      byte[] buf = new byte[len];
      rc = doRead(buf, len, timeOut, false);
      //Log.d(TAG, "read in readField: " + new String(buf));
      if( rc != len ) {
        Log.d(TAG, "bad read in readField on id " + id);
        return false;
      }
      addToLRC(buf, len);
      return set(id, buf);
    }
    else
    if( field_def[id][0] == long_length ) {
      //Log.d(TAG, "long length read");
      byte[] buf1 = new byte[3];
      int rc = doRead(buf1, 3, timeOut, false);
      if( rc != 3 ) {
        Log.d(TAG, "bad len read in readField on id " + id);
        return false;
      }
      addToLRC(buf1, 3);
      int len = new Integer(new String(buf1, 0, 3)).intValue();
      //Log.d(TAG, "long length " + len + " buf1 " + buf1);
      byte[] buf = new byte[len];
      rc = doRead(buf, len, timeOut, false);
      //Log.d(TAG, "read in readField: " + new String(buf));
      if( rc != len ) {
        Log.d(TAG, "bad read in readField on id " + id);
        return false;
      }
      addToLRC(buf, len);
      return set(id, buf);
    }
    else
    if( field_def[id][0] == vlong_length ) {
      //Log.d(TAG, "long length read");
      byte[] buf1 = new byte[6];
      int rc = doRead(buf1, 6, timeOut, false);
      if( rc != 6 ) {
        Log.d(TAG, "bad len read in readField on id " + id);
        return false;
      }
      addToLRC(buf1, 6);
      int len = new Integer(new String(buf1, 0, 6)).intValue();
      //Log.d(TAG, "long length " + len + " buf1 " + buf1);
      byte[] buf = new byte[len];
      rc = doRead(buf, len, timeOut, false);
      //Log.d(TAG, "read in readField: " + new String(buf));
      if( rc != len ) {
        Log.d(TAG, "bad read in readField on id " + id);
        return false;
      }
      addToLRC(buf, len);
      return set(id, buf);
    }
    return false;
  }

  public int doRead( byte[] buf, int len, int timeOut, boolean checkInput ) throws Exception {
    int total  = 0;
    int reads  = 0;
    int rc     = 0;
    byte[] bbuf = new byte[1];

    //long timeNow = System.currentTimeMillis();
    long timeNow = SystemClock.elapsedRealtime(); // in millisecs
    long timeEnd = timeNow + timeOut;
    buf[0] = 0;
    
    Log.d(TAG, "doRead reading " + len + " to " + timeOut);

    //while( ( total != len ) && ( reads < (len + 50) ) ) {
    while( total != len ) {
      reads++;
      rc = 0;
      Log.d(TAG, "doRead reads " + reads);

      //timeNow = System.currentTimeMillis();
      timeNow = SystemClock.elapsedRealtime(); // in millisecs

      long t = 0;

      if( ( timeOut > 0 ) && ( timeNow < timeEnd ) )
        t = timeEnd - timeNow;
      else
      if( timeOut > 0 ) {
        Log.d(TAG, "doRead time expired " );
        if( total > 2 )
          return total * -1;
        return -2;
      }

      int fileReady = 0;
      boolean readtimedout = false;
      try {
        Log.d(TAG, "doRead checking inputstream");
        fileReady = inputstream.read(bbuf);
        Log.d(TAG, "doRead fileReady " + fileReady);
      } catch( java.net.SocketTimeoutException e ) {
        Log.d(TAG, "socket read timeout");
        readtimedout = true;
      } catch( Exception e ) {
        Log.d(TAG, "bad socket read");
        throw new Exception(e);
      }
      if( fileReady > 0 ) {
        buf[total] = bbuf[0];
        total += fileReady; 
        //Log.d(TAG, "doRead char " + bbuf[0]);
      } 
      else
      if( ! readtimedout ) {
        total = -1;
        throw new IOException("Invalid read");
      }
    }
    Log.d(TAG, "doRead total " + total);
    return total;
  }

  public int read( int timeOut ) throws Exception {
    byte[] buf = new byte[256];

    lrc = 0;

    Log.d(TAG, "in msg read");

    buf[0] = 0;
    int len1 = doRead( buf, 1, timeOut, false );
    if( len1 != 1 ) {
      Log.d(TAG, "bad STX read " + buf + " len " + len1);
      return len1;
    }
    int bytesRead = len1;
    if( buf[0] != 2 ) {
      Log.d(TAG, "not STX, read " + buf[0]);
      passChar = buf[0];
      return 2;
    }
    Log.d(TAG, "got STX, reading payload");
    // read the message
    int rc = read_payload( buf, timeOut );

    // get the etx/lrc
    buf[0] = 0;
    len1 = doRead( buf, 1, timeOut, false );
    bytesRead += len1;
    if( ( len1 != 1) || ( buf[0] != 3 ) ) {
      Log.d(TAG, "bad ETX read " + buf[0]);
      return -1;
    }
    addToLRC( buf, 1 );
    
    len1 = doRead( buf, 1, timeOut, false );

    if( ( len1 != 1 ) || ( buf[0] != lrc ) ) {
      Log.d(TAG, "bad lrc read buf0 " + buf[0] + " lrc " + lrc);
      return -1;
    }

    Log.d(TAG, "read is done " + buf[0] + " " + lrc);

    return 1;
  }

  private int read_payload( byte[] buf, int timeOut ) throws Exception {
    int readLen = 4;
    int bytesRead = 0;
    boolean rc = true;

    // get mti
    int len1 = doRead( buf, readLen, timeOut, false );

    Log.d(TAG, "read payload " + len1);

    bytesRead += len1;
    if( len1 != readLen ) {
      Log.d(TAG, "bad read len: " + len1);
      return 0;
    }
    addToLRC(buf, len1);

    messageType = new Integer(new String(buf, 0, len1)).intValue();
 
    Log.d(TAG, "read mti " + messageType);

    readLen = 16;
    len1 = doRead( buf, readLen, timeOut, false );
    bytesRead += len1;
    if( len1 != readLen ) {
      Log.d(TAG, "bad read len: " + len1);
      return 0;
    }
    addToLRC(buf, len1);
    rc = setHeader(buf, 1);
   
    //Log.d(TAG, "set header " + new String(buf));

    if( rc && header.get(0) != null ) {
      //Log.d(TAG, "setting second header"); 
      buf[0] = 0;
      len1 = doRead( buf, readLen, timeOut, false );
      bytesRead += len1;
      if( len1 != readLen ) {
        Log.d(TAG, "bad 2nd header read len: " + len1);
        return 0;
      }
      addToLRC(buf, len1);
      rc = setHeader(buf, 2);
      //Log.d(TAG, "set second header " + new String(buf));
    } 
    for( int i = 2;( (i <= 128) && rc ); i++ ) {
      if( rc && header.get(i - 1) != null )
        rc = readField( i, timeOut );
    }
    if( ! rc ) 
      return 0;

    return 1; 
  }
  
  private boolean setHeader( byte[] in, int which ) {
    if( which == 1 )
      header.clear();

    if( which == 2 )
      header.put(0, 1);

    if( in.length < 16 ) {
      Log.d(TAG, "bad length in setHeader1: " + in.length);
      return false;
    }
  
    int j = 0;
    if( which == 2 )
      j = 64;

    for( int i = 0; i < 16; i++, j+=4 ) {
      if( in[i] == 'F' || in[i] == 'f' )  {
        header.put(0+j, 1);
        header.put(1+j, 1);
        header.put(2+j, 1);
        header.put(3+j, 1);
      }
      else
      if( in[i] == 'E' || in[i] == 'e' )  {
        header.put(0+j, 1);
        header.put(1+j, 1);
        header.put(2+j, 1);
      }
      else
      if( in[i] == 'D' || in[i] == 'd' )  {
        header.put(0+j, 1);
        header.put(1+j, 1);
        header.put(3+j, 1);
      }
      else
      if( in[i] == 'C' || in[i] == 'c' )  {
        header.put(0+j, 1);
        header.put(1+j, 1);
      }
      else
      if( in[i] == 'B' || in[i] == 'b' )  {
        header.put(0+j, 1);
        header.put(2+j, 1);
        header.put(3+j, 1);
      }
      else
      if( in[i] == 'A' || in[i] == 'a' )  {
        header.put(0+j, 1);
        header.put(2+j, 1);
      }
      else
      if( in[i] == '9' )  {
        header.put(0+j, 1);
        header.put(3+j, 1);
      }
      else
      if( in[i] == '8' )  {
        header.put(0+j, 1);
      }
      else
      if( in[i] == '7' )  {
        header.put(1+j, 1);
        header.put(2+j, 1);
        header.put(3+j, 1);
      }
      else
      if( in[i] == '6' )  {
        header.put(1+j, 1);
        header.put(2+j, 1);
      }
      else
      if( in[i] == '5' )  {
        header.put(1+j, 1);
        header.put(3+j, 1);
      }
      else
      if( in[i] == '4' )  {
        header.put(1+j, 1);
      }
      else
      if( in[i] == '3' )  {
        header.put(2+j, 1);
        header.put(3+j, 1);
      }
      else
      if( in[i] == '2' )  {
        header.put(2+j, 1);
      }
      else
      if( in[i] == '1' )  {
        header.put(3+j, 1);
      }
      else
      if( in[i] != '0' )  {
        Log.d(TAG, "setHeader bad char: " + in[i]);
        return false;
      }
    }
    return true;
  }

  private int strncmp( byte[] buf, String s, int l ) {
    if( l == 0 )
      return -1;
    for( int i = 0; i < l; i++ ) {
      if( buf[i] != s.charAt(i) )
        return -1;
    }
    return 0;
  }

  private byte bit_to_hex( byte[] buf ) {
    if( strncmp( buf, "0000", 4 ) == 0 )
      return '0';
    else
    if( strncmp( buf, "0001", 4 ) == 0 )
      return '1';
    else
    if( strncmp( buf, "0010", 4 ) == 0 )
      return '2';
    else
    if( strncmp( buf, "0011", 4 ) == 0 )
      return '3';
    else
    if( strncmp( buf, "0100", 4 ) == 0 )
      return '4';
    else
    if( strncmp( buf, "0101", 4 ) == 0 )
      return '5';
    else
    if( strncmp( buf, "0110", 4 ) == 0 )
      return '6';
    else
    if( strncmp( buf, "0111", 4 ) == 0 )
      return '7';
    else
    if( strncmp( buf, "1000", 4 ) == 0 )
      return '8';
    else
    if( strncmp( buf, "1001", 4 ) == 0 )
      return '9';
    else
    if( strncmp( buf, "1010", 4 ) == 0 )
      return 'A';
    else
    if( strncmp( buf, "1011", 4 ) == 0 )
      return 'B';
    else
    if( strncmp( buf, "1100", 4 ) == 0 )
      return 'C';
    else
    if( strncmp( buf, "1101", 4 ) == 0 )
      return 'D';
    else
    if( strncmp( buf, "1110", 4 ) == 0 )
      return 'E';
    else
    if( strncmp( buf, "1111", 4 ) == 0 )
      return 'F';
    else
    {
      Log.d(TAG, "bad conversion bit_to_hex: " + buf );
      return '0';
    }
  }
 
  private void bytecpy( byte[] f, byte[] s, int len, int offset ) {
    for( int i = 0; i < len; i++ ) {
      f[i + offset] = s[i];
    }
  }

  private void strncpy( byte[] f, String s, int len, int offset ) {
    int l = len;
    if( s.length() < len ) 
      l = s.length();
    for( int i = 0; i < l; i++ ) {
      f[i + offset] = (byte)( s.charAt(i) & 127 );
    }
  }

  private byte[] printField( int id ) {
    //Log.d(TAG, "in printField " + id);
    if( id > 127 )
      return new byte[1];
 
    int len = field_def[id][1];

    if( field_def[id][0] == fixed_length ) {
      byte[] f = new byte[len];
      lenbuf = lenbuf.format("%" + len + "s", new String(fields.get(id)));
      strncpy( f, lenbuf, len, 0 );
      if( f.length > len )
        f[len] = 0;

      if( field_def[id][2] == 0 ) {
        boolean done = false;
        int i = 0;
        while( ! done ) {
          if( f[i] == ' ' )
            f[i] = '0';
          else
            done = true;
          if(i++ > len)
            done = true;
        }
      }
      return f;
    }
    else
    if( field_def[id][0] == short_length ) {
      byte[] f = fields.get(id);
      byte[] f2 = new byte[f.length + 2];
      lenbuf = lenbuf.format("%02d", f.length);
      strncpy( f2, lenbuf, 2, 0 ); 
      bytecpy( f2, f, f.length, 2 ); 

      return f2;
    }
    else
    if( field_def[id][0] == long_length ) {
      byte[] f = fields.get(id);
      byte[] f2 = new byte[f.length + 3];
      lenbuf = lenbuf.format("%03d", f.length);
      strncpy( f2, lenbuf, 3, 0 ); 
      //Log.d(TAG, "f2 = " + new String(f2) + " len " +  f.length);
      bytecpy( f2, f, f.length, 3 ); 
      return f2;
    }
    else
    if( field_def[id][0] == vlong_length ) {
      byte[] f = fields.get(id);
      byte[] f2 = new byte[f.length + 6];
      lenbuf = lenbuf.format("%06d", f.length);
      strncpy( f2, lenbuf, 6, 0 ); 
      bytecpy( f2, f, f.length, 6 ); 

      return f2;
    } else {
      Log.d(TAG, "wrong field type for: " + id );
    }
    byte[] f = new byte[1];
    return f;
  }

  public boolean readExtendedData(int len) throws Exception {
    if( len < 1 )
      return false;

    //Log.d(TAG, "readExtended len " + len);
    outbuf = ByteBuffer.allocate(len);
    int read = inputstream.read(outbuf.array(), 0, len);
    if( read == len ) {
      Log.d(TAG, "read file OK");
      return true;
    }
    Log.d(TAG, "read file FAIL");
    return false;
  }

  public void write() throws Exception {
    outbuf = ByteBuffer.allocate(2048);
    byte[] stx = new byte[1];
    stx[0] = 0x2;
    outbuf.put(stx,0,1); //STX
    int outlen = 1;

    Log.d(TAG, "message start " + outbuf.array() );

    lenbuf = lenbuf.format("%04d", messageType);
    byte[] m2 = new byte[4];
    strncpy( m2, lenbuf, 4, 0 );
    outbuf.put(m2);

    outlen += 4;

    Log.d(TAG, "writing message type " + messageType);

    int s = 60;
    if( header.get(0) != null ) {
      outlen += 32;
      s = 124;
    } else {
      outlen += 16;
    }

    byte[] hbuf = new byte[4];

    for( int i = 0, k = 0; i <= s; i +=4, k++ ) {
      for( int n = 0;n < 4; n++ ) {
        int p = n + i;
        if( header.get(i+n) == null )
          hbuf[n] = '0';
        else {
          hbuf[n] = '1';
        }
      }
      byte h = 0;
      h = bit_to_hex(hbuf);
      outbuf.put(h);
    }

    s += 4;

    for( int i = 1; i <= s; i++ ) {
      if( header.get(i) != null )  {
        byte[] f = printField( i + 1 );
        Log.d(TAG, "read field " + i + " " + new String(f));
        outlen += f.length;
        outbuf.put(f);
      }
    }

    Log.d(TAG, "outlen : " + outlen);
    lrc = 0;

    for( int i = 1; i < outlen + 1; i++ )
      lrc ^= outbuf.get(i);

    Log.d(TAG, "lrc : " + lrc);

    lrc ^= 3;
    outbuf.put((byte)3);
    outbuf.put((byte)lrc);

    outlen += 2;

    outbuf.limit(outlen);

    Log.d(TAG, "writing message: " + new String(outbuf.array()));

    outputstream.write(outbuf.array(), 0, outlen);

    outbuf.limit(0);
  }

  public void writePasschar(byte c) throws Exception {
    //Log.d(TAG, "passchar is: " + c);

    outputstream.write(c);
  }

  public int parsePrompts() {
    // 341,2,1,8,enter odometer,
    promptKey = "0";
    numPrompts = 0;
    if( ! test(dpsmsg.additionalData) ) {
      Log.d(TAG, "no prompts");
      return 0;
    }
    String prompts = new String(get(dpsmsg.additionalData));
    StringTokenizer tokens = new StringTokenizer(prompts, ",");
    int numTokens = tokens.countTokens();
    Log.d(TAG, "have " + numTokens + " tokens");
    int x = 0;
    numPrompts = (numTokens - 1) / 4;
    promptText = new String[numPrompts];
    promptType = new String[numPrompts];
    promptLen  = new String[numPrompts];
    promptId   = new String[numPrompts];
    if( numTokens > 0 ) {
      promptKey = tokens.nextToken();
      Log.d(TAG, "have " + promptKey + " promptkey");
      boolean done = false;
      while( ! done ) {
        promptId[x]   = tokens.nextToken();
        promptType[x] = tokens.nextToken();
        promptLen[x]  = tokens.nextToken();
        promptText[x] = tokens.nextToken();
        //Log.d(TAG, "have id " + promptId[x] + " t " + promptType[x] + " l " + promptLen[x] + " t " + promptText[x]);
        x++;
        if( ! tokens.hasMoreTokens() )
          done = true;
      }
    }
    numPrompts = x;
    return x;
  }

  public String getPromptKey() {
    return promptKey;
  }

  public int getNumPrompts() {
    return numPrompts;
  }

  public String getPromptText(int i) {
    if( i < 0 || i >= numPrompts )
      return "";

    return promptText[i];
  }

  public String getPromptLen(int i) {
    if( i < 0 || i >= numPrompts )
      return "";

    return promptLen[i];
  }

  public String getPromptType(int i) {
    if( i < 0 || i >= numPrompts )
      return "";

    return promptType[i];
  }

  public String getPromptId(int i) {
    if( i < 0 || i >= numPrompts )
      return "";

    return promptId[i];
  }
}
