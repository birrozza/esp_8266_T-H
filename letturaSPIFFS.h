//https://www.mischianti.org/it/2019/08/30/wemos-d1-mini-esp8266-sistema-operativo-integrato-spiffs-parte-2/
#include "Arduino.h"
#include "FS.h"
 
String letturaSPIFFS()
{
    
    // Get all information of your SPIFFS
    FSInfo fs_info;
    SPIFFS.info(fs_info);
 
    Serial.println("File sistem info.");
 
    Serial.print("Total space:      ");
    Serial.print(fs_info.totalBytes);
    Serial.println("byte");
 
    Serial.print("Total space used: ");
    Serial.print(fs_info.usedBytes);
    Serial.println("byte");
 
    Serial.print("Block size:       ");
    Serial.print(fs_info.blockSize);
    Serial.println("byte");
 
    Serial.print("Page size:        ");
    Serial.print(fs_info.totalBytes);
    Serial.println("byte");
 
    Serial.print("Max open files:   ");
    Serial.println(fs_info.maxOpenFiles);
 
    Serial.print("Max path lenght:  ");
    Serial.println(fs_info.maxPathLength);
 
    Serial.println();
 
    // Open dir folder
    Dir dir = SPIFFS.openDir("/");
    String stringa="dir :\ \n";
    // Cycle all the content
    while (dir.next()) {
        // get filename
        Serial.print(dir.fileName());
        Serial.print(" - ");
        stringa+=dir.fileName()+" - ";
        // If element have a size display It else write 0
        if(dir.fileSize()) {
            File f = dir.openFile("r");
            Serial.println(f.size());
            stringa+= String(f.size())+"\n";
            f.close();
        }else{
            Serial.println("0");
            stringa+="0\n";
        }
    }
  return stringa;
}
