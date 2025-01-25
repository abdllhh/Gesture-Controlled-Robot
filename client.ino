#include <WiFi.h>
#include <HTTPClient.h>
#include <freertos/semphr.h>
// WiFi Credentials
const char* apSSID = "GestureRobotNetwork";
const char* apPassword = "RobotControl123";
// Server URL
const String serverUrl = "http://192.168.4.1";

TaskHandle_t flexSensorTaskHandle;
TaskHandle_t ultrasonicTaskHandle;
// Global Variables
long duration;
float distanceCm;

SemaphoreHandle_t obstacleSemaphore;
SemaphoreHandle_t httpClientMutex;
obstacleSemaphore = xSemaphoreCreateBinary();
httpClientMutex = xSemaphoreCreateMutex();

void ultrasonicTask(void* parameter) {
  Serial.println("Ultrasonic Task running...");
  for (;;) {
    // Measure distance using ultrasonic sensor
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    distanceCm = duration * SOUND_SPEED / 2;
    if (distanceCm < 10) {
    // Obstacle detected, stop motors and suspend the flex sensor task
    stopMotors();
    Serial.println("Obstacle detected! Stopping motors.");
    vTaskSuspend(flexSensorTaskHandle); // Suspend Flex Sensor Task
    } else {
    // No obstacle, resume flex sensor task if suspended
    if (eTaskGetState(flexSensorTaskHandle) == eSuspended) {
    Serial.println("No obstacle, resuming Flex Sensor Task.");
    vTaskResume(flexSensorTaskHandle); // Resume Flex Sensor Task
    }
    moveForward(); // Move forward if no obstacle
    }
    // Wait for 100ms before checking again
    vTaskDelay(pdMS_TO_TICKS(100));
    }
}
    
void flexSensorTask(void* parameter) {
  Serial.println("Flex Sensor Task running...");
  for (;;) {
  // Wait for the semaphore if obstacle detected
    if (xSemaphoreTake(obstacleSemaphore, 0) == pdTRUE) {
      Serial.println("Flex Sensor Task paused due to obstacle.");
      xSemaphoreTake(obstacleSemaphore, portMAX_DELAY); // Wait until 
      //ultrasonic task releases it
      Serial.println("Flex Sensor Task resumed.");
    }
    // Get command from the server
    String command = getCommandFromServer();
    if (!command.isEmpty()) {
        processCommand(command);
        }
    vTaskDelay(pdMS_TO_TICKS(200)); // Command processing interval
  }
}


