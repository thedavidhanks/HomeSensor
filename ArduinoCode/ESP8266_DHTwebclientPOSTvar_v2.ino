//ESP8266_DHT_POSTvar_DeepSleep
//
// The program determines the air humitidy of a DHT22 sensor and reports it wirelessly to the 
// web server via POST variable.
// the reporting interval is set 
// Utilizes post variable to send information to the record server more securely
// Will enter deep sleep for a length of {interval}.  
// Note that this requires GPI06 (D0) to be connected to RST.  BUT ONLY AFTER PROGRAM IS INSTALLED.
// 
// LED sequence
// RED -> started
// GREEN -> connected to wifi
// 5x RED -> 5 quick red blinks indicate sensor issue .
// 3x GREEN -> 3 green blinks indicates a successful connection to the server
// 
//CONFIGURATION:
const char serveraddress[] = "*";
const char* ssid = "*";         //WIFI to connect to
const char* password = "*";   //WIFI Password for SSID
const char passcode[] = "*";  // the passcode is looked up and verified by the server.  
const int interval = 5;               // set interval in minutes to take readings
String readString;

//Configure Temp/humidity sensor DHT22
#include <DHT.h>
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

//Wireless
#include <ESP8266WiFi.h>
#include <WiFiUdp.h> //to get time from server

//LED
#define GREENLED D1
#define REDLED D2

//Setup time Client
#include <NTPClient.h>
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);  //see https://github.com/arduino-libraries/NTPClient

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void blink(int color, int times, int hold=250){
  int previous_state = digitalRead(color);
  //turn the LED off.
  digitalWrite(color,LOW);
  
  //blink {color} LED {times} times for {hold} msec
  // color should be GREENLED or REDLED
  for(int x = 0; x < times; x++){
    digitalWrite(color, HIGH);
    delay(hold);
    digitalWrite(color, LOW);
    delay(hold);
  }
  //return to the prior state.
  digitalWrite(color,previous_state);
}


void setup()
{
  //Start sensor
  dht.begin();
  //delay(2000);  Needs 2 sec to initalize, but it takes that long to connect to the wifi
  
  //Get clock
  timeClient.begin();
  
  //Configure LED
  pinMode(GREENLED,OUTPUT);  
  pinMode(REDLED,OUTPUT);

  //Setup LEDs initial state.  RED LED indicates on, but not connected.
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED, HIGH);

  Serial.begin(9600);
  Serial.print("Attempting to connect to WiFi SSID: ");
  Serial.println(ssid);

  //Connect to Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  digitalWrite(GREENLED, HIGH);
  digitalWrite(REDLED, LOW);

  //Get current time
  int unixTimestamp;
  String timeString = "";
  if(timeClient.update()){
    unixTimestamp = timeClient.getEpochTime();
    timeString = String("&timestamp=" + String(unixTimestamp));
  }
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old'
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    for(int x = 0; x < 4; x++){
      blink(REDLED,5,80);
      delay(500);
    }
    return;  //UPDATE does this just start setup over?
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  //make a $_POST string
  String postString = String("pass=" + String(passcode) + "&tempF=" + String(f,1) + "&humidity=" + String(h,2) + "&rel_humidity=" + String(hif,2) + timeString);
  
  Serial.println(String("Current values to send: " + postString));

  //connect to server
  WiFiClient client;
  Serial.println("\nStarting connection...");
  if(!client.connect(serveraddress, 80)){
    Serial.println("connection failed");
    blink(REDLED,3,100);
    delay(5000);
    return;
  }
  
  if (client.connected()) {
    blink(GREENLED,2);
    Serial.println("Connected to server. Sending POST data.\n");
    
    // Make an HTTP POST request:
    client.println("POST /homeSensor/ HTTP/1.1"); 
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Host: " + String(serveraddress));
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(postString.length());
    client.println();
    client.print(postString);
    client.println();    
    
    //Successful blink
    //blink(GREENLED,3);      
  }

  //Print response from client
  // wait for data to be available
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      delay(60000);
      return;
    }
  }


  // Read all the lines of the reply from server and print them to Serial
  Serial.println("receiving from remote server");
  // not testing 'client.connected()' since we do not need to send data here

  
  while (client.available()) {
    char c = client.read();
    readString += String(c); 
  }
  Serial.print(readString);
  
  if(readString.length()<1){
    Serial.println("Server did not respond");
    blink(REDLED,10,100);
  }else{
    //Find if there was a code available
    int pos = readString.indexOf("CODE");
    String codeNo;
    if(pos>-1){
      codeNo = readString.substring(pos+5,pos+8);  //.substring(a,b) a => index to start at, b => index to end the substring before  5,8 is a string of 3. 5,6,7
      Serial.println("CODE value of "+codeNo);
      switch(codeNo.toInt()){
        case 1: //SUCCESS response from server
          blink(GREENLED,5,100);
          break;
        default:
          blink(REDLED,2,200);
          break;
      }
    }else{  //Could not find "CODE"  Likely did not get a response from the server.
        Serial.println("Could not find \"CODE\" keyword in resonse");
        blink(REDLED,10,100);
    }
  }
  
  // if the server's disconnected, stop the client
  if (!client.connected()) {
    Serial.println();
    Serial.println("Disconnecting from server...");
    client.stop();
  };

 //SLEEP
 Serial.println("Sleeping for " + String(interval) + " min.");
 Serial.println(" __________________________________________________________________\n\n\n");
 int microsec_delay = interval * 60 * 1000 * 1000;
 ESP.deepSleep(microsec_delay);
}
void loop(){}
