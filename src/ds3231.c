#include "ds3231.h"

// =============================================================================
// == Internal Register & Bit Definitions                                     ==
// =============================================================================

// -- Register Addresses --
#define DS3231_REG_TIME         0x00
#define DS3231_REG_ALARM1       0x07
#define DS3231_REG_ALARM2       0x0B
#define DS3231_REG_CONTROL      0x0E
#define DS3231_REG_STATUS       0x0F

// -- Control Register Bits (0x0E) --
#define CONTROL_A1IE_BIT        (1 << 0) // Alarm 1 Interrupt Enable
#define CONTROL_A2IE_BIT        (1 << 1) // Alarm 2 Interrupt Enable
#define CONTROL_INTCN_BIT       (1 << 2) // Interrupt Control
#define CONTROL_RS1_BIT         (1 << 3) // Rate Select 1
#define CONTROL_RS2_BIT         (1 << 4) // Rate Select 2

// -- Status Register Bits (0x0F) --
#define STATUS_A1F_BIT          (1 << 0) // Alarm 1 Flag
#define STATUS_A2F_BIT          (1 << 1) // Alarm 2 Flag
#define STATUS_EN32KHZ_BIT      (1 << 3) // Enable 32kHz Output

// -- Alarm Register Bits --
#define ALARM_MASK_BIT          0x80 // Bit 7 of each alarm register (AxMy)
#define ALARM_DYDT_BIT          0x40 // Bit 6 of day/date register (DY/DT)


// =============================================================================
// == Static Helper Functions (Internal I2C Communication)                    ==
// =============================================================================

/**
 * @brief Reads a single byte from a specified DS3231 register.
 * @param reg_addr The address of the register to read from.
 * @param data Pointer to a uint8_t to store the read value.
 * @return `true` on success, `false` on I2C failure.
 */
static bool s_read_register(uint8_t reg_addr, uint8_t *data) {
    if (i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, &reg_addr, 1, true) != 1) {
        return false;
    }
    if (i2c_read_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, data, 1, false) != 1) {
        return false;
    }
    return true;
}

/**
 * @brief Writes a single byte to a specified DS3231 register.
 * @param reg_addr The address of the register to write to.
 * @param value The byte value to write.
 * @return `true` on success, `false` on I2C failure.
 */
static bool s_write_register(uint8_t reg_addr, uint8_t value) {
    uint8_t buf[2] = {reg_addr, value};
    return (i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, buf, 2, false) == 2);
}

/**
 * @brief Performs a read-modify-write operation on a register.
 * @param reg_addr Address of the register to update.
 * @param mask A bitmask of the bits to clear before setting new values.
 * @param bits The new bit values to set (ORed) into the register.
 * @return `true` on success, `false` on I2C failure.
 */
static bool s_update_register_bits(uint8_t reg_addr, uint8_t mask, uint8_t bits) {
    uint8_t reg_value;
    if (!s_read_register(reg_addr, &reg_value)) {
        return false;
    }

    reg_value &= ~mask; // Clear the bits specified by the mask
    reg_value |= bits;  // Set the new bits

    return s_write_register(reg_addr, reg_value);
}


// =============================================================================
// == Public API Implementation                                               ==
// =============================================================================

// --- Time and Date Functions ---

bool ds3231_set_time(const ds3231_datetime_t *dt) {
    if (dt == NULL) {
        return false;
    }

    const size_t buffer_size = 8;
    uint8_t buf[buffer_size];
    buf[0] = DS3231_REG_TIME;
    buf[1] = dec_to_bcd(dt->sec);
    buf[2] = dec_to_bcd(dt->min);
    buf[3] = dec_to_bcd(dt->hour);
    buf[4] = dec_to_bcd(dt->dow);
    buf[5] = dec_to_bcd(dt->day);
    buf[6] = dec_to_bcd(dt->month);
    buf[7] = dec_to_bcd(dt->year);

    return (i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, buf, buffer_size, false) == buffer_size);
}

bool ds3231_read_time(ds3231_datetime_t *dt) {
    if (dt == NULL) {
        return false;
    }

    uint8_t read_buf[7];
    uint8_t start_reg = DS3231_REG_TIME;

    if (i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, &start_reg, 1, true) != 1) {
        return false;
    }
    if (i2c_read_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, read_buf, 7, false) != 7) {
        return false;
    }

    dt->sec   = bcd_to_dec(read_buf[0]);
    dt->min   = bcd_to_dec(read_buf[1]);
    dt->hour  = bcd_to_dec(read_buf[2]);
    dt->dow   = bcd_to_dec(read_buf[3]);
    dt->day   = bcd_to_dec(read_buf[4]);
    dt->month = bcd_to_dec(read_buf[5] & 0x7F); // Century bit is not used
    dt->year  = bcd_to_dec(read_buf[6]);

    return true;
}


// --- Alarm Functions ---

bool ds3231_set_alarm1(const ds3231_alarm_time_t *at, ds3231_alarm1_mode_t mode) {
    if (at == NULL) {
        return false;
    }

    uint8_t buf[5];
    buf[0] = DS3231_REG_ALARM1;
    buf[1] = dec_to_bcd(at->sec);
    buf[2] = dec_to_bcd(at->min);
    buf[3] = dec_to_bcd(at->hour);
    buf[4] = dec_to_bcd(at->day);

    // Set mask bits based on the selected mode
    // Note: Clearing a mask bit means that value is used in the comparison
    switch (mode) {
        case ALARM1_ONCE_PER_SECOND:
            buf[1] |= ALARM_MASK_BIT; buf[2] |= ALARM_MASK_BIT; buf[3] |= ALARM_MASK_BIT; buf[4] |= ALARM_MASK_BIT;
            break;
        case ALARM1_SECONDS_MATCH:
            buf[2] |= ALARM_MASK_BIT; buf[3] |= ALARM_MASK_BIT; buf[4] |= ALARM_MASK_BIT;
            break;
        case ALARM1_MINUTES_SECONDS_MATCH:
            buf[3] |= ALARM_MASK_BIT; buf[4] |= ALARM_MASK_BIT;
            break;
        case ALARM1_HOURS_MINUTES_SECONDS_MATCH:
            buf[4] |= ALARM_MASK_BIT;
            break;
        case ALARM1_DATE_H_M_S_MATCH:
            buf[4] &= ~ALARM_DYDT_BIT; // Match date
            break;
        case ALARM1_DAY_OF_WEEK_H_M_S_MATCH:
            buf[4] |= ALARM_DYDT_BIT; // Match day of week
            break;
    }

    // Write the 4 alarm registers in one transaction
    if (i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, buf, 5, false) != 5) {
        return false;
    }

    // Enable Alarm 1 Interrupt
    return s_update_register_bits(DS3231_REG_CONTROL, 0, CONTROL_A1IE_BIT | CONTROL_INTCN_BIT);
}

bool ds3231_set_alarm2(const ds3231_alarm_time_t *at, ds3231_alarm2_mode_t mode) {
    if (at == NULL) {
        return false;
    }

    uint8_t buf[4];
    buf[0] = DS3231_REG_ALARM2;
    buf[1] = dec_to_bcd(at->min);
    buf[2] = dec_to_bcd(at->hour);
    buf[3] = dec_to_bcd(at->day);
    
    switch (mode) {
        case ALARM2_ONCE_PER_MINUTE:
            buf[1] |= ALARM_MASK_BIT; buf[2] |= ALARM_MASK_BIT; buf[3] |= ALARM_MASK_BIT;
            break;
        case ALARM2_MINUTES_MATCH:
            buf[2] |= ALARM_MASK_BIT; buf[3] |= ALARM_MASK_BIT;
            break;
        case ALARM2_HOURS_MINUTES_MATCH:
            buf[3] |= ALARM_MASK_BIT;
            break;
        case ALARM2_DATE_H_M_MATCH:
            buf[3] &= ~ALARM_DYDT_BIT; // Match date
            break;
        case ALARM2_DAY_OF_WEEK_H_M_MATCH:
            buf[3] |= ALARM_DYDT_BIT; // Match day of week
            break;
    }

    if (i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, buf, 4, false) != 4) {
        return false;
    }

    // Enable Alarm 2 Interrupt
    return s_update_register_bits(DS3231_REG_CONTROL, 0, CONTROL_A2IE_BIT | CONTROL_INTCN_BIT);
}

bool ds3231_check_alarm_flag(ds3231_alarm_num_t alarm_num) {
    uint8_t status_reg;
    if (!s_read_register(DS3231_REG_STATUS, &status_reg)) {
        return false; // Communication error
    }

    uint8_t alarm_bit = (alarm_num == ALARM_1) ? STATUS_A1F_BIT : STATUS_A2F_BIT;
    return (status_reg & alarm_bit) != 0;
}

bool ds3231_clear_alarm_flag(ds3231_alarm_num_t alarm_num) {
    uint8_t alarm_bit = (alarm_num == ALARM_1) ? STATUS_A1F_BIT : STATUS_A2F_BIT;
    // Clearing the flag requires writing 0 to its bit position
    return s_update_register_bits(DS3231_REG_STATUS, alarm_bit, 0);
}

bool ds3231_disable_alarm(ds3231_alarm_num_t alarm_num) {
    uint8_t alarm_bit = (alarm_num == ALARM_1) ? CONTROL_A1IE_BIT : CONTROL_A2IE_BIT;
    // Disabling the alarm means clearing its interrupt enable bit
    return s_update_register_bits(DS3231_REG_CONTROL, alarm_bit, 0);
}


// --- Output Control Functions ---

bool ds3231_enable_32khz_output(bool enable) {
    uint8_t bits_to_set = enable ? STATUS_EN32KHZ_BIT : 0;
    return s_update_register_bits(DS3231_REG_STATUS, STATUS_EN32KHZ_BIT, bits_to_set);
}

bool ds3231_enable_sqw_output(ds3231_sqw_freq_t freq) {
    // To enable SQW, INTCN bit must be 0
    // The frequency is set by RS2 and RS1 bits
    uint8_t mask = CONTROL_INTCN_BIT | CONTROL_RS1_BIT | CONTROL_RS2_BIT;
    uint8_t bits = (freq << 3); // Frequency enum values match bit positions
    return s_update_register_bits(DS3231_REG_CONTROL, mask, bits);
}

bool ds3231_enable_interrupt_mode() {
    // To enable interrupt mode, INTCN bit must be 1
    return s_update_register_bits(DS3231_REG_CONTROL, 0, CONTROL_INTCN_BIT);
}