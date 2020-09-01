#include "EmonLib.h"             // Include Emon Library
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mqtt.h"

#define read_send_period 1000 //set to 1s
#define power 4
#define button 0

//WiFi:MQTT parameters
#define MQTT_SERVER "192.168.1.100"
char* ssid = "-------";
char* password = "-------";

//mqtt topics
char* relayTopic = "/pwrMeter/plug1/relay";
char* readingsTopic = "/pwrMeter/plug1/readings";

EnergyMonitor emon1;                   // Create an instance

//variables
unsigned long current_time;
unsigned long last_time;
boolean pressed = false;
double Irms;

//MQTT prepar
WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

/**************************************************/

void setup()
{  
  Serial.begin(9600);

  //26.667   86.207
  emon1.current(0, 25.5);             // Current: input pin, calibration.

  //setup input button
  pinMode(button,INPUT_PULLUP);
  //start relay as ON
  pinMode(power,OUTPUT);
  digitalWrite(power,LOW);

  //readings rate setup
  last_time = current_time = millis();

  //start wifi subsystem
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
  delay(2000);
}

/**************************************************/

void loop()
{
  char data[80];

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10);
  
  handleButton();
    
  current_time = millis();
  if (current_time > (last_time+read_send_period)){
    last_time = current_time;
    handleSensor();
    
    //send message
    String value = "\"power\": " + String(Irms*230.0) ;
    String value2 = ", \"current\": " + String(Irms) ;
    String value3 = ", \"relay\": " + String(!digitalRead(power)) ;

    // Add both value together to send as one string. 
    value = value + value2 + value3;

    // This sends off your payload. 
    String payload = "{" + value + "}";
    payload.toCharArray(data, (payload.length() + 1));
    client.publish(readingsTopic, data);
    }
  
  
}

/**************************************************/
/**************************************************/

void handleSensor(){
    
  Irms = emon1.calcIrms(1480);  // Calculate Irms only
  
  Serial.print(Irms*230.0);         // Apparent power
  Serial.print(" ");
  Serial.println(Irms);          // Irms
}

/**************************************************/

void handleButton(){
  if (digitalRead(button)){
    pressed = false; 
  }
  
  if (!digitalRead(button) && !pressed){
    pressed = true;
    digitalWrite(power,!(digitalRead(power))); 
  } 
}

/**************************************************/

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  
  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);

  if(topicStr == relayTopic){
    //turn the light on if the payload is '1' and publish to the MQTT server a confirmation message
    if(payload[0] == '1'){
      digitalWrite(power, LOW);
      //client.publish("/test/confirm", "1");
    }
  
    //turn the light off if the payload is '0' and publish to the MQTT server a confirmation message
    else if (payload[0] == '0'){
      digitalWrite(power, HIGH);
      //client.publish("/test/confirm", "0");
    }
  }
  /*if(topicStr == "/test/THreq"){
    client.publish("/test/TEMPandHUM", str_temp1);
  }*/
  
}

/**************************************************/

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        Serial.print("\tMTQQ Connected");
        client.subscribe(relayTopic,1);
        
      }

      //otherwise print failed for debugging
      else{Serial.println("\tFailed."); abort();}
    }
  }
}

/**************************************************/

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}




