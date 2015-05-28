
#include "logger.h"

#define DEFAULT_STRING_SIZE 200
#define PROJ_ID 'a'
#define FIMDEATENDIMENTO "fim_atendimento"
#define FIMDEATENDIMENTOSIZE 15

#define SHM_SIZE 10 

//Var Global
pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;

//funcao presente nos slides do professor
int readline(int fd, char *str)
{
		char* cpy = str;
        int n;
        do
        {
                while((n = read(fd,str,1)) == 0);
        }
        while (n>0 && *str++ != '\0');
        return strlen(cpy);
}

	//Thread de Atendimento dos Clientes
	//arg é o pathname do FIFO do cliente

	void *clientServing(void * arg){
		struct atendimentoData *atendimento = (struct atendimentoData *) arg;
		int waitingTime = atendimento->argStruct->clientOnLine;
		waitingTime++;
		if(waitingTime > 10)
			waitingTime = 10;

		atendimento->argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(atendimento->argStruct->numberBalcao-1) + 6] += waitingTime;


		atendimento->argStruct->clientOnLine++;
		pthread_mutex_lock(&mut);
		writeLOG(atendimento->argStruct->shmemName,time(NULL),BALCAO,atendimento->argStruct->numberBalcao,BALCAOSTARTSSERVICE,atendimento->clientFIFOID);
		sleep(waitingTime);
		int fdCliente = open(atendimento->clientFIFOID,O_WRONLY);
		if(fdCliente<0)
			fprintf(stderr, "Erro ao Executar Open ao Fifo do Cliente -> %s\n", strerror(errno));
		write(fdCliente,FIMDEATENDIMENTO,FIMDEATENDIMENTOSIZE+1);
		close(fdCliente);
		atendimento->argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(atendimento->argStruct->numberBalcao-1) + 5]++;
		atendimento->argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(atendimento->argStruct->numberBalcao-1) + 4]--;
		writeLOG(atendimento->argStruct->shmemName,time(NULL),BALCAO,atendimento->argStruct->numberBalcao,BALCAOSTOPSSERVICE,atendimento->clientFIFOID);
		free(atendimento);
		pthread_mutex_unlock(&mut);

		return 0;
	}

	void balcaoManagement(struct balcaoData *arg){
		char privateFIFOPathname[DEFAULT_STRING_SIZE], clientFIFOID[DEFAULT_STRING_SIZE];
		sprintf(privateFIFOPathname,"/tmp/fb_%i",arg->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(arg->numberBalcao-1) + 3]);
		//fprintf(stderr, "%s\n", privateFIFOPathname);
		int fd = open(privateFIFOPathname,O_RDONLY);

		//fprintf(stderr,"Passou o bloqueio\n")

		if(fd < 0){
			fprintf(stderr,"Erro ao abrir o FIFO do Balcao -> %s\n",strerror(errno));
		}

		while(readline(fd,clientFIFOID) > 0){
			struct atendimentoData *atendimento = malloc(sizeof(struct atendimentoData));
			atendimento->argStruct = arg;

			//fprintf(stderr, "clientes em fila %d\n", atendimento->argStruct->clientOnLine);

			//fprintf(stderr, "Entrou no readline ->%s\n",clientFIFOID);
			strcpy(atendimento->clientFIFOID, clientFIFOID);
			
			pthread_t tid;
			pthread_create(&tid,NULL,clientServing,atendimento);
		}
		//fprintf(stderr,"Saiu do readline");// nao corre
		return;	
	}


	void *cycle_function(void *arg){


		struct balcaoData *argStruct;

		argStruct = (struct balcaoData *) arg;

		time_t openingTime = time(NULL);
		time_t current_time = openingTime;

		int shmfd = shm_open(argStruct->shmemName,O_RDWR,0600); 

		if(shmfd == -1){
		//quando nao e criada shared mem
			shmfd = shm_open(argStruct->shmemName,O_CREAT|O_RDWR,0600);

			if (ftruncate(shmfd,4086) < 0)
			{
				fprintf(stderr, "Erro ao executar truncate da SHM -> %s\n", strerror(errno));
				exit(2);
			};

		//cria apontador para shared mem
			argStruct->pt = (int *) mmap(0,4086,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
			if(argStruct->pt == MAP_FAILED)
			{
				fprintf(stderr, "Erro ao executar o mapeamento da SHM -> %s\n", strerror(errno));
				exit(3);
			} 

			writeLOG(argStruct->shmemName,openingTime,BALCAO,1,SHMEMINIT,"-");
			argStruct->pt[OPENINGTIME] = openingTime;
			argStruct->pt[NUMOFBALCOES] = 0;
			argStruct->pt[NUMOFOPENBALCOES] = 0;
			argStruct->pt[NUMOFACTIVEBALCOES] = 0;
		}
	else argStruct->pt = (int *) mmap(0,4086,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0); /*create pointer to shared memory*/

	//revisao de variaveis
		argStruct->pt[NUMOFBALCOES] = argStruct->pt[NUMOFBALCOES] + 1;
		argStruct->numberBalcao = argStruct->pt[NUMOFBALCOES];
		argStruct->pt[NUMOFOPENBALCOES]++;
		argStruct->pt[NUMOFACTIVEBALCOES]++;
		fprintf(stderr,"Balcoes abertos = %i\n",argStruct->pt[NUMOFOPENBALCOES]);
		fprintf(stderr,"Balcoes ativos = %i\n",argStruct->pt[NUMOFACTIVEBALCOES]);

	//cria o balcao FIFO
		char privateFIFOPathname[DEFAULT_STRING_SIZE], privateFIFOID[DEFAULT_STRING_SIZE];
		int currentPID = getpid();
		sprintf(privateFIFOPathname,"/tmp/fb_%i",currentPID);
		sprintf(privateFIFOID,"fb_%i",currentPID);
		if(mkfifo(privateFIFOPathname,0660) < 0){
			printf("Erro ao criar o fifo do Balcao\n");
			return 0;
		}
		writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,BALCAOFIFOCREATION,privateFIFOID);


	//cria as variaveis para o balcao em causa
		argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1)] = argStruct->pt[NUMOFBALCOES];
		argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 1] = openingTime;
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 2] = -1; // tempo de abertura
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 3] = currentPID;
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 4] = 0; // clientes em espera
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 5] = 0; // clientes atendidos
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 6] = 0; // tempo medio de atendimento

//chamada a thread principal
	pid_t pid;
	pid = fork();

	if(pid == 0)
	{
		balcaoManagement(argStruct);
		return 0;
	}
	else if(pid > 0)
	{
	/*cycling through time to close balcao
	Bloqueia este processo*/
		while(difftime(current_time,openingTime) < argStruct->timeIsOpen)
			current_time = time(NULL);

		writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,CLOSEBALCAO,"-");

		/*closing the balcao to the customers, but balcao still working if needed*/
		argStruct->pt[NUMOFOPENBALCOES]--;
		fprintf(stderr,"Balcoes abertos = %i\n",argStruct->pt[NUMOFOPENBALCOES]);

		argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 2] = argStruct->timeIsOpen;

	/*Ciclo para confirmar se existem clientes em fila ou não
	Bloqueia novamente este pocesso
	*/
	while(argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 4] != 0);
	/*Quando não houver enviar sinal para parar a espera bloqueante*/
	kill(pid,SIGKILL);


	int status;
	waitpid(pid,&status,0);

	argStruct->pt[NUMOFACTIVEBALCOES]--;
	fprintf(stderr,"Balcoes ativos = %i\n",argStruct->pt[NUMOFACTIVEBALCOES]);
	unlink(privateFIFOPathname);

	writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,BALCAOSTOPSSERVING,"-");

	//quando atinge o ultimo balcao a shared mem e apagada
	if(argStruct->pt[NUMOFACTIVEBALCOES] == 0){
		writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,STORECLOSE,"-");
		//writeSHM(argStruct->pt);
		sleep(1);
		if (munmap(argStruct->pt,4086) < 0)
		{
			fprintf(stderr, "Erro ao executar o desmapeamento da SHM -> %s\n", strerror(errno));
			exit(5);
		} 
		if (shm_unlink(argStruct->shmemName) < 0)
		{
			fprintf(stderr, "Erro ao executar o unlink da SHM -> %s\n", strerror(errno));
			exit(6);
		} 
	}else{
		if (munmap(argStruct->pt,4086) < 0)
		{
			fprintf(stderr, "Erro ao executar o desmapeamento da SHM -> %s\n", strerror(errno));
			exit(5);
		}
	}
	free(argStruct->mut);
	free(argStruct);
	return 0;
}
else{
	fprintf(stderr,"Erro a criar o Fork -> %s\n",strerror(errno));
}
return 0;
}
/*
//MAIN
//	criacao da cycle_function thread.
*/
	int main(int argc, char *argv[])
	{
	//argv[1] = nome da memória partilhada
	//argv[2] = tempo de abertura do balcao
	/*
		confirmação da existencia de memoria partilhada
	*/
		if (argc != 3)
		{
			fprintf(stderr, "Usage: ./balcao SHM_name tempoAberto\n");
			exit(1);
		}
		struct balcaoData *argStruct = malloc(sizeof(struct balcaoData));

		argStruct->mut = malloc(sizeof(pthread_mutex_t));

		pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, 0);
		pthread_mutex_init(argStruct->mut, &mattr); 
		argStruct->timeIsOpen = atoi(argv[2]);
		argStruct->shmemName = argv[1];
		argStruct->clientOnLine = 0;

		if(argStruct->timeIsOpen <= 0)
		{
			fprintf(stderr, "Erro na passagem do tempo de abertura\n");
			exit(0);
		}

		pthread_t tid;
		if(fork() == 0){
			pthread_create(&tid,NULL,cycle_function,argStruct);
		}
		pthread_exit(NULL);
	}