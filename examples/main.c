#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ds3231.h"
#include <string.h>
#include <time.h>

#define I2C_BAUDRATE (100 * 1000)
#define I2C_SCL 20
#define I2C_SDA 21

#define SET_TIME_ONCE 0
#define SET_ALARM_1 1
#define SET_ALARM_2 1

// IMPORTANT: Choose only one of the options below
#define ENABLE_INTERUPT 1
#define ENABLE_FREQ_1HZ 0
#define ENABLE_FREQ_1024HZ 0
#define ENABLE_FREQ_4096HZ 0
#define ENABLE_FREQ_8192HZ 0

// Adjust this if you want to set the time a few seconds into the future
// to account for compile and upload time
#define COMPILE_UPLOAD_TIME 6

int main()
{

	stdio_init_all();
	sleep_ms(1000);
	printf("DS3231 RTC Example\n\n");

	i2c_init(I2C_PORT, I2C_BAUDRATE);
	gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
	gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(I2C_SDA);
	gpio_pull_up(I2C_SCL);

#if SET_TIME_ONCE
	printf("Setting RTC time from PC compile time...\n");
	struct tm date_tm = {0};

	const char *compile_date = __DATE__;
	const char *compile_time = __TIME__;

	printf("Compile Date: %s\n", compile_date);
	printf("Compile Time: %s\n", compile_time);

	int year, month, day, hour, min, sec;
	char month_str[4];

	sscanf(compile_time, "%d:%d:%d", &hour, &min, &sec);
	sscanf(compile_date, "%s %d %d", month_str, &day, &year);

	if (strcmp(month_str, "Jan") == 0)
		month = 1;
	else if (strcmp(month_str, "Feb") == 0)
		month = 2;
	else if (strcmp(month_str, "Mar") == 0)
		month = 3;
	else if (strcmp(month_str, "Apr") == 0)
		month = 4;
	else if (strcmp(month_str, "May") == 0)
		month = 5;
	else if (strcmp(month_str, "Jun") == 0)
		month = 6;
	else if (strcmp(month_str, "Jul") == 0)
		month = 7;
	else if (strcmp(month_str, "Aug") == 0)
		month = 8;
	else if (strcmp(month_str, "Sep") == 0)
		month = 9;
	else if (strcmp(month_str, "Oct") == 0)
		month = 10;
	else if (strcmp(month_str, "Nov") == 0)
		month = 11;
	else if (strcmp(month_str, "Dec") == 0)
		month = 12;

	date_tm.tm_year = year - 1900;
	date_tm.tm_mon = month - 1;
	date_tm.tm_mday = day;

	DateTime dt_to_set = {
		.year = year % 100,
		.month = month,
		.day = day,
		.dow = date_tm.tm_wday,
		.hour = hour,
		.min = min + (sec + COMPILE_UPLOAD_TIME > 59 ? 1 : 0),
		.sec = sec + COMPILE_UPLOAD_TIME % 60};

	if (ds3231_set_time(&dt_to_set))
	{
		printf("Time set successfully!\n");
	}
	else
	{
		printf("ERROR: Failed to set time.\n");
	}
#endif

#if SET_ALARM_1

	AlarmTime alarm_time_1 = {
		.hour = 20,
		.min = 18,
		.sec = 45};

	printf("Setting Alarm 1: day: %d, hour: %d, minute: %d, second: %d\n", alarm_time_1.day, alarm_time_1.hour, alarm_time_1.min, alarm_time_1.sec);

	if (ds3231_set_alarm1(&alarm_time_1, ALARM1_MINUTES_SECONDS_MATCH))
	{
		printf("Alarm 1 configured.\n\n");
	}
	else
	{
		printf("ERROR: Failed to configure Alarm 1.\n\n");
	}

#endif

#if SET_ALARM_2

	AlarmTime alarm_time_2 = {
		.min = 16};

	printf("Setting Alarm 2: day: %d, hour: %d, minute: %d\n", alarm_time_2.day, alarm_time_2.hour, alarm_time_2.min);

	if (ds3231_set_alarm2(&alarm_time_2, ALARM2_MINUTES_MATCH))
	{
		printf("Alarm 2 configured.\n\n");
	}
	else
	{
		printf("ERROR: Failed to configure Alarm 2.\n\n");
	}

#endif

#if ENABLE_INTERUPT
	if (ds3231_enable_interrupt_mode())
	{
		printf("RTC configured for interrupt mode.\n");
	}
#elif ENABLE_FREQ_1HZ
	if (ds3231_enable_sqw_output(SQW_FREQ_1HZ))
	{
		printf("RTC configured for frequency 1HZ.\n");
	}
#elif ENABLE_FREQ_1024HZ
	if (ds3231_enable_sqw_output(SQW_FREQ_1024HZ))
	{
		printf("RTC configured for frequency 1024HZ.\n", SQW_FREQ_1024HZ);
	}
#elif ENABLE_FREQ_4096HZ
	if (ds3231_enable_sqw_output(SQW_FREQ_4096HZ))
	{
		printf("RTC configured for frequency 4096HZ.\n");
	}
#elif ENABLE_FREQ_8192HZ
	if (ds3231_enable_sqw_output(SQW_FREQ_8192HZ))
	{
		printf("RTC configured for frequency 8192HZ.\n");
	}
#endif

	DateTime now;
	while (true)
	{
		if (ds3231_read_time(&now))
		{
			printf("Current Time: 20%02d-%02d-%02d %02d:%02d:%02d (Day %d)\n",
				   now.year, now.month, now.day, now.hour, now.min, now.sec, now.dow);
		}
		else
		{
			printf("ERROR: Failed to read time from RTC.\n");
		}

		if (ds3231_check_alarm_flag(ALARM_1))
		{
			printf("<<<<< ALARM 1 TRIGGERED! >>>>>\n");

			ds3231_clear_alarm_flag(ALARM_1);

			if (ds3231_set_alarm1(&alarm_time_1, ALARM1_MINUTES_SECONDS_MATCH))
			{
				printf("Alarm 1 configured.\n\n");
			}
			else
			{
				printf("ERROR: Failed to configure Alarm 1.\n\n");
			}
		}

		if (ds3231_check_alarm_flag(ALARM_2))
		{
			printf("<<<<< ALARM 2 TRIGGERED! >>>>>\n");

			ds3231_clear_alarm_flag(ALARM_2);
		}

		sleep_ms(1000);
	}

	return 0;
}