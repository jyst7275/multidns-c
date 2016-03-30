#include <fcntl.h>
#include <signal.h>
#include "Logger.h"
#include "BindServer.h"
#include <sys/wait.h>
#define LOCKFILE "/var/run/multidns.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

#define DAEMON_NOT 0
#define DAEMON_START 1
#define DAEMON_STOP 2
#define DAEMON_RESTART 4
int main(int argc, char* args[]) {
	bool running_daemon = false;
	char* log_path = "/var/log/multi.log";
	char* config_path = "/etc/multidns.json";
	int daemon_mode = DAEMON_NOT;
	char pid_buf[20];
	int fd;
	for (int i = 0; args[i] != NULL; i++) {
		if (strcmp(args[i],"-d")==0) {
			running_daemon = true;
			if(args[i+1] == NULL) {
				printf("Error usage\n");
				exit(0);
			}
			if(strcmp(args[i+1], "start")==0)
				daemon_mode = DAEMON_START;
			else if(strcmp(args[i+1], "stop")==0)
				daemon_mode = DAEMON_STOP;
			else if(strcmp(args[i+1], "restart")==0)
				daemon_mode = DAEMON_RESTART;
			else{
				printf("Error -d\n");
				exit(0);
			}
			i++;
		}
		else if(strcmp(args[i], "-c")==0){
			if(args[i+1] == NULL){
				printf("Error usage\n");
				exit(0);
			}
			config_path = new char[100];
			bzero(config_path, sizeof(config_path));
			strcpy(config_path,args[i+1]);
			i++;
		}
		else if(strcmp(args[i], "-l")==0){
			if(args[i+1] == NULL){
				printf("Error usage\n");
				exit(0);
			}
			log_path = new char[100];
			bzero(log_path, sizeof(log_path));
			strcpy(log_path, args[i+1]);
			i++;
		}
		else if(strcmp(args[i], "-h")==0){
			printf("Usage: -c config_file -l log_file -d start|restart|stop\n");
			exit(0);
		}
	}



	if (daemon_mode == DAEMON_STOP || daemon_mode == DAEMON_RESTART) {
		fd = open(LOCKFILE, O_RDONLY, LOCKMODE);
		if (fd < 0) {
			printf("Not Started\n");
			return 0;
		}
		long pid_ld;
		long pid_fa;
		fscanf(fdopen(fd, "r"), "%ld %ld", &pid_fa, &pid_ld);
		printf("pid %ld,%ld\n", pid_fa, pid_ld);
		kill(pid_fa, SIGKILL);
		kill(pid_ld, SIGKILL);
		close(fd);
		unlink(LOCKFILE);
	}
	if (daemon_mode == DAEMON_STOP)
		return 0;


	Logger::getStreamLogger();
	if(DnsDispatcher::init(config_path) < 0)
		exit(0);
	if(running_daemon){
		fd = open(LOCKFILE, O_CREAT|O_RDWR|O_EXCL, LOCKFILE);
		if(fd < 0){
			printf("Already started\n");
			return 0;
		}
		ftruncate(fd, 0);
		pid_t pid;
		pid_t pid_son;
		pid_t pid_daemon;
		pid = fork();
		if(pid < 0)
			exit(0);
		//father return
		else if(pid > 0)
			exit(0);
		for(int i = 0;i < 3;i ++){
			close(i);
		}
		int fd0 = open("/dev/null", O_RDWR);
		int fd1 = dup(0);
		int fd2 = dup(0);

		Logger *logger = Logger::getFileLogger(log_path);
		pid_daemon = getpid();
		while(true){
			pid_son = fork();
			if(pid_son == 0)
				break;
			else if(pid_son < 0) {
				//fork error
				break;
			}
			else{
				close(fd);
				unlink(LOCKFILE);
				fd = open(LOCKFILE, O_CREAT|O_RDWR|O_EXCL, LOCKFILE);
				ftruncate(fd, 0);
				bzero(pid_buf, sizeof(pid_buf));
				sprintf(pid_buf, "%ld %ld", pid_daemon, pid_son);
				write(fd, pid_buf, strlen(pid_buf));
				logger->logInfo("Fork New Process");
				waitpid(pid_son,NULL,0);
			}
		}

	}

	BindServer bindServer = BindServer(DnsDispatcher::get_bind());
	bindServer.start();
	return 0;
}
