/*
 * temp.h
 *
 *  Created on: 25-12-2013
 *      Author: kaczanowsky
 */

#ifndef TEMP_H_
#define TEMP_H_

typedef struct{
	int timestamp;
	float temperature;
	unsigned int sensor;
} dbTempRow;

void create_table(sqlite3 *db);
int addTemp(sqlite3 *db, int timestamp, float val, int sensorq);
dbTempRow* getTemp(sqlite3 *db, int sensor, int count, int* resCount);

#endif /* TEMP_H_ */
