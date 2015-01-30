/*
 * action.h
 *
 *  Created on: 22-12-2013
 *      Author: kaczanowsky
 */

#ifndef ACTION_H_
#define ACTION_H_
#include "../map/map_lib.h"
#include <stdio.h>
#include <sqlite3.h>

typedef enum timeFormat {
	CELCIUS,
	FARENHEIT
} timeFormat;

struct user_options {
	int NUM_SECONDS;
	int sensor1Enabled;
	int sensor2Enabled;
	int temperatureAlarmMin;
	int temperatureAlarmMax;
	int notifyPerioidMinutes;
};

char* getTempAction(sqlite3* db, timeFormat format, map_t *data);
char* changeSettingsAction(sqlite3* db, struct user_options *userOpts, map_t *data);
char* getSettingsAction(sqlite3* db, struct user_options *userOpts, map_t *data);

#endif /* ACTION_H_ */
