#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "motor_control.h"
#include "ultrasonic.h"

void app_main(void) {
  float speed = 0.6f;
  float distance_cm = 0.0f;

  us_init();
  motors_init();

  xTaskCreate(us_task, "us_task", 4096, NULL, 5, NULL);

  while (1) {
    /* 
        if a task calls xQueueReceive(distance_queue, &distance, portMAX_DELAY);
        it blocks until an item is added
        when another task calls xQueueSend(...), 
        the receiving task is automatically unblocked
    */
    if (xQueueReceive(distance_queue, &distance_cm, pdMS_TO_TICKS(1000)) ==
        pdPASS) {
      // distance updated
    }

    /*
        logic for motor control based on distance measurement;
        this is a very simple example, you can implement more complex behavior here
    */
    if (distance_cm < 0.0f) {
      ESP_LOGI("main", "Distance measurement timeout, keep previous state.");
    } else if (distance_cm > 10.0f) {
      otto_forward(speed);
    } else if (distance_cm > 5.0f) {
      otto_forward(0.25f);  // slow down when close
    } else {
      otto_stop();
    }
  }
  vTaskDelay(pdMS_TO_TICKS(US_TIMEOUT_MS));  // delay before next check
}
