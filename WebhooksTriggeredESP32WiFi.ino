/****************************************************************************************************************************
  For ESP8266 / ESP32 boards

  WebhooksTriggeredESP32WiFi (https://github.com/we-do-it-lu/WebhooksTriggeredESP32WiFi)
  Turn your ESP32 into an easy to use and manage wireless micro web-server allowing it to process reliably and asynchronouly incoming http/https requests (webhooks).
  The code can be re-used as boilerplate to build your own self-hosted low-energy, uncensorable, async web server.
  Why reinvent the wheel? Just branch this project and start from there with all you need to get running in no time.

  Built by JayDeLux https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite
  Licensed under MIT license
 *****************************************************************************************************************************/
  
#include "defines.h"
#include "Credentials.h"
#include "dynamicParams.h"
#include <ESPAsyncWebServer.h>
#include <Ticker.h>

// The port the module listens to for incoming webhooks
#define SERVER_PORT 80
#define HEARTBEAT_INTERVAL 500L

#define WIFI_HEALTHCHECK_LED_GPIO 15
#define ON_GPIO 16

#define TRIGGER_1 25
#define TRIGGER_2 26
#define TRIGGER_3 27
#define TRIGGER_4 32
#define NBR_DEFINED_TRIGGERS 4

Ticker ticker_health_led;


struct trigger_ticker_pair { 
  int trigger; 
  Ticker ticker; 
};

// Define the GPIOs you want to use here and associate each with a Ticker
static trigger_ticker_pair trigger_ticker[] = { 
    { TRIGGER_1, Ticker() }, 
    { TRIGGER_2, Ticker() }, 
    { TRIGGER_3, Ticker() }, 
    { TRIGGER_4, Ticker() } 
};

Ticker *getTicker( int trigger ) {
  for(int i = 0; i < NBR_DEFINED_TRIGGERS; i++) {
    if(trigger == trigger_ticker[i].trigger )
      return &trigger_ticker[i].ticker;
  }
  return NULL;
}

void setGPIOsAsOutput(){
  for(int i = 0; i < NBR_DEFINED_TRIGGERS; i++)
      pinMode(trigger_ticker[i].trigger, OUTPUT);

}
void turnOffAllGPIOs() {
  for(int i = 0; i < NBR_DEFINED_TRIGGERS; i++)
    digitalWrite(trigger_ticker[i].trigger, LOW); 
}

volatile int triggerInterruptCounter = 0; 

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

const char* PARAM_GPIO = "gpio";
const char* PARAM_STATE = "state";
const char* PARAM_DURATION = "duration";


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Streaming page</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  
</head>
<body>
  <h2>BTC Chicken</h2>
  <iframe src="https://latribu.duckdns.org:3001/stream" width='800px' height='1000px' />
  
</body>
</html>
)rawliteral";


const char manual_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "https://latribu.duckdns.org:3001/update?gpio="+element.id+"&state=1", true); }
  else { xhr.open("GET", "https://latribu.duckdns.org:3001/update?gpio="+element.id+"&state=0", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Output - TRIGGER_1</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - TRIGGER_2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"5\" " + outputState(5) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - TRIGGER_3</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"21\" " + outputState(21) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - TRIGGER_4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"21\" " + outputState(21) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}


void IRAM_ATTR turnOnGpio(int gpio) {
  portENTER_CRITICAL_ISR(&timerMux);
  triggerInterruptCounter++;
  digitalWrite(gpio, HIGH); 
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR turnOffGpio(int gpio) {
  portENTER_CRITICAL_ISR(&timerMux);
  digitalWrite(gpio, LOW); 
  portEXIT_CRITICAL_ISR(&timerMux);
}

void heartBeatLED()
{
  if (WiFi.status() == WL_CONNECTED)
    digitalWrite(WIFI_HEALTHCHECK_LED_GPIO, HIGH);
  else
    digitalWrite(WIFI_HEALTHCHECK_LED_GPIO, !digitalRead(WIFI_HEALTHCHECK_LED_GPIO)); 
}


ESPAsync_WiFiManager_Lite* ESPAsync_WiFiManager;

#if USING_CUSTOMS_STYLE
const char NewCustomsStyle[] /*PROGMEM*/ = "<style>div,input{padding:5px;font-size:1em;}input{width:95%;}body{text-align: center;}\
button{background-color:blue;color:white;line-height:2.4rem;font-size:1.2rem;width:100%;}fieldset{border-radius:0.3rem;margin:0px;}</style>";
#endif

AsyncWebServer server(SERVER_PORT);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}


void setup()
{
  // Debug console
  Serial.begin(115200);
  while (!Serial);

  delay(200);

  pinMode(ON_GPIO, OUTPUT);
  pinMode(WIFI_HEALTHCHECK_LED_GPIO, OUTPUT);
  setGPIOsAsOutput();

  digitalWrite(ON_GPIO, HIGH); 
  digitalWrite(WIFI_HEALTHCHECK_LED_GPIO, HIGH); 
  turnOffAllGPIOs();

  ticker_health_led.attach(0.5, heartBeatLED);
  
  Serial.print(F("\nStarting ESPAsync_WiFi using ")); Serial.print(FS_Name);
  Serial.print(F(" on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_ASYNC_WIFI_MANAGER_LITE_VERSION);

#if USING_MRD  
  Serial.println(ESP_MULTI_RESET_DETECTOR_VERSION);
#else
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);
#endif

  ESPAsync_WiFiManager = new ESPAsync_WiFiManager_Lite();

  // Optional to change default AP IP(192.168.4.1) and channel(10)
  ESPAsync_WiFiManager->setConfigPortalIP(IPAddress(192, 168, 4, 42));
  ESPAsync_WiFiManager->setConfigPortalChannel(0);

#if USING_CUSTOMS_STYLE
  ESPAsync_WiFiManager->setCustomsStyle(NewCustomsStyle);
#endif

#if USING_CUSTOMS_HEAD_ELEMENT
  ESPAsync_WiFiManager->setCustomsHeadElement("<style>html{filter: invert(10%);}</style>");
#endif

#if USING_CORS_FEATURE  
  ESPAsync_WiFiManager->setCORSHeader("Your Access-Control-Allow-Origin");
#endif

  // Set customized DHCP HostName
  //ESPAsync_WiFiManager->begin(HOST_NAME);
  ESPAsync_WiFiManager->begin("BTCChiken");

 
  server.on("/manual", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", manual_html, processor);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Get route");
  });

  // Send a GET request to <ESP_IP>/update?gpio=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?gpio=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_GPIO) && request->hasParam(PARAM_STATE)) {
      inputMessage1 = request->getParam(PARAM_GPIO)->value();
      inputMessage2 = request->getParam(PARAM_STATE)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  
  // Send a GET request to <ESP_IP>/trigger?gpio=<inputMessage1>&duration=<inputMessage2>
  server.on("/trigger", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "This service works on POST requests, not on GET requests.");
  });


  // Send a POST request to <ESP_IP>/trigger?gpio=<inputMessage1>&duration=<inputMessage2>
  server.on("/trigger", HTTP_POST, [] (AsyncWebServerRequest *request) {
    int gpio = 0;
    int duration = 0;

    // GET input1 value on <ESP_IP>/trigger?gpio=<inputMessage1>&duration=<inputMessage2>
    if (request->hasParam(PARAM_GPIO) ) {
      Serial.println( request->getParam(PARAM_GPIO)->value() );
      gpio = request->getParam(PARAM_GPIO)->value().toInt();
    }
    else {
      Serial.println("Missing param 'gpio'");
      request->send(400, "text/plain", "Missing param 'gpio'");
      return;
    }
    
    if (request->hasParam(PARAM_DURATION) ) {
      Serial.println( request->getParam(PARAM_DURATION)->value() );
      duration = request->getParam(PARAM_DURATION)->value().toInt();
      if(duration <= 0 ) {
        Serial.println("param 'duration' must be > 0");
        request->send(400, "text/plain", "param 'duration' must be > 0");
        return;  
      }
        
    }
    else { 
      Serial.println("Missing param 'duration'");
      request->send(400, "text/plain", "Missing param 'duration'");
      return;
    }

    Ticker *ticker = getTicker(gpio);
    if(ticker) {
      // Turn the GPIO on ...
      turnOnGpio(gpio);
      ticker->attach_ms(duration, turnOffGpio, gpio);
      request->send(200, "text/plain", "OK, GPIO triggered");
    }
    else {
      Serial.println("Missing param 'gpio'");
      request->send(400, "text/plain", "Invalid param 'gpio'");
    }    
  });

  server.onNotFound(notFound);
  server.begin();
}

#if USE_DYNAMIC_PARAMETERS
void displayCredentials()
{
  Serial.println(F("\nYour stored Credentials :"));

  for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
  {
    Serial.print(myMenuItems[i].displayName);
    Serial.print(F(" = "));
    Serial.println(myMenuItems[i].pdata);
  }
}

void displayCredentialsInLoop()
{
  static bool displayedCredentials = false;

  if (!displayedCredentials)
  {
    for (int i = 0; i < NUM_MENU_ITEMS; i++)
    {
      if (!strlen(myMenuItems[i].pdata))
      {
        break;
      }

      if ( i == (NUM_MENU_ITEMS - 1) )
      {
        displayedCredentials = true;
        displayCredentials();
      }
    }
  }
}

#endif

void loop()
{
  ESPAsync_WiFiManager->run();
  
#if USE_DYNAMIC_PARAMETERS
  displayCredentialsInLoop();
#endif
}
