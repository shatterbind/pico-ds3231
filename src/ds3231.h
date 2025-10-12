#ifndef __DS3231_H
#define __DS3231_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// =============================================================================
// I2C Configuration
// =============================================================================
#define RTC_ADDR     0x68
#define I2C_PORT i2c0

// =============================================================================
// Data Structures
// =============================================================================

// Structure for holding and passing date/time data
typedef struct {
    uint8_t year;   // 0-99
    uint8_t month;  // 1-12
    uint8_t day;    // 1-31
    uint8_t dow;    // 1-7 (Day of week)
    uint8_t hour;   // 0-23
    uint8_t min;    // 0-59
    uint8_t sec;    // 0-59
} DateTime;

// Structure for setting alarm values. Not all fields are used by all modes.
typedef struct {
    uint8_t day;  // Day of month (1-31) or day of week (1-7)
    uint8_t hour; // 0-23
    uint8_t min;  // 0-59
    uint8_t sec;  // 0-59 (Only used for Alarm 1)
} AlarmTime;

// =============================================================================
// Alarm Configuration Enums
// =============================================================================

// Identifiers for the two alarms
typedef enum {
    ALARM_1 = 0,
    ALARM_2 = 1
} AlarmNum;

// Defines how Alarm 1 will trigger. The names describe the condition.
typedef enum {
    ALARM1_ONCE_PER_SECOND,                 // Every second
    ALARM1_SECONDS_MATCH,                   // Every minute, when seconds match
    ALARM1_MINUTES_SECONDS_MATCH,           // Every hour, when min & sec match
    ALARM1_HOURS_MINUTES_SECONDS_MATCH,     // Every day, when h, m, s match
    ALARM1_DATE_H_M_S_MATCH,                // When date, h, m, s match
    ALARM1_DAY_OF_WEEK_H_M_S_MATCH          // When day of week, h, m, s match
} Alarm1Mode;

// Defines how Alarm 2 will trigger. Note: Alarm 2 has no seconds field.
typedef enum {
    ALARM2_ONCE_PER_MINUTE,                 // Every minute (at 00 seconds)
    ALARM2_MINUTES_MATCH,                   // Every hour, when minutes match
    ALARM2_HOURS_MINUTES_MATCH,             // Every day, when h, m match
    ALARM2_DATE_H_M_MATCH,                  // When date, h, m match
    ALARM2_DAY_OF_WEEK_H_M_MATCH            // When day of week, h, m match
} Alarm2Mode;

typedef enum {
    SQW_FREQ_1HZ    = 0b00,
    SQW_FREQ_1024HZ = 0b01,
    SQW_FREQ_4096HZ = 0b10,
    SQW_FREQ_8192HZ = 0b11
} SQWFrequency;


// =============================================================================
// Helper Functions (can be used by the implementation .c file)
// =============================================================================

// BCD conversion is needed for all RTC register writes
static inline uint8_t decToBcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}
static inline uint8_t bcdToDec(uint8_t val) {
    return (val / 16 * 10) + (val % 16);
}

// =============================================================================
// Function Prototypes
// =============================================================================

// --- Time and Date Functions ---
bool ds3231_set_time(const DateTime *dt);
bool ds3231_read_time(DateTime *dt);


// --- Alarm Functions ---
/**
 * @brief Set and enable Alarm 1 with a specific trigger mode.
 */
bool ds3231_set_alarm1(const AlarmTime *at, Alarm1Mode mode);

/**
 * @brief Set and enable Alarm 2 with a specific trigger mode.
 */
bool ds3231_set_alarm2(const AlarmTime *at, Alarm2Mode mode);

/**
 * @brief Check if an alarm's trigger flag is set in the status register.
 * @note This is for polling. For interrupts, you would check this in your ISR.
 */
bool ds3231_check_alarm_flag(AlarmNum alarm_num);

/**
 * @brief Clear an alarm's trigger flag. This must be called after an alarm
 * triggers to reset the INT/SQW pin and allow for the next alarm.
 */
bool ds3231_clear_alarm_flag(AlarmNum alarm_num);

/**
 * @brief Disable an alarm, preventing it from triggering.
 */
bool ds3231_disable_alarm(AlarmNum alarm_num);


bool ds3231_enable_32khz_output(bool enable);

bool ds3231_enable_sqw_output(SQWFrequency freq);

bool ds3231_enable_interrupt_mode();
#endif // __DS3231_H