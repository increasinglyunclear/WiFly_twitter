/* This sketch tweets via Wifly, created using Uno, Maxbotix EZ1, and WiFly RN-XV. 
 * by @Kevin7 for happenstanceproject.com, Aug 2012. Code home: https://github.com/hackinstance/WiFly_twitter
 *
 * Based on WiFlyHQ Example httpclient.ino
 * and http://arduino.cc/forum/index.php/topic,63031.msg705537.html#msg705537 - huge thanks to DeathRay1977!
 *
 * Connect Wifly to Arduino as follows:
 * Wifly Pin 1 to Arduino 3.3V pin
 * Wifly Pin 2 to Arduino pin 8
 * Wifly Pin 3 to Arduino pin 9
 * Wifly Pin 10 to Arduino GND pin
 * Distance sensor PWM to pin 7
 * Distance sensor VCC to Arduino 5V
 * Distance sensor GND to Arduino GND
 * See http://www.flickr.com/photos/kevioen/7583259984/in/photostream
 * You need the following files in your Arduino library folder:
 */

#include <WiFlyHQ.h> // Download from https://github.com/harlequin-tech/WiFlyHQ
#include <SoftwareSerial.h> // included in Arduino install
#include <SPI.h> // included in Arduino install

// Updatable settings:
const char mySSID[] = "<wifi network ID>"; // Wifi network name
const char myPassword[] = "<wifi network password>"; // Wifi network password
char token[] = "<twitter app dev token>"; // Get this from arduino-tweet.appspot.com
String location = "#LocationName "; // hashtag for this sensor location
int threshold = 60; // Sensor calibration

// Fixed settings:
const char site[] = "arduino-tweet.appspot.com"; // Site that sends our tweets
SoftwareSerial wifiSerial(8,9); // This is where Arduino i/o pins are set
WiFly wifly; // Initialise wifly
char msg[140]; // Tweet length;
String tweetText; // Variable to hold the tweet text
unsigned long readings = 0; // Holds readings below the threshold
unsigned long start_time; // Timer stuff
unsigned long current_time;
unsigned long elapsed_time;


void setup()
{
  pinMode(7, INPUT); // Distance sensor
  pinMode(13, OUTPUT); // LED for feedback
  digitalWrite(13, LOW); // Turn it off to start
  char buf[32]; // Serial buffer size
  Serial.begin(115200); // Serial port baud rate: SET BAUD RATE TO THIS IN SERIAL MONITOR
  Serial.println("Starting");
  Serial.print("Free memory: ");
  Serial.println(wifly.getFreeMemory(),DEC);

  wifiSerial.begin(9600); // Wifly baud rate: It's OK that this doesn't match serial port rate above.
  if (!wifly.begin(&wifiSerial, &Serial)) {
    Serial.println("Failed to start wifly");
    terminal();
  }

  /* Join wifi network if not already associated */
  if (!wifly.isAssociated()) {
    /* Setup the WiFly to connect to a wifi network */
    Serial.println("Joining network");
    wifly.setSSID(mySSID);
    wifly.setPassphrase(myPassword);
    wifly.enableDHCP();
    if (wifly.join()) {
      Serial.println("Joined wifi network");
    } 
    else {
      Serial.println("Failed to join wifi network");
      terminal();
    }
  } 
  else {
    Serial.println("Already joined network");
  }

  Serial.print("MAC: ");
  Serial.println(wifly.getMAC(buf, sizeof(buf)));
  Serial.print("IP: ");
  Serial.println(wifly.getIP(buf, sizeof(buf)));
  Serial.print("Netmask: ");
  Serial.println(wifly.getNetmask(buf, sizeof(buf)));
  Serial.print("Gateway: ");
  Serial.println(wifly.getGateway(buf, sizeof(buf)));
  wifly.setDeviceID("Wifly-WebClient");
  Serial.print("DeviceID: ");
  Serial.println(wifly.getDeviceID(buf, sizeof(buf)));
  if (wifly.isConnected()) {
    Serial.println("Old connection active. Closing");
    wifly.close();
  }
  start_time = millis();
}



void loop()
{   
  int pulse = pulseIn(7, HIGH); // Distance sensor PWM
  int inches = pulse/147; //147uS per inch
  wifly.println(inches); // STREAM the reading in realtime over wifi - uncomment this to calibrate
  if (inches < threshold) { // If a reading is below threshold
    readings = readings+1; // Log the reading
  }
  current_time = millis();
  elapsed_time = current_time - start_time;
  if(elapsed_time > 60000) // Don't tweet more than once a minute!
  {
    if (readings > 6) { // Due to some stray readings
      tweetText = location + readings + " random" + random(255); // Construct the tweet; random because Twitter doesn't like to see the same thing repeatedly
       Serial.println(tweetText); // for debugging
      //tweet(); // THIS IS THE LINE THAT TWEETS: details below
    }
    readings = 0; // reset everything
    start_time = millis();
  }

  // Here we just send network messages to serial and vice versa
  if (wifly.available() > 0) {
    char ch = wifly.read();
    Serial.write(ch);
    if (ch == '\n') {
      /* add a carriage return */
      Serial.write('\r');
    }
  }

  if (Serial.available() > 0) {
    wifly.write(Serial.read());
  }

}


/* Connect the WiFly serial to the serial monitor. */
void terminal()
{
  while (1) {
    if (wifly.available() > 0) {
      Serial.write(wifly.read());
    }

  }
}

void tweet()
{
  digitalWrite(13, HIGH); // Turn on the LED to show we're sending

  // The following copied from setup() above:

  wifiSerial.begin(9600); // Wifly baud rate: It's OK that this doesn't match serial port rate above.
  if (!wifly.begin(&wifiSerial, &Serial)) {
    Serial.println("Failed to start wifly");
    terminal();
  }

  /* Join wifi network if not already associated */
  if (!wifly.isAssociated()) {
    /* Setup the WiFly to connect to a wifi network */
    Serial.println("Joining network");
    wifly.setSSID(mySSID);
    wifly.setPassphrase(myPassword);
    wifly.enableDHCP();
    if (wifly.join()) {
      Serial.println("Joined wifi network");
    } 
    else {
      Serial.println("Failed to join wifi network");
      terminal();
    }
  } 
  else {
    Serial.println("Already joined network");
  }

  if (wifly.open(site, 80)) { // Go to the website on http port 80
    Serial.print("Connected to ");
    Serial.println(site);
    // end of text copied from setup()

    /* Send the tweet */
    for ( int i = 0; i < sizeof( msg ); i++ ) {
      msg[i] = '\0';
    }
    tweetText.toCharArray( msg, tweetText.length() + 1 );
    Serial.println("connected");
    wifly.println("POST http://arduino-tweet.appspot.com/update HTTP/1.0");
    wifly.print("Content-Length: ");
    wifly.println(strlen(msg)+strlen(token)+14);
    wifly.println();
    wifly.print("token=");
    wifly.print(token);
    wifly.print("&status=");
    wifly.println(msg);
  } 
  else {
    Serial.println("Failed to connect");
  }

  digitalWrite(13, LOW); // We're done.
}











































