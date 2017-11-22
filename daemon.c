/*Все пишется в syslog
 * В 104 строке нужжно вставить путь к файлу с конфигом
 */
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>


struct config {
	char path[20];
	char exec[10];
	char param[10];
	char mode[8];
};

static const struct config EmptyStruct;
int MAXPROC=0;
pid_t pid_list[100];
char respawn[7] = "respawn";

int hup = 0;
int broke = 0;

void sighandler(int);
void init();

int main(int argc, char **argv) {
	int fd;

	//Становимся демоном
	struct rlimit flim;
	if (getppid() != 1) {
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGHUP, sighandler);

		if (fork() != 0) {
			exit(0);
		}
		setsid();
	}

	getrlimit(RLIMIT_NOFILE, &flim);
	for (fd=0; fd < flim.rlim_max; fd++) {
		close(fd);
	}

	chdir("/");
	openlog("demon", LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Start working");
	closelog();

	init();

	return 0;
}

void init() {

	int p;
	int i;
	pid_t cpid;
	int status;
	int es;
	FILE *fconfig;
	struct config myconfig[100];
	int pid_count;
	int ret;
	int attempt = 50;
	time_t start, end;
	double elapsed;
	int terminate;
	status = 0;
	es = 0;

	//Обрабатываем сигнал -HUP
	if (hup == 1) {
		hup = 0;

		openlog("demon", LOG_PID | LOG_CONS, LOG_DAEMON);
		syslog(LOG_INFO, "caught hup");
		closelog();

		for (p=0; p<MAXPROC; p++) {
			if (pid_list[p] != 0) {
				kill(pid_list[p], SIGKILL);
			}
		}
		myconfig[100] = EmptyStruct;
		MAXPROC = 0;
	}

	/*      PATH ВСТАВЛЯТЬ СЮДА      */
	fconfig = fopen("PATH", "r");

	//Считываем конфиг
	while (fscanf(fconfig, "%s%s%s", myconfig[MAXPROC].path, myconfig[MAXPROC].param, myconfig[MAXPROC].mode) != EOF) {
		int len = strlen(myconfig[MAXPROC].path);
		len--;
		i = 0;
		char temp[10];
		while ((myconfig[MAXPROC].path)[len] != '/') {
			temp[i] = (myconfig[MAXPROC].path)[len];
			len--;
			i++;
		}
		len = 0;
		i--;
		while (i >= 0) {
			(myconfig[MAXPROC].exec)[len] = temp[i];
			len++;
			i--;
		}

		openlog("demon", LOG_PID | LOG_CONS, LOG_DAEMON);
		syslog(LOG_INFO, "%s %s %s %s\n", myconfig[MAXPROC].path, myconfig[MAXPROC].param, myconfig[MAXPROC].mode, myconfig[MAXPROC].exec);
		closelog();

		MAXPROC++;
	}

	if (fclose(fconfig) != 0) {
		exit (1);
	}

	pid_count = 0;

	//Создаем нужное число форков
	for (p=0; p<MAXPROC; p++) {

		cpid = fork();
		switch (cpid) {
		case -1:
			break;
		case 0:
			cpid = getpid();

			while (1) {
				start = time(NULL);
				terminate = 1;
				while (terminate) {
					end = time(NULL);
					elapsed = difftime(end, start);
					ret = execl(myconfig[p].path, myconfig[p].exec, myconfig[p].param, (char *)0);
					if (ret == -1) {
						attempt += 1;
					}
					else {
					}
					if (elapsed >= 5.0)
						terminate = 0;
					else
						usleep(50000);
				}
				if (attempt > 50) {
					perror("waitpid failed");
					exit(1);
				}
			}
			exit(0);
		default:
			pid_list[p] = cpid;
			pid_count++;
		}
	}

	while (pid_count) {
		cpid = waitpid(-1, &status, 0);

		for (p=0; p<MAXPROC; p++) {
			if (pid_list[p] == cpid) {
				if ( WIFEXITED(status)) {
					es = WEXITSTATUS(status);
				}

				pid_list[p] = 0;
				pid_count--;

				//Ошибка выполнение процесса
				if (es == 1) {
					openlog("demon", LOG_PID | LOG_CONS, LOG_DAEMON);
					syslog(LOG_INFO, "%s %s is broken", myconfig[p].exec, myconfig[p].param);
					closelog();

					cpid = fork();
					switch (cpid) {
					case -1:
						break;
					case 0:
						cpid = getpid();
						sleep(10);

						//Даем 50 попыток на 5 сек, если не получается запустить, откладываем на час
						while (1) {
							start = time(NULL);
							terminate = 1;
							while (terminate) {
								end = time(NULL);
								elapsed = difftime(end, start);
								ret = execl(myconfig[p].path, myconfig[p].exec, myconfig[p].param, (char *)0);
								if (ret == -1) {
									attempt += 1;
									}
								if (elapsed >= 3600.0)
									terminate = 0;
								else
									usleep(50000);
							}
							if (attempt > 50) {
								exit (1);
							}
						}
						exit(0);
					default:
						pid_list[p] = cpid;
						pid_count++;
					}

				}
				//Процесс успешно завершился
				else {
					openlog("demon", LOG_PID | LOG_CONS, LOG_DAEMON);
					syslog(LOG_INFO, "%s %s stopped work with number %d, cpid: %d", myconfig[p].exec, myconfig[p].param, p, cpid);
					closelog();
				}

				//Если нужно перезапустить
				if (strcmp(myconfig[p].mode, respawn) == 0) {

					cpid = fork();
					switch (cpid) {
					case -1:
						break;
					case 0:
						cpid = getpid();

						while (1) {
							start = time(NULL);
							terminate = 1;
							while (terminate) {
								end = time(NULL);
								elapsed = difftime(end, start);
								ret = execl(myconfig[p].path, myconfig[p].exec, myconfig[p].param, (char *)0);
								if (ret == -1) {
									attempt += 1;
								}
								if (elapsed >= 3600.0)
									terminate = 0;
								else
									usleep(50000);
							}
							if (attempt > 50) {
								exit (1);
							}
						}
						exit(0);
					default:
						pid_list[p] = cpid;
						pid_count++;
					}
				}
			}
		}
	}
}

//Обработчик сигналов(-HUP)
void sighandler(int signum) {
	hup = 1;
	init();
}
