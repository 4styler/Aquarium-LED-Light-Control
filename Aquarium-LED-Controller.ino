//NTP: https://fipsok.de/Projekt/esp8266-ntp-zeit-dual
//WIFI: ESP32 WIFI WPS Example from Arduino IDE
//WIFI Recon: https://stackoverflow.com/questions/48024780/esp32-wps-reconnect-on-power-on
//Website: https://electropeak.com/learn/create-a-web-server-w-esp32/
//Hostname: https://randomnerdtutorials.com/esp32-set-custom-hostname-arduino/#:~:text=The%20default%20ESP32%20hostname%20is,method%20provided%20by%20the%20WiFi.
//String Combine: https://docs.arduino.cc/built-in-examples/strings/StringAdditionOperator
//Read from Site: https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/ form needs to contain the whole Site
//Website with Variable: https://github.com/me-no-dev/ESPAsyncWebServer#send-large-webpage-from-progmem-containing-templates
//Insert in HTML Code: https://techtutorialsx.com/2018/07/23/esp32-arduino-http-server-template-processing-with-multiple-placeholders/
//Perferences: https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
//ESP32Time: https://github.com/fbiego/ESP32Time

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "WiFi.h"
#include "esp_wps.h"
//#include <WebServer.h>

#include <time.h>
#include <sys/time.h>
#include <ESP32Time.h>
ESP32Time rtc;

#include <Adafruit_NeoPixel.h>
#define PIN 22
#define NUMPIXELS 60
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#include <Preferences.h>
Preferences preferences;

const int ledPin = 10;  // 10 corresponds to GPIO10
const int freq = 200;
const int ledChannel = 0;
const int resolution = 8;

#define ESP_WPS_MODE      WPS_TYPE_PBC
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"


String hostname = "Aqua-Light";
int pause_fade = 600;
String code;

String input_whiteDay;
String input_whiteNight;
String input_colorDay1;
String input_colorN1;
String input_split;
String input_startDay;
String input_startNight;
String input_pauseFrom;
String input_pauseTo;
String input_fadeUp;
String input_fadeDown;
String input_save;
int led_split;

int r;
int g;
int b;

const char* split = "split";
int red_day;
int green_day;
int blue_day;
int red_night;
int green_night;
int blue_night;

String colorread;
char* colorNight[32];
char* colorDay[32];

long fade_to;
long fade_to_save;
float colordown_red;
float colordown_green;
float colordown_blue;
float reduce_red;
float reduce_green;
float reduce_blue;
float full_reduce_red;
float full_reduce_green;
float full_reduce_blue;
long today_seconds_save_red;
long today_seconds_save_green;
long today_seconds_save_blue;


char time1[80];
unsigned long epochTime; 

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<title>Aquarium LED Light Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
</head><body style="background-color:#000000; color: #ffffff";>
<form action="/get">
<center>
<h1>Aquarium LED Light Control</h1>
<h3>Light settings:</h3>
<b>White brightness:</b></br>
<table>
  <tr>
    <th>Day</th>
    <th>Night</th>
  </tr>
  <tr style="background-color:#ffffff; color: #000000">
    <td><center><input type="range" name="whiteDay" step="1" value="%pref_whiteDay%" min="0" max="100" oninput="this.nextElementSibling.value = this.value" required><output>%pref_whiteDay%</output></center></td>
    <td><center><input type="range" name="whiteNight" step="1" value="%pref_whiteNight%" min="0" max="100" oninput="this.nextElementSibling.value = this.value" required><output>%pref_whiteNight%</output></center></td>
  </tr>
</table>
</br>

<h3>Mode:</h3>
<label>Split RGB in n Parts (you can choose for every Split a Single Colour):</label></br>
<details>
<summary>Example</summary>
<label>As an example: If you enter a 3 then a color will be displayed for every 3rd LED.</label></br>
<label>1. Split: LED: 1,4,7</label></br>
<label>2. Split: LED: 2,5,8</label></br>
<label>3. Split: LED: 3,6,9</label></br>
<label>1. Split: LED: 10,13,16</label></br>
<label>2. Split: LED: 11,14,17</label></br>
<label>3. Split: LED: 12,15,18</label></br>
<label>and so on.</label></br></p>
</details>
<input type="number" style="text-align:center;" name="split" value="%pref_split%" step="1" min="1" max="12">
</b></br>
<b>RGB settings:</b></br>
<center>
<table border=1 width=500px>
  <tr>
    <th>Color</th>
    <th>Day</th>
    <th>Night</th>
  </tr>
  %PLACEHOLDER_SPLIT%
</table>
</center>
</br>

<h3>Time settings:</h3>
The time for the day must come before the night. Errors are not Handled.
<table>
  <tr>
    <th>Day</th>
    <th>Night</th>
  </tr>
  <tr>
    <td><input type="time" name="startDay" value="%pref_startDay%"></td>
    <td><input type="time" name="startNight" value="%pref_startNight%"></td>
  </tr>
</table>
</br>

Pause (fades over 10 Minutes to Night Mode):</br>
Pause From must be less than Pause To. Break must be within the day. Errors are not Handled.
<table>
  <tr>
    <th>From</th>
    <th>To</th>
  </tr>
  <tr>
    <td><input type="time" name="pauseFrom" value="%pref_pauseFrom%"></td>
    <td><input type="time" name="pauseTo" value="%pref_pauseTo%"></td>
  </tr>
</table>
</br>

<b>Fade settings:</b></br>
</br>
<table>
  <tr>
    <th>Fade Up Time:</th>
    <th>Fade Down Time:</th>
  </tr>
  <tr>
    <td><center><input type="time" name="fadeUp" value="%pref_fadeUp%"></center></td>
    <td><center><input type="time" name="fadeDown" value="%pref_fadeDown%"></center></td>
  </tr>
</table>
</br>

<b>Date and Time Settings</b></br>
</br>
<table>
  <tr>
    <th>Current System Time</th>
  </tr>
  <tr>
    <td><center><label><b>%pref_time%</b></label></center></td>
  </tr>
</table>
</br>
LEDs are flickering shortly when Save.</br>
<button type="submit" name="save">Save</button></form>
</body></html>)rawliteral";

static esp_wps_config_t config;

void wpsInitConfig(){
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}

String wpspin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0;i<8;i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

void WiFiEvent(WiFiEvent_t event, system_event_info_t info){
  switch(event){
    case SYSTEM_EVENT_STA_START:
      Serial.println("Station Mode Started");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("Connected to :" + String(WiFi.SSID()));
      Serial.print("Got IP: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from station, attempting reconnection");
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WPS Successful, stopping WPS and connecting to: " + String(WiFi.SSID()));
      esp_wifi_wps_disable();
      delay(10);
      WiFi.begin();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WPS Failed, retrying");
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WPS Timeout, retrying");
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WPS_PIN = " + wpspin2string(info.sta_er_pin.pin_code));
      break;
    default:
      break;
  }
}

AsyncWebServer server(80);

//NTP:
const char* ntpServer1 = "time.google.com";
const char* ntpServer2 = "ptbtime1.ptb.de";
const char* ntpServer3 = "fritz.box";

int i = 0;


String processor(const String& var){
   Serial.println(var);
   if(var == "PLACEHOLDER_SPLIT"){
     code = "<tr>\
     <td><center>Colour Split 1:</center></td>\
     <td><center><input type=\"color\" name=\"colorDay1\" value=\"%pref_colorDay1%\"></center></td>\
     <td><center><input type=\"color\" name=\"colorN1\" value=\"%pref_colorN1%\"></center></td>\
     </tr>";
     if (input_split.toInt() > 1) {
      for (int i = 2; input_split.toInt() >= i; i++){
        code += "<tr>\
        <td><center>Colour Split " + String(i) + ":</center></td>\
        <td><center><input type=\"color\" name=\"colorDay" + String(i) + "\" value=\"%pref_colorDay" + String(i) + "%\"></center></td>\
        <td><center><input type=\"color\" name=\"colorN" + String(i) + "\" value=\"%pref_colorN" + String(i) + "%\"></center></td>\
        </tr>";
      }
    }
   return String(code);
   }else{
     if (!preferences.isKey(var.c_str())){
        Serial.print("Value not found: ");
        Serial.println(var.c_str());
     }else{
       code = preferences.getString(var.c_str());
       Serial.print("Load: ");
       Serial.print(var.c_str());
       Serial.print(" ");
       Serial.println(code);
     }
     if(var == "pref_time"){
      printTime();
      code = time1;
     }
     return String(code);    
   }
 }
 
void setup(){
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("WPS connected!");
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); //define hostname
  
  while(WiFi.status() != WL_CONNECTED){
    i++;
    WiFi.begin();
    if (i >= 10){
      WiFi.onEvent(WiFiEvent);
      WiFi.mode(WIFI_MODE_STA);
      //Serial.println("Starting WPS");
      wpsInitConfig();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
    }
    delay(1000);
    //Serial.println("Wait for WPS Reconnect!");
  }
  
  i = 0;
  
  preferences.begin("htmlfile", false);
  input_split = preferences.getString("pref_split");
  
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("System Saved!!");
    int paramsNr = request->params();
    if (request->hasParam(split)) {
      input_split = request->getParam(split)->value();
      preferences.putString("pref_split", input_split);
    }else {
      input_split = 1;
    }
    for(int i=0;i<paramsNr;i++){
 
      AsyncWebParameter* p = request->getParam(i);
      String name1 = p->name();
      if(!name1.startsWith("split")){
        //input_colorDay1 = request->getParam(colorDay1)->value();
        String name1 = "pref_" + p->name();
        String value1 = p->value();
        preferences.putString(name1.c_str(), value1);
        Serial.print("Written: ");
        Serial.print(name1.c_str());
        Serial.print(" ");
        Serial.println(value1);
      }
    }
    request->redirect("/");
    delay(1000);
    request->redirect("/");
  });
  server.onNotFound(notFound);
  server.begin();
  
  Serial.println("HTTP server started");
  delay(100); 

  configTime(0, 3600, ntpServer1, ntpServer2, ntpServer3);
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  //configTime("CET-1CEST,M3.5.0,M10.5.0/3", ntpServer);  // Zeitzone einstellen https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

  pixels.begin();
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(ledPin, ledChannel);
  
  rainbow(1);
}

void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    strip.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void printTime(){  
  struct tm time;
  if(!getLocalTime(&time)){
    Serial.println("Could not obtain time info");
    return;
  }
  //Serial.println("\n---------TIME----------");
  //Serial.println(asctime(&time));
  //char buffer[80];
  //strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &time);
  //Serial.println(buffer);
  //strftime(buffer, sizeof(buffer), "%H:%M:%S", &time);
  strftime(time1, sizeof(time1), "%H:%M:%S", &time);
  //Serial.println(buffer);
  //strftime(buffer, sizeof(buffer), "%Y/%m/%d", &time);
  //Serial.println(buffer);
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

long timeshift1;
long timeshift2;
long timeshift3;
long today_seconds;
String shift;
String hours;
String minutes;
int white_Night;
int white_Day;
int shift_white;
long start_day;
long start_night;
long pause_from;
long pause_to;
long fade_up;
long fade_down;
String led_split_string;

void loop(){
  timeshift1 = rtc.getHour(true) * 3600;
  timeshift2 = rtc.getMinute() * 60;
  timeshift3 = rtc.getSecond();
  today_seconds = timeshift1 + timeshift2 + timeshift3;

  led_split_string = preferences.getString("pref_split");
  led_split = led_split_string.toInt();

  if (preferences.isKey("pref_startDay")){
    shift = preferences.getString("pref_startDay");
    hours = shift.substring(0,2);
    minutes = shift.substring(3,5);
    start_day = (hours.toInt() * 3600) + (minutes.toInt() * 60);
  }
  if (preferences.isKey("pref_startNight")){
    shift = preferences.getString("pref_startNight");
    hours = shift.substring(0,2);
    minutes = shift.substring(3,5);
    start_night = (hours.toInt() * 3600) + (minutes.toInt() * 60);
  }
  if (preferences.isKey("pref_pauseFrom")){
    shift = preferences.getString("pref_pauseFrom");
    hours = shift.substring(0,2);
    minutes = shift.substring(3,5);
    pause_from = (hours.toInt() * 3600) + (minutes.toInt() * 60);
  }
  if (preferences.isKey("pref_pauseTo")){
    shift = preferences.getString("pref_pauseTo");
    hours = shift.substring(0,2);
    minutes = shift.substring(3,5);
    pause_to = (hours.toInt() * 3600) + (minutes.toInt() * 60);
  }
  if (preferences.isKey("pref_fadeUp")){
    shift = preferences.getString("pref_fadeUp");
    hours = shift.substring(0,2);
    minutes = shift.substring(3,5);
    fade_up = (hours.toInt() * 3600) + (minutes.toInt() * 60);
  }
  if (preferences.isKey("pref_fadeDown")){
    shift = preferences.getString("pref_fadeDown");
    hours = shift.substring(0,2);
    minutes = shift.substring(3,5);
    fade_down = (hours.toInt() * 3600) + (minutes.toInt() * 60);
  }
  if (preferences.isKey("pref_whiteDay")){
    shift = preferences.getString("pref_whiteDay");
    shift = map(shift.toInt(), 0, 100, 0, 255);
    white_Day = shift.toInt();
  }
  if (preferences.isKey("pref_whiteNight")){
    shift = preferences.getString("pref_whiteNight");
    shift = map(shift.toInt(), 0, 100, 0, 255);
    white_Night = shift.toInt();

  }

  if (today_seconds >= start_day and today_seconds <= start_night){
    if (today_seconds >= pause_from and today_seconds <= (pause_to + pause_fade)){
        //PAUSE Start
        if (today_seconds <= pause_to){
          Serial.println("Pause Run.");
          for(int a=0; a<led_split; a++){
              colorday(a);
              colornight(a);
              fade_to = (pause_from + pause_fade) - today_seconds;
              if (fade_to >= 0){
                reduce_red = map(fade_to, pause_fade, 0, red_day, red_night);
                reduce_green = map(fade_to, pause_fade, 0, green_day, green_night);
                reduce_blue = map(fade_to, pause_fade, 0, blue_day, blue_night);
                Serial.print("Fade_to: ");
                Serial.println(fade_to);
                
                Serial.print("MAP: ");
                Serial.print(reduce_red);
                Serial.print(", ");
                Serial.print(reduce_green);
                Serial.print(", ");
                Serial.println(reduce_blue);
                run_leds(a, reduce_red, reduce_blue, reduce_green);
                shift_white = map(fade_to, pause_fade, 0, white_Day, white_Night);
                Serial.print("White: ");
                Serial.println(shift_white);
                ledcWrite(ledChannel, shift_white);
              }else{
                run_leds(a, red_night, blue_night, green_night);
                ledcWrite(ledChannel, white_Night);
              }
          }
        }else{
          //PAUSE End
          Serial.println("Pause End.");
          for(int a=0; a<led_split; a++){
              colorday(a);
              colornight(a);
              fade_to = (pause_to + pause_fade) - today_seconds;
              if (fade_to >= 0){
                reduce_red = map(fade_to, pause_fade, 0, red_night, red_day);
                reduce_green = map(fade_to, pause_fade, 0, green_night, green_day);
                reduce_blue = map(fade_to, pause_fade, 0, blue_night, blue_day);
                Serial.print("Fade_to: ");
                Serial.println(fade_to);
                              
                Serial.print("MAP: ");
                Serial.print(reduce_red);
                Serial.print(", ");
                Serial.print(reduce_green);
                Serial.print(", ");
                Serial.println(reduce_blue);
                run_leds(a, reduce_red, reduce_blue, reduce_green);
                shift_white = map(fade_to, pause_fade, 0, white_Night, white_Day);
                Serial.print("White: ");
                Serial.println(shift_white);
                ledcWrite(ledChannel, shift_white);
              }
          }
        }
    }else{
      //Day
      Serial.println("Run Day.");
      for(int a=0; a<led_split; a++){
        colorday(a);
        colornight(a);
        fade_to = (start_day + fade_up) - today_seconds;
        if (fade_to >= 0){
          reduce_red = map(fade_to, fade_down, 0, red_night, red_day);
          reduce_green = map(fade_to, fade_down, 0, green_night, green_day);
          reduce_blue = map(fade_to, fade_down, 0, blue_night, blue_day);
          Serial.print("Fade_to: ");
          Serial.println(fade_to);

          Serial.print("MAP: ");
          Serial.print(reduce_red);
          Serial.print(", ");
          Serial.print(reduce_green);
          Serial.print(", ");
          Serial.println(reduce_blue);
          run_leds(a, reduce_red, reduce_blue, reduce_green);
          shift_white = map(fade_to, fade_down, 0, white_Night, white_Day);
          Serial.print("White: ");
          Serial.println(shift_white);
          ledcWrite(ledChannel, shift_white);
        }else{
          run_leds(a, red_day, blue_day, green_day);
          ledcWrite(ledChannel, white_Day);
        }
      }
    }
  }else if (today_seconds <= start_day){
      //Night bevor Morning
      Serial.println("Run Midnight.");
      for(int a=0; a<led_split; a++){
        //colorday(a);
        colornight(a);
        /*fade_to = (start_night + fade_down) - today_seconds;
        if (fade_to >= 0){
          reduce_red = map(fade_to, fade_down, 0, red_day, red_night);
          reduce_green = map(fade_to, fade_down, 0, green_day, green_night);
          reduce_blue = map(fade_to, fade_down, 0, blue_day, blue_night);
          Serial.print("Fade_to: ");
          Serial.println(fade_to);
  
          Serial.print("MAP: ");
          Serial.print(reduce_red);
          Serial.print(", ");
          Serial.print(reduce_green);
          Serial.print(", ");
          Serial.println(reduce_blue);
          run_leds(a, reduce_red, reduce_blue, reduce_green);
          shift_white = map(fade_to, fade_down, 0, white_Day, white_Night);
          Serial.print("White: ");
          Serial.println(shift_white);
          ledcWrite(ledChannel, shift_white);
        }else{*/
          run_leds(a, red_night, blue_night, green_night);
          ledcWrite(ledChannel, white_Night);
        }
      //}

  }else{
      //Night
      Serial.println("Run Night.");
      for(int a=0; a<led_split; a++){
        colorday(a);
        colornight(a);
        fade_to = (start_night + fade_down) - today_seconds;
        if (fade_to >= 0){
          reduce_red = map(fade_to, fade_down, 0, red_day, red_night);
          reduce_green = map(fade_to, fade_down, 0, green_day, green_night);
          reduce_blue = map(fade_to, fade_down, 0, blue_day, blue_night);
          Serial.print("Fade_to: ");
          Serial.println(fade_to);
  
          Serial.print("MAP: ");
          Serial.print(reduce_red);
          Serial.print(", ");
          Serial.print(reduce_green);
          Serial.print(", ");
          Serial.println(reduce_blue);
          run_leds(a, reduce_red, reduce_blue, reduce_green);
          shift_white = map(fade_to, fade_down, 0, white_Day, white_Night);
          Serial.print("White: ");
          Serial.println(shift_white);
          ledcWrite(ledChannel, shift_white);
        }else{
          run_leds(a, red_night, blue_night, green_night);
          ledcWrite(ledChannel, white_Night);
        }
      }
  }
}

void run_leds(int a, int power_red, int power_blue, int power_green){
          r = power_red;
          g = power_green;
          b = power_blue;
          
          int NUMPIXELS1 = NUMPIXELS + led_split;
          for(int i=a; i<=NUMPIXELS1; i += led_split) {
            pixels.setPixelColor(i, r, g, b);
            pixels.show();
          }
        
}
void colorday(int a){
        colorread = "pref_colorDay" + String((a+1));
        if (preferences.isKey(colorread.c_str())){
          String str = preferences.getString(colorread.c_str());
          char temp[32];
          strcpy(temp, str.c_str());
          colorDay[a] = temp;

          String hexstring = String(colorDay[a]);
          int number = (int) strtol(&hexstring[1], NULL, 16);
          red_day = number >> 16;
          green_day = number >> 8 & 0xFF;
          blue_day = number & 0xFF;
          //r = red_day;
          //g = green_day;
          //b = blue_day;
      }
}
void colornight(int a){
        colorread = "pref_colorN" + String((a+1));
        if (preferences.isKey(colorread.c_str())){
          String str = preferences.getString(colorread.c_str());
          char temp[32];
          strcpy(temp, str.c_str());
          colorNight[a] = temp;

          String hexstring = String(colorNight[a]);
          int number = (int) strtol(&hexstring[1], NULL, 16);
          red_night = number >> 16;
          green_night = number >> 8 & 0xFF;
          blue_night = number & 0xFF;
          //r = red_night;
          //g = green_night;
          //b = blue_night;
      }
}
