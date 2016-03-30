#include "HttpClient2.h"
#include "digcalc.h"

//#define LOGGING

static const uint16_t TIMEOUT = 5000; // Allow maximum 5s between data packets.

/**
* Constructor.
*/
HttpClient2::HttpClient2()
{
    credentials.username = "";
    credentials.password = "";
}

/**
* Method to send a header, should only be called from within the class.
*/
void HttpClient2::sendHeader(String aHeaderName, String aHeaderValue)
{
    client.print(aHeaderName);
    client.print(": ");
    client.println(aHeaderValue);

    #ifdef LOGGING
    Serial.print(aHeaderName);
    Serial.print(": ");
    Serial.println(aHeaderValue);
    #endif
}

void HttpClient2::sendHeader(String aHeaderName, const int aHeaderValue)
{
    client.print(aHeaderName);
    client.print(": ");
    client.println(aHeaderValue);

    #ifdef LOGGING
    Serial.print(aHeaderName);
    Serial.print(": ");
    Serial.println(aHeaderValue);
    #endif
}

void HttpClient2::sendHeader(String aHeaderName)
{
    client.println(aHeaderName);

    #ifdef LOGGING
    Serial.println(aHeaderName);
    #endif
}

/**
* Method to send an HTTP Request. Allocate variables in your application code
* in the aResponse struct and set the headers and the options in the aRequest
* struct.
*/
void HttpClient2::requestInner(http_request_t &aRequest, http_response_t &aResponse, const char* aHttpMethod)
{
    bool Test;
    String HeaderKeys, HKey, HValue;

    // If a proper response code isn't received it will be set to -1.
    aResponse.status = -1;
    aResponse.body = "";

    // NOTE: The default port tertiary statement is unpredictable if the request structure is not initialised
    // http_request_t request = {0} or memset(&request, 0, sizeof(http_request_t)) should be used
    // to ensure all fields are zero
    bool connected = false;
    bool doLoop = true;
    int NTries = 0, NMTries = 3;
    bool ConnectedServer;

    do {
        if(aRequest.ip!=NULL) {
            #ifdef LOGGING
	    Serial.println("Connecting IP address");
            #endif
            connected = client.connect(aRequest.ip, (aRequest.port) ? aRequest.port : 80 );
            #ifdef LOGGING
	    Serial.println("After connection");
            #endif
        }   else {
            #ifdef LOGGING
	    Serial.println("Connecting to hostname");
	    Serial.println(aRequest.hostname);
	    Serial.println(aRequest.port);
            #endif
            connected = client.connect(aRequest.hostname.c_str(), (aRequest.port) ? aRequest.port : 80 );
            #ifdef LOGGING
	    Serial.println("After connection");
            #endif
        }

        ConnectedServer = client.connected();

	if ( ! ConnectedServer ) {
	    NTries++;
            #ifdef LOGGING
	    Serial.print("Not connected ");
	    Serial.print(String(NTries));
	    Serial.print("/");
	    Serial.println(String(NMTries));
            #endif
	    client.stop();
	    if ( NTries >= NMTries ) {
                Serial.println("Lost connection to server");
		return;
	    }
	    delay(2000);
	} else {
            doLoop = false;
	}
    } while ( doLoop );

    #ifdef LOGGING
    if (connected) {
        if(aRequest.hostname!=NULL) {
            Serial.print("HttpClient2>\tConnecting to: ");
            Serial.print(aRequest.hostname);
        } else {
            Serial.print("HttpClient2>\tConnecting to IP: ");
            Serial.print(aRequest.ip);
        }
        Serial.print(":");
        Serial.println(aRequest.port);
    } else {
        Serial.println("HttpClient2>\tConnection failed.");
    }
    #endif

    if (!connected) {
        client.stop();
        // If TCP Client can't connect to host, exit here.
        #ifdef LOGGING
	Serial.println("Cannot connect to host");
        #endif
        return;
    }

    //
    // Send HTTP Headers
    //

    // Send initial headers (only HTTP 1.0 is supported for now).
    client.print(aHttpMethod);
    client.print(" ");
    client.print(aRequest.path);
    client.print(" HTTP/1.0\r\n");

    #ifdef LOGGING
    Serial.println("HttpClient2>\tStart of HTTP Request.");
    Serial.print(aHttpMethod);
    Serial.print(" ");
    Serial.print(aRequest.path);
    Serial.print(" HTTP/1.0\r\n");
    #endif

    // Send General and Request Headers.
    sendHeader("Connection", "close"); // Not supporting keep-alive for now.
    if(aRequest.hostname!=NULL) {
        sendHeader("HOST", aRequest.hostname.c_str());
    }

    //Send Entity Headers
    // TODO: Check the standard, currently sending Content-Length : 0 for empty
    // POST requests, and no content-length for other types.
    #ifdef LOGGING
    Serial.println("Request body");
    Serial.println(aRequest.body);
    #endif
    if (aRequest.body != NULL) {
        //Serial.println("Content-Length " + String((aRequest.body).length()));
        sendHeader("Content-Length", String((aRequest.body).length()));
    } else {
        if (strcmp(aHttpMethod, HTTP_METHOD_POST) == 0) { //Check to see if its a Post method.
	  //Serial.println("This is a post method");
          sendHeader("Content-Length", 0);
        }
    }

    #ifdef LOGGING
    Serial.println("Request Headers");
    Serial.println(aRequest.headers);
    #endif
    if ( aRequest.headers != NULL)
    {
        GetHeaderFromKey(aRequest.headers, "_ALL", HeaderKeys, Test, "REQUEST");
	bool GetHeaderLoop = true;
	int HStart = 0, HEnd;
	while ( GetHeaderLoop ) {
	    HEnd = HeaderKeys.indexOf(":", HStart);
	    if ( HEnd != -1 ) {
	        HKey = HeaderKeys.substring(HStart, HEnd);
	    } else {
	        HKey = HeaderKeys.substring(HStart);
		GetHeaderLoop = false;
	    }
            GetHeaderFromKey(aRequest.headers, HKey, HValue, Test, "REQUEST");
            if (HValue != NULL) {
                #ifdef LOGGING
		Serial.print("Sending Header ");
		Serial.println(HKey + ": " + HValue);
                #endif
                sendHeader(HKey, HValue);
            } else {
                #ifdef LOGGING
		Serial.print("Sending Null Header ");
                #endif
                sendHeader(HKey);
            }
	    HStart = HEnd + 1;
	}
    }

    // Empty line to finish headers
    client.println();
    client.flush();

    //
    // Send HTTP Request Body
    //

    if (aRequest.body != NULL) {
        client.println(aRequest.body);

        #ifdef LOGGING
        Serial.println(aRequest.body);
        #endif
    }

    #ifdef LOGGING
    Serial.println("HttpClient2>\tEnd of HTTP Request.");
    #endif

    // clear response buffer
    //memset(&buffer[0], 0, sizeof(buffer));


    //
    // Receive HTTP Response
    //
    // The first value of client.available() might not represent the
    // whole response, so after the first chunk of data is received instead
    // of terminating the connection there is a delay and another attempt
    // to read data.
    // The loop exits when the connection is closed, or if there is a
    // timeout or an error.

    //unsigned int bufferPosition = 0;
    unsigned long lastRead = millis();
    unsigned long firstRead = millis();
    bool error = false;
    bool timeout = false;
    String raw_response="";

    int NTimes=1;
    int NSize=256;
    int BSize = NSize;
    char* buffer = new char[BSize];
    int NumC=0;

    do {
        #ifdef LOGGING
	Serial.println("Getting bytes");
        int bytes = client.available();
	Serial.println("Got bytes");
        if(bytes) {
            Serial.print("\r\nHttpClient2>\tReceiving TCP transaction of ");
            Serial.print(bytes);
            Serial.println(" bytes.");
        }
        #endif

        #ifdef LOGGING
	Serial.println("From Server");
	Serial.println(String(bytes));
        #endif
        while (client.available()) {
	    //Serial.println("Reading TCP client");
            char c = client.read();
	    //Serial.println("Read TCP client");
            #ifdef LOGGING
            Serial.print(c);
	    //Serial.println(raw_response);
            #endif
            lastRead = millis();

            if (c == -1) {
                error = true;

                Serial.println("HttpClient2>\tError: No data available.");
                #ifdef LOGGING
                Serial.println("HttpClient2>\tError: No data available.");
                #endif

                break;
            }
	    //raw_response.concat(String(c));
	    NumC++;
	    if ( NumC >= BSize ) {
		NTimes++;
                BSize += NSize;
                char* buffer2 = new char[BSize];
	        memcpy( buffer2, buffer, NumC-1);
	        delete [] buffer;
                buffer = buffer2;
		//Serial.print("Buffer is now this big ");
		//Serial.println(String(BSize));
	    }
	    //Serial.print("Num chars " );
	    //Serial.println(String(NumC));
	    buffer[NumC-1] = c;
	    buffer[NumC] = '\0';
	    //Serial.print("Client connected 1 ");
	    //Serial.println(String(client.connected()));
        }

        #ifdef LOGGING
        if (bytes) {
            Serial.print("\r\nHttpClient2>\tEnd of TCP transaction.");
        }
        #endif

        // Check that there hasn't been more than 5s since last read.
        timeout = millis() - lastRead > TIMEOUT;

        // Unless there has been an error or timeout wait 200ms to allow server
        // to respond or close connection.
        if (!error && !timeout) {
            delay(200);
        }
	//Serial.print("Client connected ");
	//Serial.println(String(client.connected()));
	//Serial.print("Timeout ");
	//Serial.println(String(timeout));
	//Serial.print("Error ");
	//Serial.println(String(error));
    } while (client.connected() && !timeout && !error);

    raw_response = String(buffer);
    delete [] buffer;

    if (timeout) {
        Serial.println("\r\nHttpClient2>\tError: Timeout while reading response.");
    }
    #ifdef LOGGING
    Serial.println("raw_response");
    Serial.println(raw_response);
    Serial.print("\r\nHttpClient2>\tEnd of HTTP Response (");
    Serial.print(millis() - firstRead);
    Serial.println("ms).");
    #endif
    client.stop();



    int bodyPos = raw_response.indexOf("\r\n\r\n");
    if (bodyPos == -1) {
        #ifdef LOGGING
        Serial.println("HttpClient2>\tError: Can't find HTTP response body.");
        #endif

        return;
    }
    // Return the entire message body from bodyPos+4 till end.
    aResponse.body = raw_response.substring(bodyPos+4);

    aResponse.headers = raw_response.substring(0, bodyPos+4);

    #ifdef LOGGING
    Serial.print("Raw Header ");
    Serial.println(aResponse.headers);
    #endif

    String statusCode;
    
    GetHeaderFromKey(aResponse.headers, "Status Code", statusCode, Test, "RESPONSE");

    aResponse.status = atoi(statusCode.c_str());
    #ifdef LOGGING
    Serial.print("HttpClient2>\tStatus Code: ");
    Serial.println(statusCode);
    Serial.println(String(aResponse.status));
    #endif

    delete raw_response;
    delete HeaderKeys, HKey, HValue;
    delete statusCode;
}

void HttpClient2::request(http_request_t &aRequest, http_response_t &aResponse, const char* aHttpMethod) {

    requestInner(aRequest, aResponse, aHttpMethod);
    String AuthMethod, AuthURI, AuthValue;
    String username = credentials.username;
    String password = credentials.password;
    bool Try;
    #ifdef LOGGING
    Serial.println("Return from inner request");
    #endif

    if ( aResponse.status == 401 ) {
      Try = CheckAuthentication(aResponse.headers, credentials.username, credentials.password, aHttpMethod, aRequest.path, AuthValue);
      if (Try ) {
        // Make New Request headers
        aRequest.headers = "";
        MakeHeaderString(aRequest.headers, "", "Content-Type", "application/json", ": ", "", "\r\n");
        MakeHeaderString(aRequest.headers, "", "Accept", "*/*", ": ", "", "\r\n");
        MakeHeaderString(aRequest.headers, "", "Authorization", AuthValue, ": ", "", "\r\n");
        requestInner(aRequest, aResponse, aHttpMethod);
        #ifdef LOGGING
        Serial.print("Application>\tResponse status Auth: ");
        Serial.println(aResponse.status);
        Serial.println("Returned Headers");
        #endif
      }
    }
    delete AuthMethod, AuthURI, AuthValue;
    delete username;
    delete password;
}

bool HttpClient2::CheckAuthentication(String ResponseHeaders, String username, String password, String AuthMethod, String AuthURI, String &AuthValue) {

    bool Try;
    String AuthType, AuthRealm, AuthDomain, AuthQop, AuthAlgo, AuthNonce, AuthOpaque;
    String AuthNC, AuthCNonce, AuthResponse;

    GetHttpAuthentication(ResponseHeaders, AuthType, AuthRealm, AuthDomain, AuthQop, AuthAlgo, AuthNonce, AuthOpaque);

    #ifdef LOGGING
    Serial.print("Auth Type ");
    Serial.println(AuthType);
    #endif
    if ( AuthType == "Digest" ) {
// Have 1 chance at authenticating and pass back headers and body from this one
      AuthNC="00000001";
      AuthCNonce="d1b93a52d3a9706a";
      MD5HASHHEX HA1;
      MD5HASHHEX HA2 = "";
      MD5HASHHEX Response;
      // Get MD5 hash
      DigestCalcHA1(AuthAlgo, username, AuthRealm, password, AuthNonce, AuthCNonce, HA1);
      DigestCalcResponse(HA1, AuthNonce, AuthNC, AuthCNonce, AuthQop, AuthMethod, AuthURI, HA2, Response);
      AuthResponse = String(Response);
      //String AuthValue;
      AuthValue = AuthType;
      // Make Digest authentication header
      MakeHeaderString(AuthValue, " ", "username", username, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "realm", AuthRealm, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "nonce", AuthNonce, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "uri", AuthURI, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "response", AuthResponse, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "opaque", AuthOpaque, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "algorithm", AuthAlgo, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "qop", AuthQop, "=", "\"", "");
      MakeHeaderString(AuthValue, ", ", "nc", AuthNC, "=", "", "");
      MakeHeaderString(AuthValue, ", ", "cnonce", AuthCNonce, "=", "\"", "");
      //Serial.print("Header String ");
      //Serial.println(AuthValue);
      Try = true;
    }

  delete AuthType, AuthRealm, AuthDomain, AuthQop, AuthAlgo, AuthNonce, AuthOpaque;
  delete AuthNC, AuthCNonce, AuthResponse;
  delete ResponseHeaders, username, password, AuthMethod, AuthURI;
  return Try;
}

void HttpClient2::GetHeaderFromKey(String &rawHeader, String GetKey, String &Value, bool &RetStat, String Type) {
  int start=0, end;
  int HStart, HEnd;
  String header;
  bool Loop=true;
  String HttpType, HttpStatusCode, HttpStatusDescription;
  String HttpKey, HttpValue;
  String DL = "";
  Value = "";

  //Serial.println("processHttpHeader");
  //Serial.print("Get Key ");
  //Serial.println(GetKey);
  int lineNum=1;
  while ( Loop ) {

    end = rawHeader.indexOf("\r\n", start);
    if ( end == -1 ) {
      Loop = false;
    } else {


      header = rawHeader.substring(start, end);

      //Serial.println("New Line");
      //Serial.println(header);
      //Serial.print("Application>\tFrom: ");
      //Serial.print(String(start));
      //Serial.print("  To: ");
      //Serial.println(String(end));
      //Serial.print("Application>\tLine: ");
      //Serial.println(String(i));

      if ( Type == "RESPONSE" ) {
        if ( lineNum == 1 ) {
          HStart = 0;
          HEnd = header.indexOf(" ", HStart);
	  HttpKey = "Http Type";
          HttpValue = header.substring(HStart, HEnd);

	  if ( GetKey == "_ALL" ) {
            //Serial.print("Found Key ");
	    //Serial.println(HttpKey);
            RetStat = true;
	    //Value += DL + HttpKey;
	    Value.concat(DL);
	    Value.concat(HttpKey);
	    DL = ":";
	  } else if ( GetKey == HttpKey ) {
            //Serial.println("Found Key");
            Value = HttpValue;
            RetStat = true;
          }

          HStart = HEnd + 1;
          HEnd = header.indexOf(" ", HStart);
	  HttpKey = "Status Code";
          HttpValue = header.substring(HStart, HEnd);

	  if ( GetKey == "_ALL" ) {
            //Serial.print("Found Key ");
	    //Serial.println(HttpKey);
            RetStat = true;
	    //Value += DL + HttpKey;
	    Value.concat(DL);
	    Value.concat(HttpKey);
	    DL = ":";
	  } else if ( GetKey == HttpKey ) {
            //Serial.println("Found Key");
            Value = HttpValue;
            RetStat = true;
          }

          HStart = HEnd + 1;
	  HttpKey = "Status Description";
          HttpValue = header.substring(HStart);

	  if ( GetKey == "_ALL" ) {
            //Serial.print("Found Key ");
	    //Serial.println(HttpKey);
            RetStat = true;
	    //Value += DL + HttpKey;
	    Value.concat(DL);
	    Value.concat(HttpKey);
	    DL = ":";
	  } else if ( GetKey == HttpKey ) {
            //Serial.println("Found Key");
            Value = HttpValue;
            RetStat = true;
          }

          //Serial.print("Type: ");
          //Serial.println(HttpType);
          //Serial.print("Status Code: ");
          //Serial.println(HttpStatusCode);
          //Serial.print("Status Description: ");
          //Serial.println(HttpStatusDescription);
        }
      }

      if ( (Type == "REQUEST") | ((Type == "RESPONSE") & (lineNum > 1)) ) {
        HStart = 0;
        HEnd = header.indexOf(":", HStart);

        HttpKey = header.substring(HStart, HEnd).trim();
        HttpValue = header.substring(HEnd+1).trim();

	if ( GetKey == "_ALL" ) {
          //Serial.print("Found Key ");
	  //Serial.println(HttpKey);
          RetStat = true;
	  //Value += DL + HttpKey;
	  Value.concat(DL);
	  Value.concat(HttpKey);
	  DL = ":";
	} else if ( GetKey == HttpKey ) {
          //Serial.println("Found Key");
          Value = HttpValue;
          RetStat = true;
        }
      }
      //Serial.print("@");
      //Serial.print(header);
      //Serial.println("@");

      start = end + 2;
    }

    lineNum++;
  }
  //Serial.println("Found Header Values ");
  //Serial.println(Value);
}

void HttpClient2::GetHttpAuthentication(String &rawHeader,
  String &AuthType, String &AuthRealm, String &AuthDomain, String &AuthQop,
  String &AuthAlgo, String &AuthNonce, String &AuthOpaque) {
  int i=0;
  bool Loop=true, Loop2;
  String Key;
  String Value;
  int HStart, HEnd, PStart, PEnd;
  String ParamLine, ParamVar, ParamVal, ParamVal2;
  //String AuthType, AuthRealm, AuthQop, AuthAlgo, AuthNonce, AuthOpaque;
// WWW-Authenticate: Digest realm="api.watermeter.baoyue.co.uk",
// domain="/", qop=auth, algorithm=MD5, nonce="61e3e27f0c6858ce196918b09628ec74",
// opaque="94619f8a70068b2591c2eed622525b0e"
      bool Test;
      GetHeaderFromKey(rawHeader, "WWW-Authenticate", Value, Test, "RESPONSE");
      if ( Test ) {
        HStart = 0;
        HEnd = Value.indexOf(" ", HStart);
        AuthType = Value.substring(HStart, HEnd).trim();
        //Serial.print("Auth Type ");
        //Serial.println(AuthType);
        if ( AuthType == "Digest" ) {
          Loop2 = true;
          while ( Loop2 ) {
            HStart = HEnd + 1;
            if ( HEnd != -1 ) {
              HEnd = Value.indexOf(",", HStart);
            }
            //Serial.print("HStart ");
            //Serial.println(HStart);
            //Serial.print("HEnd ");
            //Serial.println(HEnd);
            if ( HEnd == -1 ) {
              if ( HStart == 0 ) {
                Loop2 = false;
              }
            }
            if ( Loop2 ) {
              if ( HEnd != -1 ) {
                ParamLine = Value.substring(HStart, HEnd).trim();
              } else {
                ParamLine = Value.substring(HStart).trim();
              }
              PStart = 0;
              PEnd = ParamLine.indexOf("=", PStart);
              ParamVar = ParamLine.substring(PStart, PEnd).trim();
              ParamVal = ParamLine.substring(PEnd+1).trim();
              if ( ParamVar == "realm" ) {
                ParamVal2 = ParamVal.replace("\"", " ").trim();
                AuthRealm = ParamVal2;
              } else if ( ParamVar == "domain" ) {
                ParamVal2 = ParamVal.replace("\"", " ").trim();
                AuthDomain = ParamVal2;
              } else if ( ParamVar == "qop" ) {
                AuthQop = ParamVal;
              } else if ( ParamVar == "algorithm" ) {
                AuthAlgo = ParamVal;
              } else if ( ParamVar == "nonce" ) {
                ParamVal2 = ParamVal.replace("\"", " ").trim();
                AuthNonce = ParamVal2;
              } else if ( ParamVar == "opaque" ) {
                ParamVal2 = ParamVal.replace("\"", " ").trim();
                AuthOpaque = ParamVal2;
              } // Auth Params if
            } // Auth Line if
          } // Auth Line while
        } // Auth Type if
      } // WWW-Auth Line if
}

void HttpClient2::MakeHeaderString(String &AuthValue, String Spacer, String Key, String Value, String Equals, String Quotes, String PostSpacer) {
  //bool Str=true;
  //Serial.println("-------->MakeHeaderString");
  //Serial.println("Key " + Key);
  //if ( Str ) {
      AuthValue.concat(Spacer);
      AuthValue.concat(Key);
      AuthValue.concat(Equals);
      AuthValue.concat(Quotes);
      AuthValue.concat(Value);
      AuthValue.concat(Quotes);
      AuthValue.concat(PostSpacer);
  //} else {
      //char* CAuthValue = new char[2048];
      //char* CSpacer = new char[Spacer.length()+1];
      //char* CKey = new char[Key.length()+1];
      //char* CEquals = new char[Equals.length()+1];
      //char* CQuotes = new char[Quotes.length()+1];
      //char* CValue = new char[Value.length()+1];
      //char* CPostSpacer = new char[PostSpacer.length()+1];
      /*
      char CAuthValue[4096];
      char CSpacer[Spacer.length()+2];
      char CKey[Key.length()+2];
      char CEquals[Equals.length()+2];
      char CQuotes[Quotes.length()+2];
      char CValue[Value.length()+2];
      char CPostSpacer[PostSpacer.length()+2];

      Spacer.toCharArray(CSpacer, Spacer.length()+1);
      CSpacer[Spacer.length()+1] = '\0';

      Key.toCharArray(CKey, Key.length()+1);
      CKey[Key.length()+1] = '\0';
      */
      //Serial.println("Key length " + String(Key.length()));
      //Serial.println("CKey " + String(CKey));
      /*
      int j=0;
      while ( j <= (Key.length() + 1) ) {
          Serial.println(String(j) + " " + String(CKey[j]));
	  j++;
      }

      Equals.toCharArray(CEquals, Equals.length()+1);
      CEquals[Equals.length()+1] = '\0';

      Quotes.toCharArray(CQuotes, Quotes.length()+1);
      CQuotes[Quotes.length()+1] = '\0';

      Value.toCharArray(CValue, Value.length()+1);
      CValue[Value.length()+1] = '\0';

      PostSpacer.toCharArray(CPostSpacer, PostSpacer.length()+1);
      CPostSpacer[PostSpacer.length()+1] = '\0';

      //MakeHeaderChar(CAuthValue, CSpacer, CKey, CValue, CEquals, CQuotes, CPostSpacer);
      strcpy(CAuthValue, CSpacer);
      strcat(CAuthValue, CKey);
      strcat(CAuthValue, CEquals);
      strcat(CAuthValue, CQuotes);
      strcat(CAuthValue, CValue);
      strcat(CAuthValue, CQuotes);
      strcat(CAuthValue, CPostSpacer);
      AuthValue += String(CAuthValue);
      Serial.println("AuthValue");
      Serial.println(AuthValue);
      */
  //}
  //Serial.println("--------<MakeHeaderString");
}

void HttpClient2::MakeHeaderChar(char* &AuthValue, char* Spacer, char* Key, char* Value, char* Equals, char* Quotes, char* PostSpacer) {
      Serial.println("Value " + String(Value));
      strcpy(AuthValue, Spacer);
      strcpy(AuthValue, Key);
      strcpy(AuthValue, Equals);
      strcpy(AuthValue, Quotes);
      strcpy(AuthValue, Value);
      strcpy(AuthValue, Quotes);
      strcpy(AuthValue, PostSpacer);
}

