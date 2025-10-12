# DS3231 RTC Driver for Raspberry Pi Pico

A simple, clean C driver for the DS3231 Real-Time Clock (RTC) designed for the Raspberry Pi Pico (RP2040) C/C++ SDK. Provides a high-level API for time/date, dual alarms, interrupts, and output configuration.

## Features
- Read/set time and date (year, month, day, day-of-week, hour, minute, second)
- Dual alarms (Alarm 1: seconds/minutes/hours/date|day; Alarm 2: minutes/hours/date|day)
- Alarm via polling (status flags) or INT/SQW interrupt
- Square-wave output on INT/SQW (1 Hz, 1.024 kHz, 4.096 kHz, 8.192 kHz)
- 32 kHz dedicated output enable/disable
- Robust API: functions return `true` on success, `false` on I2C failure

## Hardware Requirements
- Raspberry Pi Pico or compatible RP2040 board  
- DS3231 RTC module  
- Jumper wires for I2C

## Wiring
Connect the DS3231 to the Pico's I2C port (example uses `i2c0` with GP20/GP21):

| DS3231 Pin | Raspberry Pi Pico Pin | Description         |
|------------|------------------------|---------------------|
| VCC        | 3V3 (OUT) (Pin 36)     | 3.3V power          |
| GND        | GND (Pin 38)           | Ground              |
| SDA        | GP20 (Pin 26)          | I2C Data            |
| SCL        | GP21 (Pin 27)          | I2C Clock           |
| INT/SQW    | Any GPIO               | Interrupt / SQW Out |

You can change pins and I2C instance in your application code.

## Software Setup
1. Copy `ds3231.h` and `ds3231.c` into your project directory.  
2. Add the driver source to your CMakeLists:

```cmake
# ... other sources
src/ds3231.c
# ...
```

3. Include the header in your application:

```c
#include "ds3231.h"
```

All I2C communication is handled internally by the driver.

## Data Structures

Example datetime struct:

```c
typedef struct {
    uint8_t year;  // 0-99 (2000-2099)
    uint8_t month; // 1-12
    uint8_t day;   // 1-31
    uint8_t dow;   // 1-7 (1 = Sunday)
    uint8_t hour;  // 0-23
    uint8_t min;   // 0-59
    uint8_t sec;   // 0-59
} ds3231_datetime_t;
```

There is also a `ds3231_alarm_time_t` for alarm settings; fields used depend on alarm mode.

## API Overview

Time and date:
- `bool ds3231_set_time(const ds3231_datetime_t *dt);`  
  Set RTC time/date. Returns `true` on success.

- `bool ds3231_read_time(ds3231_datetime_t *dt);`  
  Read RTC time/date into `dt`. Returns `true` on success.

Alarms:
- `bool ds3231_set_alarm1(const ds3231_alarm_time_t *at, ds3231_alarm1_mode_t mode);`  
  Configure & enable Alarm 1 (supports seconds).

- `bool ds3231_set_alarm2(const ds3231_alarm_time_t *at, ds3231_alarm2_mode_t mode);`  
  Configure & enable Alarm 2 (no seconds field).

- `bool ds3231_clear_alarm_flag(ds3231_alarm_num_t alarm_num);`  
  Clear alarm flag (must be called after handling an alarm).

- `bool ds3231_disable_alarm(ds3231_alarm_num_t alarm_num);`  
  Disable a specific alarm interrupt.

Output / interrupt control:
- `bool ds3231_enable_interrupt_mode(void);`  
  Configure INT/SQW for alarm interrupt (active low). Required for alarms.

- `bool ds3231_enable_sqw_output(ds3231_sqw_freq_t freq);`  
  Configure INT/SQW as a square-wave generator (disables alarm interrupts).

- `bool ds3231_enable_32khz_output(bool enable);`  
  Enable/disable the dedicated 32 kHz output pin.

Return values: functions return `true` on success, `false` on I2C communication failure.

## Notes
- Alarm modes determine which fields of the alarm struct are used. Consult the header for enum names like `ALARM1_*`, `ALARM2_*`, and `SQW_FREQ_*`.
- After handling an alarm you must clear its flag with `ds3231_clear_alarm_flag()` to allow further interrupts.

For full usage examples, see the project's example code (if provided) or inspect `ds3231.h` for detailed enums and struct definitions.
