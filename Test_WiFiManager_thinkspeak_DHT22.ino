// per ESP8266 clone WeMos D1 R1 usare WeMos D1 R1

#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
//#include <ArduinoJson.h>
#include "ESP8266WiFi.h"
//needed for library
#include <FS.h>
#include "ESPAsyncWiFiManager.h"
#include "ESP8266mDNS.h"        // Include the mDNS library
#include "ThingSpeak.h"
#include <ESP8266HTTPClient.h>
#include "Ticker.h"

#include "utility.h"
#include "secret.h"   // 
#include "sensore_DHT_22.h"     //  
#include "testiHTML.h" // la sintassi HTML con indentazione leggibile
#include "letturaSPIFFS.h"

//// variabili

unsigned long  myChannelNumber = SECRET_CH_ID;
const char     * myWriteAPIKey = SECRET_WRITE_APIKEY;
WiFiClient     client;
String         boardName;
AsyncWebServer server(80);
DNSServer      dns;
sensors_event_t event;

//for LED status
Ticker ticker;
int count=148000000;

  // function prototypes for HTTP handlers
void dirRequest (AsyncWebServerRequest *request);
void handleLogin(AsyncWebServerRequest *request);
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final); 



//// metodi
 
void tick() {
  //toggle state
  int state = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !state);
}
 
//gets called when WiFiManager enters configuration mode
void configModeCallback (AsyncWiFiManager  *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}
 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
   
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
 
  //WiFiManager
  //Local intialization. Once its business is done,
  //there is no need to keep it around
  AsyncWiFiManager wifiManager(&server,&dns);
  //reset settings - for testing
  //wifiManager.resetSettings();
 
  //set callback that gets called when connecting to previous WiFi fails
  //and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
 
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("ESP8266 AP Wifi")) {// INSERISCI QUI IL NOME DELLA RETE COME ACCES POINT
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
 
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

  server.on("/", HTTP_GET,  [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/login",     HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login" 
  server.on("/login",     HTTP_GET , [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/login.html", "text/html");
  });
  server.on("/iprequest", HTTP_POST, [](AsyncWebServerRequest *request) {
    String ip="IP "+WiFi.localIP().toString();
    Serial.println(ip);
    request->send(200, "text/html", ip);
  });
  server.on("/setpage", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/setting.html","text/html");   
  });
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) { // chiedi il reset
    request->send(SPIFFS, "/index.html", "text/html");  
    ESP.reset();
  });
  server.on("/directory", HTTP_GET,  dirRequest);
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
  },handleFileUpload);
  
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/html", sitoNonTrovato);  
  });
 
  
  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  // modify TTL associated  with the domain name (in seconds)
  // default is 60 seconds
  //dnsServer.setTTL(300);
  // set which return code will be used for all other domains (e.g. sending
  // ServerFailure instead of NonExistentDomain will reduce number of queries
  // sent by clients)
  // default is DNSReplyCode::NonExistentDomain
  //dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
/*
  // start DNS server for a specific domain name
  if(!dnsServer.start(DNS_PORT, "angelo", WiFi.localIP())){
    Serial.println("dnsserver fallito");
  }
    Serial.println("dnsserver partito");
*/  

   if (!MDNS.begin("myesp", WiFi.localIP(),3600)) {             // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
   // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  WiFi.mode(WIFI_STA);

  ThingSpeak.begin(client);
  SPIFFS.begin();                           // Start the SPI Flash Files System
  String a=letturaSPIFFS();   /// variabile inutilizzata
  File txtFile = SPIFFS.open(F("/board.txt"),"r"); // apri il file board.txt dove c'è il nome della board
  boardName = txtFile.readString();
  Serial.println(boardName); // stampala sulla console
  txtFile.close(); // chiudi il file
  sensore_DHT_22(); //setup del sensore
  
} // end setup

String stato=" ESP start, ";
String statoIp=stato; 

void loop() {
  
 // put your main code here, to run repeatedly:
 
  MDNS.update();
  //sensors_event_t event;

  if (WiFi.status() != WL_CONNECTED) ESP.reset(); // verifica lo stato della  connessione

  if (stato!="---") statoIp=stato; // conserva l'ultimo stato utile da riportare sul web
    else statoIp="All ok";
  
  if (count==150000000){  // 5 milioni sono circa 1 minuto (150.000.000
                        //50 milioni sono circa 10 minuti   
    
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
      stato=stato+" Error reading temperature";
      count-=20000;
    } else {
      Serial.print(F("Temperatura: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      // Write value to Field 1 of a ThingSpeak Channel
      ThingSpeak.setField(1, event.temperature);
    }
  }    
  if (count==150500000) {  
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
      count-=20000;
      stato=stato+" Error reading humidity";
    } else {
      float hum=event.relative_humidity;//0.9-34.6; // per calibrare il valore dell'umidità
      Serial.print(F("Umidità: "));
      Serial.print(hum);
      Serial.println(F("%"));
      // Write value to Field 1 of a ThingSpeak Channel
      ThingSpeak.setField(2, hum);
      stato=stato+" Read ok";
      ThingSpeak.setStatus(stato);
      stato="---"; // azzera lo stato
    } 
  }
  if (count==159500000) {  // garantisce una lettura quasi ogni 30 minuti
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if(x == 200){
      Serial.println("Channel update successful.");
      //statoIP+=" ch update ok";
      count=0;
    } else {
      Serial.println("Problem updating channel. HTTP error  " + msgFeedBack(x)); 
      stato=stato+"*** Problem updating channel last time. HTTP error " + msgFeedBack(x)+" --- "; 
      count=140000000; // se ci sono problemi nell'aggiornamento dei dati, fa tutto nuovamente
    }
  }
  count++;
} // end loop

///////// WEB SERVER metod ///////////////////


void handleLogin(AsyncWebServerRequest *request) {                         // If a POST request is made to URI /login
  Serial.println("in handlelogin");
  if( !request->hasArg("username") || !request->hasArg("password") // verifica se li argomenti esistono
      || request->arg("username") == NULL || request->arg("password") == NULL) { // If the POST request doesn't have username and password data
    request->send(400, "text/html", invalidRequest);         // The request is invalid, so send HTTP status 400
    return;
  }
  if(request->arg("username") == "Heineken" && request->arg("password") == "password123") { // If both the username and the password are correct
    
    request->send(SPIFFS, "/login.html");
    } else {                                                                              // Username and password don't match
      request->send(401, "text/html", unauthorized);// unauthorized = stringa in formato HTML (vedi testiHTML.h)
  }
}

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.println((String)"UploadStart: " + filename);
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open(filename, "w");
  }
  if(len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data,len);
  }
  if(final){
    Serial.println((String)"UploadEnd: " + filename + "," + index+len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    
    //request->send(200, "text/plain", "File Uploaded !");
    request->redirect("/setpage");
  } 
  /* blocca la scheda
  if (filename == "/board.txt") {// ora  leggi dal file board.txt il nome della scheda qualora sia stato fatto l'upload di questo file
    File txtFile = SPIFFS.open(F("/board.txt"),"r"); // apri il file board.txt dove c'è il nome della board
    boardName = txtFile.readString();
    txtFile.close(); // chiudi il file
  }*/
}

void dirRequest (AsyncWebServerRequest *request){
// {"dir": [{"file": ["filename","size"]},{"file":["filename","size"]}]}    
// per la verifica del json>>>>    https://filosophy.org/code/fixing-syntaxerror-unexpected-string-token-in-json-at-position/ 
// oppure                  >>>>    http://jsonviewer.stack.hu/
    
    String json = "{\"dir\": [";
/// comncia a popolare il json   
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
        // get filename
        json+="{\"file\": [\""+ dir.fileName()+"\",";
        // If element have a size display It else write 0
        if(dir.fileSize()) {
            File f = dir.openFile("r");
            json+="\""+ String(f.size())+"\"]},";
            f.close();
        }else{
            json+="\"0\"]},";
        }
    }
    json.remove(json.length()-1,1); // togli la virgola
    json+="], \"ip\": \"" + WiFi.localIP().toString()+"\","; // inserisce nel json l'indirizzo ip e ci mette la virgola per il prossimo elemento

/// inseriamo nel json i datitecnici della scheda
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    json+="\"infosys\": [\"Total Space:     " + String(fs_info.totalBytes)    + " byte\", ";
    json+=              "\"Space Used:      " + String(fs_info.usedBytes)     + " byte\", ";
    json+=              "\"Block Size:      " + String(fs_info.blockSize)     + " byte\", ";
    json+=              "\"Page Size:       " + String(fs_info.totalBytes)    + " byte\", ";
    json+=              "\"Max open files:  " + String(fs_info.maxOpenFiles)  + " files\", ";
    json+=              "\"Max path lenght: " + String(fs_info.maxPathLength) + "\", ";
    json+=              "\"Count:           " + String(count)                 + "\", "; // inserisco il conteggio per la lettura
    json+=              "\"Stato:           " + statoIp                         + "\"]"; // inserisco stato
    
    json+=", \"RSSI\": \""+ String(WiFi.RSSI())+"\""; // inserisco nel json il valore del segnale wifi

    json+=", \"SSID\": \""+ String(WiFi.SSID())+"\""; // inserisco nel json il nome del ssid
    
    json+=", \"Board_Name\": \"" + boardName + "\""; // inserisco il nome identificativo della scheda letto dal file board.txt

   // json+=", \"Temperature\": \"" + temperatureRead(event) + "\"";

   //    json+=", \"Humidity\": \"" + humidityRead(event) + "\"";
        
    json+="}"; // chiude il json
    //Serial.println("in dirRequest, json= "+ json);
    Serial.println("StatoIp -> "+ statoIp);
    request->send(200, "text/json", json);
}

/*
{
  "dir": [
    {
      "file": [
        "/board.txt",
        "10"
      ]
    },
    {
      "file": [
        "/index.html",
        "6933"
      ]
    },
    {
      "file": [
        "/login.html",
        "4468"
      ]
    },
    {
      "file": [
        "/setting.html",
        "7426"
      ]
    }
  ],
  "ip": "192.168.1.71",
  "infosys": [
    "Total Space:     957314 byte",
    "Space Used:      20331 byte",
    "Block Size:      8192 byte",
    "Page Size:       957314 byte",
    "Max open files:  5 files",
    "Max path lenght: 32",
    "Count:           149409355",
    "Stato:            ESP start, "
  ],
  "RSSI": "-83",
  "SSID": "FASTWEB-F31559",
  "Board_Name": "esp_aef385"
}*/
