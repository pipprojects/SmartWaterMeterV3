#ifndef __HTTP_CLIENT_H_
#define __HTTP_CLIENT_H_

#include "application.h"
#include "spark_wiring_string.h"
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_usbserial.h"

/**
 * Defines for the HTTP methods.
 */
static const char* HTTP_METHOD_GET    = "GET";
static const char* HTTP_METHOD_POST   = "POST";
static const char* HTTP_METHOD_PUT    = "PUT";
static const char* HTTP_METHOD_DELETE = "DELETE";
static const char* HTTP_METHOD_PATCH = "PATCH";

/**
 * This struct is used to pass additional HTTP headers such as API-keys.
 * Normally you pass this as an array. The last entry must have NULL as key.
 */
typedef struct
{
//  const char* header;
//  const char* value;
  String header;
  String value;
} http_header_t;

/**
 * HTTP Request struct.
 * hostname request host
 * path	 request path
 * port     request port
 * body	 request body
 */
typedef struct
{
  String hostname;
  IPAddress ip;
  String path;
  // TODO: Look at setting the port by default.
  //int port = 80;
  int port;
  String body;
  String headers;
} http_request_t;

/**
 * HTTP Response struct.
 * status  response status code.
 * body	response body
 */
typedef struct
{
  int status;
  String body;
  String headers;
} http_response_t;

typedef struct
{
  String username;
  String password;
} http_credentials_t;

class HttpClient2 {
public:
    /**
    * Public references to variables.
    */
    TCPClient client;
    http_credentials_t credentials;

    /**
    * Constructor.
    */
    HttpClient2(void);

    /**
    * HTTP request methods.
    * Can't use 'delete' as name since it's a C++ keyword.
    */
    void get(http_request_t &aRequest, http_response_t &aResponse)
    {
        request(aRequest, aResponse, HTTP_METHOD_GET);
    }

    void post(http_request_t &aRequest, http_response_t &aResponse)
    {
        request(aRequest, aResponse, HTTP_METHOD_POST);
    }

    void put(http_request_t &aRequest, http_response_t &aResponse)
    {
        request(aRequest, aResponse, HTTP_METHOD_PUT);
    }

    void del(http_request_t &aRequest, http_response_t &aResponse)
    {
        request(aRequest, aResponse, HTTP_METHOD_DELETE);
    }

    void patch(http_request_t &aRequest, http_response_t &aResponse)
    {
        request(aRequest, aResponse, HTTP_METHOD_PATCH);
    }

    void MakeHeaderString(String &AuthValue, String Spacer, String Key, String Value, String Equals, String Quotes, String PostSpacer);
    void MakeHeaderChar(char* &AuthValue, char* Spacer, char* Key, char* Value, char* Equals, char* Quotes, char* PostSpacer);

private:
    /**
    * Underlying HTTP methods.
    */
    void requestInner(http_request_t &aRequest, http_response_t &aResponse, const char* aHttpMethod);
    void request(http_request_t &aRequest, http_response_t &aResponse, const char* aHttpMethod);
    void sendHeader(String aHeaderName, String aHeaderValue);
    void sendHeader(String aHeaderName, const int aHeaderValue);
    void sendHeader(String aHeaderName);
    bool CheckAuthentication(String ResponseHeaders, String username, String password, String AuthMethod, String AuthURI, String &AuthValue);
    void GetHeaderFromKey(String &rawHeader, String GetKey, String &Value, bool &RetStat, String Type);
    void GetHttpAuthentication(String &rawHeader, String &AuthType, String &AuthRealm, String &AuthDomain, String &AuthQop, String &AuthAlgo, String &AuthNonce, String &AuthOpaque);
};

#endif /* __HTTP_CLIENT_H_ */
