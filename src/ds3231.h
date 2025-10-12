/**
 * @file ds3231.h
 * @brief Driver for the DS3231 I2C Real-Time Clock (RTC).
 * @author (Your Name or Company)
 * @version 1.0
 * @date 2025-10-12
 *
 * @copyright Copyright (c) 2025
 *
 * This header provides a high-level interface for interacting with the
 * DS3231 RTC module using the Raspberry Pi Pico SDK. It includes functions
 * for setting and reading time, managing alarms, and configuring the
 * square-wave output.
 */

#ifndef DS3231_H
#define DS3231_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

// =============================================================================
// == Configuration Macros                                                    ==
// =============================================================================

/**
 * @brief Default I2C address for the DS3231 RTC.
 */
#define DS3231_I2C_ADDR 0x68

/**
 * @brief The default I2C port to use with the DS3231.
 * @note This can be overridden by defining it before including this header,
 * e.g., in your CMakeLists.txt:
 * `add_compile_definitions(DS3231_I2C_PORT=i2c1)`
 */
#ifndef DS3231_I2C_PORT
#define DS3231_I2C_PORT i2c0
#endif

// =============================================================================
// == Public Data Types                                                       ==
// =============================================================================

/**
 * @brief Structure to hold all date and time information.
 */
typedef struct {
    uint8_t year;  /**< Year (0-99). Represents the years 2000-2099. */
    uint8_t month; /**< Month of the year (1-12). */
    uint8_t day;   /**< Day of the month (1-31). */
    uint8_t dow;   /**< Day of the week (1-7, where 1 = Sunday). */
    uint8_t hour;  /**< Hour in 24-hour format (0-23). */
    uint8_t min;   /**< Minute (0-59). */
    uint8_t sec;   /**< Second (0-59). */
} ds3231_datetime_t;

/**
 * @brief Structure to hold alarm time information.
 * @note The fields used depend on the alarm mode selected.
 */
typedef struct {
    uint8_t day;  /**< Day of month (1-31) or day of week (1-7). */
    uint8_t hour; /**< Hour (0-23). */
    uint8_t min;  /**< Minute (0-59). */
    uint8_t sec;  /**< Second (0-59), only applicable for Alarm 1. */
} ds3231_alarm_time_t;

/**
 * @brief Identifiers for the two available alarms.
 */
typedef enum {
    ALARM_1 = 0, /**< Selects Alarm 1. */
    ALARM_2 = 1  /**< Selects Alarm 2. */
} ds3231_alarm_num_t;

/**
 * @brief Defines the trigger condition for Alarm 1.
 */
typedef enum {
    ALARM1_ONCE_PER_SECOND,            /**< Triggers every second. */
    ALARM1_SECONDS_MATCH,              /**< Triggers when seconds match. */
    ALARM1_MINUTES_SECONDS_MATCH,      /**< Triggers when minutes and seconds match. */
    ALARM1_HOURS_MINUTES_SECONDS_MATCH,/**< Triggers when hours, minutes, and seconds match. */
    ALARM1_DATE_H_M_S_MATCH,           /**< Triggers when date, hours, minutes, and seconds match. */
    ALARM1_DAY_OF_WEEK_H_M_S_MATCH     /**< Triggers when day of week, hours, minutes, and seconds match. */
} ds3231_alarm1_mode_t;

/**
 * @brief Defines the trigger condition for Alarm 2.
 * @note Alarm 2 does not have a seconds match capability.
 */
typedef enum {
    ALARM2_ONCE_PER_MINUTE,       /**< Triggers every minute (at second 00). */
    ALARM2_MINUTES_MATCH,         /**< Triggers when minutes match. */
    ALARM2_HOURS_MINUTES_MATCH,   /**< Triggers when hours and minutes match. */
    ALARM2_DATE_H_M_MATCH,        /**< Triggers when date, hours, and minutes match. */
    ALARM2_DAY_OF_WEEK_H_M_MATCH  /**< Triggers when day of week, hours, and minutes match. */
} ds3231_alarm2_mode_t;

/**
 * @brief Defines the frequency for the square-wave output pin (SQW).
 */
typedef enum {
    SQW_FREQ_1HZ    = 0b00, /**< 1 Hz square wave. */
    SQW_FREQ_1024HZ = 0b01, /**< 1.024 kHz square wave. */
    SQW_FREQ_4096HZ = 0b10, /**< 4.096 kHz square wave. */
    SQW_FREQ_8192HZ = 0b11  /**< 8.192 kHz square wave. */
} ds3231_sqw_freq_t;


// =============================================================================
// == Function Prototypes                                                     ==
// =============================================================================

// --- Time and Date Functions ---

/**
 * @brief Sets the current date and time on the DS3231.
 * @param dt Pointer to a `ds3231_datetime_t` struct containing the time to set.
 * @return `true` if the time was set successfully, `false` on I2C communication failure.
 */
bool ds3231_set_time(const ds3231_datetime_t *dt);

/**
 * @brief Reads the current date and time from the DS3231.
 * @param dt Pointer to a `ds3231_datetime_t` struct to store the retrieved time.
 * @return `true` if the time was read successfully, `false` on I2C communication failure.
 */
bool ds3231_read_time(ds3231_datetime_t *dt);


// --- Alarm Functions ---

/**
 * @brief Sets and enables Alarm 1 with a specific trigger mode.
 * @param at Pointer to a `ds3231_alarm_time_t` struct with the desired alarm time.
 * @param mode The trigger mode for Alarm 1, from `ds3231_alarm1_mode_t`.
 * @return `true` on success, `false` on failure.
 */
bool ds3231_set_alarm1(const ds3231_alarm_time_t *at, ds3231_alarm1_mode_t mode);

/**
 * @brief Sets and enables Alarm 2 with a specific trigger mode.
 * @param at Pointer to a `ds3231_alarm_time_t` struct with the desired alarm time.
 * @param mode The trigger mode for Alarm 2, from `ds3231_alarm2_mode_t`.
 * @return `true` on success, `false` on failure.
 */
bool ds3231_set_alarm2(const ds3231_alarm_time_t *at, ds3231_alarm2_mode_t mode);

/**
 * @brief Checks if an alarm's trigger flag is set in the status register.
 * @note This is intended for polling. After an alarm triggers, its flag must
 * be cleared with `ds3231_clear_alarm_flag()`.
 * @param alarm_num The alarm to check (`ALARM_1` or `ALARM_2`).
 * @return `true` if the specified alarm flag is set, `false` otherwise.
 */
bool ds3231_check_alarm_flag(ds3231_alarm_num_t alarm_num);

/**
 * @brief Clears an alarm's trigger flag in the status register.
 * @note This must be called after an alarm triggers to allow the INT/SQW pin
 * to return to its high state and to enable future alarms.
 * @param alarm_num The alarm flag to clear (`ALARM_1` or `ALARM_2`).
 * @return `true` on success, `false` on failure.
 */
bool ds3231_clear_alarm_flag(ds3231_alarm_num_t alarm_num);

/**
 * @brief Disables a specified alarm, preventing it from triggering.
 * @param alarm_num The alarm to disable (`ALARM_1` or `ALARM_2`).
 * @return `true` on success, `false` on failure.
 */
bool ds3231_disable_alarm(ds3231_alarm_num_t alarm_num);


// --- Output Control Functions ---

/**
 * @brief Enables or disables the 32kHz output signal on the 32KHz pin.
 * @param enable Set to `true` to enable the output, `false` to disable it.
 * @return `true` on success, `false` on failure.
 */
bool ds3231_enable_32khz_output(bool enable);

/**
 * @brief Enables the square-wave output on the INT/SQW pin.
 * @note This disables the interrupt alarm functionality.
 * @param freq The desired frequency from `ds3231_sqw_freq_t`.
 * @return `true` on success, `false` on failure.
 */
bool ds3231_enable_sqw_output(ds3231_sqw_freq_t freq);

/**
 * @brief Enables interrupt-driven alarms on the INT/SQW pin.
 * @note This is the default mode and disables the square-wave output.
 * @return `true` on success, `false` on failure.
 */
bool ds3231_enable_interrupt_mode();


// =============================================================================
// == Internal Helper Functions                                               ==
// =============================================================================
// These functions are provided as `static inline` for performance and are not
// part of the public API, though they are necessary for the driver's operation.

/**
 * @brief Converts a decimal number to Binary-Coded Decimal (BCD).
 */
static inline uint8_t dec_to_bcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}

/**
 * @brief Converts a Binary-Coded Decimal (BCD) number to decimal.
 */
static inline uint8_t bcd_to_dec(uint8_t val) {
    return (val / 16 * 10) + (val % 16);
}

#endif // DS3231_H