//ESP8266_DHTwebclientPOSTvar
//
// The program determines the air humitidy of a DHT22 sensor and reports it wirelessly to the 
// web server via POST variable.
// the reporting interval is set 
// Utilizes post variable to send information to the record server more securely
// 
// A LED comes on when connected and blinks after sending data.
// A Red LED will come on if there was a connecition issue.
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
//CONFIGURATION:
char serveraddress[] = "thedavidhanks.com";
const char* ssid = "MADhouse";         //WIFI to connect to
const char* password = "1234554321";   //WIFI Password for SSID
const char passcode[] = "1234567890";  // the passcode is looked up and verified by the server.  
const char location[ ] = "test";       //set location newer setups do not use location.  Location is determined by the passcode given.
const int interval = 15;               // set interval in minutes to take readings

//Configure Temp/humidity sensor DHT22
#include <DHT.h>
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

//Wireless
#include <ESP8266WiFi.h>
WiFiClient client;

//LED
#define GREENLED D1
#define REDLED D2

//Update - get time
void setup()
{
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  //Connect to Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  //Start sensor
  dht.begin();
  //delay(2000);  Needs 2 sec to initalize, but there's a 2 sec. light test.

  //Configure LED
  pinMode(GREENLED,OUTPUT);  
  pinMode(REDLED,OUTPUT);


  //LIGHT Check
  digitalWrite(REDLED, HIGH);
  digitalWrite(GREENLED, HIGH);
  delay(250);
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  delay(250);
  digitalWrite(REDLED, HIGH);
  digitalWrite(GREENLED, HIGH);
  delay(250);
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  delay(250);
  digitalWrite(REDLED, HIGH);
  digitalWrite(GREENLED, HIGH);
  delay(250);
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  delay(3000);
  //Setuip LEDs initial state
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED, HIGH);
}

void loop()
{
  //UPDATE check time every week?

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }


  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  // make a string for assembling the data to log:
  //create a string of values (location id, fahrenheit, humitidy
  String dataString = String(String(f,1) + "F , " + String(hif,2) + "%");

  //make a $_Get string
  String postString = String("loc=" + String(location) + "&pass=" + String(passcode) + "&tempF=" + String(f,1) + "&humidity=" + String(h,2) + "&rel_humidity=" + String(hif,2) );
  String getString = String("?mode=record&" + postString);
  

  Serial.println(String("Current values to send: " + dataString));

    //connect to server
    Serial.println("\nStarting connection...");
    if (client.connect(serveraddress, 80)) {
      Serial.println("Connected to server. \n");
      digitalWrite(GREENLED, HIGH);
      digitalWrite(REDLED, LOW);
      
      // Make a HTTP POST request:
      client.println("POST /tools/index.php?mode=postrecord HTTP/1.1"); 
      client.println(String("Host: " + String(serveraddress)));
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(postString.length());
      client.println();
      client.print(postString);
      client.println();    

      //serial print
      Serial.println("POST /tools/index.php?mode=postrecord HTTP/1.1"); 
      Serial.println(String("Host: " + String(serveraddress)));
      Serial.println("Content-Type: application/x-www-form-urlencoded");
      Serial.println("Connection: close");
      Serial.print("Content-Length: ");
      Serial.println(postString.length());
      Serial.println();
      Serial.print(postString);
      Serial.println();     

      //Successful blink
      blink(GREENLED,3);
  } else {
    Serial.println("connection failed");
    /*digitalWrite(GREENLED, LOW);
    digitalWrite(REDLED, HIGH);  //UPDATE ERROR CODE BLINK
    */
    blink(REDLED,3);
  }
  
  //Uncomment below to see server response
  //
  //String webpage;
  //if (client.available()) {
  //  while (!webpage.endsWith("</html>")){
  //  char c = client.read();
  //  webpage += c;
  //  //Serial.println(webpage);
  //  yield();
  //  }
  //  Serial.println(webpage);
  //}

 Serial.println(" __________________________________________________________________\n\n\n");
 
 int msec_delay = interval * 60 * 1000;
 delay(msec_delay); 
 //delay(10000);

}
void blink(int color, int times){
  //UPDATE read the current states
  
  //blink {color} LED {times} times
  // color should be GREENLED or REDLED
  for(int x = 0; x < times; x++){
    digitalWrite(color, HIGH);
    delay(250);
    digitalWrite(color, LOW);
    delay(250);
  }
  
}

