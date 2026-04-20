#pragma once  // include guard

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define US_PIN 19  // Ultrasonic sensor trigger AND echo pin

// Timing constants for the ultrasonic sensor trigger signal
#define US_TRIGGER_LOW_DELAY 4
#define US_TRIGGER_HIGH_DELAY 10
// Constants for distance calculation based on the speed of sound
#define US_ROUNDTRIP_M 5800.0f
#define US_ROUNDTRIP_CM 58
#define US_TIMEOUT_MS 180  // timeout for echo signal in milliseconds

// states of the state machine for ultrasonic sensor measurement;
// minimal example, no error handling, no retries, no averaging, no filtering
enum STATE { IDLE, WAITING_FOR_ECHO_START, WAITING_FOR_ECHO_END };

extern QueueHandle_t
    distance_queue;  // FreeRTOS queue for sending distance measurements to
                     // other tasks; -1 to indicate timeout / no measurement

/** @brief Initialize the ultrasonic sensor */
void us_init();
/** @brief Logging distance measurements */
void display_distance(void);
/** @brief Interrupt service routine for starting the echo measurement */
void IRAM_ATTR us_start_echo(void* arg);
/** @brief Interrupt service routine for ending the echo measurement */
void IRAM_ATTR us_end_echo(void* arg);
/** @brief Task for handling ultrasonic sensor measurements */
void us_task(void* arg);
