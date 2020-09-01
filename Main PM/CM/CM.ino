#include "EmonLibC.h"                   // Include Emon Library

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mqtt.h"

#define LED 16
#define read_send_period 1000 //set to 1s

/***************************************************************************/

//WiFi:MQTT parameters
#define MQTT_SERVER "192.168.1.100"
char* ssid = "Vodafone-C593BE";
char* password = "5QwG3NUyGW";

//mqtt topics
char* readingsTopic = "/pwrMeter/plug0/readings";

//MQTT prepar
WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

/***************************************************************************/

EnergyMonitor emon1;             // Create an instance
EnergyMonitor emon2;             // Create an instance
EnergyMonitor emon3;             // Create an instance

//main
float realPower;      
float apparentPower; 
float powerFActor;    
float supplyVoltage ;
float Irms ;    
//aux A
float realPowerA;      
float apparentPowerA; 
float powerFActorA;    
float IrmsA;
//aux B
float realPowerB;      
float apparentPowerB; 
float powerFActorB;    
float IrmsB;    

//variables
unsigned long current_time;
unsigned long last_time;

/***************************************************************************/

void setup()
{  
  Serial.begin(9600);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  //readings rate setup
  last_time = current_time = millis();

  // 17.689
  float AB_calib = 17.689;
  float volt_calib = 223.89;
  float phase_cal = 1.17;
  float phase_calAB = 1.2;

  emon1.voltage(1, volt_calib, phase_cal);  // Voltage: input pin, calibration, phase_shift
  emon1.current(0, 90.9);             // Current: input pin, calibration.

  emon2.voltage(1, volt_calib, phase_calAB);  // Voltage: input pin, calibration, phase_shift
  emon2.current(2, AB_calib);       // Current: input pin, calibration.

  emon3.voltage(1, volt_calib, phase_calAB);  // Voltage: input pin, calibration, phase_shift
  emon3.current(3, AB_calib);       // Current: input pin, calibration.

  //start wifi subsystem
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
  delay(2000);
}

void loop()
{
  char data[200];
  
  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10);
  
  current_time = millis();
  if (current_time > (last_time+read_send_period)){
    last_time = current_time;
    read_sensors();

    //send message
    String value = "\"realPower\": " + String(realPower) ;
    String value2 = ", \"apparentPower\": " + String(apparentPower) ;
    String value3 = ", \"powerFactor\":" + String(powerFActor) ;
    String value4 = ", \"supplyVoltage\": " + String(supplyVoltage) ;
    String value5 = "\"Irms\": " + String(Irms) ;
    String value6 = ", \"realPowerA\":" + String(realPowerA) ;
    String value7 = ", \"apparentPowerA\": " + String(apparentPowerA) ;
    String value8 = ", \"powerFactorA\": " + String(powerFActorA) ;
    String value9 = "\"IrmsA\": " + String(IrmsA) ;
    String value10 = ", \"realPowerB\": " + String(realPowerB) ;
    String value11 = ", \"apparentPowerB\": " + String(apparentPowerB) ;
    String value12 = ", \"powerFactorB\": " + String(powerFActorB) ;
    String value13 = "\"IrmsB\": " + String(IrmsB) ;
        
    // Add both value together to send as one string. 
    //value = value + value2 + value3 + value4 + value5 + value6 + value7 + value8 + value9 + value10 + value11 + value12 + value13;
    value = value + value2 + value3 + value4;
    value5 = value5 + value6 + value7 + value8;
    value9 = value9 + value10 + value11 + value12;
        
    // This sends off your payload. 
    String payload = "{" + value + "}";
    payload.toCharArray(data, (payload.length() + 1));
    client.publish(readingsTopic, data); 
    
    payload = "{" + value5 + "}";
    payload.toCharArray(data, (payload.length() + 1));
    client.publish(readingsTopic, data); 
    
    payload = "{" + value9 + "}";
    payload.toCharArray(data, (payload.length() + 1));
    client.publish(readingsTopic, data); 

    payload = "{" + value13 + "}";
    payload.toCharArray(data, (payload.length() + 1));
    client.publish(readingsTopic, data); 
  
  }
}

void read_sensors(){
  
/* TWO SENSORS main*/
  emon1.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  //emon1.serialprint();
  
  realPower       = emon1.realPower;        //extract Real Power into variable
  apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
  powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
  supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  Irms            = emon1.Irms;             //extract Irms into Variable
  
/* TWO SENSORS aux A*/
  //Serial.println("$$$$$$$$$$$$$$$$$$$$");  
  emon2.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  //emon2.serialprint();
  
  realPowerA       = emon2.realPower;        //extract Real Power into variable
  apparentPowerA   = emon2.apparentPower;    //extract Apparent Power into variable
  powerFActorA     = emon2.powerFactor;      //extract Power Factor into Variable
  IrmsA            = emon2.Irms;             //extract Irms into Variable

  /* TWO SENSORS aux B*/
  //Serial.println("$$$$$$$$$$$$$$$$$$$$");  
  emon3.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  //emon3.serialprint();
  
  
  realPowerB       = emon3.realPower;        //extract Real Power into variable
  apparentPowerB   = emon3.apparentPower;    //extract Apparent Power into variable
  powerFActorB     = emon3.powerFactor;      //extract Power Factor into Variable
  IrmsB            = emon3.Irms;             //extract Irms into Variable

  //Serial.println("$$$$$$$$$$$$$$$$$$$$");  
  //Serial.println("$$$$$$$$$$$$$$$$$$$$");  
}

/**************************************************/

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  
  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
  /*
  if(topicStr == relayTopic){

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
