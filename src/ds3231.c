#include "ds3231.h"

// =============================================================================
// Internal Register Definitions
// =============================================================================
#define DS3231_REG_TIME         0x00
#define DS3231_REG_ALARM1       0x07
#define DS3231_REG_ALARM2       0x0B
#define DS3231_REG_CONTROL      0x0E
#define DS3231_REG_STATUS       0x0F

// Control Register Bits
#define DS3231_CONTROL_A1IE     0x01 // Alarm 1 Interrupt Enable
#define DS3231_CONTROL_A2IE     0x02 // Alarm 2 Interrupt Enable
#define DS3231_CONTROL_INTCN    0x04 // Interrupt Control

// Status Register Bits
#define DS3231_STATUS_A1F       0x01 // Alarm 1 Flag
#define DS3231_STATUS_A2F       0x02 // Alarm 2 Flag


// =============================================================================
// Time and Date Functions
// =============================================================================

// Corrected function name to match header
bool ds3231_set_time(const DateTime *dt) {
    if (dt == NULL) return false;

    uint8_t buf[8];
    buf[0] = DS3231_REG_TIME; // Start write at register 0x00 (Seconds)
    buf[1] = decToBcd(dt->sec);
    buf[2] = decToBcd(dt->min);
    buf[3] = decToBcd(dt->hour);
    buf[4] = decToBcd(dt->dow);
    buf[5] = decToBcd(dt->day);
    buf[6] = decToBcd(dt->month);
    buf[7] = decToBcd(dt->year);

    int bytes_written = i2c_write_blocking(I2C_PORT, RTC_ADDR, buf, 8, false);
    return (bytes_written == 8);
}

// Implemented and corrected read function
bool ds3231_read_time(DateTime *dt) {
    if (dt == NULL) return false;

    uint8_t read_buf[7];
    uint8_t start_reg = DS3231_REG_TIME;

    // Set the register pointer to 0x00 to start reading time
    if (i2c_write_blocking(I2C_PORT, RTC_ADDR, &start_reg, 1, true) != 1) {
        return false;
    }
    
    // Read all 7 bytes of time/date data
    if (i2c_read_blocking(I2C_PORT, RTC_ADDR, read_buf, 7, false) != 7) {
        return false;
    }

    // Convert BCD data to decimal and populate the struct
    dt->sec   = bcdToDec(read_buf[0]);
    dt->min   = bcdToDec(read_buf[1]);
    dt->hour  = bcdToDec(read_buf[2]);
    dt->dow   = bcdToDec(read_buf[3]);
    dt->day   = bcdToDec(read_buf[4]);
    dt->month = bcdToDec(read_buf[5]);
    dt->year  = bcdToDec(read_buf[6]);

    return true;
}

// =============================================================================
// Alarm Functions
// =============================================================================

bool ds3231_set_alarm1(const AlarmTime *at, Alarm1Mode mode) {
    if (at == NULL) return false;

    uint8_t buf[5];
    buf[0] = DS3231_REG_ALARM1; // Start register for Alarm 1
    buf[1] = decToBcd(at->sec);
    buf[2] = decToBcd(at->min);
    buf[3] = decToBcd(at->hour);
    buf[4] = decToBcd(at->day);
    
    // Set the alarm mask bits based on the selected mode
    switch (mode) {
        case ALARM1_ONCE_PER_SECOND:
            buf[1] |= 0x80; // A1M1 = 1
            buf[2] |= 0x80; // A1M2 = 1
            buf[3] |= 0x80; // A1M3 = 1
            buf[4] |= 0x80; // A1M4 = 1
            break;
        case ALARM1_SECONDS_MATCH:
            buf[1] &= ~0x80; // A1M1 = 0
            buf[2] |= 0x80;  // A1M2 = 1
            buf[3] |= 0x80;  // A1M3 = 1
            buf[4] |= 0x80;  // A1M4 = 1
            break;
        case ALARM1_MINUTES_SECONDS_MATCH:
            buf[1] &= ~0x80; // A1M1 = 0
            buf[2] &= ~0x80; // A1M2 = 0
            buf[3] |= 0x80;  // A1M3 = 1
            buf[4] |= 0x80;  // A1M4 = 1
            break;
        case ALARM1_HOURS_MINUTES_SECONDS_MATCH:
            buf[1] &= ~0x80; // A1M1 = 0
            buf[2] &= ~0x80; // A1M2 = 0
            buf[3] &= ~0x80; // A1M3 = 0
            buf[4] |= 0x80;  // A1M4 = 1
            break;
        case ALARM1_DATE_H_M_S_MATCH:
            buf[1] &= ~0x80; // A1M1 = 0
            buf[2] &= ~0x80; // A1M2 = 0
            buf[3] &= ~0x80; // A1M3 = 0
            buf[4] &= ~0x80; // A1M4 = 0
            buf[4] &= ~0x40; // DY/DT = 0 (Match Date)
            break;
        case ALARM1_DAY_OF_WEEK_H_M_S_MATCH:
            buf[1] &= ~0x80; // A1M1 = 0
            buf[2] &= ~0x80; // A1M2 = 0
            buf[3] &= ~0x80; // A1M3 = 0
            buf[4] &= ~0x80; // A1M4 = 0
            buf[4] |= 0x40;  // DY/DT = 1 (Match Day of Week)
            break;
    }

    // Write the alarm settings
    if (i2c_write_blocking(I2C_PORT, RTC_ADDR, buf, 5, false) != 5) {
        return false;
    }

    // Enable Alarm 1 Interrupt (A1IE) in the Control Register
    uint8_t control_reg;
    uint8_t reg_addr = DS3231_REG_CONTROL;
    i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true);
    i2c_read_blocking(I2C_PORT, RTC_ADDR, &control_reg, 1, false);

    control_reg |= DS3231_CONTROL_A1IE | DS3231_CONTROL_INTCN;
    
    uint8_t control_buf[2] = {DS3231_REG_CONTROL, control_reg};
    return (i2c_write_blocking(I2C_PORT, RTC_ADDR, control_buf, 2, false) == 2);
}

bool ds3231_set_alarm2(const AlarmTime *at, Alarm2Mode mode) {
    if (at == NULL) return false;

    uint8_t buf[4];
    buf[0] = DS3231_REG_ALARM2; // Start register for Alarm 2
    buf[1] = decToBcd(at->min);
    buf[2] = decToBcd(at->hour);
    buf[3] = decToBcd(at->day);

    switch (mode) {
        case ALARM2_ONCE_PER_MINUTE:
            buf[1] |= 0x80; // A2M2 = 1
            buf[2] |= 0x80; // A2M3 = 1
            buf[3] |= 0x80; // A2M4 = 1
            break;
        case ALARM2_MINUTES_MATCH:
            buf[1] &= ~0x80; // A2M2 = 0
            buf[2] |= 0x80;  // A2M3 = 1
            buf[3] |= 0x80;  // A2M4 = 1
            break;
        case ALARM2_HOURS_MINUTES_MATCH:
            buf[1] &= ~0x80; // A2M2 = 0
            buf[2] &= ~0x80; // A2M3 = 0
            buf[3] |= 0x80;  // A2M4 = 1
            break;
        case ALARM2_DATE_H_M_MATCH:
            buf[1] &= ~0x80; // A2M2 = 0
            buf[2] &= ~0x80; // A2M3 = 0
            buf[3] &= ~0x80; // A2M4 = 0
            buf[3] &= ~0x40; // DY/DT = 0 (Match Date)
            break;
        case ALARM2_DAY_OF_WEEK_H_M_MATCH:
            buf[1] &= ~0x80; // A2M2 = 0
            buf[2] &= ~0x80; // A2M3 = 0
            buf[3] &= ~0x80; // A2M4 = 0
            buf[3] |= 0x40;  // DY/DT = 1 (Match Day of Week)
            break;
    }

    if (i2c_write_blocking(I2C_PORT, RTC_ADDR, buf, 4, false) != 4) {
        return false;
    }
    
    // Enable Alarm 2 Interrupt (A2IE)
    uint8_t control_reg;
    uint8_t reg_addr = DS3231_REG_CONTROL;
    i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true);
    i2c_read_blocking(I2C_PORT, RTC_ADDR, &control_reg, 1, false);

    control_reg |= DS3231_CONTROL_A2IE | DS3231_CONTROL_INTCN;

    uint8_t control_buf[2] = {DS3231_REG_CONTROL, control_reg};
    return (i2c_write_blocking(I2C_PORT, RTC_ADDR, control_buf, 2, false) == 2);
}

bool ds3231_check_alarm_flag(AlarmNum alarm_num) {
    uint8_t status_reg;
    uint8_t reg_addr = DS3231_REG_STATUS;
    i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true);
    i2c_read_blocking(I2C_PORT, RTC_ADDR, &status_reg, 1, false);

    if (alarm_num == ALARM_1) {
        return (status_reg & DS3231_STATUS_A1F) != 0;
    } else {
        return (status_reg & DS3231_STATUS_A2F) != 0;
    }
}

bool ds3231_clear_alarm_flag(AlarmNum alarm_num) {
    uint8_t status_reg;
    uint8_t reg_addr = DS3231_REG_STATUS;
    i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true);
    i2c_read_blocking(I2C_PORT, RTC_ADDR, &status_reg, 1, false);

    if (alarm_num == ALARM_1) {
        status_reg &= ~DS3231_STATUS_A1F;
    } else {
        status_reg &= ~DS3231_STATUS_A2F;
    }

    uint8_t status_buf[2] = {DS3231_REG_STATUS, status_reg};
    return (i2c_write_blocking(I2C_PORT, RTC_ADDR, status_buf, 2, false) == 2);
}

bool ds3231_disable_alarm(AlarmNum alarm_num) {
    uint8_t control_reg;
    uint8_t reg_addr = DS3231_REG_CONTROL;
    i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true);
    i2c_read_blocking(I2C_PORT, RTC_ADDR, &control_reg, 1, false);

    if (alarm_num == ALARM_1) {
        control_reg &= ~DS3231_CONTROL_A1IE;
    } else {
        control_reg &= ~DS3231_CONTROL_A2IE;
    }

    uint8_t control_buf[2] = {DS3231_REG_CONTROL, control_reg};
    return (i2c_write_blocking(I2C_PORT, RTC_ADDR, control_buf, 2, false) == 2);
}

bool ds3231_enable_32khz_output(bool enable) {
    uint8_t status_reg;
    uint8_t reg_addr = DS3231_REG_STATUS; // 0x0F

    if (i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true) != 1) return false;
    if (i2c_read_blocking(I2C_PORT, RTC_ADDR, &status_reg, 1, false) != 1) return false;

    if (enable) {
        status_reg |= (1 << 3); // Установить бит в 1
    } else {
        status_reg &= ~(1 << 3); // Сбросить бит в 0
    }

    uint8_t buffer[2] = {DS3231_REG_STATUS, status_reg};
    return (i2c_write_blocking(I2C_PORT, RTC_ADDR, buffer, 2, false) == 2);
}

bool ds3231_enable_sqw_output(SQWFrequency freq) {
    uint8_t control_reg;
    uint8_t reg_addr = DS3231_REG_CONTROL; // 0x0E

    // 1. Прочитать регистр
    if (i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true) != 1) return false;
    if (i2c_read_blocking(I2C_PORT, RTC_ADDR, &control_reg, 1, false) != 1) return false;

    // 2. Изменить биты
    // Сбросить бит INTCN в 0, чтобы включить режим SQW
    control_reg &= ~(1 << 2); // INTCN = 0

    // Сначала очистим биты выбора частоты (RS2, RS1)
    control_reg &= ~((1 << 4) | (1 << 3)); 
    
    // Теперь установим новые значения частоты
    control_reg |= (freq << 3);

    // 3. Записать новое значение
    uint8_t buffer[2] = {DS3231_REG_CONTROL, control_reg};
    return (i2c_write_blocking(I2C_PORT, RTC_ADDR, buffer, 2, false) == 2);
}

bool ds3231_enable_interrupt_mode() {
    uint8_t control_reg;
    uint8_t reg_addr = DS3231_REG_CONTROL; // 0x0E

    // 1. Прочитать регистр
    if (i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg_addr, 1, true) != 1) return false;
    if (i2c_read_blocking(I2C_PORT, RTC_ADDR, &control_reg, 1, false) != 1) return false;

    control_reg |= (1 << 2); // INTCN = 1

    uint8_t buffer[2] = {DS3231_REG_CONTROL, control_reg};
    return (i2c_write_blocking(I2C_PORT, RTC_ADDR, buffer, 2, false) == 2);
}