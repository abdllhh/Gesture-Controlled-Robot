#include <WiFi.h>
#include <WebServer.h> <freertos/semphr.h>
// Access Point Credentials
const char* apSSID = "GestureRobotNetwork";
const char* apPassword = "RobotControl123";
// Web Server Setup
WebServer server(80); // Standard HTTP port

void loop() {
// Handle client requests
int flexValue;
flexValue = analogRead(flexPin);
Serial.print("sensor: ");
Serial.println(flexValue);
delay(20);
server.handleClient();
}

void handleMove() {
// Read flex sensor value
int flexValue = analogRead(flexPin);
// Determine movement command based on flex sensor
String command = mapFlexToCommand(flexValue);
// Send response to client
server.send(200, "text/plain", command);
}
String mapFlexToCommand(int flexValue) {
if (flexValue > 2700) {
return "FORWARD";
}
 else if (flexValue <2200) {
return "BACKWARD";
}
else {
return "STOP";
}
}

// if (obstacle_detected && distance < CRITICAL_THRESHOLD) {
//     // Safety override - stop immediately
//     execute_emergency_stop();
// } else if (obstacle_detected && distance < WARNING_THRESHOLD) {
//     // Reduce speed but allow gesture control
//     apply_gesture_command(speed * 0.5);
// } else {
//     // Full manual control
//     apply_gesture_command(speed);
// }

