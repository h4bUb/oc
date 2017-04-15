// ./prog.exe file1 file2
// Числа берутся так же и из последнего файла. Т.к в файлах может быть мусор, то '-' не считаю за знак минус, поэтому все числа положительные


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>

#include <fcntl.h>
#include <unistd.h>

#define MAX_NUM_LENGTH	10
#define MAX_NUM_CAP	100000

typedef int bool;
#define true 1
#define false 0


bool str_cmp (char s[], int n, int p); //Проверяем, что при приведении типов число не вышло за пределы int

int compare( const void* a, const void* b); //Функция сравнения для sort

int main (int argc, char * argv[]) {

		 ssize_t ret;
         int arg;
         int fd;
         int flags = O_RDONLY;
         int i;
         int num;
         int pos = 0;
         int pos_n = 0;
         int numbers[MAX_NUM_CAP];
         char ch;
         char mass[MAX_NUM_LENGTH];
         bool number = false;

         for(arg = 1 ; arg < argc; arg++) {

        	 //Пытаемся открыть файл
			 fd = open(argv[arg], flags);
			 if (fd < 0)
			 {
				fprintf(stderr, "ERROR: can't open file: %s\n", argv[arg]);
				exit (1);
			 }

			 //Считываем данные
			 while ((ret = read (fd, &ch, 1)) > 0) {
					if((ch>='0') && (ch<='9')) {
						number = true;
					}
					else {
						if (number == true) {
							sscanf(mass, "%i", &num);

							//Если число вышло за пределы int
							if (str_cmp(mass, num, pos) == false) {
								fprintf (stderr, "ERROR: number ");
								for(i = 0; i < strlen(mass); i++)
									fprintf(stderr, "%c", mass[i]);
								fprintf(stderr, " is too large\n");
								exit(1);
							}

							if (pos_n+1 != MAX_NUM_CAP) {
								numbers[pos_n] = num;
								pos_n++;
							}
							else {
								fprintf (stderr, "ERROR: can't hold more numbers\n");
								exit(1);
							}

							memset(mass, 0, sizeof(char)*pos);
							pos = 0;
						}
						number = false;
					}
					if (number == true) {
						if (pos != MAX_NUM_LENGTH) {
							mass[pos] = ch;
							pos++;
						}
						else {
							fprintf (stderr, "ERROR: number ");
							for(i = 0; i < strlen(mass); i++)
								fprintf(stderr, "%c", mass[i]);
							fprintf(stderr, " is too large\n");
							exit(1);
						}
					}
			 }

			 //Упорядочиваем числа
			 qsort(numbers, pos_n, sizeof(int), compare );

			 //Ошибка чтения из файла
			 if (ret < 0)
			 {
				fprintf (stderr, "ERROR: can't read file\n", argv[arg]);
				exit (1);
			 }

			 //Открываем файл на запись
			 if (arg == argc-1) {
				 FILE *f = fopen(argv[arg], "w");

				 if (f == NULL)
				 {
					 fprintf(stderr, "ERROR: can't open file: %s\n", argv[arg]);
					 exit(1);
				 }

				 //Пишем
				 for (i = 0 ; i < pos_n; i++){
					 fprintf(f, "%i ", numbers[i]);
				 }

				 //Пытаемся закрыть файл
				 if (fclose(f) != 0) {
					 fprintf (stderr, "ERROR: can't close file %s\n", argv[arg]);
					 exit (1);
				 }
			 }

			 //Пытаемся закрыть файл
			 if (close (fd) != 0)
			 {
				fprintf (stderr, "ERROR: can't close file %s\n", argv[arg]);
				exit (1);
			 }
         }
         printf("Done!\n");
	     return 0;
 }

bool str_cmp (char s[], int n, int p) {
	char str[16];
    sprintf(str, "%i", n);
	int i;

	for (i = 0; i < p; i++) {
		if ((s[i]) != (str[i])) {
			return false;
		}
	}
	return true;
}

int compare( const void* a, const void* b)
{
     int int_a = * ( (int*) a );
     int int_b = * ( (int*) b );

     if ( int_a == int_b ) return 0;
     else if ( int_a < int_b ) return -1;
     else return 1;
}