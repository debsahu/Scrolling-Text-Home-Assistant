#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

// Update these with values suitable for your network.
const char* ssid = "WiFi_SSID";
const char* password = "WiFi_Password";
IPAddress server(192, 168, 0, 100); // MQTT Server IP
const char* host = "scrolling-text-home-assistant";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "password";
uint8_t nStatus = 0;

const char* inMsgTopic = "ScrollingText/message";
const char* inHTempTopic = "homeassistant/sensor/average_home_temperature/state";
const char* inOTempTopic = "homeassistant/sensor/dark_sky_temperature/state";
const char* inPrecipitationTopic = "homeassistant/sensor/dark_sky_hourly_summary/state";
const char* inPollutionTopic = "homeassistant/sensor/us_air_pollution_level/state";
const char* inIconTopic = "homeassistant/sensor/dark_sky_template/state";

String scrolling_msg = "";
String datentp="";
String timentp="";
String HTemp="";
String OTemp="";
String Precipitation="";
String Pollution="";
String Icon="";
boolean otabool=false;

//Custom Font
uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 }; // Deg C
uint8_t tempIcon[] = { 5, 32, 80, 143, 80, 32}; // Thermometer Icon

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = (char*)payload;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if(strcmp(topic,inMsgTopic)==0) {
    scrolling_msg=message;
  }else if(strcmp(topic,inHTempTopic)==0) {
    HTemp="H~: " + message + " $";
  }else if(strcmp(topic,inOTempTopic)==0) {
    OTemp="O~: " + message + " $";
  }else if(strcmp(topic,inPrecipitationTopic)==0) {
    Precipitation=message;
  }else if(strcmp(topic,inPollutionTopic)==0) {
    Pollution="Air Q: " + message;
  }else if(strcmp(topic,inIconTopic)==0) {
    Icon=message;
  }else if(strcmp(topic,"ScrollingText/ota")==0) {
    if(message == "1") {
      otabool=true;
    }else{
      otabool=false;
    }
  }
}

WiFiClient ethClient;
PubSubClient client(ethClient);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect("ScrollingTextHomeAssistant")) {
      client.subscribe(inMsgTopic);
      client.subscribe(inHTempTopic);
      client.subscribe(inOTempTopic);
      client.subscribe(inPrecipitationTopic);
      client.subscribe(inPollutionTopic);
      client.subscribe(inIconTopic);
      client.subscribe("ScrollingText/ota");
  }
  return client.connected();
}

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define	MAX_DEVICES	8
#define	CLK_PIN		D5
#define	DATA_PIN	D7
#define	CS_PIN		D8

MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

int mapdotmatrix(float x)
{
  float in_min = 0.0, in_max = 1023.0, out_min = 1.0, out_max = 16.0;
  float mapfloat = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  if(mapfloat>15.0)
    mapfloat=15.0;
  return (int)mapfloat;
}

void mdns_begin() {
  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
}

void setup_wifi() {
  // config static IP
  IPAddress ip(192, 168, 1, xx);     // where xx is the desired IP Address
  IPAddress gateway(192, 168, 1, 1); // set gateway to match your network
  Serial.print(F("Setting static ip to : "));
  Serial.println(ip);
  IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1500);
}

void setup_NTP() {
  NTP.begin("pool.ntp.org", 1, true);
  NTP.setInterval(63);
  NTP.setTimeZone(-5);

  NTP.onNTPSyncEvent([](NTPSyncEvent_t ntpEvent) {
    if (ntpEvent) {
      Serial.print("Time Sync error: ");
      if (ntpEvent == noResponse)
        Serial.println("NTP server not reachable");
      else if (ntpEvent == invalidAddress)
        Serial.println("Invalid NTP server address");
    }
    else {
      Serial.print("Got NTP time: ");
      Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
    }
  });
}

void setup(void)
{
  Serial.begin(115200);

  client.setServer(server, 1883);
  client.setCallback(callback);
  lastReconnectAttempt = 0;

  setup_wifi();
  setup_NTP();
  mdns_begin();
  
  P.begin();
  P.addChar('$', degC);
  P.addChar('~', tempIcon);
  P.setInvert(false);
  P.setIntensity(7);
}

void loop(void)
{
    if (!client.connected()) {
      long now = millis();
      if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        // Attempt to reconnect
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
    } else {
      client.loop(); yield();
    }
    
    int intensity = mapdotmatrix(analogRead(A0)); yield();
    P.setIntensity(intensity);
      
    if(otabool){
          P.displayClear();
          httpServer.handleClient(); yield();
    }else{
      if (P.displayAnimate()) // True if animation ended
      {
        switch (nStatus) {
          if (nStatus >14){nStatus ==0;}
        case 0:
          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
          break;
        case 1:
          datentp=NTP.getDateStr();
          P.displayText(const_cast<char*>(datentp.c_str()),PA_CENTER,25,4000,PA_WIPE_CURSOR,PA_WIPE_CURSOR);
          break;
        case 2:
          timentp=NTP.getTimeStr();
          P.displayText(const_cast<char*>(timentp.c_str()),PA_CENTER,25,4000,PA_WIPE_CURSOR,PA_WIPE_CURSOR);
          break;
        case 3:
          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
          break;
        case 4:
          P.displayText(const_cast<char*>(scrolling_msg.c_str()),PA_CENTER,25,1000,PA_SCROLL_LEFT,PA_SCROLL_LEFT);
          break;
        case 5:
          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
          break;
        case 6:
          P.displayText(const_cast<char*>(HTemp.c_str()),PA_CENTER,25,5000,PA_CLOSING_CURSOR,PA_CLOSING_CURSOR);
          break;
        case 7:
          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
          break;  
        case 8:
          P.displayText(const_cast<char*>(OTemp.c_str()),PA_CENTER,25,5000,PA_CLOSING_CURSOR,PA_CLOSING_CURSOR);
          break;  
        case 9:
          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
          break;  
        case 10:
          P.displayText(const_cast<char*>(Precipitation.c_str()),PA_CENTER,25,1000,PA_SCROLL_LEFT,PA_SCROLL_LEFT);
          break;  
        case 11:
          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
          break; 
        case 12:
          P.displayText(const_cast<char*>(Pollution.c_str()),PA_CENTER,25,5000,PA_CLOSING_CURSOR,PA_CLOSING_CURSOR);
          break; 
        case 13:
          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
          break; 
        case 14:
          P.displayText(const_cast<char*>(Icon.c_str()),PA_CENTER,25,5000,PA_CLOSING_CURSOR,PA_CLOSING_CURSOR);
          break; 
//        case 15:
//          P.displayText("@debsahu",PA_CENTER,25,1000,PA_OPENING_CURSOR,PA_OPENING_CURSOR);
//          break;
        }
        client.loop(); yield();
        nStatus ++;
      }
    }
}
