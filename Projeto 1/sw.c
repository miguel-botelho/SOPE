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
	//argv[1] é igual
	//manda-se outro
	//fgets devolve uma frase ate encontrar um enter
	//abrir
	  
	int fd[2],status;
	int pid;
	char str_fgets[100], wordsPath[100], filePath[100], tempPath[100]; 


	char* realArg1 = realpath(argv[1], NULL);

	sprintf(filePath,"%s/%s.txt", realArg1, argv[2]); //Path do ficheiro a analisar - recebe no argv[2] o numero do ficheiro
	sprintf(wordsPath,"%s/words.txt", realArg1);
	sprintf(tempPath, "%s/temp%s.txt", realArg1, argv[2]);

	FILE * wordsFile = fopen(wordsPath,"r"); // abre o ficheiro a ser usado no fgets
	FILE * temp = fopen(tempPath,"w"); // temp
	

	while(fgets(str_fgets,100,wordsFile) != NULL){
			str_fgets[strlen(str_fgets)-1] = '\0';
			if(pipe(fd) < 0){
			fprintf(stderr, "Falhou a criação do pipe\n");
			return 1;
			}
			pid = fork();
			if(pid == 0){ //Se for filho
				close(fd[READ]);
				dup2(fd[WRITE],STDOUT_FILENO);
				execlp("grep","grep","-wn", "-o", str_fgets, filePath, (char *)NULL);
				
				fprintf(stderr, "Falhou o exec\n");
			}
			else if(pid >0){ //O pai esta a escrever
				waitpid(pid,&status,0);
				char line[50];
				int contador = 0;
				close(fd[WRITE]);
				int linha = 0;
				char * lixo = NULL;
				char car[1];
				close(fd[WRITE]);
				while (read(fd[READ], car, sizeof(char)) > 0)
				{
					if (car[0] == '\n')
					{
						line[contador] = '\0';
						sscanf(line, "%d:%s", &linha, lixo);

						fprintf(temp, "%s : %s-%d\n", str_fgets, argv[2], linha); 

						contador = 0;
					}
					else
					{
						line[contador] = car[0];
						contador++;
					}
				}
				close(fd[READ]);
			} 
			else{
				fprintf(stderr, "Fork_sw error");
			}
	}
	return 0;
	
}
