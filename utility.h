String msgFeedBack(int httpCode) { /// diagnostica: codici d'errore ThinkSpeak

  switch (httpCode) {
    case  200: return String("Code (200) OK / Success");
    case  400: return String("Code (400) The request cannot be fulfilled due to bad syntax");
    case  404: return String("Code (404) Incorrect API key (or invalid ThingSpeak server address)");
    case -101: return String("Code (-101) Value is out of range or string is too long (> 255 characters)");
    case -201: return String("Code (-201) Invalid field number specified");
    case -210: return String("Code (-210) setField() was not called before writeFields()");
    case -301: return String("Code (-301) Failed to connect to ThingSpeak");
    case -302: return String("Code (-302) Unexpected failure during write to ThingSpeak");
    case -303: return String("Code (-303) Unable to parse response");
    case -304: return String("Code (-304) Timeout waiting for server to respond");
    case -401: return String("Code (-401) Point was not inserted (most probable cause is the rate limit of once every 15 seconds)");
    case    0: return String("Code (0) altro errore!");
    default  : return String("Codice non identificato!!! ("+String(httpCode)+")!!!");
  }
}
/*
void aggStato(client) {
  HTTPClient http;    //Declare object of class HTTPClient
  //https://api.thingspeak.com/update.json?api_key=F32EPARZLNSE4W0K&status=test
  http.begin(client,"https://api.thingspeak.com/update.json");      //Specify request destination
  http.addHeader("Content-Type", "text/plain");  //Specify content-type header
  int httpCode = http.POST("?api_key=F32EPARZLNSE4W0K&status=test");   //Send the request
  String payload = http.getString();                  //Get the response payload
 
   Serial.println(httpCode);   //Print HTTP return code
   Serial.println(payload);    //Print request response payload
 
   http.end();  //Close connection
}
*/
