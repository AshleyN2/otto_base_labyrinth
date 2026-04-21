#include "driver/gpio.h" // ESP32 General Purpose I/O driver
#include "esp_log.h" // Logging library for debugging
#include "esp_timer.h"  // Timer services for delays
#include "freertos/FreeRTOS.h" // FreeRTOS kernel for task management
#include "freertos/task.h" // FreeRTOS task management functions
#include "motor_control.h" // Motor control functions
#include "ultrasonic.h" // Ultrasonic sensor functions

// Main application entry point
void app_main(void) { 
  float speed = 0.5f; // variable defining the speed of the robot/motor power (0.0 to 1.0 range)
  float distance_cm = 0.0f; // variable to store currentdistance of objects measured by the ultrasonic sensor in centimeters

  // H/w initialization: Initialize the ultrasonic sensor and motor control systems
  us_init(); // Initialize ultrasonic sensor
  motors_init(); // Initialize motor control

  /* Create a FreeRTOS task for handling ultrasonic sensor measurements
  Create a task for ultrasonic sensor measurements with a stack size of 4096 bytes and priority 5 */
  xTaskCreate(us_task, "us_task", 4096, NULL, 5, NULL); 

  while (1) {
    /* 
        if a task calls xQueueReceive(distance_queue, &distance, portMAX_DELAY);
        it blocks until an item is added
        when another task calls xQueueSend(...), 
        the receiving task is automatically unblocked
    */
    // Checks the data queue for a new measurement. If no data arrives within 1000ms, the 'if' is skipped.
    if (xQueueReceive(distance_queue, &distance_cm, pdMS_TO_TICKS(1000)) ==
        pdPASS) {
      // distance updated
    }

    /*
        logic for motor control based on distance measurement;
        this is a very simple example, you can implement more complex behavior here
    */
    if (distance_cm < 0.0f) {
      ESP_LOGI("main", "Distance measurement timeout, keep previous state."); // Handle sensor error (negative readings)
    } else if (distance_cm > 10.0f) { // Clear path: Move forward at 50% speed
      otto_forward(speed);
    } else if (distance_cm > 5.0f) { // Object approaching: Reduce speed to 25%
      otto_forward(0.25f);  // slow down when close
    } else {
      // Object very close: Manuever 
      //otto_stop(); // Stop the robot
      vTaskDelay(pdMS_TO_TICKS(200)); // Short delay to ensure the robot has stopped

      otto_reverse(0.5f); // Move backward at 50% speed
      vTaskDelay(pdMS_TO_TICKS(500)); // Move backward for 500ms

      otto_turn_right(speed); // Turn right 
      vTaskDelay(pdMS_TO_TICKS(950)); // Turn for 950ms which is an almost perfect 90° angle for wonky the otto (adjust as needed for a 90 degree turn)

      otto_forward(0.25f); // Move forward at 25% speed to try to navigate around the obstacle
      vTaskDelay(pdMS_TO_TICKS(500)); // Move forward for 500ms

      otto_stop(); // Object too close: Safety stop

      //Clear out the distance queue to avoid reacting to stale measurements after maneuvering
      xQueueReset(distance_queue); 
    }
  }
  vTaskDelay(pdMS_TO_TICKS(US_TIMEOUT_MS));  // delay before next check
}
