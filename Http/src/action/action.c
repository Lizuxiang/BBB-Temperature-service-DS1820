/*
 * action.c
 *
 *  Created on: 22-12-2013
 *      Author: kaczanowsky
 */
#include "action.h"
#include "../sqlite/temp.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* getTempAction(sqlite3* db, timeFormat format, map_t *postParams){
	size_t sz, beforeBrackets;
	char* responseText;
	char* temp;
	char* responseTextFinal;

	const char* baseString = "{\"timestamp\" : %d, \"temperature\" : %.02f, \"sensor\" : %d},";
	int resCount, i;
	int count = atoi(map_get(postParams,"count"));
	int sensor = atoi(map_get(postParams,"sensor"));
	dbTempRow* temps = getTemp(db, sensor, count, &resCount);

	// Calculate the size
	sz = snprintf(NULL, 0, baseString, temps[0].timestamp, temps[0].temperature, temps[0].sensor);
	responseText = malloc((sz+1)*resCount); /* +1 for zero terminator */
	temp = malloc(sz+1);

	// Concatenate the result
	for(i = 0; i < resCount; i++){
		snprintf(temp, sz+1, baseString, temps[i].timestamp, temps[i].temperature, temps[i].sensor);

		if(i == 0){
			strcpy(responseText, temp);
		}else{
			strcat(responseText, temp);
		}
	}

	// Get rid of last comma
	responseText[strlen(responseText)-1] = 0;

	beforeBrackets = snprintf(NULL, 0, "[%s]", responseText);
	responseTextFinal = malloc(beforeBrackets+1);
	snprintf(responseTextFinal, beforeBrackets+1, "[%s]", responseText);

	free(responseText);
	free(temp);
	free(temps);

	return responseTextFinal;
}

char* getSettingsAction(sqlite3* db, struct user_options *userOpts, map_t *data){
	  char* responseText;

	  if (strcmp(map_get(data,"type"), "all") == 0){
		  asprintf (&responseText, "{\"register\":%d, \"sensor1\":%d,\"sensor2\":%d, \"temperatureAlarmMin\":%d, \"temperatureAlarmMax\":%d, \"NUM_SECONDS\": %d, \"notifyPerioidMinutes\": %d}",
				  userOpts->NUM_SECONDS,
				  userOpts->sensor1Enabled,
				  userOpts->sensor2Enabled,
				  userOpts->temperatureAlarmMin,
				  userOpts->temperatureAlarmMax,
				  userOpts->NUM_SECONDS,
				  userOpts->notifyPerioidMinutes

		  );
	  }else if(strcmp(map_get(data,"type"), "sensors") == 0){
		  asprintf (&responseText, "{\"sensor1\":%d,\"sensor2\":%d}", userOpts->sensor1Enabled, userOpts->sensor2Enabled);
	  }

	  return responseText;
}

char* changeSettingsAction(sqlite3* db, struct user_options *userOpts, map_t *data){
	userOpts->sensor1Enabled = atoi(map_get(data,"sensor1Enabled"));
	userOpts->sensor2Enabled = atoi(map_get(data,"sensor2Enabled"));
	userOpts->temperatureAlarmMin = atoi(map_get(data,"temperatureAlarmMin"));
	userOpts->temperatureAlarmMax = atoi(map_get(data,"temperatureAlarmMax"));
	userOpts->NUM_SECONDS = atoi(map_get(data,"NUM_SECONDS"));
	userOpts->notifyPerioidMinutes = atoi(map_get(data,"notifyPerioidMinutes"));

	char *responseText;
	asprintf(&responseText, "{\"result\": 1 }");

	return responseText;
}
