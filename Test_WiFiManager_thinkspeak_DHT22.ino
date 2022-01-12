/* ------------- MEMO -----------------
  - per ESP8266 clone WeMos D1 R1 usare LOLIN(WeMos) D1 R1
  - esp core 3.0.2 funzionante con telegram https://github.com/esp8266/Arduino
  - driver esp8266 http://www.wch.cn/download/CH341SER_ZIP.html
*/
#include "ESP8266WiFi.h"        // library ESP8266 Arduino Core (ver 3.0.2)
#include "Ticker.h"             // library ESP8266 Arduino Core (ver 3.0.2)
#include "ESP8266mDNS.h"        // library ESP8266 Arduino Core (ver 3.0.2)
#include <FS.h>                 // library ESP8266 Arduino Core (ver 3.0.2)
#include "ESPAsyncTCP.h"        // https://github.com/me-no-dev/ESPAsyncTCP
#include "ESPAsyncWebServer.h"  // https://github.com/me-no-dev/ESPAsyncWebServer
#include "ESPAsyncWiFiManager.h"// WifiManager 2.0.3 alpha
#include "AsyncTelegram.h"      // ver 1.1.3
#include <ArduinoJson.h>        // ver 6.15.1
#include "ThingSpeak.h"         // ver 1.5.0 -> 2.0.1
#include <TimeLib.h>            // ver 1.6.1   
#include <math.h>
       
#include "utility.h"
#include "C:\Users\computer\Desktop\esp8266 sketch\secret.h"   // always comment on this line (for development only) 
//#include "secret.h" // uncomment if it's commented out
#include "sensore_DHT_22.h"     //  
#include "testiHTML.h" // la sintassi HTML con indentazione leggibile
//#include "letturaSPIFFS.h"

#define GOOGLE_SHEET_ENABLED //video 529
#define TELEGRAM_BOT_ENABLED

//// variable
unsigned long  myChannelNumber = SECRET_CH_ID;        // ┌───────────┐
const char     * myWriteAPIKey = SECRET_WRITE_APIKEY; // │from secret.h file│
const char* token = SECRET_TELEGRAM_BOT_TOKEN;        // └───────────┘
WiFiClient     client;
String         boardName;
String         password;
String         user;
String         hostName="myesp";
AsyncWebServer server(80);
DNSServer      dns;
sensors_event_t event;
float lastTemperatureRead = 0.0;
float lastHumidityRead = 0.0;
AsyncTelegram myBot;
TBMessage msg;
String stato=" ESP start"; // stato corrente del sistema
String statoIp=stato;
int count=0;//14800000 * rateo;  
//float rateo = 1.57; // costante per regolare l'intervallo delle letture

//for LED status
Ticker ticker;


  // function prototypes for HTTP handlers
void dirRequest (AsyncWebServerRequest *request);
void handleLogin(AsyncWebServerRequest *request);
void handleIpRequest(AsyncWebServerRequest *request);
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
  setTime(05,58,00,9,1,2022); // inserisco una data fittizia dal quale calcolare i timer, 5 min prima dalla prima lettura
  count = minute(); // setto al minuto del tempo impostato
  Serial.begin(57600);
  SPIFFS.begin();   // Start the SPI Flash Files System
  
  File txtFile = SPIFFS.open(F("/config.json"),"r"); // apri il file di configurazione
  String configuration = txtFile.readString();
  txtFile.close(); // chiudi il file
  Serial.println("\nConfiguration file uploaded!!!\n");
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, configuration);

  boardName = String(doc["board"]["board_id"]);
  hostName = String(doc["board"]["local_host_name"]);
  user = String(doc["login"]["user"]);
  password = String(doc["login"]["password"]);
  //rateo = float(doc["board"]["rateo"]);
  
  //myChannelNumber = (doc["thinkspeak"]["id"]);
  //myWriteAPIKey = (doc["thinkspeak"]["api_key"]);
  //const char* token = (doc["Telegram_bot"]["token"]);
    
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
 
  //WiFiManager
  //Local intialization. Once its business is done,
  //there is no need to keep it around
  WiFi.persistent(true);
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

  // setup server services
  server.on("/", HTTP_GET,  [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/login",     HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login" 
  server.on("/login",     HTTP_GET , [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/login.html", "text/html");
  });
  /*
  server.on("/iprequest", HTTP_POST, [](AsyncWebServerRequest *request) {
    String ip="IP "+WiFi.localIP().toString();
    Serial.println(ip);
    request->send(200, "text/html", ip);
  });
  */
  server.on("/iprequest", HTTP_GET, handleIpRequest);
  server.on("/setpage", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/setting.html","text/html");   
  });
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) { // chiedi il reset
    request->send(SPIFFS, "/index.html", "text/html");  
    ESP.restart(); // avvio completo della scheda
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

  if (!MDNS.begin(hostName, WiFi.localIP(),3600)) {             // Start the mDNS responder for myesp.local
        Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  WiFi.mode(WIFI_STA);

  ThingSpeak.begin(client);
  
  sensore_DHT_22(); //setup del sensore

  // Set the Telegram bot properies
  myBot.setClock("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00");  //CET-1CEST,M3.5.0,M10.5.0/3
  myBot.setUpdateTime(2000);
  myBot.setTelegramToken(token);
  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  //myBot.begin() ? Serial.println("Telegram OK") : Serial.println("Telegram NO OK");
  if (myBot.begin()){
    Serial.println("Telegram OK");
    //alla fine mandi un messaggio al bot telegram dopo un'attesa di 2 sec
    delay(2000);
    String replay = "Avvio " + boardName;
    myBot.sendMessage(msg, replay);
  } else {
    Serial.println("Telegram NO OK");
  }
  
} // end setup

void loop() {
  MDNS.update();
  //sensors_event_t event;

  if (WiFi.status() != WL_CONNECTED) ESP.reset(); // verifica lo stato della  connessione

  if (stato!="---") statoIp=stato; // conserva l'ultimo stato utile da riportare sul web
    else statoIp="All ok";
  
  //if (count==(15000000 * rateo)){  // 5 milioni sono circa 1 minuto (150.000.000
  if((minute()==59 || minute()== 29) && (second()==0)){ //alla mezz'ora e al cambio ora leggo temperatura e umidità
    dht.temperature().getEvent(&event);
    float temp = event.temperature;
    if (isnan(event.temperature)) { // se errore lettura temperatura
      Serial.println(F("Error reading temperature!"));
      stato=stato+" Error reading temperature";
      setTime(now()-65);// porto indietro di 1 min l'orologio se ci sono problemi nella lettuta      
    } else {
      Serial.print(F("Temperatura: "));
      Serial.print(temp);
      Serial.println(F("°C"));
      lastTemperatureRead = temp;
    }
    delay(1000);    
    dht.humidity().getEvent(&event);
    float hum=event.relative_humidity;
    if (isnan(event.relative_humidity)) { // se errore lettura umidità
      Serial.println(F("Error reading humidity!"));
      stato=stato+" Error reading humidity";
      setTime(now()-65);// porto indietro di 1 min l'orologio se ci sono problemi nella lettuta    
    } else {
      Serial.print(F("Umidità: "));
      Serial.print(hum);
      Serial.println(F("%"));
      Serial.println("lettura dati ok!!!");
      stato=stato+"Read data ok";
      ThingSpeak.setStatus(stato);
      //stato="---"; // azzera lo stato
      lastHumidityRead = hum;
    }
  }    
  /*
  //if (count==(15050000 * rateo)) {  
  if((minute()==2 || minute()== 32) && (second()==0)){ // dopo due minuti leggo umidità
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
      //count-=2000 * rateo;
      stato=stato+" Error reading humidity";
      setTime(now()-60);// porto indietro di 1 min l'orologio se ci sono problemi nella lettuta
    } else {
      float hum=event.relative_humidity;//0.9-34.6; // per calibrare il valore dell'umidità
      Serial.print(F("Umidità: "));
      Serial.print(hum);
      Serial.println(F("%"));
      // Write value to Field 1 of a ThingSpeak Channel
      //ThingSpeak.setField(2, hum);
      stato=stato+" Read Hum ok";
      ThingSpeak.setStatus(stato);
      stato="---"; // azzera lo stato
      lastHumidityRead = hum;
    } 
  }
  */
  //if (count>=(15950000 * rateo)) {  // garantisce una lettura quasi ogni 30 minuti
  if((minute()==0 || minute()== 30) && (second()==0)){ // dopo altri due minuti invia la lettura
    ThingSpeak.setField(1, lastTemperatureRead);
    ThingSpeak.setField(2, lastHumidityRead);
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if(x == 200){
      Serial.println("Channel update successful.");
      stato="-"+String(x)+"-"; // azzera lo stato
      //statoIP+=" ch update ok";
      //count=0;
      delay(1000);
    } else {
      Serial.println("Problem updating channel. HTTP error  " + msgFeedBack(x)); 
      stato=stato+"* Problem updating channel. HTTP error " + msgFeedBack(x)+" --- "; 
      //count=14000000 * rateo; // se ci sono problemi nell'aggiornamento dei dati, fa tutto nuovamente
      setTime(now()-120); // se ci sono problemi riporta indietro il tempo di 2 min e ricomincia daccapo
    }
  }
  count=minute()>=30? 60-minute():30-minute(); // countdown in base al minuto
  
  //telegram step
  if (myBot.getNewMessage(msg)) { //se è presente un messaggio
    if (msg.text.equalsIgnoreCase("read")) {
      String replay = "Ciao " + String(msg.sender.firstName) + "!!!\nGli ultimi dati letti:";
      replay += "\nLast temperature: " + String(lastTemperatureRead) + "°C";
      replay += "\nLast humidity: " + String(lastHumidityRead) + "%";
      double s = lastTemperatureRead - 37.25*(2 - log10(double(lastHumidityRead)));
      replay += "\nDev point: " + String(s); 
      myBot.sendMessage(msg, replay );
    } // read
    else if (msg.text.equalsIgnoreCase("wifi")){
      String replay = "SSID: "+WiFi.SSID()+"\nRSSI: " +WiFi.RSSI()+"\nIP: " + WiFi.localIP().toString()+"\nLocal host: "+hostName+"\nBoard: "+boardName;
      myBot.sendMessage(msg, replay);
    } // wifi
    else if (msg.text.equalsIgnoreCase("stato")){
      String replay = "Stato: " + stato;
      replay += "\nCount: -"+String(count)+" min";
      myBot.sendMessage(msg, replay);
    } // stato
    else if (msg.text.equalsIgnoreCase("reset")){
      String replay = "Tra 5 secondi la scheda si riavvia" ;
      myBot.sendMessage(msg, replay);
      delay(5000);
      ESP.reset();
    } // reset
    else {
      String replay = "Ciao!!! \nScelte disponibili:\n- read -> temp & Humidity;\n- wifi -> wifi status;\n- stato -> stato.";
      myBot.sendMessage(msg, replay);
    }
  } // end telegram bot
} // end loop

///////// WEB SERVER metod ///////////////////

void handleIpRequest(AsyncWebServerRequest *request){ // invia json con ip, city, country e url_fields
  
  StaticJsonDocument <1024> rispostaJson; //creazione del json di risposta
  DynamicJsonDocument doc(1024); // creazione del contenitore per il json di configurazione
  String json;
  // carica il file config.json
  File txtFile = SPIFFS.open(F("/config.json"),"r"); // apri il file config.json dove c'è la configurazione
  String configuration = txtFile.readString(); //metti nella stringa tutto il testo scaricato
  txtFile.close(); // chiudi il file
  
  deserializeJson(doc, configuration); // da stringa scaricata a json
  
  rispostaJson["ip"] = WiFi.localIP().toString();
  rispostaJson["city"] = doc["location"]["city"];
  rispostaJson["country"] = doc["location"]["country"];
  rispostaJson["temp"] = doc["thinkspeak"]["field_1"]["url"];
  rispostaJson["humi"] = doc["thinkspeak"]["field_2"]["url"];
  
  serializeJson(rispostaJson, json); // da json a stringa per l'invio
  //Serial.print('json: ' + json);
  request->send(200, "text/json", json); //invia json
}

void handleLogin(AsyncWebServerRequest *request) {                         // If a POST request is made to URI /login
  Serial.println("in handlelogin");
  if( !request->hasArg("username") || !request->hasArg("password") // verifica se li argomenti esistono
      || request->arg("username") == NULL || request->arg("password") == NULL) { // If the POST request doesn't have username and password data
    request->send(400, "text/html", invalidRequest);         // The request is invalid, so send HTTP status 400
    return;
  }
  if(request->arg("username") == user && request->arg("password") == password) { // If both the username and the password are correct
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
    // prima di rimandare alla pagina setpage
    Serial.println(filename);
    if(filename=="config.json"){ //se il file caricato è quello di configurazione
        Serial.println("in");
        //** carico in un json il file di configurazione **//
        DynamicJsonDocument fileConfig(1024); // creazione del contenitore per il json di configurazione
        File txtFile = SPIFFS.open(F("/config.json"),"r"); // apri il file config.json dove c'è la configurazione
        String configuration = txtFile.readString(); //metti nella stringa tutto il testo scaricato
        txtFile.close(); // chiudi il file
        deserializeJson(fileConfig, configuration); // da stringa scaricata a json
        //aggiorno i parametri sensibili
        boardName = String(fileConfig["board"]["board_id"]);
        String newHostName = String(fileConfig["board"]["local_host_name"]);
        //rateo = float(fileConfig["board"]["rateo"]);
        user = String(fileConfig["login"]["user"]);
        password = String(fileConfig["login"]["password"]);
        if (newHostName!=hostName){
          hostName=newHostName;
          MDNS.end();
          if (!MDNS.begin(hostName, WiFi.localIP(),3600)) {             // Start the mDNS responder for myesp.local
            Serial.println("Error setting up MDNS responder!");
          }  
        }  
    }    
    request->redirect("/setpage");
  } 
}

void dirRequest (AsyncWebServerRequest *request){
    // per la verifica del json>>>>    https://filosophy.org/code/fixing-syntaxerror-unexpected-string-token-in-json-at-position/ 
    // oppure                  >>>>    http://jsonviewer.stack.hu/
    String json;
    
    //creo il json da inviare
    StaticJsonDocument <2048> doc; /// per creare il json di risposta
    JsonArray dir_json = doc.createNestedArray("dir");
    
    Dir dir = SPIFFS.openDir("/");
    int x = 0;
    while (dir.next()) {
        // get filename
        JsonArray dir_file = dir_json[x++].createNestedArray("file");
        dir_file.add(String(dir.fileName()));
        // If element have a size display It else write 0
        if(dir.fileSize()) {
            File f = dir.openFile("r");
            dir_file.add(String(f.size()));
            f.close();
        }else{
            dir_file.add("0");            
        }
    }
    doc["ip"] = WiFi.localIP().toString();
    /// inseriamo nel json i datitecnici della scheda
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    JsonArray infosys = doc.createNestedArray("infosys");
    infosys.add(String("Total Space:     " + String(fs_info.totalBytes)    + " byte"));
    infosys.add(String("Space Used:      " + String(fs_info.usedBytes)     + " byte"));
    infosys.add(String("Block Size:      " + String(fs_info.blockSize)     + " byte"));
    infosys.add(String("Page Size:       " + String(fs_info.totalBytes)    + " byte"));
    infosys.add(String("Max open files:  " + String(fs_info.maxOpenFiles)  + " files"));
    infosys.add(String("Max path lenght: " + String(fs_info.maxPathLength)));
    if (count!=1){  //se non è all'ultimo minuto conta i minuti
      infosys.add(String("Count down:      -" + String(count) + " min"));
    } else{ // se è all'ultimo minuto conta i secondi 
      infosys.add(String("Count down:      -" + String(60-second()) + " sec"));
    }
    //infosys.add(String("Rateo:           " + String(rateo)));
    infosys.add(String("Status:          " + statoIp + ""));
    doc["RSSI"] = String(WiFi.RSSI());
    doc["SSID"] = String(WiFi.SSID());
    doc["Board_Name"] = boardName;
    doc["Temperature"] = String(lastTemperatureRead);
    doc["Humidity"] = String(lastHumidityRead);
    //doc["Rateo"] = rateo;
    doc["Host_name"] = hostName;
    doc["Count"]= count;
    //doc["bot"] = myBot.begin(); //inchioda la scheda
    //doc["City"] = "Taranto (Italy)";
    
    serializeJson(doc, json);
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
  "Board_Name": "esp_aef385", oppure "esp-bd13a8"
  "Temperature": "28,9",
  "Humidity": "68,9",
  
}*/
