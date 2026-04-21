#include "motor_control.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "otto_wheels"
// calibration values for the motors
#define CALI_LEFT 1.05f
#define CALI_RIGHT 0.95f
// Toogled OTTO's wonky wheels to make them more balanced. Adjust as needed for your specific hardware setup.

/* ********** */
/*   HELPERS  */
/* ********** */
/** @brief Convert microseconds → LEDC duty count */
uint32_t us_to_duty(uint32_t pulse_us) {
  uint32_t max_duty = (1 << MOTOR_TIMER_BITS) - 1;
  return (uint32_t)((float)pulse_us / PERIOD_US * max_duty);
}

/** @brief Initialize LEDC for both wheel servos */
void motors_init(void) {
  ledc_timer_config_t timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
      .duty_resolution = MOTOR_TIMER_BITS,
      .freq_hz = MOTOR_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer));

  ledc_channel_config_t left = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = CH_LEFT,
      .timer_sel = LEDC_TIMER_0,
      .gpio_num = MOTOR_LEFT_GPIO,
      .duty = us_to_duty(PULSE_NEUTRAL),
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&left));

  ledc_channel_config_t right = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = CH_RIGHT,
      .timer_sel = LEDC_TIMER_0,
      .gpio_num = MOTOR_RIGHT_GPIO,
      .duty = us_to_duty(PULSE_NEUTRAL),
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&right));

  ESP_LOGI(TAG, "Motors initialized on GPIO %d (left) and GPIO %d (right)",
           MOTOR_LEFT_GPIO, MOTOR_RIGHT_GPIO);
}

/** @brief Set individual motor speed
   speed: -1.0 (full reverse) → 0.0 (stop) → +1.0 (full forward)
   invert: true for the right motor (faces opposite direction)
*/
void motor_set(ledc_channel_t ch, float speed, bool invert) {
  if (speed > 1.0f) speed = 1.0f;
  if (speed < -1.0f) speed = -1.0f;
  if (invert) speed = -speed;

  // Map -1..+1 → PULSE_FULL_FWD..PULSE_FULL_REV
  float t = (speed + 1.0f) / 2.0f;  // 0.0 → 1.0
  uint32_t pulse_us =
      (uint32_t)(PULSE_FULL_FWD + t * (PULSE_FULL_REV - PULSE_FULL_FWD));

  ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, us_to_duty(pulse_us));
  ledc_update_duty(LEDC_LOW_SPEED_MODE, ch);
}

/* *********************** */
/* High-level movement API */
/* *********************** */
/** @brief Move the robot forward
 *  @param speed: 0.0 → 1.0
 */
void otto_forward(float speed) {
  ESP_LOGI(TAG, "Forward %.0f%%", speed * 100);
  motor_set(CH_LEFT, speed * CALI_LEFT, false);
  motor_set(CH_RIGHT, speed * CALI_RIGHT, true);  // right wheel faces opposite way
}

/** @brief Move the robot backward
 *  @param speed: 0.0 → 1.0
 */
void otto_reverse(float speed) {
  ESP_LOGI(TAG, "Reverse %.0f%%", speed * 100);
  motor_set(CH_LEFT, -speed * CALI_LEFT, false);
  motor_set(CH_RIGHT, -speed * CALI_RIGHT, true);
}

/** @brief  Spin in place: left fwd + right rev (or vice versa)
 *  @param speed: 0.0 → 1.0
 */
void otto_turn_left(float speed) {
  ESP_LOGI(TAG, "Turn left %.0f%%", speed * 100);
  motor_set(CH_LEFT, -speed * CALI_LEFT, false);
  motor_set(CH_RIGHT, speed * CALI_RIGHT, true);
}

/** @brief  Spin in place: left fwd + right rev (or vice versa)
 *  @param speed: 0.0 → 1.0
 */
void otto_turn_right(float speed) {
  ESP_LOGI(TAG, "Turn right %.0f%%", speed * 100);
  motor_set(CH_LEFT, speed * CALI_LEFT, false);
  motor_set(CH_RIGHT, -speed * CALI_RIGHT, true);
}

/** @brief Stop the robot
 *  @param speed: 0.0 → 1.0
 */
void otto_stop(void) {
  ESP_LOGI(TAG, "Stop");
  motor_set(CH_LEFT, 0.0f, false);
  motor_set(CH_RIGHT, 0.0f, true);
}
