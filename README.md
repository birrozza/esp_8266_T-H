# ESP_8266 Temperature & Humidity project
<p>
<img alt="GitHub" src="https://img.shields.io/github/license/birrozza/esp_8266_T-H">
<img alt="GitHub all releases" src="https://img.shields.io/github/downloads/birrozza/esp_8266_T-H/total">

</p>
My wireless temperature and humidity sensor needed a case to protect it from the weather. I designed and printed this my idea of  case for this sensor consisting of a Wemos D1 mini card (a clone of it is fine too) and a DHT22 module.

<p>The DHT22 module, thanks to its sensors, can detect the temperature and humidity of the surrounding environment, and send this data to the Wemos board which, thanks to its wifi module, can transmit them to a PC or smartphone.</p>

In this case, thanks to code inside (sketch), the data is sent to a web platform (www.thingspeak.com) and displayed through graphics. Moreover, these and other data can be viewed on all network devices because the wemos card works as a <strong>web-server </strong>(see below)


[This is my chanel were data are sent ](https://thingspeak.com/channels/867219)  The data are in real-time

<h3>Necessary material:</h3>

<blockquote>
<ul>
<li><p> <a href="https://www.banggood.com/Geekcreit-D1-mini-V2_2_0-WIFI-Internet-Development-Board-Based-ESP8266-4MB-FLASH-ESP-12S-Chip-p-1143874.html?p=MX1504307245201310VT&custlinkid=673886">Wemos D1 mini clone based on ESP8266 chip </a></p></li>
<li><p> <a href="https://www.banggood.com/AM2302-DHT22-Temperature-And-Humidity-Sensor-Module-For-Arduino-SCM-p-937403.html?rmmds=search&cur_warehouse=CN">DHT 22 sensor (temeperature & Humidity)</a></p></li>
<li><p> <a href="https://www.thingiverse.com/thing:3965015">My printable case project on Thingiverse.com</a></p></li>
</ul>
</blockquote>

<h3>Arduino code and library:</h3>
<blockquote> 
<ul>
<li>
<p> <a href="https://github.com/birrozza/esp_8266_T-H">Here you can download the arduino sketch (Git-Hub repository)</a></p>
</li>
<li>
<p> <a href="https://github.com/alanswx/ESPAsyncWiFiManager">ESPAsyncWiFiManager library</a></p>
</li>
</ul>
</blockquote>
<h3>Software istruction</h3>

<p>First you need to register the service (free but with some limitations) here:  <a href="https://thingspeak.com">www.thingspeak.com</a>. 
<br> 
 So,  download the code for Arduino from the <a href="https://github.com/birrozza/esp_8266_T-H">link</a> above.
<br>
With the Arduino IDE, you must enter in the <b>_secret.h_</b> file and update the <em><strong>Channel ID</em></strong> and the <em><strong>Channel Write Key</strong></em> values whith obtained when the service was activated on Thingspeak.</p>

In this project, in addition to managing the DHT22 sensor, the Wemos card also acts as a <strong>web-server</strong>. For this purpose, <strong>web pages</strong> have been created which function as a user interface. These must be loaded into the SPIFFS of the ESP8266. Here the plug-in about it [ESP8266 Sketch Data Upload](https://github.com/esp8266/arduino-esp8266fs-plugin)

When your ESP starts up, it sets it up in Station mode and tries to connect to a previously saved Access Point.
If this is unsuccessful (or no previous network saved) it moves the ESP into Access Point mode and spins up a  WebServer (default ip <strong>192.168.4.1</strong>)
Using any wifi enabled device with a browser (computer, phone, tablet) connect to the newly created Access Point (<strong>ESP8266 AP Wifi</strong>).
Because of the Captive Portal and the DNS server you will either get a 'Join to network' type of popup or get any domain you try to access redirected to the configuration portal.
Choose one of the access points scanned, enter password, click save.
ESP will try to connect. If successful, it relinquishes control back to your app. If not, reconnect to AP and reconfigure.
[Here this tutorial](https://github.com/alanswx/ESPAsyncWiFiManager)

On the <strong>setting page</strong> you can find different values, such as the <strong>IP address</strong> of the card, its <strong>ID name</strong>, the name of the <strong>wi-fi</strong> network and its power (<strong>RSSI</strong>), and other specific card data.
Below, in the <strong>File Manager</strong> section, you have the image of the <strong>SPIFFS</strong> and further down, you have the possibility to update the Html files as well as upload new ones.

<h3>Configuration</h3>

Through the <b>_config.json_</b> file it is possible to configure the program.
In fact, in the file we find the following fields and modifiable sub fields:

*  <b>"_board_"</b>:

    *  <b>"_board_id_"</b>: the identification name of the card;
    *  <b>"_type_"</b>: it must remain unchanged;
    *  <b>"_local_host_name_"</b>: the host name of the card to connect to with your browser;
    *  <b>"_rateo_"</b>: constant used to vary the time between one reading and another.

*  <b>"_login_"</b>:    
    *  <b>"_user_"</b>: user;
    *  <b>"_password_"</b>: password.
    

*  <b>"_location_"</b>:    
    *  <b>"_city_"</b>: the name of the city where the sensor is installed;
    *  <b>"_country_"</b>: the name of the country where the sensor is installed.
    

*  <b>"_field_1_"</b> & <b>"_field_2_"</b>:    
    *  <b>"_name_"</b>: it must remain unchanged;
    *  <b>"_url_"</b>: Thingspeak channel fields URL.

The other fields are not currently used.



<h2><strong>+ NEWS +  Enabled a Telegram bot to be able to interrogate the sensor </strong></h2>
First of all you need to create a bot on the Telegram with <b>BothFather</b> function. The procedure 
can be found at this <a href="https://core.telegram.org/bots#6-botfather">link</a>.

Obtained the token of our new bot, it must be inserted in the <b>_secret.h_</b> file.

Through the bot, you can interrogate the sensor with commands:
<ul>
    <li><b>"read"</b>:  returns the latest temperature and humidity reading;</li>
    <li><b>"wifi"</b>:  returns the parameters of the wifi connection;</li>
    <li><b>"stato"</b>: returns the last status of the data transmission to ThingSpeak.</li>
</ul> 
<br>
<strong>Stay tuned!!! (last update december 2021)</strong>

If you want to support me, you can always donate to my paypal account https://www.paypal.me/birrozza  or tiping me. Thanks