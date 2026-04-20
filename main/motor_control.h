#pragma once

#include "driver/ledc.h"

/* ********** */
/*   CONFIG   */
/* ********** */

//  HP Otto GPIO mapping
#define MOTOR_LEFT_GPIO 14   // Connector 10
#define MOTOR_RIGHT_GPIO 13  // Connector 11

//  Continuous rotation - PWM parameters
// Standard servo: 50 Hz, pulse width controls speed + direction
#define MOTOR_FREQ_HZ 50
#define MOTOR_TIMER_BITS LEDC_TIMER_14_BIT  // 14-bit → 0–16383

// Pulse widths in microseconds — TODO: calibrate NEUTRAL per your servos!
#define PULSE_FULL_FWD 1000  // µs → full speed forward
#define PULSE_NEUTRAL 1500   // µs → stop (trim this if wheels drift)
#define PULSE_FULL_REV 2000  // µs → full speed reverse

// Period length: 1/50Hz = 20,000 µs
#define PERIOD_US 20000

// Motor channel assignments
#define CH_LEFT LEDC_CHANNEL_0
#define CH_RIGHT LEDC_CHANNEL_1

uint32_t us_to_duty(uint32_t pulse_us);
void motors_init(void);
void motor_set(ledc_channel_t ch, float speed, bool invert);
void otto_forward(float speed);
void otto_reverse(float speed);
void otto_turn_left(float speed);
void otto_turn_right(float speed);
void otto_stop(void);
void motor_task(void* arg);