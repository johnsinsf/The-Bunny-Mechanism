class LObj {
  public:
    LObj( void ) {
      rc     = 0;
      jsonrc = 0;
      jsonerror = 0;
      type   = 0;
      pos    = 0;
      count  = 0;
      content_length = 0;
      request_type = 0;
      datasource = 0;
      keepalive = 0;
      bytes_written = 0;
    }
    map<int,string>  headers;
    map<string,string>  header;
    map<string,string>  cookie;
    map<string,string>  get;
    multimap<string,string>  post;

    string method;
    string packet;
    string datain;
    string dataout;
    string opt;
    string query;
    string uri;
    string url;
    string protocol;
    string host;
    string redirect;
    string content_type;
    string ipaddress;
    int    type;
    int    rc;
    int    pos;
    int    datasource;
    int    jsonrc;
    int    jsonerror;
    int    content_length;
    int    request_type;
    int    port_opt;
    int    keepalive;
    int    count;
    off_t  bytes_written;
    string range;
  protected:
};
