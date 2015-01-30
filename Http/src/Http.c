#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <microhttpd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <pthread.h>
#include "map/map_lib.h"
#include "action/action.h"
#include "sqlite/temp.h"

#define PORT 8888
#define POST_BUFFER_SIZE  512
#define MAX_NAME_SIZE     20

#define MAX_ANSWER_SIZE   512
#define HTTP_ROOT_DIR "/home/root/Workspace/HttpTempFiles/src/development/"

#define GET 0
#define POST 1

static sqlite3* db;
const char *errorpage = "<html><body>This doesn't seem to be right.</body></html>";

// User options
struct user_options userOpts;

struct connection_info_struct {
	int connectionType;
	struct map_t *data;
	struct MHD_PostProcessor *postProcessor;
};

/********************* MIMETYPE ************************/
const char *get_filename_ext(const char *filename) {
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename) {
		return "";
	}

	return dot + 1;
}

static const char* mimeTypes[] = { "text/html", "text/css", "image/png", "text/javascript", "application/json" };

const char* getMimeType(const char* url) {
	const char *ext = get_filename_ext(url);
	const char *mimetype;

	if (strcmp(ext, "html") == 0) {
		mimetype = strdup(mimeTypes[0]);
	} else if (strcmp(ext, "css") == 0) {
		mimetype = strdup(mimeTypes[1]);
	} else if (strcmp(ext, "png") == 0) {
		mimetype = strdup(mimeTypes[2]);
	} else if (strcmp(ext, "js") == 0) {
		mimetype = strdup(mimeTypes[3]);
	} else if (strcmp(ext, "json") == 0) {
		mimetype = strdup(mimeTypes[4]);
	} else {
		mimetype = strdup(mimeTypes[0]);
	}

	return mimetype;
}

/***************************END MIMETYPE*************************************/
int handlePOST(const char *url, struct MHD_Connection *connection, map_t *data) {
	int ret;
	struct MHD_Response *response;

//		struct map_t *curr = data;
//		while(curr != NULL){
//			printf("%s : %s\n", curr->name, curr->value);
//			curr = curr->nxt;
//		}

	char* responseText= NULL;
	if(strcmp(url, "/getTemp") == 0){
		responseText = getTempAction(db, FARENHEIT, data);
	}else if(strcmp(url, "/setSettings") == 0){
		responseText = changeSettingsAction(db, &userOpts, data);
	}else if(strcmp(url, "/getSettings") == 0){
		responseText = getSettingsAction(db, &userOpts, data);
	}

	response = MHD_create_response_from_buffer(strlen(responseText), (void *) responseText, MHD_RESPMEM_MUST_FREE);

	if (!response) {
		return MHD_NO;
	}

	MHD_add_response_header(response, "Content-Type", mimeTypes[4]);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

	// Free memory
	MHD_destroy_response(response);

	return ret;
}

int handleGET(const char *url, struct MHD_Connection *connection) {
	struct MHD_Response *response;
	int fd;
	int ret;
	struct stat sbuf;
	char path[255];

	const char* mimetype = getMimeType(url);
	sprintf(path, HTTP_ROOT_DIR ".%s", url);

	if ((-1 == (fd = open(path, O_RDONLY))) || (0 != fstat(fd, &sbuf))) {

		/* error accessing file */
		if (fd != -1){
			close(fd);
		}

		const char *errorstr = "<html><body>An internal server error has occured!</body></html>";
		response = MHD_create_response_from_buffer(strlen(errorstr), (void *) errorstr, MHD_RESPMEM_PERSISTENT);

		if (response) {
			ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
			MHD_destroy_response(response);

			return MHD_YES;
		} else{
			return MHD_NO;
		}
	}

	response = MHD_create_response_from_fd_at_offset(sbuf.st_size, fd, 0);

	MHD_add_response_header(response, "Content-Type", mimetype);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

	MHD_destroy_response(response);

	return ret;
}

int handleError(const char *url, struct MHD_Connection *connection) {
	int ret;
	struct MHD_Response *response;

	response = MHD_create_response_from_buffer(strlen("ERROR"), (void *) "ERROR", MHD_RESPMEM_PERSISTENT);
	if (!response) {
		return MHD_NO;
	}

	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

static int iteratePost(void *coninfo_cls, enum MHD_ValueKind kind, const char *key, const char *filename,
		const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size) {

	struct connection_info_struct *con_info = coninfo_cls;

	if ((size > 0) && (size <= MAX_NAME_SIZE)) {
		map_set(con_info->data, key, data);
	}

	return MHD_YES;
}

static void requestCompleted(void *cls, struct MHD_Connection *connection, void **con_cls,
		enum MHD_RequestTerminationCode toe) {
	struct connection_info_struct *con_info = *con_cls;

	if (NULL == con_info)
		return;

	if (con_info->connectionType == POST) {
		MHD_destroy_post_processor(con_info->postProcessor);

		if (con_info->data) {
			free(con_info->data);
		}
	}

	free(con_info);
	*con_cls = NULL;
}

static int answerToConnection(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
		const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {

	if (NULL == *con_cls) {
		struct connection_info_struct *con_info;

		con_info = malloc(sizeof(struct connection_info_struct));
		if (NULL == con_info){
			return MHD_NO;
		}

		con_info->data = map_create();

		if (0 == strcmp(method, "POST")) {
			con_info->postProcessor = MHD_create_post_processor(connection, POST_BUFFER_SIZE, iteratePost,
					(void *) con_info);

			if (NULL == con_info->postProcessor) {
				free(con_info);
				return MHD_NO;
			}

			con_info->connectionType = POST;
		} else {
			con_info->connectionType = GET;
		}

		*con_cls = (void *) con_info;

		return MHD_YES;
	}

	if (0 == strcmp(method, "GET")) {
		return handleGET(url, connection);
	}

	if (0 == strcmp(method, "POST")) {
		struct connection_info_struct *con_info = *con_cls;

		if (*upload_data_size != 0) {
			MHD_post_process(con_info->postProcessor, upload_data, *upload_data_size);
			*upload_data_size = 0;

			return MHD_YES;
		} else if (NULL != con_info->data ) {
			return handlePOST(url, connection, con_info->data);
		}
	}

	return handleError(url, connection);
}

float randomValue(int min, int max){
   return min + (float)rand() / (RAND_MAX / (max - min + 1) + 1);
}

float getTempFromSensor(int sensor){
	int d = open((sensor == 1)? "/sys/bus/w1/devices/28-0000035f5e27/w1_slave" : "/sys/bus/w1/devices/28-000004b58c5d/w1_slave", O_RDONLY);
	char dataT1[512];
	char *token;

	read(d, dataT1, 512);
	token = strtok (dataT1,"t\=");
	while (token != NULL){
		if(strtok (NULL, "t\=") != NULL){
			token = strtok (NULL, "t\=");
		}else{
			break;
		}
	}

	return (float)atoi(token) / 1000;
}

void* readTempLoop(void* param){
	sqlite3 *db = (sqlite3*) param;
	float sensor1Temp, sensor2Temp;
	int currTime;
	int lastTimeSent = NULL;
	int res;
	char *mailText;

	while(1){
		sleep(userOpts.NUM_SECONDS);
		currTime = (int) time(NULL);
//		sensor1Temp =(float) randomValue(18, 23);
//		sensor2Temp =(float) randomValue(18, 23);
		sensor1Temp = getTempFromSensor(1);
		sensor2Temp = getTempFromSensor(2);

		if(userOpts.sensor1Enabled){
			res = addTemp(db, currTime, sensor1Temp, 1);
		}

		if(userOpts.sensor2Enabled){
			res = addTemp(db, currTime, sensor2Temp, 2);
		}

		if(sensor1Temp >= userOpts.temperatureAlarmMax || sensor2Temp >= userOpts.temperatureAlarmMax
				|| sensor1Temp <= userOpts.temperatureAlarmMin || sensor2Temp <= userOpts.temperatureAlarmMin){

			if(currTime - lastTimeSent >= userOpts.notifyPerioidMinutes*60){
				//send mail here
				asprintf(&mailText, "echo -e 'Subject: Temperature notification\r\n\r\nDear Sir,\nOne of The sensors detected the temperature out of the bound!\n\n Please take a look!\n Sensor 1: %.02f\n Sensor 2: %.02f'| msmtp --from=default -t kaczanowski.mateusz@gmail.com &", sensor1Temp, sensor2Temp);

				system(mailText);
				free(mailText);

				lastTimeSent = currTime;
			}
		}
	}

	return 0;
}

int main() {
	struct MHD_Daemon *daemon;
	int rc, iret;
	pthread_t loopThread;

	userOpts.NUM_SECONDS = 10;
	userOpts.sensor1Enabled = 1;
	userOpts.sensor2Enabled = 1;
	userOpts.temperatureAlarmMax = 17;
	userOpts.temperatureAlarmMin = -5;
	userOpts.notifyPerioidMinutes = 5;

	daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &answerToConnection, NULL,
			MHD_OPTION_NOTIFY_COMPLETED, requestCompleted, NULL, MHD_OPTION_END);

	if (NULL == daemon) {
		return 1;
	}

	/**********DB**************/
	/* Open database */
	rc = sqlite3_open("test.db", &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	}

	create_table(db);
	iret = pthread_create(&loopThread, NULL, readTempLoop, (void*) db);

	if(iret != 0){
		fprintf(stderr, "Can't create new thread (reading from device)");
	}

	getchar();

	sqlite3_close(db);
	MHD_stop_daemon(daemon);
	return 0;
}
