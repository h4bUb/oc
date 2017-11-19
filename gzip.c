#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


int main (int argc, char ** argv) {
	char ch;
	ssize_t ret;
	int num_zeros = 0;
	char mass[10000] = "hello";
	int pos = 0;
	int fd;
	int wrote = 0;

	//Открываем файл на запись
	if ((fd = open (argv[1], O_WRONLY |  O_CREAT | O_TRUNC, 0666)) == -1) {
		printf ("Cannot open file.\n");
		exit(1);
	}

	while ((ret = read (0, &ch, 1)) > 0) {
		//Считаем дыры
		if ((int)ch == 0) {
			//Пишем данные
			if (pos > 0) {
				wrote = 1;
				lseek(fd, num_zeros, SEEK_SET);
				int hm = write (fd, mass, pos);
				if (hm != pos) {
					printf("Write Error\n");
					exit(1);
				}
				num_zeros += pos;
				pos = 0;
			}
			num_zeros += 1;
		}
		//Считаем реальные данные
		else {
			mass[pos] = ch;
			pos++;
		}
	}

	//Пишем данные
	if (pos > 0) {
		wrote = 1;
		lseek(fd, num_zeros, SEEK_SET);
		int hm = write (fd, mass, pos);
		if (hm != pos) {
			printf("Write Error\n");
			exit(1);
		}
	}

	if (wrote == 0){
		printf("file full of holes!\n");
	}
	close(fd);
	return 0;
}
