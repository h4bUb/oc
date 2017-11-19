/*   У меня компилируется только так: gcc -pthread gol_server.c -o gol_server   */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <fcntl.h>


#define for_x for (int x = 0; x < w; x++)
#define for_y for (int y = 0; y < h; y++)
#define for_xy for_x for_y

int *buffer;
int **univ;
int w = 0, h = 0;

/*   game   */

//Вывод на экран
void show(void *u)
{
	printf("\033[H");
	for_y {
		for_x printf(univ[y][x] ? "\033[07m  \033[m" : "  ");
		printf("\033[E");
	}
	fflush(stdout);
}

//Новое состояние
void evolve(void *u)
{
	unsigned new[h][w];

	for_y for_x {
		int n = 0;
		for (int y1 = y - 1; y1 <= y + 1; y1++)
			for (int x1 = x - 1; x1 <= x + 1; x1++)
				if (univ[(y1 + h) % h][(x1 + w) % w])
					n++;

		if (univ[y][x]) n--;
		new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
	}
	for_y for_x univ[y][x] = new[y][x];
}

/*   timer   */

struct periodic_info {
	int timer_fd;
	unsigned long long wakeups_missed;
};

//Запуск таймера
int make_periodic(unsigned int period, struct periodic_info *info)
{
	int ret;
	unsigned int ns;
	unsigned int sec;
	int fd;
	struct itimerspec itval;

	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	info->wakeups_missed = 0;
	info->timer_fd = fd;
	if (fd == -1)
		return fd;

	sec = period / 1000000;
	ns = (period - (sec * 1000000)) * 1000;
	itval.it_interval.tv_sec = sec;
	itval.it_interval.tv_nsec = ns;
	itval.it_value.tv_sec = sec;
	itval.it_value.tv_nsec = ns;
	ret = timerfd_settime(fd, 0, &itval, NULL);
	return ret;
}

//Считываем состояние раз в секунду
void wait_period(struct periodic_info *info)
{
	unsigned long long missed;
	int ret;

	evolve(univ);

	ret = read(info->timer_fd, &missed, sizeof(missed));
	//Не успел вычислить новое состояние
	if (ret == -1) {
		perror("ERROR: update state\n");
		return;
	}

	info->wakeups_missed += missed;
}

//Создаем отдельный трэд для отсчета
int thread_1_count;

void *thread_1(void *arg)
{
	struct periodic_info info;

	make_periodic(1000000, &info);
	while (1) {
		thread_1_count++;
		wait_period(&info);
	}
	return NULL;
}

/*   server   */

//Вешаем хэндлер на клиента
void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc, n;
    int read_size;
    int g, t;
    char client_message[10];
    int univ_inet[w][h];

    //Отвечаем на запрос
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ) {

		for (g = 0; g < w; g++) {
			for (t = 0; t < h; t++) {
				univ_inet[g][t] = htonl(univ[g][t]);
			}
		}

		n = write(sock, univ_inet, sizeof(univ_inet));
		printf("done\n");
		if (n < 0) {
			perror("ERROR: writing to socket\n");
			exit(1);
		}
    }

    if(read_size == 0)
    {
        printf("Client disconnected\n");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
    	perror("ERROR: connection failed\n");
        exit(1);
    }

    free(socket_desc);
}

//Обработка подключений
void game()
{
	int listenfd = 0, connfd = 0, clilen, *new_sock;
	struct sockaddr_in serv_addr, cli_addr;

	char sendBuff[1025];

	pthread_t t_1;

	pthread_create(&t_1, NULL, thread_1, NULL);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(sendBuff, '0', sizeof(sendBuff));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000);

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(listenfd, 10);

	clilen = sizeof(cli_addr);

	//Клиент подключился
	while ((connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen)) >= 0) {

		puts("Connection accepted\n");

		pthread_t sniffer_thread;
		new_sock = (int*) malloc(1);
		*new_sock = connfd;

		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
			perror("ERROR: handler broken\n");
			exit(1);
		}

	 }
}

int main(int argc, char * argv[])
{
	w = 30, h = 30;
	ssize_t ret2;
	char ch;
	char mass[100];
	int fd = open(argv[1], O_RDONLY);
	int line = 0, column = 0, pos = 0, num, i ,g, t;

	sigset_t alarm_sig;
	sigemptyset(&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++)
		sigaddset(&alarm_sig, i);
	sigprocmask(SIG_BLOCK, &alarm_sig, NULL);

	if(argc != 2)
	{
		printf("\n Usage: %s <file_with_settings> \n",argv[0]);
		return 1;
	}

	buffer = malloc(sizeof(int)*w*h);
	univ = malloc(sizeof(int*)*h);

	for(i = 0; i<h; i++)
		univ[i]=&buffer[i*w];

	if (fd < 0)
	{
		fprintf(stderr, "ERROR: can't open file: %s\n", argv[1]);
		return 1;
	}

	//Считываем настройки
	while ((ret2 = read (fd, &ch, 1)) > 0) {
		if((ch>='0') && (ch<='9')) {
			mass[pos] = ch;
			pos++;
			sscanf(mass, "%i", &num);
			univ[line][column] = num;
			column++;

			memset(mass, 0, sizeof(char)*pos);
			pos--;
		}
		else {
			line++;
			w = column;
			column = 0;
		}
	}
	h = line;

	game();
}
