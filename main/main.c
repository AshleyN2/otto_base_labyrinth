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
    if (xQueueReceive(distance_queue, &distance_cm, pdMS_TO_TICKS(1000)) == pdPASS)  // distance updated 
    {
      otto_forward(speed); // Move forward at the defined speed while waiting for distance updates
      /*
        logic for motor control based on distance measurement;
        this is a very simple example, you can implement more complex behavior here
      */
      if (distance_cm < 0.0f) {
        ESP_LOGI("main", "Distance measurement timeout, keep previous state."); // Handle sensor error (negative readings)
          } 
          else if (distance_cm > 10.0f) { 
            otto_forward(speed); // Clear path: Move forward at 50% speed
          } 
          else if (distance_cm > 5.0f) { 
            otto_forward(0.25f); // slow down when close i.e object approaching: Reduce speed to 25%
          }
          else {
            // Object very close: Manuever 
            otto_stop(); // Stop the robot
            otto_reverse(0.5f); // Move backward at 50% speed to create some distance from the obstacle
            vTaskDelay(pdMS_TO_TICKS(200)); // Short delay to ensure the robot has stopped before executing the maneuver
            otto_stop(); // Stop before turning to ensure a more accurate turn

            otto_turn_left(speed); // Turn left at the defined speed to try to navigate around the obstacle
            vTaskDelay(pdMS_TO_TICKS(950)); // Turn for 950ms which is an almost perfect 90° angle for wonky the otto (adjust as needed for a 90 degree turn)
            otto_stop(); // Stop after turning to ensure a more accurate forward movement
            float left_distance; // Variable to store distance measurement after the turn
            if (xQueueReceive(distance_queue, &left_distance, pdMS_TO_TICKS(400))) {
              if (distance_cm > 10.0f) 
              { // Clear path after turn: Move forward at the defined speed
                otto_forward(speed); // Move forward at the defined speed
                return; // Exit the loop to continue normal operation
              } // If the path is still blocked after the turn, try turning right instead
            } 
            otto_turn_right(speed); // Turn right at the defined speed to try to navigate around the obstacle
            vTaskDelay(pdMS_TO_TICKS(1900)); // Turn for 1900ms which is an almost perfect 180° angle for wonky the otto (adjust as needed for a 180 degree turn)
            otto_stop(); // Stop after turning to ensure a more accurate forward movement
            float right_distance; // Variable to store distance measurement after the turn
            if (xQueueReceive(distance_queue, &right_distance, pdMS_TO_TICKS(400))) {
              if (distance_cm > 10.0f)              
              { // Clear path after turn: Move forward at the defined speed
                otto_forward(speed); // Move forward at the defined speed
                return; // Exit the loop to continue normal operation
              } // If the path is still blocked after the turn, try turning left again to attempt to navigate around the obstacle 
            }
            
            otto_turn_right(speed); // Turn right at the defined speed to try to navigate around the obstacle
            vTaskDelay(pdMS_TO_TICKS(900));  // Turn to face the original direction (adjust as needed for a 90 degree turn)
            otto_stop(); // Stop after turning to ensure a more accurate forward movement

            otto_reverse(0.5f); // Move backward at 50% speed to create some distance from the obstacle
            vTaskDelay(pdMS_TO_TICKS(200)); // Short delay to ensure the robot
            otto_stop(); // Stop before turning to ensure a more accurate turn
            xQueueReset(distance_queue); // Clear the distance queue to avoid processing stale measurements after the maneuver
            }
          }
        } 
        vTaskDelay(pdMS_TO_TICKS(US_TIMEOUT_MS));  // delay before next check
    }




    
