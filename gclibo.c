/** @addtogroup cpp_api
  * @{
  */
  
/*! \file gclibo.c
*
* Partial implementation of gclibo.h
*
*/

#include "gclibo.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS //use traditional C calls like strncpy()
#endif

#include <stdlib.h> //atoi, atof
#include <string.h> //strcpy
#include <stdio.h> //fopen
#include <math.h> //log()
#include <time.h>


void GCALL GSleep(unsigned int timeout_ms)
{
	GUtility(0, G_UTIL_SLEEP, &timeout_ms, 0);
}

GReturn GCALL GVersion(GCStringOut ver, GSize ver_len)
{
	int str_len;
	GReturn rc;
	if ((rc = GUtility(0, G_UTIL_VERSION, ver, &ver_len)) != G_NO_ERROR)
		return rc;

#ifdef G_USE_GCAPS
	str_len = strlen(ver) + 1;
	ver_len -= str_len;
	if (ver_len > 0 
		&& GUtility(0, G_UTIL_GCAPS_VERSION, ver + str_len, &ver_len) == G_NO_ERROR)
	{
		ver[str_len - 1] = ' '; //add a delimiter
	}
#endif 

	return rc;
}

GReturn GCALL GInfo(GCon g, GCStringOut info, GSize info_len)
{
	return GUtility(g, G_UTIL_INFO, info, &info_len);
}

GReturn GCALL GAddresses(GCStringOut addresses, GSize addresses_len)
{
#ifdef G_USE_GCAPS
	GReturn rc;
	if (G_NO_ERROR == (rc = GUtility(0, G_UTIL_GCAPS_ADDRESSES, addresses, &addresses_len)))
		return rc;		
#endif

	return GUtility(0, G_UTIL_ADDRESSES, addresses, &addresses_len);		
}

GReturn GCALL GTimeout(GCon g, short timeout_ms)
{
	return GUtility(g, G_UTIL_TIMEOUT_OVERRIDE, &timeout_ms, 0);
}

GReturn GCALL GAssign(GCStringIn ip, GCStringIn mac)
{
	/*
	*  On Linux and Apple, the IP address is pinged prior to assigning.
	*  On Windows, pinging first can make the arp table stale, and the
	*  IP address unreachable for several seconds. We skip ping so that
	*  we can immediately connect.
	*/

	GReturn rc;
	int reply = 0; //ping reply is nonzero

#ifdef G_USE_GCAPS

#if defined(__linux__) || defined(__APPLE__)
	if (strlen(ip)) //can use null string to remove IP assignment from gcaps DHCP feature
	{
		GUtility(0, G_UTIL_GCAPS_PING, (void*)ip, &reply); //ping to see if IP address is already taken
		if (reply)
			return G_GCLIB_UTILITY_IP_TAKEN;
	}
#endif

	if (G_NO_ERROR == (rc = GUtility(0, G_UTIL_GCAPS_ASSIGN, (void*)ip, (void*)mac)))
		return rc;
#endif

#if defined(__linux__) || defined(__APPLE__)
	GUtility(0, G_UTIL_PING, (void*)ip, &reply);  //ping to see if IP address is already taken
	if (reply)
		return G_GCLIB_UTILITY_IP_TAKEN;
#endif
	
	return GUtility(0, G_UTIL_ASSIGN, (void*)ip, (void*)mac);
}

GReturn GCALL GIpRequests(GCStringOut requests, GSize requests_len)
{
	GReturn rc;
#ifdef G_USE_GCAPS
	rc = GUtility(0, G_UTIL_GCAPS_IPREQUEST, requests, &requests_len);
	if (rc == G_NO_ERROR)
	{
		if (requests_len == 0 || strlen(requests))
			return rc;
		else
		{
			//try once more after a 5 second wait
			GSleep(5000);
			return GUtility(0, G_UTIL_GCAPS_IPREQUEST, requests, &requests_len);
		}
	}
#endif

	//non-gcaps version opens port listens for 5 seconds, and closes
	return GUtility(0, G_UTIL_IPREQUEST, requests, &requests_len);
}

GReturn GCALL GSetServer(GCStringIn server_name)
{
	GReturn rc;
#ifdef G_USE_GCAPS
	rc = GUtility(0, G_UTIL_GCAPS_SET_SERVER, (void*)server_name, 0);
#else
	rc = G_GCAPS_OPEN_ERROR;
#endif

#ifdef GCLIB_LOGGING
	if (rc != G_NO_ERROR)
	{
		char buf[256];
		sprintf(buf, "GSetServer: %d", rc);
		LogMsg(buf);
	}
#endif

	return rc;
}

GReturn GCALL GServerStatus(GCStringOut status, GSize status_len)
{
	GReturn rc = G_NO_ERROR;
#ifdef G_USE_GCAPS
	rc = GUtility(0, G_UTIL_GCAPS_SERVER_STATUS, status, &status_len);
#else
	rc = G_GCAPS_OPEN_ERROR;
#endif 

#ifdef GCLIB_LOGGING
	if (rc != G_NO_ERROR)
	{
		char buf[256];
		sprintf(buf, "GServerStatus: %d", rc);
		LogMsg(buf);
	}
#endif
	return rc;
}

GReturn GCALL GListServers(GCStringOut servers, GSize servers_len)
{
	GReturn rc;
#ifdef G_USE_GCAPS
	rc = GUtility(0, G_UTIL_GCAPS_LIST_SERVERS, servers, &servers_len);
#else
	rc = G_GCAPS_OPEN_ERROR;
#endif

#ifdef GCLIB_LOGGING
	if (rc != G_NO_ERROR)
	{
		char buf[256];
		sprintf(buf, "GListServers: %d", rc);
		LogMsg(buf);
	}
#endif
	return rc;
}

GReturn GCALL GPublishServer(GCStringIn name, GOption publish, GOption save)
{
	GReturn rc;
#ifdef  G_USE_GCAPS
	//bit 0 -> publish = 1, remove = 0
	//bit 1 -> save = 1, do not save = 0
    unsigned short options = publish | (save << 1);

	if (G_NO_ERROR == (rc = GUtility(0, G_UTIL_GCAPS_PUBLISH_SERVER, (void*)name, &options)))
	{

	}
#else
	rc = G_GCAPS_OPEN_ERROR;
#endif

#ifdef GCLIB_LOGGING
	if (rc != G_NO_ERROR)
	{
		char buf[256];
		sprintf(buf, "GPublishServer: %d", rc);
		LogMsg(buf);
	}
#endif

	return rc;
}

GReturn GCALL GRemoteConnections(GCStringOut connections, GSize connections_length)
{
	GReturn rc;
#ifdef G_USE_GCAPS
	rc = GUtility(0, G_UTIL_GCAPS_REMOTE_CONNECTIONS, connections, &connections_length);
#else
	rc = G_GCAPS_OPEN_ERROR;
#endif

#ifdef GCLIB_LOGGING
	if (rc != G_NO_ERROR)
	{
		char buf[256];
		sprintf(buf, "GRemoteConnections: %d", rc);
		LogMsg(buf);
	}
#endif
	return rc;
}

GReturn GCALL GCmd(GCon g, GCStringIn command)
{
	char buf[G_SMALL_BUFFER]; //response usually brief, e.g. :
	return GCommand(g, command, buf, G_SMALL_BUFFER, 0);
}

GReturn GCALL GCmdT(GCon g, GCStringIn command, GCStringOut trimmed_response, GSize response_len, GCStringOut* front)
{
	GSize read;
	GReturn rc;
	int i;
	char c;
	if ((rc = GCommand(g, command, trimmed_response, response_len, &read)) != G_NO_ERROR)
		return rc;
	//if here, the data is already null-terminated, just trim.
	for (i = read - 1; i >= 0; i--) //read does NOT include null terminator.
	{
		c = trimmed_response[i];
		if ((c == ':') || (c == '\n') || (c == '\r'))
			trimmed_response[i] = 0; //trim it
		else
			break; //we hit non-trimmable data, bail out.
	}

	if (front) //null to skip "trim" on front.
	{
		*front = trimmed_response;
		i = 0;
		do
		{
			c = trimmed_response[i++];
			if (c == ' ')
				(*front)++;
			else
				break;
		} while (1); //exit will be any non-space, including null terminator
	}

	return G_NO_ERROR;
}

GReturn GCALL GCmdI(GCon g, GCStringIn command, int* value)
{
	char buf[G_SMALL_BUFFER]; //response should be ~19 chars
	GSize read;
	GReturn rc;
	if ((rc = GCommand(g, command, buf, G_SMALL_BUFFER, &read)) != G_NO_ERROR)
		return rc;
	*value = atoi(buf);
	return G_NO_ERROR;
}

GReturn GCALL GCmdD(GCon g, GCStringIn command, double* value)
{
	char buf[G_SMALL_BUFFER]; //response should be ~19 chars
	GSize read;
	GReturn rc;
	if ((rc = GCommand(g, command, buf, G_SMALL_BUFFER, &read)) != G_NO_ERROR)
		return rc;
	*value = atof(buf);
	return G_NO_ERROR;
}

GReturn GCALL GMotionComplete(GCon g, GCStringIn axes)
{
	char pred[] = "_BGm=0"; //predicate for polling the axis' motion status, m is a place holder replaced below.
	GReturn rc;
	GSize i = 0; //C, not C++
	GSize len = strlen(axes);

	for (i = 0; i < len; i++) //iterate through all chars in axes
	{
		pred[3] = axes[i]; //set the axis
		rc = GWaitForBool(g, pred, -1); //poll forever. Change this if a premature exit is desired.
		if (rc != G_NO_ERROR)
			return rc;
	}//axes

	return G_NO_ERROR;
}

GReturn GCALL GWaitForBool(GCon g, GCStringIn predicate, int trials)
{
	char cmd[G_LINE_BUFFER];
	char buf[G_LINE_BUFFER];
	int rc;
	strcpy(cmd, "MG (");
	strcat(cmd, predicate);
	strcat(cmd, ")"); //enclose in parenthesis

	for (; trials != 0; --trials) //negative value will poll "forever"
	{
		rc = GCommand(g, cmd, buf, G_SMALL_BUFFER, 0); //check the predicate
		if (rc != G_NO_ERROR)
			return rc;

		if (atoi(buf)) //nonzero is true, bad atoi returns 0
			return G_NO_ERROR;
		else
			GSleep(POLLINGINTERVAL);
	}
	//if we're here, the trials ran out
	return G_GCLIB_POLLING_FAILED;
}

GReturn GCALL GRecordRate(GCon g, double period_ms)
{
	char buf[G_SMALL_BUFFER];
	double dt;
	double period_arg;

	if (period_ms == 0) //turn off
		return GCmd(g, "DR 0");

	if (GCmdD(g, "TM?", &dt) == G_NO_ERROR)
	{
		dt /= 1024.0; //ms per controller sample
		if (!dt) dt = 1; //don't want to divide by zero below
	}
	else
	{
		dt = 0.9765625; //RIO doesn't have TM
	}

	period_arg = period_ms / dt; //data record specified in samples between records

	if (GCmdT(g, "\x12\x16", buf, sizeof(buf), 0) == G_NO_ERROR) //Revision string, ^R^V
	{
		if (strstr(buf, "DMC18")) //PCI controller
			period_arg = log(period_arg) / log(2.0); //PCI DR arg is 2^n.
		else if ((strstr(buf, "DMC40") != NULL) //4000
			|| (strstr(buf, "DMC500") != NULL) //50000
			|| (strstr(buf, "RIO") != NULL)) // RIO
		{
			if (period_arg < 2) period_arg = 2; //lowest non-zero DR
		}
		else if ((strstr(buf, "DMC41") != NULL) || (strstr(buf, "DMC21") != NULL)) //4103, 2103
		{
			if (period_arg < 8) period_arg = 8; //lowest non-zero DR
		}
		else if ((strstr(buf, "DMC3") != NULL)) //30010, 31010
		{
			if (period_arg < 4) period_arg = 4; //lowest non-zero DR
		}
	}
	
	sprintf(buf, "DR %d", (int)period_arg);
	return GCmd(g, buf);
}

GReturn GCALL GProgramDownloadFile(GCon g, GCStringIn file_path, GCStringIn preprocessor)
{
	FILE *file;
	long file_size;
	char* program_buffer;
	GReturn rc = G_NO_ERROR;

	if (!(file = fopen(file_path, "rb"))) //open file for reading, binary mode
		return G_BAD_FILE;

	fseek(file, 0, SEEK_END); //find end of file
	file_size = ftell(file); //add one to null terminate below
	rewind(file);

	if (file_size) //don't malloc 0.
	{

		if (!(program_buffer = malloc(file_size + 1))) //allocate memory for the data, +1 for null termination below
		{
			fclose(file);
			return G_BAD_FULL_MEMORY;
		}

		if (file_size != fread(program_buffer, 1, file_size, file))
		{
			fclose(file);
			free(program_buffer); //free memory
			return G_BAD_FILE;
		}
		program_buffer[file_size] = 0; //null terminate, malloc was one byte larger for this
	}
	else
	{
		program_buffer = ""; //nullstring
	}

	fclose(file); //done with file, close it

	rc = GProgramDownload(g, program_buffer, preprocessor); //call the gclib downloader
	if (file_size) free(program_buffer); //free memory
	return rc;
}

GReturn GCALL GProgramUploadFile(GCon g, GCStringIn file_path)
{
	FILE *file;
	GReturn rc = G_NO_ERROR;
	char* program_buffer;
	long file_size;

	if (!(file = fopen(file_path, "wb"))) //open file for writing, binary mode
		return G_BAD_FILE;

	if (!(program_buffer = malloc(MAXPROG))) //allocate memory for the data
	{
		fclose(file);
		return G_BAD_FULL_MEMORY;
	}

	if ((rc = GProgramUpload(g, program_buffer, MAXPROG)) == G_NO_ERROR)
	{
		file_size = strlen(program_buffer);
		if (file_size != fwrite(program_buffer, 1, file_size, file))
			rc = G_BAD_FILE;
	}

	fclose(file);
	free(program_buffer);
	return rc;
}


void GCALL GError(GReturn rc, GCStringOut error, GSize error_len)
{
	char* error_message;

	switch (rc)
	{
	case G_NO_ERROR:
		error_message = G_NO_ERROR_S;
		break;

	case G_GCLIB_ERROR:
		error_message = G_GCLIB_ERROR_S;
		break;

	case G_GCLIB_UTILITY_ERROR:
		error_message = G_GCLIB_UTILITY_ERROR_S;
		break;

	case G_GCLIB_UTILITY_IP_TAKEN:
		error_message = G_GCLIB_UTILITY_IP_TAKEN_S;
		break;

	case G_GCLIB_NON_BLOCKING_READ_EMPTY:
		error_message = G_GCLIB_NON_BLOCKING_READ_EMPTY_S;
		break;

	case G_TIMEOUT:
		error_message = G_TIMEOUT_S;
		break;

	case G_OPEN_ERROR:
		error_message = G_OPEN_ERROR_S;
		break;

	case G_READ_ERROR:
		error_message = G_READ_ERROR_S;
		break;

	case G_WRITE_ERROR:
		error_message = G_WRITE_ERROR_S;
		break;

	case G_COMMAND_CALLED_WITH_ILLEGAL_COMMAND:
		error_message = G_COMMAND_CALLED_WITH_ILLEGAL_COMMAND_S;
		break;

	case G_DATA_RECORD_ERROR:
		error_message = G_DATA_RECORD_ERROR_S;
		break;

	case G_UNSUPPORTED_FUNCTION:
		error_message = G_UNSUPPORTED_FUNCTION_S;
		break;

	case G_BAD_ADDRESS:
		error_message = G_BAD_ADDRESS_S;
		break;

	case G_BAD_FIRMWARE_LOAD:
		error_message = G_BAD_FIRMWARE_LOAD_S;
		break;

	case G_FIRMWARE_LOAD_NOT_SUPPORTED:
		error_message = G_FIRMWARE_LOAD_NOT_SUPPORTED_S;
		break;

	case G_ARRAY_NOT_DIMENSIONED:
		error_message = G_ARRAY_NOT_DIMENSIONED_S;
		break;

	case G_CONNECTION_NOT_ESTABLISHED:
		error_message = G_CONNECTION_NOT_ESTABLISHED_S;
		break;

	case G_ILLEGAL_DATA_IN_PROGRAM:
		error_message = G_ILLEGAL_DATA_IN_PROGRAM_S;
		break;

	case G_UNABLE_TO_COMPRESS_PROGRAM_TO_FIT:
		error_message = G_UNABLE_TO_COMPRESS_PROGRAM_TO_FIT_S;
		break;

	case G_INVALID_PREPROCESSOR_OPTIONS:
		error_message = G_INVALID_PREPROCESSOR_OPTIONS_S;
		break;

	case G_BAD_RESPONSE_QUESTION_MARK:
		error_message = G_BAD_RESPONSE_QUESTION_MARK_S;
		break;

	case G_BAD_VALUE_RANGE:
		error_message = G_BAD_VALUE_RANGE_S;
		break;

	case G_BAD_FULL_MEMORY:
		error_message = G_BAD_FULL_MEMORY_S;
		break;

	case G_BAD_LOST_DATA:
		error_message = G_BAD_LOST_DATA_S;
		break;

	case G_BAD_FILE:
		error_message = G_BAD_FILE_S;
		break;

	case G_GCAPS_OPEN_ERROR:
		error_message = G_GCAPS_OPEN_ERROR_S;
		break;

	case G_GCAPS_SUBSCRIPTION_ERROR:
		error_message = G_GCAPS_SUBSCRIPTION_ERROR_S;
		break;

	default:
		error_message = "internal error";
		break;
	}

	strncpy(error, error_message, error_len);
	error[error_len - 1] = 0; //ensure null termination
}

#ifdef GCLIB_LOGGING
void LogMsg(const char* msg)
{
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char* time_string = asctime(timeinfo);	

#ifdef _WIN32
	const char* filename = "C:/ProgramData/Galil/gcaps/gclib_mdns_log.txt";
#else
	const char* filename = "/var/tmp/gclib_mdns_log.txt";
#endif // _WIN32

	FILE *logfile;
	logfile = fopen(filename, "a");
	fprintf(logfile, "%s | %s", msg, time_string);
	fclose(logfile);
}
#endif

/** @}*/
