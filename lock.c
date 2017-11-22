#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>


int main(int argc, char * argv[]) {

	if(argc != 3)
	{
		printf("\n Usage: %s <text> <file>\n",argv[0]);
		return 1;
	}

	char buffer[100];

	const char* extension = ".lck";
	char* name_with_extension;
	name_with_extension = malloc(strlen(argv[2])+1+4);
	strcpy(name_with_extension, argv[2]);
	strcat(name_with_extension, extension);

	//Открываем файл из входа
	 FILE *file = fopen(argv[2], "a");
	 if (file == NULL)
	 {
		 fprintf(stderr, "ERROR: can't open file: %s\n", argv[2]);
		 exit(1);
	 }

	int fd;
	while (1) {
		fd = open(name_with_extension, O_WRONLY | O_CREAT | O_EXCL, 0644);
		if (fd < 0 && errno == EEXIST) {
			;
		}
		else if (fd < 0) {
			perror("unexpected error\n");
			exit (1);
		}
		else {
			break;
		}
	}

	int curr_pid = getpid();

	sprintf(buffer, "%d", curr_pid);
	strcat(buffer, "read-write_lock");
	write(fd, buffer, strlen(buffer));

	//Спим для теста
	sleep(10);

	//Пишем в файл из входа
	printf("writing in %s text: %s\n", argv[2], argv[1]);
	fprintf(file, "%s", argv[1]);

	if (fclose(file) != 0) {
		fprintf (stderr, "ERROR: can't close file: %s\n", argv[2]);
		exit (1);
	}

	//Сбрасываем блокировку
	printf("closing %s\n", name_with_extension);
	unlink (name_with_extension);

	return 0;
}
