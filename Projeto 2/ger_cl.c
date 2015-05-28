#include "logger.h"

void *chooseBalcao(void *arg){
	void *ret;
	int *pt = (int *)arg;
	int balcao, minClientes = INT_MAX, numBalcoes = pt[NUMOFBALCOES];
	while(numBalcoes > 0)
	{
		if(pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numBalcoes-1) + 2] == -1)
			if(minClientes > pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numBalcoes-1) + 4])
			{

				minClientes = pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numBalcoes-1) + 4];
				balcao = numBalcoes;
			}

	numBalcoes--;
	}
	ret = malloc(sizeof(int));
	*(int *)ret = balcao; 
	return ret;
}

int main(int argc, char *argv[])
{
	//argv[1] = nome da memória partilhada
	//argv[2] = numero de clientes a gerar

	/*confirmação da existencia de memoria partilhada*/

	if (argc != 3)
	{
		fprintf(stderr, "Usage: ./ger_cl SHM_name nrClientes\n");
		exit(1);
	}
	char memPath[DEFAULT_STRING_SIZE];

	sprintf(memPath,"%s",argv[1]);

	int shmfd = shm_open(memPath,O_RDWR,0600); 


	if(shmfd == -1){
		printf("Não há memória partilhada aberta!\n");
		exit(0);
	}

	if (ftruncate(shmfd,4086) < 0)
	{
		fprintf(stderr, "Erro ao executar truncate da SHM -> %s\n", strerror(errno));
		exit(2);
	};

	int *pt = (int *) mmap(0,4086,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
	if(pt == MAP_FAILED)
	{
		fprintf(stderr, "Erro ao executar o mapeamento da SHM -> %s\n", strerror(errno));
		exit(3);
	} 

	if(pt[NUMOFOPENBALCOES] == 0){
		printf("Não há balcoes abertos!\n");
		exit(0);
	}

	int numberOfCli;

	sscanf(argv[2],"%i",&numberOfCli);

	pid_t pid;

	while(numberOfCli > 0){

		pid = fork();

		if(pid < 0){
			printf("Erro ao executar o fork de criação de Clientes\n");
			return 1;
		}
		else if(pid == 0){
			char privateFIFOPathname[DEFAULT_STRING_SIZE], balcaoFIFO[DEFAULT_STRING_SIZE], balcaoAnswer[DEFAULT_STRING_SIZE];
			int currentPID = getpid();
			sprintf(privateFIFOPathname,"/tmp/fc_%i",currentPID);
			if(mkfifo(privateFIFOPathname,0660) < 0){
				printf("Erro ao criar o fifo privado do Cliente\n");
				return 2;
			}
			//fprintf(stderr,"1\n");
			
			pthread_t tid;
			void *retval; //valor de retorno da escolha do balcao
			pthread_create(&tid,NULL,chooseBalcao,pt);
			pthread_join(tid,&retval);
			int chosenBalcao = *(int *)retval;
			free (retval);// free the malloc in chooseBalcao
			pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(chosenBalcao-1) + 4]++;
			sprintf(balcaoFIFO,"/tmp/fb_%i",pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(chosenBalcao-1) + 3]);
			//fprintf(stderr,"balcaofifo -> %s\n",balcaoFIFO);
			int fdBalcao = open(balcaoFIFO,O_WRONLY);
			//fprintf(stderr,"2\n");
			write(fdBalcao,privateFIFOPathname,strlen(privateFIFOPathname)+1);
			close(fdBalcao);//not sure if its doing anything
			writeLOG(memPath,time(NULL),CLIENT,chosenBalcao,CLIASKSSERVICE,privateFIFOPathname);
			//fprintf(stderr,"3\n");
			//fprintf(stderr,"%s\n",privateFIFOPathname);
			int fd = open(privateFIFOPathname,O_RDONLY);
			//fprintf(stderr,"4\n");
			read(fd,balcaoAnswer,FIMDEATENDIMENTOSIZE+1);
			//fprintf(stderr,"5 -> %s\n",balcaoAnswer);
			if(strcmp(balcaoAnswer,FIMDEATENDIMENTO) == 0)
				writeLOG(memPath,time(NULL),CLIENT,chosenBalcao,CLIENTENDSERVICE,privateFIFOPathname);
			unlink(privateFIFOPathname);// temporario
			if (munmap(pt,4086) < 0)
			{
				fprintf(stderr, "Erro ao executar o desmapeamento da SHM -> %s\n", strerror(errno));
				exit(5);
			}
			return 0; // temporario
		}
		else{
			numberOfCli--;
		}
	}
	return 0;
}
