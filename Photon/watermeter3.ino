

//http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
#include "md5.h"

#include "HttpClient2.h"
//#include "dnsclient.h"

#include "digcalc.h"

typedef struct {
  String Host;
  IPAddress IP;
  int Port;
  String Path;
  http_credentials_t Credentials;
  String DeviceID;
} APIAccess;

#include "application.h"



int sw, Psw;
String Click;
String SVolume;
float Multiplier;
float Volume;
String Version;




int now, pnow;

Thread* ThreadGetSwitchStatus;
Thread* ThreadPhotonStatus;
Thread* ThreadFlashLED;

//If just after switch on then send status to ThingSpeak
bool FirstTime;

String Line;

int Diff, MinS;

//Sets LED response to switch position
//D1 Set Green
int Switch1[2] = {HIGH, LOW};
//D2 Set Blue
int Switch2[2] = {LOW, HIGH};




WLanSelectAntenna_TypeDef AntSel;

int StatusDelay;

APIAccess APIData;


HttpClient2 http;
http_request_t request;
http_response_t response;


HttpClient2 httpStatus;
http_request_t requestStatus;
http_response_t responseStatus;





void setup() {
  Serial.begin(9600);

  //Switch
  pinMode(D0, INPUT);
//Multicoloured LED
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
//Onboard Blue LED
  pinMode(D7, OUTPUT);

//Each closed contact on the switch is 10litres of water
  Multiplier = 10.0;
//Inital Volume used
  Volume = 0;
//Time between checks to send to ThinkSpeak
  MinS=10;

  pnow = 0;

  Version="3.0";


  FirstTime = true;

  AntSel = ANT_EXTERNAL;

  WiFi.selectAntenna(AntSel);

  StatusDelay=300000;

  APIData.Host = "api.watermeter.localhost";
// IP Address of this host
  APIData.IP = IPAddress(192,168,0,1);
  APIData.Port = 80;
  APIData.Path="/events";
// This is set in the FRAPI admin interface under Partners
  APIData.Credentials.username = "test";
  APIData.Credentials.password = "34c285b25ac62f9472265d1e41f8a77f5d2382f6";
  APIData.DeviceID = "7f96af4aac49acfcb9b10198a2280665";


//Get Threads ready
  ThreadGetSwitchStatus = new Thread("Get Switch Status", LoopGetSwitchStatus);
  ThreadPhotonStatus = new Thread("Photon Status", LoopPhotonStatus);
  ThreadFlashLED = new Thread("Flash the onboard Blue LED", LoopFlashLED);

}


void loop() {
    //Main loop does nothing
}



//Get the status of the water meter switch
os_thread_return_t LoopGetSwitchStatus(void* param){
    int MissedTries;
    MissedTries=0;
    for(;;) {
        GetSwitchStatus(MissedTries, APIData);
    }
}

//Get the status of the Photon, free meory and send to API
os_thread_return_t LoopPhotonStatus(void* param){
    int StatusMissedTries;
    String SData="";
    String SComment="Boot";
    StatusMissedTries=0;
    while ( ! WiFi.ready() ) {
      Serial.println("Waiting for WiFi...");
    }
    Serial.println("WiFi ready");
    SendToAPI(SData, SComment, StatusMissedTries, APIData);
    Serial.println("Sent boot data");
    Serial.flush();
    for(;;) {
        PhotonStatus(StatusMissedTries, APIData);
        delay(StatusDelay);
    }
}

//Flashes onboard Blue LED
os_thread_return_t LoopFlashLED(void* param){
    int Stat;
    Stat = LOW;
  // Turn onboard Blue LED off
    digitalWrite(D7, Stat);
    for(;;) {
      FlashLED(Stat);
    }
}

//Flash the Blue onboard LED
void FlashLED(int &Stat){
//Could do somehting more fancy here
  int i;
  for (i=0; i < 1; i=i+1) {
      if (Stat == LOW){Stat=HIGH;}else{Stat=LOW;}
      digitalWrite(D7, Stat);
      //Serial.print("D7 LED ");
      //Serial.println(Stat);
      delay(500);
  }
}


// Reads switch status
void GetSwitchStatus(int &MissedTries, APIAccess APIData) {

  //Serial.print("Version ");
  //Serial.println(Version);
  String SData, SComment;

  sw = digitalRead(D0);

// If just turned on then send data
  if ( FirstTime ) {
    Psw = sw;
    SVolume = String(Volume);
    //Particle.publish("Volume", SVolume);
    //ThingSpeak.setField(2,SVolume);
    FirstTime = false;
    pnow = millis();
    //Set multicoloured LED
    digitalWrite(D1, Switch1[sw]);
    digitalWrite(D2, Switch2[sw]);
    digitalWrite(D3, LOW);
  }


  if (sw != Psw ) {

      //Set multicoloured LED
      digitalWrite(D1, Switch1[sw]);
      digitalWrite(D2, Switch2[sw]);

      Serial.println("Switch Change");
      Click = String(sw);
      if (sw == 0) {
        Serial.println("Switch Closed");
        Volume = Volume + Multiplier;
        SVolume = String(Volume);
        SData = String(Multiplier);
        SComment = "Closed";
        Serial.print("Volume ");
        Serial.println(SVolume);
      } else {
        SData = "";
        SComment = "Open";
        Serial.println("Switch Open");
      }
      digitalWrite(D3, HIGH);
      SendToAPI(SData, SComment, MissedTries, APIData);
      digitalWrite(D3, LOW);
      pnow = millis();
  }

  now = millis();
  //Serial.println("---Time>>");
  //Serial.println(String(pnow));
  //Serial.println(String(now));
  //Serial.println("---Time<<");

  Diff = (now - pnow)/1000;

//Previous switch state
  Psw = sw;

  delay(200);

}

// Reads Status of Photon ang logs it
void PhotonStatus(int &MissedTries, APIAccess APIData) {

  uint32_t freemem = System.freeMemory();
  String Data = String(freemem);

  Serial.print("Free memory ");
  Serial.println(Data);



  requestStatus.hostname = APIData.Host;
  requestStatus.ip = APIData.IP;
  requestStatus.port = APIData.Port;
  requestStatus.path = APIData.Path;
  httpStatus.credentials = APIData.Credentials;
  String DeviceID = APIData.DeviceID;

  requestStatus.headers = "";
  httpStatus.MakeHeaderString(requestStatus.headers, "", "Content-Type", "application/json", ": ", "", "\r\n");
  httpStatus.MakeHeaderString(requestStatus.headers, "", "Accept", "*/*", ": ", "", "\r\n");

  //Serial.println("Main Request Headers Status");
  //Serial.print(requestStatus.headers);
  //Serial.println("Main Request Headers Status End");


  bool StatusLoop = true;
  int MaxTries=6, Tries=1;
  while ( StatusLoop ) {

    //Serial.println("Trying " + String(Tries) + "/" + String(MaxTries));

    String Comment = "Free memory,T:" + String(Tries) + "/" + String(MaxTries) + ",M:" + String(MissedTries);

    //requestStatus.body = "{\"device_id\": \"" + DeviceID + "\", " +
      "\"value\": \"" + Data + "\", " +
      "\"comment\": \"" + Comment + "\"}";
    requestStatus.body = "";
    requestStatus.body.concat("{\"device_id\": \"");
    requestStatus.body.concat(DeviceID);
    requestStatus.body.concat("\", ");
    requestStatus.body.concat("\"value\": \"");
    requestStatus.body.concat(Data);
    requestStatus.body.concat("\", ");
    requestStatus.body.concat("\"comment\": \"");
    requestStatus.body.concat(Comment);
    requestStatus.body.concat("\"}");

    //Serial.println("Body " + requestStatus.body);

    httpStatus.post(requestStatus, responseStatus);
    //Serial.println("Application>\tResponse Status status: ");
    //Serial.println(responseStatus.status);

    Serial.println("Application 1 >\tHTTP Response Status Body: ");
    Serial.println(responseStatus.body);

    if ( responseStatus.status != 200 ) {
      if ( Tries < MaxTries ) {
        Serial.println("Trying again");
      } else {
        Serial.print("Too many tries ");
        Serial.println(String(MaxTries));
        MissedTries++;
        StatusLoop = false;
      }
    } else {
      MissedTries=0;
      StatusLoop = false;
    }
    Tries++;
  }

}


void SendToAPI(String Data, String Comment, int &MissedTries, APIAccess APIData) {


  //Serial.println();
  //Serial.println();
  //Serial.println();
  //Serial.println("Application 2 >\tStart of API Send Data.");

  request.hostname = APIData.Host;
  request.ip = APIData.IP;
  request.port = APIData.Port;
  request.path = APIData.Path;
  http.credentials = APIData.Credentials;
  String DeviceID = APIData.DeviceID;

  request.headers = "";
  http.MakeHeaderString(request.headers, "", "Content-Type", "application/json", ": ", "", "\r\n");
  http.MakeHeaderString(request.headers, "", "Accept", "*/*", ": ", "", "\r\n");

  Serial.println("Main Request Headers");
  Serial.print(request.headers);
  Serial.println("Main Request Headers End");


  request.body = "{\"device_id\": \"" + DeviceID + "\", " +
      "\"value\": \"" + Data + "\", " +
      "\"comment\": \"" + Comment + "\"}";
  //Serial.println("Body " + request.body);


  bool StatusLoop = true;
  int MaxTries=6, Tries=1;
  while ( StatusLoop ) {

    http.post(request, response);
    //Serial.println("Application 2 >\tResponse status: ");
    //Serial.println(response.status);


    //Serial.println("Application 2 >\tHTTP Response Body: ");
    //Serial.println(response.body);

    if ( response.status != 200 ) {
      if ( Tries < MaxTries ) {
        Serial.println("Trying again");
      } else {
        Serial.println("Too many tries " + String(MaxTries));
        MissedTries++;
        StatusLoop = false;
      }
    } else {
      MissedTries=0;
      StatusLoop = false;
    }
    Tries++;
  }

  uint32_t freemem = System.freeMemory();

  Serial.print("Free memory");
  Serial.println(String(freemem));

}
