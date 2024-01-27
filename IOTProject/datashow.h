const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IOTProject</title>
</head>
<body>
    <h2>ESP32 sensors</h2> 
    <h3>TEMPERATURE = %TEMPERATURE%&deg;C</h3>
    <h3>LDR = %LDR%</h3>
    <h3>GAS = %GAS%</h3>
    <h3>ULTRASONIC = %ULTRASONIC%</h3>
    <h3>MOTION = %MOTION%</h3>
    <h2>ESP Email Notification</h2>
    <form action="/get">
    Email Address <input type="email" name="email_input" value="%EMAIL_INPUT%" required><br><br>
    Enable Email Notification <input type="checkbox" name="enable_email_input" value="true" %ENABLE_EMAIL%><br><br>
    Temperature Threshold <input title="to open fan if sensor read more than this data" type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br><br>
    Light Threshold <input title="to open leds if sensor read more than this data" type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br><br>
    GAS Threshold <input title="to send mail if sensor read more than this data" type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br><br>
    <input type="submit" value="leds on">
    <input type="submit" value="leds off">
    <input type="submit" value="fans on">
    <input type="submit" value="fans off"><br><br>
    <input type="submit" value="Submit"></form>
</body>
</html>)rawliteral";

void sendSensorReadings(AsyncWebServerRequest *request) {
  String sensorReadings = String("{\"temperature\":") + readDHTTemperature() +
                          String(",\"ldr\":") + String(analogRead(ldr)) +
                          String(",\"gas\":") + String(gass()) +
                          String(",\"ultrasonic\":") + String(distance_cm) +
                          String(",\"motion\":") + String(digitalRead(PIR)) + "}";
  request->send(200, "text/html", sensorReadings);
}