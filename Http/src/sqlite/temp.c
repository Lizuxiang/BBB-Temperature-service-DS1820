/*
 * temp.c
 *
 *  Created on: 25-12-2013
 *      Author: kaczanowsky
 */
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "temp.h"

void create_table(sqlite3 *db){
	int rc = 0, row = 0;
	char *zErrMsg = 0;
	const char *sql;
	const char *tail;
	sqlite3_stmt *res;

	//rc = sqlite3_prepare_v2(db, "SELECT CASE WHEN tbl_name = 'TEMPERATURE_HISTORY' THEN 1 ELSE 0 END FROM sqlitemaster WHERE tbl_name = 'TEMPERATURE_HISTORY' AND type = 'table'",1000, &res, &tail);
	rc = sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='TEMPERATURE_HISTORY';",1000, &res, &tail);
	row = sqlite3_step(res);

	// If table exists
	if(row == SQLITE_DONE){
		/* Create SQL statement */
		sql = "CREATE TABLE TEMPERATURE_HISTORY("  \
			  "TIMESTAMP   DATETIME DEFAULT CURRENT_TIMESTAMP," \
			  "TEMPERATURE REAL NOT NULL DEFAULT 0," \
			  "SENSOR INTEGER NOT NULL DEFAULT 1," \
			  "PRIMARY KEY (TIMESTAMP, SENSOR)" \
		      ");";

		/* Execute SQL statement */
		rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			sqlite3_close(db);
			exit(0);
		}
	}
}

int addTemp(sqlite3 *db, int timestamp, float val, int sensor){
	int rc;
	char* sql;
	char *zErrMsg = 0;
	const char* query = "INSERT INTO TEMPERATURE_HISTORY (TIMESTAMP,TEMPERATURE, SENSOR) VALUES (%d, %f, %d);";

	int queryLen = snprintf(NULL, 0, query, timestamp, val, sensor);
	sql = malloc(queryLen+1);

	sprintf(sql, query, timestamp, val, sensor);

	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
	if( rc != SQLITE_OK ){
		sqlite3_free(zErrMsg);
		free(sql);
		return 0;
	}

	free(sql);
	return 1;
}

dbTempRow* getTemp(sqlite3 *db, int sensor, int limit, int* resCount){
	int row = 0, i;
	char* sql;
	const char *tail;
	dbTempRow* result;
	sqlite3_stmt *res;
	*resCount = 0;

	const char* query = "SELECT * FROM TEMPERATURE_HISTORY WHERE SENSOR = %d ORDER BY `TIMESTAMP` DESC LIMIT %d;";
	int queryLen = snprintf(NULL, 0, query, sensor, limit);

	sql = malloc((sizeof(char) * queryLen)+1);
	result = malloc(sizeof(dbTempRow) * limit);

	sprintf(sql, query, sensor, limit);

	sqlite3_prepare_v2(db, sql, 1000, &res, &tail);

	for(i = 0; i < limit; i++){
		row = sqlite3_step(res);
		if(row == SQLITE_ROW){
			result[i].timestamp = sqlite3_column_int(res, 0);
			result[i].temperature = sqlite3_column_double(res, 1);
			result[i].sensor = sqlite3_column_int(res, 2);
			*resCount = *resCount + 1;
		}else{
			break;
		}
	}

	sqlite3_finalize(res);
	free(sql);

	return result;
}
