#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>


#define for_x for (int x = 0; x < w; x++)
#define for_y for (int y = 0; y < h; y++)
#define for_xy for_x for_y

//Вывод на экран
void show(void *u, int w, int h)
{
	int (*univ)[w] = u;
	printf("\033[H");
	for_y {
		for_x printf(univ[y][x] ? "\033[07m  \033[m" : "  ");
		printf("\033[E");
	}
	fflush(stdout);
}

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    char msg[4] = "give";
    struct sockaddr_in serv_addr;
    int t, g;
    int w = 30, h = 30;

    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    }

    //Готовимся к подключению
    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("ERROR: can't create socket\n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        perror("ERROR: inet_pton\n");
        return 1;
    }

    //Подключаемся к серверу
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
	   printf("ERROR: connection failed\n");
	   return 1;
	}

    int univ[h][w];

    //Общаемся с сервером, отвечает на нажатие клавиши Enter
    while (1) {
		getchar();
		n = write(sockfd,msg,strlen(msg));
		if (n < 0)
			perror("ERROR: writing to socket\n");
		memset(univ, 0, sizeof(univ[0][0]) * w * h);
		n = read(sockfd, univ, sizeof(univ));
		if (n < 0) {
			perror("ERROR: reading from socket\n");
		}

		for (g = 0; g < w; g++) {
			for (t = 0; t < h; t++) {
				univ[g][t] = ntohl(univ[g][t]);
			}
		}
		show(univ, w, h);
    }
    return 0;
}
