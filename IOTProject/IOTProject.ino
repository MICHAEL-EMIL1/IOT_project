#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ESP32_MailClient.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Keypad.h>
#include <ESP32Servo.h>

#define SERVO_PIN 19 // ESP32 pin GIOP26 connected to servo motor
#define RELAY_PIN 16
#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns
#define ledPin      15
#define ledPin2     2
#define gas 36 //bouns
#define buzz 18 //bouns
#define PIR 4
#define ldr 35
#define TRIG_PIN 23 //bouns
#define ECHO_PIN 22 
DHT dht(21, DHT11);

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM]      = {13, 12, 14, 27};
byte pin_column[COLUMN_NUM] = {26, 25, 33, 32};

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
Servo servoMotor;
const String password = "7890"; // change your password here
String input_password;
int rigthpass = 0 ;
int wrongpass = 0;

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Michaell Emill";
const char* passwordWifi = "0123456789";
AsyncWebServer server(80);

// Temperature monitoring and email notification setup
const long interval = 5000; // Interval between sensor readings
unsigned long previousMillis = 0;
String lastTemperature;
bool emailSent = false;

// To send Emails using Gmail on port 465 (SSL)
#define emailSenderAccount    "esp28205@gmail.com"
#define emailSenderPassword   "kwrg nasf pgcm sozn"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "[ALERT] ESP32 MOTION "
#define emailSubject1         "[ALERT] ESP32 GAS "
#define emailSubject2         "[ALERT] ESP32 ultra "
#define emailSubject3         "[ALERT] ESP32 wrongpass "
// Default Recipient Email Address
String inputMessage = "micheal.emil31@gmail.com";
String enableEmailChecked = "checked";
String inputMessage2 = "true";
// Default Threshold Temperature Value
String inputMessage3 = "10.0"; //temp data
String inputMessage4 = "1800.0"; //light data
String inputMessage5 = "140.0"; //gas data
float duration_us, distance_cm;

// HTML web page to handle 3 input fields (email_input, enable_email_input, threshold_input)
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IOTProject</title>
    <style>
    .btn {
      display: block;
      width: 80px;
      height: 40px;
      margin-top: 10px;
    }
    </style>
</head>
<body>
    <h2>ESP32 sensors</h2> 
    <h3>TEMPERATURE = %TEMPERATURE%&deg;C</h3>
    <h3>LDR = %LDR%</h3>
    <h3>GAS = %GAS%</h3>
    <h3>ULTRASONIC = %ULTRASONIC% </h3>
    <h3>MOTION = <span id="motion">%MOTION%</span> </h3>
    <h2>ESP Email Notification</h2>
    <form action="/get">
    Email Address <input type="email" name="email_input" value="%EMAIL_INPUT%" required><br><br>
    Enable Email Notification <input type="checkbox" name="enable_email_input" value="true" %ENABLE_EMAIL%><br><br>
    Temperature Threshold <input title="to open fan if sensor read more than this data" type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br><br>
    Light Threshold <input title="to open leds if sensor read more than this data" type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br><br>
    GAS Threshold <input title="to send mail if sensor read more than this data" type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br><br>
    <button type="button" class="btn" onclick="turnOnLED()">Turn On LED</button>
    <button type="button" class="btn" onclick="turnOffLED()">Turn Off LED</button>
    <button type="button" class="btn" onclick="turnOnfan()">Turn On fan</button>
    <button type="button" class="btn" onclick="turnOfffan()">Turn Off fan</button><br><br>
    <input type="submit" value="Submit"></form>
    <script>
    function turnOnLED() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/turnOnLED", true);
      xhr.send();
    }

    function turnOffLED() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/turnOffLED", true);
      xhr.send();
    }
    function turnOnLED() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/turnOnfan", true);
      xhr.send();
    }

    function turnOffLED() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/turnOfffan", true);
      xhr.send();
    }
  </script>
</body>
</html>)rawliteral";

void sendSensorReadings(AsyncWebServerRequest *request) {
  String sensorReadings = String("{\"temperature\":") + readDHTTemperature() +
                          String(",\"ldr\":") + String(analogRead(ldr)) +
                          String(",\"gas\":") + String(gass()) +
                          String(",\"ultrasonic\":") + String(distance_cm) +
                          String(",\"motion\":") + String(digitalRead(PIR)) + "}";
  String response = String(index_html); // Get the HTML content
  // Replace placeholders with sensor readings in the HTML response
  response.replace("%TEMPERATURE%", readDHTTemperature());
  response.replace("%LDR%", String(analogRead(ldr)));
  response.replace("%GAS%", String(gass()));
  response.replace("%ULTRASONIC%", String(distance_cm));
  response.replace("%MOTION%", String(digitalRead(PIR)));

  request->send(200, "text/html", response);
}

bool sendEmailNotification(String emailMessage,String emailSubjectt);

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

SMTPData smtpData;

void turnOnLED() {
  digitalWrite(ledPin, HIGH);  // Turn on the LED
  Serial.println("LED Turned On");
}

void turnOffLED() {
  digitalWrite(ledPin, LOW);  // Turn off the LED
  Serial.println("LED Turned Off");
}

void turnOnfan() {
  digitalWrite(ledPin2, HIGH);  // Turn on the LED
  Serial.println("fan Turned On");
}

void turnOfffan() {
  digitalWrite(ledPin2, LOW);  // Turn off the LED
  Serial.println("fan Turned Off");
}
String readDHTTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.print("temp = ");
    Serial.println(t);
    delay(500);
    return String(t);
  }
}
void keypad_fun(){
  char key = keypad.getKey();
  if(key){
    Serial.println(key);
    if( input_password.length() <= 3){
      input_password += key;
      Serial.print("input_password length=");
      Serial.println(input_password.length());
    } else {
      if (password == input_password) {
        Serial.println("The password is correct, ACCESS GRANTED!");
        // servo to open the door
        rigthpass++;
        input_password = "";
        servoMotor.write(90);
        delay(4000);
        servoMotor.write(0);
      } else {
        Serial.println("The password is incorrect, ACCESS DENIED!");
        input_password = "";
        wrongpass++;
        rigthpass = 0;
        Serial.print("wrongpass =");
        Serial.println(wrongpass);
      }
      if(wrongpass == 3){
        wrongpass = 0;
        // Send an email here
        String emailMessage = "Too many wrong password attempts detected. Possible intrusion!";
        if(sendEmailNotification(emailMessage,emailSubject3)) {
          Serial.println("Email sent: " + emailMessage);
        } else {
          Serial.println("Failed to send email: " + emailMessage);
          }
        }
      }
  }
}

/*void mot_temp_ldr(){
  int motion = digitalRead(PIR);
  String x = readDHTTemperature();
  float temp = x.toFloat(); 
  int ldrr = analogRead(ldr);
  Serial.print("motion sensor : ");
  Serial.println(motion);
  Serial.print("temp sensor : ");
  Serial.println(temp);
  Serial.print("ldr sensor : ");
  Serial.println(ldrr);
  delay(1000);
  if(motion == 1){
    //there is light => so get low values
    if(ldrr > 1800 && temp > 22 ){//dark => so leds on
      //relay(leds & fan) on 
      digitalWrite(ledPin,HIGH);
      digitalWrite(ledPin2,HIGH);//as fan
    }else if (ldrr > 1800 && temp < 22 ) {
      //relay(leds on & fan off)
      digitalWrite(ledPin,HIGH);
      digitalWrite(ledPin2,LOW);
    }else if (ldrr < 1800 && temp > 22 ) {
      //relay(leds off & fan on )
      digitalWrite(ledPin,LOW);
      digitalWrite(ledPin2,HIGH);
    }else{
      //relay(leds & fan) off
      digitalWrite(ledPin,LOW);
      digitalWrite(ledPin2,LOW);
    }
  }
  else{
    digitalWrite(ledPin,LOW);
    digitalWrite(ledPin2,LOW);
  }
}*/

int gass(){
  int gasValue = analogRead(gas);
  Serial.print("MQ2 sensor AO value: ");
  Serial.println(gasValue);
  //delay(1000);
  return gasValue;
}

void ultra() {
  // generate 10-microsecond pulse to TRIG pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  // measure duration of pulse from ECHO pin
  duration_us = pulseIn(ECHO_PIN, HIGH);
  // calculate the distance
  distance_cm = 0.017 * duration_us;
  // print the value to Serial Monitor
  Serial.print("distance ultra: ");
  Serial.print(distance_cm);
  Serial.println(" cm");
  if(distance_cm < 7.0 ){
    String emailMessage = "there is a person in front of your door";
    if(sendEmailNotification(emailMessage,emailSubject2)) {
      Serial.println("Email sent: " + emailMessage);
    }else {
      Serial.println("Failed to send email: " + emailMessage);
    }
  }
}

void sendCallback(SendStatus msg) {
  Serial.println(msg.info());
  if (msg.success()) {
    Serial.println("----------------");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(PIR, INPUT);
  pinMode(gas, INPUT);
  pinMode(buzz, OUTPUT);
  pinMode(ldr, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(0);
  pinMode(RELAY_PIN, OUTPUT);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passwordWifi);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
     if (request->hasParam("Submit")) {
        String action = request->getParam("Submit")->value();
        if (action == "leds on") {
            digitalWrite(ledPin, HIGH); // Turn LED ON
            // Add any additional actions you need
        } else if (action == "leds off") {
            digitalWrite(ledPin, LOW); // Turn LED OFF
            // Add any additional actions you need
        } else if (action == "fans on") {
            // Control another pin for fans if needed
            digitalWrite(ledPin2, HIGH);
        } else if (action == "fans off") {
            // Control another pin for fans if needed
            digitalWrite(ledPin2, LOW);
        }
    }
    if (request->hasParam("email_input")) {
      inputMessage = request->getParam("email_input")->value();
      if (request->hasParam("enable_email_input")) {
        inputMessage2 = request->getParam("enable_email_input")->value();
        enableEmailChecked = "checked";
      }
      else {
        inputMessage2 = "false";
        enableEmailChecked = "";
      }
      if (request->hasParam("threshold_input")) {
        inputMessage3 = request->getParam("threshold_input")->value();
      }
    }
    else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });
  server.on("/", HTTP_GET, [] (AsyncWebServerRequest *request){
    String sensorReadings = String("{\"temperature\":") + readDHTTemperature() +
                          String(",\"ldr\":") + String(analogRead(ldr)) +
                          String(",\"gas\":") + String(gass()) +
                          String(",\"ultrasonic\":") + String(distance_cm) +
                          String(",\"motion\":") + String(digitalRead(PIR)) + "}";
                          
  String response = String(index_html); // Get the HTML content
  // Replace placeholders with sensor readings in the HTML response
  response.replace("%TEMPERATURE%", readDHTTemperature());
  response.replace("%LDR%", String(analogRead(ldr)));
  response.replace("%GAS%", String(gass()));
  response.replace("%ULTRASONIC%", String(distance_cm));
  response.replace("%MOTION%", String(digitalRead(PIR)));

  request->send(200, "text/html", response);
  });
  // Add the /sensors route to get sensor readings
  server.on("/sensors", HTTP_GET, sendSensorReadings);
  server.onNotFound(notFound);
  server.begin();
  String response = String(index_html); // Get the HTML content
}

void loop() {
  unsigned long currentMillis = millis();
    //Serial.println("Enter your password");
    if(wrongpass<3){
      keypad_fun();
    }
    if(rigthpass > 0 ){
      //ultrasonic for if any one in front of our home ///////////bouns
      if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      ultra();
      //PIR
      int motion = digitalRead(PIR);
      Serial.print("motion sensor : ");
      Serial.println(motion);
      if(motion == 1){
        Serial.println("motion detected");
        String emailMessage = String("there is a motion in the room");
        if(sendEmailNotification(emailMessage,emailSubject)) {
          Serial.println(emailMessage);
          emailSent = true;
        }
        else {
          Serial.println("Email failed to send");
        }
      }
      //temp
      lastTemperature = readDHTTemperature();
      float temperature = lastTemperature.toFloat();  
      if( temperature > inputMessage3.toFloat() ){
        digitalWrite(ledPin2,HIGH);//as fan
        //digitalWrite(RELAY_PIN,HIGH);
      }
      else if( temperature < inputMessage3.toFloat() ) {
        digitalWrite(ledPin2,LOW);//as fan
        //digitalWrite(RELAY_PIN,LOW);
      }
      //ldr
      int ldrr = analogRead(ldr);
      Serial.print("ldr sensor : ");
      Serial.println(ldrr);
      if( ldrr > inputMessage4.toFloat() ){
        digitalWrite(ledPin,HIGH);//as light
      }
      else if( ldrr < inputMessage4.toFloat() ) {
        digitalWrite(ledPin,LOW);//as light
      }
      //gas, buzzer ///////////bouns to send mail if sensor read more than this data
      int y = gass();
      if(y > inputMessage5.toFloat() && inputMessage2 == "true" && !emailSent){
        digitalWrite(buzz,HIGH);
        delay(2000);
        digitalWrite(buzz,LOW);
        String emailMessage = String("gas above threshold. Current gas: ") + String(y);
        if(sendEmailNotification(emailMessage,emailSubject1)) {
          Serial.println(emailMessage);
          emailSent = true;
        }
        else {
          //Serial.println("Email failed to send");
        }
      }else if((y < inputMessage5.toFloat()) && inputMessage2 == "true" && emailSent) {
        digitalWrite(buzz,LOW);
        String emailMessage = String("gas below threshold. Current gas: ") + String(y);
        if(sendEmailNotification(emailMessage,emailSubject1)) {
          Serial.println(emailMessage);
          emailSent = false;
        }
        else {
          Serial.println("Email failed to send");
        }
      }
    }
  }
}
bool sendEmailNotification(String emailMessage,String emailSubjectt){
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
  smtpData.setSender("ESP32", emailSenderAccount);
  smtpData.setPriority("High");
  smtpData.setSubject(emailSubjectt);
  smtpData.setMessage(emailMessage, true);
  smtpData.addRecipient(inputMessage);
  smtpData.setSendCallback(sendCallback);
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    return false;
  }
  smtpData.empty();
  return true;
}