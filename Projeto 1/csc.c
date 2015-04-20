#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

int main(int argc, char *argv[]) 
{

char temp_file_path[100],temp_index_path[100],index_path[100];
int num_file = 1, fd[2],status;
int pid;

char* realArg1 = realpath(argv[1], NULL);

sprintf(temp_file_path,"%s/temp%d.txt",realArg1,num_file);
sprintf(index_path,"%s/index.txt",realArg1);
sprintf(temp_index_path,"%s/tempindex.txt",realArg1);

FILE *indextemp = fopen(temp_index_path,"w");
FILE* index = fopen(index_path,"w");

		if(pipe(fd) < 0)
		{
			fprintf(stderr, "Falhou a criação do pipe\n");
			return 1;
		}

while(fopen(temp_file_path,"r") != NULL){

			
			pid = fork();
			if(pid == 0){ //Se for filho
			dup2(fd[WRITE],STDOUT_FILENO);
			execlp("cat","cat", temp_file_path, (char *)NULL);
			fprintf(stderr, "Falhou o exec do cat\n");
			}
			else if(pid < 0){
			fprintf(stderr, "Erro ao criar o fork\n");
			return 3;
			}
			else{
			wait(&status);
			num_file++;
			sprintf(temp_file_path,"%s/temp%d.txt",argv[1],num_file);
			}

}
close(fd[WRITE]);
char car[1];
while (read(fd[READ], car, sizeof(char)) > 0)
{
fwrite(car,sizeof(char),1,indextemp);
}
fclose(indextemp);

//sort a partir de agora

pid = fork();
if(pid == 0){ //Se for filho
execlp("sort","sort", "-fV",temp_index_path,"-o",temp_index_path,(char *)NULL);
fprintf(stderr, "Falhou o exec\n");
}
else if(pid < 0){
			fprintf(stderr, "Erro ao criar o fork\n");
			return 4;
			}
else wait(&status);

//meter bonito agora

indextemp = fopen(temp_index_path,"r");

fwrite("index",1,5,index);

char comparador[100], palavra[100], frase[250],imprimir[100];
int linha, fich;
comparador[0] = '\0';

while(fgets(frase,250,indextemp) != NULL)
{
sscanf(frase,"%s : %d-%d",comparador,&fich,&linha);

if(strcmp(comparador,palavra) != 0){
strcpy(palavra,comparador);
sprintf(imprimir,"\n\n%s : %d-%d",palavra,fich,linha);
fwrite(imprimir,1,strlen(imprimir),index);
}
else{
sprintf(imprimir,", %d-%d",fich,linha);
fwrite(imprimir,1,strlen(imprimir),index);
}
}

fclose(indextemp);

return 0;
}
