#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) 
{
if(argc != 2)
{
fprintf(stderr, "Numero de argumentos errado\n");
return 1;
}
char file_path[100], words_path[100];
int file_num = 1,status;
int pid;
int nFicheiros = 0;

char* realArg0 = dirname(realpath(argv[0],NULL));
char* realArg1 = realpath(argv[1], NULL);

char swCall[PATH_MAX + 1];
sprintf(swCall, "%s/sw", realArg0);

char cscCall[PATH_MAX + 1];
sprintf(cscCall, "%s/csc", realArg0);

sprintf(words_path,"%s/words.txt",realArg1);

if(fopen(words_path, "r") == NULL)
{
fprintf(stderr, "Não existe words.txt\n");
return 2;
}
sprintf(file_path,"%s/%d.txt",realArg1,file_num);

while(fopen(file_path,"r") != NULL){
nFicheiros++;
char numero[5];
sprintf(numero, "%d", file_num);
pid = fork();
if(pid == 0){
	execlp(swCall, "sw", argv[1], numero, NULL);
}
else if(pid < 0)
{
fprintf(stderr, "Erro ao criar o fork da chamada do sw\n");
return 3;
}
else{
wait(&status);
file_num++;
sprintf(file_path,"%s/%d.txt",realArg1,file_num);
}
}
waitpid(pid,&status,0);


pid = fork();
if(pid == 0)
{
	execlp(cscCall, "csc", argv[1], NULL);
}
else if (pid < 0)
{
	fprintf(stderr, "Erro ao criar o fork da chamada do csc\n");
	return 4;
}
else wait(&status);

if(file_num == 1){
fprintf(stderr, "Não existem ficheiros para analizar\n");
return 4;
}

char tempIndex[100];
sprintf(tempIndex, "%s/tempindex.txt", realArg1);
if (remove(tempIndex) != 0)
{
	printf("Erro a remover o file tempindex.txt\n");
}

int i = 1;
for (; i <= nFicheiros; i++)
{
	char filename[100];
	sprintf(filename, "%s/temp%d.txt", realArg1, i);
	
	if (remove(filename) != 0)
	{
		printf("Erro a remover o file temp%d.txt\n", i);
	}

}

return 0;
}
