#include "ultrasonic.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "us_1wire";

// global variable for state
enum STATE app_state = IDLE;
// global variables for timestamps; volatile means, that
volatile int64_t trigger_start = 0;  // timestamp when trigger signal is sent
volatile int64_t measure_start = 0;  // timestamp when echo signal is received
volatile int64_t measure_time =
    0;  // duration of the echo signal, used for distance calculation

QueueHandle_t distance_queue;

void us_init() {
  // Initialize the ultrasonic sensor pins
  gpio_reset_pin(US_PIN);
  gpio_set_direction(US_PIN, GPIO_MODE_OUTPUT);
  gpio_pullup_en(US_PIN);
  gpio_set_level(US_PIN, 0);    // init state of trigger pin
  gpio_install_isr_service(0);  // allow individual pin interrupt setup

  distance_queue = xQueueCreate(
      1, sizeof(float));  // queue for 1 float value, used for sending distance
                          // measurements to other tasks
}

void us_start_echo(void* arg) {
  if (app_state != WAITING_FOR_ECHO_START)
    return;  // end function if not in the expected state
  measure_start = esp_timer_get_time();
  app_state = WAITING_FOR_ECHO_END;
  // ESP_LOGI(TAG, "echo received, wait for end; neg. edge");
  // reconfigure interrupt for echo end
  gpio_set_intr_type(US_PIN,
                     GPIO_INTR_NEGEDGE);  // add interrupt for negative edge
  gpio_isr_handler_add(US_PIN, us_end_echo, NULL);
}

void us_end_echo(void* arg) {
  if (app_state != WAITING_FOR_ECHO_END)
    return;  // end function if not in the expected state
  measure_time = esp_timer_get_time() - measure_start;
  app_state = IDLE;
  gpio_intr_disable(US_PIN);
  // ESP_LOGI(TAG, "echo end received");
  // reconfigure pin for next trigger signal
  gpio_set_direction(US_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(US_PIN, 0);
}

void display_distance(void) {
  ESP_LOGI(TAG, "measure_time (µs): %lld", measure_time);
  ESP_LOGI(TAG, "distance in cm: %.2f", (float)measure_time / US_ROUNDTRIP_CM);
  ESP_LOGI(TAG, "distance in m: %.2f\r\n",
           (float)measure_time / US_ROUNDTRIP_M);
}

void us_task(void* arg) {
  float distance_cm = -1.0f;  // default to timeout

  while (1) {
    ESP_LOGI(TAG, "send us signal");

    // Trigger Pulse: Low for 2..4 us, then high 10 us
    gpio_set_level(US_PIN, 0);
    esp_rom_delay_us(US_TRIGGER_LOW_DELAY);  // blocking delay for 4 us
    gpio_set_level(US_PIN, 1);
    esp_rom_delay_us(US_TRIGGER_HIGH_DELAY);  // blocking delay for 10 us
    gpio_set_level(US_PIN, 0);

    ESP_LOGI(TAG, "switch to input mode and wait for echo; pos. edge");
    gpio_intr_enable(US_PIN);
    gpio_set_intr_type(US_PIN, GPIO_INTR_POSEDGE);  // pos. edge
    gpio_isr_handler_add(US_PIN, us_start_echo, NULL);
    gpio_set_direction(US_PIN, GPIO_MODE_INPUT);

    trigger_start = esp_timer_get_time();  // store timestamp for timeout
                                           // handling after trigger signal
    app_state = WAITING_FOR_ECHO_START;    // set state to wait for echo start

    vTaskDelay(pdMS_TO_TICKS(US_TIMEOUT_MS));  // pause before repeat
    if (app_state != IDLE) {  // TODO - use timer&ISR instead of blocking delay
                              // for timeout handling
      ESP_LOGW(TAG, "timeout expired, no echo received");
      app_state = IDLE;
      // reconfigure pin for next trigger signal
      gpio_intr_disable(US_PIN);
      gpio_set_direction(US_PIN, GPIO_MODE_OUTPUT);
      gpio_set_level(US_PIN, 0);
      distance_cm = -1.0f;  // use -1 to indicate timeout / no measurement
    } else {
      display_distance();
      distance_cm = (float)measure_time / US_ROUNDTRIP_CM;
    }
    xQueueSend(distance_queue, &distance_cm, 0);  // non-blocking send
    vTaskDelay(pdMS_TO_TICKS(10));                // delay for sensor recovery
  }
}