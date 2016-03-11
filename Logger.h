#ifndef MULTIDNS_C_LOGGER_H
#define MULTIDNS_C_LOGGER_H

#include "list"
#include "time.h"
#include "stdio.h"
#include "iostream"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "string.h"

using std::list;

class Logger{
public:

	Logger(FILE *stream):logStream(stream) {}
	~Logger() {}
	static Logger *getFileLogger(char *filename) {
		FILE *stream = fopen(filename, "a+");
		return sys_logger = new Logger(stream);
	}
	static Logger *getStreamLogger() {
		return sys_logger = new Logger(stdout);
	}
	static Logger *getLogger() {
		if(sys_logger == NULL)
			return getStreamLogger();
		else
			return sys_logger;
	}
	/*
		request data
		2016/1/20 23:32 >> from ip:10.0.0.110 -> google.com
	*/
	int logRequest(sockaddr_in *ip, char* hostname, char* server) {
		char currentTimeStr[20];
		getTimeStr(currentTimeStr);
		char* ip_str = new char[20];
		strcpy(ip_str, inet_ntoa(ip->sin_addr));
		fprintf(logStream, "%s >> from %s:%d -> %s -> %s\n", currentTimeStr, ip_str, ip->sin_port, hostname, server);
	}
	/*
		response data
		2016/1/20 23:32 >> google.com -> 216.58.196.42 from 8.8.8.8 or cached
	*/
	int logResponse(char* hostname,list<char*> *iplist,char* server, bool cached) {
		char currentTimeStr[20];
		char* cacheString = "";
		getTimeStr(currentTimeStr);
		if (cached)
			cacheString = " with cached";
		fprintf(logStream, "%s >> %s -> ", currentTimeStr, hostname);
		fprintf(logStream,"[");
		if(iplist->size()>0)
			for (char* s : *iplist)
				fprintf(logStream, "%s,", s);
		fprintf(logStream, "]");
		fprintf(logStream, "%s\n",cacheString);
		fflush(logStream);
	}
	/*
		bash execute
		2016/1/20 23:32 >> google -> bash : route add -host 8.8.8.8 dev eth0 status 0
	*/
	int logBash(char* hostname, char* bash, int status) {
		char currentTimeStr[20];
		getTimeStr(currentTimeStr);
		fprintf(logStream, "%s >> %s -> bash : %s -> status %d\n", currentTimeStr, hostname, bash, status);
	}
	int logInfo(char* info){
		char currentTimeStr[20];
		getTimeStr(currentTimeStr);
		fprintf(logStream, "%s [info] %s\n", currentTimeStr, info);
		fflush(logStream);
	}
private:
	static Logger* sys_logger;
	FILE* logStream;
	char *getTimeStr(char currentTimeStr[]) {
		time_t currentTime = time(NULL);
		strftime(currentTimeStr, 20, "%F %T", localtime(&currentTime));
	}
};
Logger* Logger::sys_logger;


#endif //MULTIDNS_C_LOGGER_H
