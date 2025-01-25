#include <WiFi.h>
#include <HTTPClient.h>
#include <freertos/semphr.h>

// WiFi Credentials
const char* apSSID = "GestureRobotNetwork";
const char* apPassword = "RobotControl123";

// Server URL
const String serverUrl = "http://192.168.4.1";

// Pin Definitions
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MOTOR_PIN_1 12
#define MOTOR_PIN_2 14

// Global Variables
long duration;
float distanceCm;
TaskHandle_t flexSensorTaskHandle;
TaskHandle_t ultrasonicTaskHandle;

// Semaphore and Mutex
SemaphoreHandle_t obstacleSemaphore;
SemaphoreHandle_t httpClientMutex;

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR_PIN_1, OUTPUT);
  pinMode(MOTOR_PIN_2, OUTPUT);

  // Initialize semaphores
  obstacleSemaphore = xSemaphoreCreateBinary();
  httpClientMutex = xSemaphoreCreateMutex();

  // Connect to WiFi
  WiFi.begin(apSSID, apPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(ultrasonicTask, "Ultrasonic Task", 10000, NULL, 1, &ultrasonicTaskHandle, 0);
  xTaskCreatePinnedToCore(flexSensorTask, "Flex Sensor Task", 10000, NULL, 1, &flexSensorTaskHandle, 1);
}

void loop() {
  // FreeRTOS handles task execution
}

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
    distanceCm = duration * 0.034 / 2;

    if (distanceCm < 10) {
      // Obstacle detected, stop motors and suspend the flex sensor task
      stopMotors();
      Serial.println("Obstacle detected! Stopping motors.");
      vTaskSuspend(flexSensorTaskHandle);  // Suspend Flex Sensor Task
    } else {
      // No obstacle, resume flex sensor task if suspended
      if (eTaskGetState(flexSensorTaskHandle) == eSuspended) {
        Serial.println("No obstacle, resuming Flex Sensor Task.");
        vTaskResume(flexSensorTaskHandle);  // Resume Flex Sensor Task
      }
      moveForward();  // Move forward if no obstacle
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
      xSemaphoreTake(obstacleSemaphore, portMAX_DELAY);  // Wait until ultrasonic task releases it
      Serial.println("Flex Sensor Task resumed.");
    }

    // Get command from the server
    String command = getCommandFromServer();
    if (!command.isEmpty()) {
      processCommand(command);
    }

    vTaskDelay(pdMS_TO_TICKS(200));  // Command processing interval
  }
}

String getCommandFromServer() {
  HTTPClient http;
  String command = "";

  xSemaphoreTake(httpClientMutex, portMAX_DELAY);
  http.begin(serverUrl + "/move");
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    command = http.getString();
  }
  http.end();
  xSemaphoreGive(httpClientMutex);

  return command;
}

void processCommand(String command) {
  if (command == "FORWARD") {
    moveForward();
  } else if (command == "STOP") {
    stopMotors();
  }
}

void moveForward() {
  digitalWrite(MOTOR_PIN_1, HIGH);
  digitalWrite(MOTOR_PIN_2, LOW);
}

void stopMotors() {
  digitalWrite(MOTOR_PIN_1, LOW);
  digitalWrite(MOTOR_PIN_2, LOW);
}
