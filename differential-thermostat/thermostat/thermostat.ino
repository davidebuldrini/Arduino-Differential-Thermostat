

/*
 Arduino Powered Thermostat
 
 The project involves the abstraction of how a real thermostat works by using the thermostat library.
 
 You can declare one or more sensors attached to analog pins to retrieve the temperature.
 
 Each sensor can be calibrated during any time by putting the current temperature measured with a thermometer and
 the program will calculate a coefficent used to convert the raw value coming from analog input to the correct
 temperature float value. 
 
 You must define a reference sensor that is used to calculate the delta t with the other sensors and to activate the thermostat
 accordingly.

 The program provides a web interface accessible by network that shows in real time the values of each sensors and the state of all analog pins 
 and digital pins. All data is updated by ajax without refreshing the page, you can else calibrate each sensors, set the minimum delta t necessary to 
 activate the thermostat and save all settings to a simple text file on the SD card.

 The web pages are stored into the SD card and I wrote a simple editor to change the content of the files via web without removing everytime the SD card.
 You can else write new files to the SD card.
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13 with SD card slot
 * Analog inputs attached to pins A0 through A5 
 
 
 created 18 Dec 2014
 by Davide Buldrini 
 ideozone.it
 
 The web page is loaded from the SD card thanks to:
 http://startingelectronics.com/tutorials/arduino/ethernet-shield-web-server-tutorial/SD-card-web-server/
 
 Sensors values in the page are updating via ajax thanks to:
 http://startingelectronics.com/tutorials/arduino/ethernet-shield-web-server-tutorial/web-server-read-switch-automatically-using-AJAX/
 
 REMEMBER (for Arduino one)
 do not use digital pin 0,1 as they are reserved for serial communication 
 do not use digital pin 10,11,12,13 as they are reserved for SPI bus 
 do not use digital pin 4 as it's used to select SD card
 */



#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <thermostat.h>

#define ANALOG_PIN_COUNT 6 // total number of analog pins
#define DIGITAL_PIN_COUNT 14 // total number of digital pins

// analog input pin for sensors
#define SOLAR_PANEL_SENSOR 0
#define ROCKET_STOVE_SENSOR 1
#define BOILER_SENSOR 2

// digital output pin for activate various devices
#define CIRCULATOR 5
#define SOLAR_PANEL_SOLENOID 6
#define ROCKET_STOVE_SOLENOID 7

// how many sensors the thermostat object can handle. it's used to dimensionate the sensors array
#define MAX_SENSORS_COUNT 5

// Uncomment to use byte values for the temperature although this doesn't seems to eat less memory
//#define TEMPERATURE_BYTE 1 

//uncomment this for dev mode
//#define DEVMODE 1

EthernetServer server(80);
char stringParam[15]; // it's used to store parsed string parameters from http requests. I declared global to avoid all series of problems caused by returning a string from a function in C
EthernetClient client;

// fires when the sensor overcome delta t
void onOvercome(TempSensor *sensor, Thermostat* thermostat) { 
  
  #if defined(DEVMODE)
    Serial.println();  
    Serial.print(F("sensor "));
    Serial.print(sensor->name);
    Serial.println(F(" overcome delta t"));
  #endif

   // TURN ON CIRCULATOR: remember that the relay modules works when the input pin have a LOW state
  digitalWrite(CIRCULATOR, LOW); 
  
  if (sensor->linkedOutputPin) { 
    digitalWrite(sensor->linkedOutputPin, LOW); 
  }
}

// fires when the sensor temperature return to values within delta t
void onNormal(TempSensor *sensor, Thermostat* thermostat) { 
  
  #if defined(DEVMODE)
    Serial.println();  
    Serial.print(F("sensor "));
    Serial.print(sensor->name);
    Serial.println(F(" return on normal values"));
  #endif
   
  if (sensor->linkedOutputPin) { 
    digitalWrite(sensor->linkedOutputPin, HIGH); 
  }
}

// TURN OFF CIRCULATOR only if all sensors are under delta t  
void onAllNormal(Thermostat* thermostat) { 
   digitalWrite(CIRCULATOR, HIGH);
}

// SENSORS DECLARATION
TempSensor boiler("boiler", BOILER_SENSOR); // feel the temperature of water inside the boiler
TempSensor solarPanel("solar panel", SOLAR_PANEL_SENSOR);
TempSensor stove("stove", ROCKET_STOVE_SENSOR); 

Thermostat thermostat;


void setup() {
    
  // Open serial communications 
  #if defined(DEVMODE)  
    Serial.begin(9600);
  #endif
  
  // Enter a MAC address and IP address for your controller below.
  byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  IPAddress ip(192,168,1,177);
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  
  server.begin();
  
  #if defined(DEVMODE)
    Serial.print(F("server is at "));
    Serial.println(Ethernet.localIP());  
  
    // initialize SD card
    Serial.println(F("Initializing SD card..."));
  #endif
  
  if (!SD.begin(4)) {
    #if defined(DEVMODE) 
      Serial.println(F("ERROR - SD card initialization failed!"));
    #endif 
    return;    // init failed
  }
  #if defined(DEVMODE) 
    Serial.println(F("SUCCESS - SD card initialized."));
  #endif 
  
  // set the digital pins output to HIGH to not activate the relays module on startup. Remember that relay modules works when the input state is LOW
  pinMode(CIRCULATOR, OUTPUT);    
  digitalWrite(CIRCULATOR, HIGH);    
 
  // link output pins to the sensors 
  solarPanel.linkOutput(SOLAR_PANEL_SOLENOID, HIGH);
  stove.linkOutput(ROCKET_STOVE_SOLENOID, HIGH);
 
  analogReference (INTERNAL);   // 1.1V reference: improve the precision especially if simple diodes are used as temperature sensors
  delay(300);     // let voltages settle
  
  thermostat.addReference(&boiler); // reference sensor: the thermostat will act upon calculating the delta t between this sensor and the others
  
  // other sensors:
  thermostat.addSensor(&solarPanel);
  thermostat.addSensor(&stove);
  
  thermostat.onOvercome = &onOvercome; // callback function that fires when a sensor temperature overcome the minimum delta t
  thermostat.onNormal = &onNormal; // callback function that fires when the sensor value return within the minimum delta t
  thermostat.onAllNormal = &onAllNormal; // callback function that fires when all sensors are within the minimum delta t
  
  loadSettings("set.txt"); 
}


void loop() {
  
  // check the sensors temperature against the reference temperature and call appropriate callbacks 
  thermostat.checkDelta();
  
  // listen for incoming clients
  client = server.available();
  String request = "";            // stores the requested page name
  char c; // stores 1 byte from the client stream
  
  if (client) {
    
    #if defined(DEVMODE) 
      Serial.println(F("new client"));
    #endif  
    
    client.findUntil("T /", "\r"); // GET / or POST / 
   
    // retrieve the requested page name from the client 
    while (client.connected()) { 
      if (client.available()) {

        c = client.read(); // read the next byte from the client

        if (c != '&' && c != ' ') { // the request name end with a space or : GET /request&params HTTP/1.1 or GET /request HTTP/1.1
          request += c;       
        }
        else { 
          break; 
        }
      }
    }
             
    #if defined(DEVMODE) 
      Serial.println();
      Serial.println(request);
    #endif                    
 
    if (request == "savefile") { // Save post data from a form into an SD card file
      saveFileToSD(parseStringParameter());  
    }             
    else if (request == "calibrate") {  // sensor calibration request: GET /calibrate&2=22.45 HTTP/1.1                   
      calibrate();
    }
    else if (request == "set_digital") {  // set digital output pin request: GET /set_digital&4=1 HTTP/1.1                   
      setDigitalOutput();
    }
    else if (request == "thermostat") {  // toggle thermostat on / off: GET /thermostat&1 HTTP/1.1                   
      toggleThermostat();
    }
    else if (request == "setDeltaT") { // set minimum delta t trigger: GET /setDeltaT&10 HTTP/1.1             
      setDeltaT();
    }  
    else if (request == "editor") { // Put the content of a SD card file into a textarea for editing: GET /editor&filename HTTP/1.1   
      fileEditor(parseStringParameter());
    }        
    else if (request == "save_settings") { // Save some settings into a file on the SD   
      saveSettings(parseStringParameter());
    }          
    else if (request == "ajax_request") { // send sensors and thermostat data to the client via ajax          
      sendAjaxData();
    }  
    else if (request == "") { // home page GET / HTTP/1.1
      sendFileFromSD("index.htm");
    }  
    else { // send to the client a file existing in the SD card           
      sendFileFromSD(request);
    }  
             
    delay(1); // give the web browser time to receive the data
    client.stop(); // close the connection:
    
    #if defined(DEVMODE) 
      Serial.println(F("client disconnected"));
    #endif  
  } // end of if (client) {
  
  //delay(150);  
} 

// send the data to the web browser as a JSON string
void sendAjaxData()
{
 sendResponseHeader(false, true);
    
  client.print(F("{\"sensors\":[")); 
  
  for (byte sid = 0; sid < thermostat.sensorsCount; sid ++) {
    client.print(F("{")); 
      client.print(F("\"name\":\""));
      client.print(thermostat.sensors[sid]->name);
      client.print(F("\",\"id\":\""));
      client.print(sid);
      client.print(F("\",\"ref\":\""));
      client.print(thermostat.isReference(sid)); // tell wheter it's the reference sensor or not
      client.print(F("\",\"value\":\""));
      client.print(thermostat.sensors[sid]->value());
      client.print(F("\",\"raw\":\""));
      client.print(thermostat.sensors[sid]->raw());
      client.print(F("\",\"coeff\":\""));
      client.print(thermostat.sensors[sid]->pin); 
      client.print(F("\",\"deltaT\":\""));
      client.print(thermostat.deltaT(sid));
    client.print(F("\"}"));
 
    if (sid != (thermostat.sensorsCount - 1)) {
      client.print(F(","));
    }
  }
   
  // write useful thermostat fields
  client.print(F("],\"thermostat\":{\"triggerDeltaT\":\""));
  client.print(thermostat.triggerDeltaT);
  client.print(F("\"}"));
  
  // write current values of all analog pins
  client.print(F(",\"apins\":["));
  
  for (byte apin = 0; apin < ANALOG_PIN_COUNT; apin ++) {
    client.print(analogRead(apin));
     
    if (apin != (ANALOG_PIN_COUNT - 1)) {
      client.print(F(","));
    }
  }
  
  // write current values of all analog pins
  client.print(F("],\"dpins\":["));
    
  for (byte dpin = 0; dpin < DIGITAL_PIN_COUNT; dpin ++) {
    client.print(digitalRead(dpin));
   
    if (dpin != (DIGITAL_PIN_COUNT - 1)) {
      client.print(F(","));
    }
  }  
  
  client.print(F("]}"));
                  
  /*
    json string example:    
    {"sensors":[{"name":"boiler","id":"0","ref":"1","value":"815.32","raw":"692","coeff":"1.18","pin":"5","deltaT":"1.18"},{"name":"solar panel","id":"1","ref":"0","value":"92.22","raw":"664","coeff":"0.14","pin":"0","deltaT":"-723.24"},{"name":"stove","id":"2","ref":"0","value":"669.00","raw":"669","coeff":"1.00","pin":"1","deltaT":"-146.32"},{"name":"stove2","id":"3","ref":"0","value":"660.00","raw":"660","coeff":"1.00","pin":"2","deltaT":"-155.14"}],"thermostat":{"triggerDeltaT":"5"},"apins":[665,670,660,789,915,692],"dpins":[1,1,0,0,1,0,0,0,0,0,1,1,1,0]}
  */   
}

// parse the parameters from the http request and calibrate the sensor
void calibrate() {
           
  byte sid = client.parseInt(); // sensor id
  
  #if defined(TEMPERATURE_BYTE)
    byte value = client.parseInt(); // current real temperature
  #else
    float value = client.parseFloat(); // current real temperature
  #endif

  sendResponseHeader(false, false);
  
  #if defined(DEVMODE)     
    Serial.println();
    Serial.print(F("Sensor calibration: sid= "));
    Serial.print(sid);
    Serial.print(F(", value= "));
    Serial.println(value);
  #endif
  
  thermostat.sensors[sid]->calibrate(value);
}


void setDigitalOutput() {

  byte pin = client.parseInt(); 
  byte state = client.parseInt(); 
  
  sendResponseHeader(false, false);
 
  pinMode(pin, OUTPUT);    
  digitalWrite(pin, state);    
} 

void toggleThermostat() {

  byte state = client.parseInt(); 
  
  sendResponseHeader(false, false);
   
  thermostat.setState(state);
}

// set the minimum delta t trigger
void setDeltaT() {

  byte deltaT = client.parseInt(); 

  sendResponseHeader(false, false);
  
  #if defined(DEVMODE) 
    Serial.println();
    Serial.print(F("Setting minimum delta t trigger to: "));
    Serial.println(deltaT);
  #endif
  
  thermostat.triggerDeltaT = deltaT;
}  

// load the content of a file inside a textarea where you can edit and save to the same file
void fileEditor(char *fileName) {
  
  sendResponseHeader(false, false);

  File file;
  
  #if defined(DEVMODE) 
    Serial.println(fileName);
  #endif
  
  client.println(F("<!DOCTYPE html><head><title>Arduino File Editor</title></head><body><p>")); 
  
  if (!SD.exists(fileName)) {
    
    #if defined(DEVMODE) 
      Serial.println(F("EDITOR: this file doesn't exists. Edit new file"));
    #endif
    
    client.print(F("New file"));
  } 
  else {
    file = SD.open(fileName);       
    
    #if defined(DEVMODE) 
      Serial.println(F("EDITOR: file opened"));    
    #endif
    
    client.print(fileName);
  }
  
  client.print(F("</p><form action=\"savefile&"));
  client.print(fileName); // put filename parameter in the action
  client.print(F("\" method=\"POST\">"));
  client.println(F("<textarea name=\"content\" rows=\"30\" cols=\"60\" style=\"height:80%;width:100%;\">"));

  // put the content of the file inside the text area
  if (file) {  
    while(file.available()) {
      client.write(file.read());
    }
    file.close();
  }
    
  client.println(F("</textarea>"));
  client.println(F("<input type=\"submit\" value=\"Save\"/>"));
  client.println(F("</form>"));
  client.println(F("</body></html>"));
}


// Open a file from the SD card and send over http to the client
void sendFileFromSD(String fileNameString) {
  
  if (fileNameString.indexOf(".css") > -1) {
    sendResponseHeader(true, false); // content type text/css
  }
  else {         
    sendResponseHeader(false, false);
  }  
  
  char fileName[fileNameString.length() + 1]; // added + 1 for null terminate string
  fileNameString.toCharArray(fileName, fileNameString.length() + 1);
  
  #if defined(DEVMODE) 
    Serial.println(fileName);  
  #endif
    
  File file = SD.open(fileName);        // open web page file
            
  if (file) {
    while(file.available()) {
      client.write(file.read()); // send web page to client
    }
  file.close();
  } 
}  


void saveFileToSD(char *fileName) {   
  
  sendResponseHeader(false, false);
  
  // remove old version of the file
  if (SD.exists(fileName)) {
    SD.remove(fileName);
  }
  
  #if defined(DEVMODE) 
    Serial.println(fileName);
  #endif
  
  File file = SD.open(fileName, FILE_WRITE); 
  
  client.find("content="); // skip to the POST data (content is the textarea name inside the html form)  
  char c;
  
  // decode html entities on the fly and write to the file directly                
  while(client.available()) {
      
    c = client.read(); // read the next byte from the client    
    
    // hex value
    if (c == '%') {               
      c = from_hex(client.read()) << 4 | from_hex(client.read()); // convert hex value to normal character
      file.write(c);
    } 
    else if (c == '+') { // handle spaces
      file.write(' ');
    } 
    else { // normal alphanumeric character, nothing to change
      file.write(c);
    }
    delay(1); // if I remove this the file will be saved only partially (DUNNO WHY)
  }
   
  file.close();
  
  // redirect to the editor page for the same file
  client.print(F("<meta HTTP-EQUIV=\"REFRESH\" content=\"0; url=/editor&"));
  client.print(fileName);
  client.print(F("\">"));
}  

// send a response header that varies upon some parameters
void sendResponseHeader(boolean css, boolean alive) {
    
  client.println(F("HTTP/1.1 200 OK"));
  
  if (css) {
    client.println(F("Content-Type: text/css"));    
  }
  else {
    client.println(F("Content-Type: text/html"));
  }
  if (alive) {
    client.println(F("Connection: keep-alive"));
  }
  else {
    client.println(F("Connection: close"));
  }
  client.println();  
}


/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

// parse a string parameter from the http request. The string must end with a space   
char *parseStringParameter() {
  char end[] = " ";
  String string = client.readStringUntil(*end);
  string.toCharArray(stringParam, string.length() + 1);
  return stringParam;
}


// Very simple settings loader. I plan to use json in the future
boolean loadSettings(char *fileName) {
 
  if (!SD.exists(fileName)) {
    return false;
  }
  
  File file = SD.open(fileName); 
  
  if (file) {
  
    #if defined(DEVMODE) 
      Serial.println(F("Loading settings from file..."));
    #endif
    
    // load calibration coefficent for every sensor (one float value per line)
    for (byte sid = 0; sid < thermostat.sensorsCount; sid ++) {

      if (file.available()) {
        
        thermostat.sensors[sid]->Vref = file.parseInt();
        thermostat.sensors[sid]->Tref = file.parseFloat();
        
        #if defined(DEVMODE) 
          Serial.println();
          Serial.print(F("Sensor calibration: sid= "));
          Serial.print(sid);
          Serial.print(F(", Vref= "));
          Serial.println(thermostat.sensors[sid]->Vref);
          Serial.print(F(", Tref= "));
          Serial.println(thermostat.sensors[sid]->Tref);
        #endif
      }
    }
    // load minimum delta t trigger (another line, int value)
    if (file.available()) {
      thermostat.triggerDeltaT = file.parseInt();
      
      #if defined(DEVMODE) 
        Serial.println();
        Serial.print(F("Minimum delta T trigger set to: "));
        Serial.print(thermostat.triggerDeltaT);
        Serial.println(F(" degrees"));
      #endif
    }
    file.close();
  }   
}

// Save some current objects variables values into a file 
boolean saveSettings(char *fileName) {
 
  sendResponseHeader(false, false);
  
  if (!SD.exists(fileName)) {
    return false;
  }
  else {
    SD.remove(fileName); 
  }
  
  File file = SD.open(fileName, FILE_WRITE); 
  
  if (file) {

    #if defined(DEVMODE) 
      Serial.println(F("\n Saving settings to file..."));
    #endif
        
    for (byte sid = 0; sid < thermostat.sensorsCount; sid ++) {  
      file.print(thermostat.sensors[sid]->Vref);
      file.print(" ");
      file.println(thermostat.sensors[sid]->Tref);
    }
    
    file.println(thermostat.triggerDeltaT);
    file.close();  
  }   
}
  
