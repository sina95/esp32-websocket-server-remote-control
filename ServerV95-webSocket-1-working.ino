#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>


#define button D1
#define redLed D5
#define orangeLed D6
#define greenLed D7
#define BOARD_ID 102


String jsonString; // Temporary storage for the JSON String


const char pushButton = 2;
bool pressedOn = true;
bool sendingChanges;
bool sendingChangesStateBefore;

  StaticJsonDocument<100> doc;
    // create an object
  JsonObject object = doc.to<JsonObject>();

//char switchStr[50];
//char idStr[50];
char mailStr[50];



AsyncWebServer server(80);
//AsyncWebSocket ws("/ws");
WebSocketsServer webSocket = WebSocketsServer(81);  //create instance for webSocket server on port"81"

  
// Replace with your SSID and Password
const char* ssid     = "TP-test";
const char* password = "test++";


// Set your Static IP address
IPAddress local_IP(192, 168, 2, 216);
// Set your Gateway IP address
IPAddress gateway(192, 168, 2, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(178, 212, 95, 5);   //optional
IPAddress secondaryDNS(178, 212, 95, 6); //optional

IPAddress clients[] = {IPAddress(192,168,0,111), IPAddress(192,168,0,112), IPAddress(192,168,0,113)};
String location = "Test";

// Replace with your unique IFTTT URL resource
const char* resource = "/trigger/event/with/key/qQg5R1t";
const char* resource1 = "http://maker.ifttt.com/trigger/event";

// Maker Webhooks IFTTT
const char* serverEvents = "maker.ifttt.com";


typedef struct struct_message {
    IPAddress ip;
    bool active_monitor;
    unsigned long lastPing;
    bool email_to_send;
} struct_message;


struct_message myData;
struct_message externalData;

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;
struct_message board3;

// Create an array with all the structures
struct_message boardsStruct[3] = {board1, board2, board3}; // set number of boards


unsigned long lastTime = 0; // broadcast poruke
unsigned long timerDelay = 12;  // broadcast poruke na n sekundi
unsigned long pingTimerDelay = 60; // provjera dostupnosti klijenata na n sekundi
unsigned long lastTime2 = 0; // provjera dostupnosti interneta
unsigned long wifiDelay = 10; // provjera dostupnosti interneta
unsigned long availabilityTimer = 14400; // mail dostupnosti na n vremena
unsigned long lastTime3 = 0; // mail dostupnosti na n vremena
int wifiDelayCounter = 0; // wifi counter


// Establish a Wi-Fi connection with your router
void initWifi() {

//  Serial.print("Connecting to: "); 
//  Serial.print(ssid);
//  WiFi.mode(WIFI_AP_STA);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  // Configures static IP address
//if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
//  Serial.println("STA Failed to configure");
//}

  WiFi.begin(ssid, password);  
//  WiFi.softAP("Test", "1255884465656",5, false, 8);

  while(WiFi.waitForConnectResult() != WL_CONNECTED) {
//    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  
  snprintf(mailStr, sizeof(mailStr), "%s-Upaljen server-%d", location.c_str(), (int) BOARD_ID);
  makeIFTTTHTTPRequest(mailStr, (char*)WiFi.macAddress().c_str(), (char*)WiFi.localIP().toString().c_str());
//
//  WiFi.setAutoReconnect(true);
//  WiFi.persistent(true);


//  Serial.println("HTTP server started");

}


void makeIFTTTHTTPRequest(char* value1, char* value2, char* value3) {
 //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      JSONVar JSONencoder;
      JSONencoder["value1"] = value1;
      JSONencoder["value2"] = value2;
      JSONencoder["value3"] = value3;
      String jsonString = JSON.stringify(JSONencoder);
      
      WiFiClient client;
      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      http.begin(client, resource1);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonString);
      http.end();
    }
}


// This function gets a call when a WebSocket event occurs
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: // enum that read status this is used for debugging.
//      Serial.print("WS Type ");
//      Serial.print(type);
//      Serial.println(": DISCONNECTED");
//      Serial.println("Disconnected");
      break;
    case WStype_CONNECTED:  // Check if a WebSocket client is connected or not
//      Serial.println("Connected...");
      broadcastToClients();
      break;
//      Serial.print("WS Type ");
//      Serial.print(type);
//      Serial.println(": CONNECTED");
//      if (digitalRead(22) == HIGH) {  //check if pin 22 is high or low
//        pin_status = "ON";
//        update_webpage(); // update the webpage accordingly
//      }
//      else {                          
//        pin_status = "OFF"; //check if pin 22 is high or low
//        update_webpage();// update the webpage accordingly
//      }
      break;
    case WStype_TEXT: // check responce from client
    {
    // deserialize incoming Json String
    DeserializationError error = deserializeJson(doc, payload); 
    if (error) { // Print erro msg if incomig String is not JSON formated
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    externalData.active_monitor = (bool)doc["active_monitor"]; // String variable tha holds LED status
//    Serial.println(externalData.active_monitor);  
    }
    break;
    case WStype_PING:
    {
      IPAddress ip = webSocket.remoteIP(num);
     for (byte i = 0; i < sizeof(boardsStruct)/sizeof(struct_message);i++) {
      if (ip == boardsStruct[i].ip){
        boardsStruct[i].lastPing = millis()/1000;
//        Serial.println(boardsStruct[i].lastPing);
      }
     }
//      Serial.println();
//      Serial.printf("[%u] Connected from %d.%d.%d.%d", num, ip[0], ip[1], ip[2], ip[3]);
    }
      break;
    case WStype_PONG:
//      Serial.println("PONG");
      break;
  }
}

void broadcastToClients(){

  object["active_monitor"] = myData.active_monitor;
  serializeJson(doc, jsonString); // serialize the object and save teh result to teh string variable.
//  Serial.println( jsonString ); // print the string for debugging.
  webSocket.broadcastTXT(jsonString);
  jsonString = ""; // clear the String.
}


void setup() {
  // Setup serial port
  pinMode(button, INPUT);
  pinMode(redLed, OUTPUT);
  pinMode(orangeLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

//  pinMode(ledPin, OUTPUT);
//  digitalWrite(ledPin, LOW);

  
  Serial.begin(9600);
  
//  myData.id = 0;
  myData.active_monitor = 1;
  sendingChanges = true;

  
  
  
  initWifi();

   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Test");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();

  webSocket.begin();  // init the Websocketserver
  
  // use HTTP Basic Authorization this is optional remove if not needed
//  webSocket.setAuthorization("test", "26826822");
  
  webSocket.onEvent(webSocketEvent);  // init the webSocketEvent function when a websocket event occurs 

     for (byte i = 0; i < sizeof(boardsStruct)/sizeof(struct_message);i++) {
      boardsStruct[i].ip = clients[i];
     }
}
  
void loop() {
  AsyncElegantOTA.loop();
  webSocket.loop();  
//  webSocket.cleanupClients();
//  digitalWrite(ledPin, ledState);
//  Serial.println(externalData.active_monitor);
  
  bool switchButtonState = digitalRead(button);
  if((switchButtonState == pressedOn) || (externalData.active_monitor == 1)) {
//    Serial.println("Pressed button ON");
    myData.active_monitor = 1;
    if (digitalRead(redLed) == LOW){
      broadcastToClients();
      digitalWrite(redLed, HIGH);
      snprintf(mailStr, sizeof(mailStr), "%s-Status prekidaca server-%d-%d", location.c_str(), (int) BOARD_ID,myData.active_monitor);
      makeIFTTTHTTPRequest(mailStr, (char*)WiFi.macAddress().c_str(), (char*)WiFi.localIP().toString().c_str());
      }
      
    if (digitalRead(greenLed) == HIGH) {
      broadcastToClients();
      digitalWrite(greenLed, LOW);
    }
//    if (sendingChangesStateBefore != switchButtonState) {
//      sendingChanges = true;
//    }
    }
   else {
    myData.active_monitor = 0;
//    Serial.println("Pressed button OFF");
    if (digitalRead(redLed) == HIGH){
      broadcastToClients();
      digitalWrite(redLed, LOW);
      }
    if (digitalRead(greenLed) == LOW){
      broadcastToClients();
      digitalWrite(greenLed, HIGH);
      snprintf(mailStr, sizeof(mailStr), "%s-Status prekidaca server-%d-%d", location.c_str(), (int) BOARD_ID,myData.active_monitor);
      makeIFTTTHTTPRequest(mailStr, (char*)WiFi.macAddress().c_str(), (char*)WiFi.localIP().toString().c_str());
      }
//    if (sendingChangesStateBefore != switchButtonState) {
//      sendingChanges = true;
//    }        
  }

    if (((millis()/1000) - lastTime) > timerDelay) {
//    if (sendingChanges == true){
////          esp_now_send(0, (uint8_t *) &boardsStruct[0], sizeof(boardsStruct[0]));  
//        broadcastToClients();
//    } 
    broadcastToClients();
//    Serial.println(boardsStruct[3
//    Serial.println("Send");
   for (byte i = 0; i < sizeof(boardsStruct)/sizeof(struct_message);i++) {
    if ((((unsigned long)((millis()/1000) - boardsStruct[i].lastPing)) > pingTimerDelay) && (boardsStruct[i].email_to_send == 0)) {
      
      snprintf(mailStr, sizeof(mailStr), "%s-Nedostupan klijent-%d.%d.%d.%d", location.c_str(), boardsStruct[i].ip[0],boardsStruct[i].ip[1],boardsStruct[i].ip[2],boardsStruct[i].ip[3]);
      makeIFTTTHTTPRequest(mailStr,(char*)WiFi.macAddress().c_str(), (char*)WiFi.localIP().toString().c_str());
      boardsStruct[i].email_to_send = 1;
      digitalWrite(orangeLed, HIGH);
    }
//    
    if ((((unsigned long)((millis()/1000)-boardsStruct[i].lastPing)) <= pingTimerDelay) && (boardsStruct[i].email_to_send == 1)) {
      snprintf(mailStr, sizeof(mailStr), "%s-Dostupan klijent-%d.%d.%d.%d", location.c_str(), boardsStruct[i].ip[0],boardsStruct[i].ip[1],boardsStruct[i].ip[2],boardsStruct[i].ip[3]);
      makeIFTTTHTTPRequest(mailStr,(char*)WiFi.macAddress().c_str(), (char*)WiFi.localIP().toString().c_str());
      boardsStruct[i].email_to_send = 0;
      digitalWrite(orangeLed, LOW);
    }
   }
//    
//   }

//    if ((sendingChanges == true) && ((millis() - lastTime2) > sendingChangesDelay)) {
//      sendingChanges = false;
//      lastTime2 = millis()/1000;
//    }
   
//    sendingChangesStateBefore = switchButtonState;
    lastTime = millis()/1000;
  }




  if (((millis()/1000) - lastTime2) > wifiDelay) {
    if ((wifiDelayCounter <= 5) && (WiFi.status() != WL_CONNECTED)){
//      Serial.println(wifiDelayCounter);
      wifiDelayCounter += 1;
    }
    if (WiFi.status() == WL_CONNECTED){
//      Serial.println(wifiDelayCounter);
      wifiDelayCounter = 0;
    }
    if (wifiDelayCounter >= 3) {
      ESP.restart();    
    }
  lastTime2 = millis()/1000;
  }


    if (((millis()/1000) - lastTime3) > availabilityTimer) {
       makeIFTTTHTTPRequest("Javljanje uredjaja na 4h",(char*)WiFi.macAddress().c_str(), (char*)WiFi.localIP().toString().c_str());
       lastTime3 = millis()/1000;
  }   
}
