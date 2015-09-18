/*o
 ============================================================================
 Name        : planificador.c
 Author      : SO didn't C that coming
 Version     : 1.0
 Copyright   : SO didn't C that coming - UTN FRBA 2015
 Description : Trabajo Practivo Sistemas Operativos 2C 2015
 Testing	 :
 ============================================================================
 */

#include "planificador.h"





t_list *PCBs, *colaReady, *colaBloqueados;

pthread_t hOrquestadorConexiones, hConsola;

int cantHilos=0;
//int PID=0; // Contador de ID Procesos mProc (semaforo)
t_pid PID; // Contador ID Procesos + semaforo ;)

sem_t semPCB, semReady, semLock;



int main(void) {

	//Si el tercer parametro es true graba en archivo y muestra en pantalla sino solo graba en archivo
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", false, LOG_LEVEL_TRACE);


	lista_cpu = list_create();

	//CargarListaComandos();

	/* Inicializar semaforos */
	sem_init(&semListaCpu,0,1);
	inicializarListas();


	// Levantamos el archivo de configuracion.
	LevantarConfig();

	int iThreadConsola;					//Hilo de consola

	//Hilo orquestador conexiones
	int iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL, (void*) HiloOrquestadorDeConexiones, NULL );
	if (iThreadOrquestador) {
		fprintf(stderr, "Error al crear hilo - pthread_create() return code: %d\n", iThreadOrquestador);
		exit(EXIT_FAILURE);
	}

	//Este hilo es el que maneja la consola
	if ((iThreadConsola = pthread_create(&hConsola, NULL, (void*) ComenzarConsola, NULL)) != 0){
		fprintf(stderr, "Error al crear hilo - pthread_create() return code: %d\n", iThreadConsola);
		exit(EXIT_FAILURE);
	}

	pthread_join(hOrquestadorConexiones, NULL );
	pthread_join(hConsola, NULL );

	return 0;
}

int crearProceso(char* path) {
	t_PCB *nuevoPCB = malloc(sizeof(t_PCB));
	nuevoPCB->estado = LISTO;
	nuevoPCB->horaCreacion = time(NULL);
	nuevoPCB->nroLinea = 0;
	nuevoPCB->path = malloc(strlen(path));
	strcpy(nuevoPCB->path, path);
	nuevoPCB->pid = nuevoPid();
	nuevoPCB->quantum = g_Quantum;

	sem_wait(&semPCB);
	list_add(PCBs, nuevoPCB);
	sem_post(&semPCB);
	sem_wait(&semReady);
	list_add(colaReady, nuevoPCB);
	sem_post(&semReady);

	return nuevoPCB->pid;
}

int finalizarProceso(int pid) {

	bool _mismoPID(t_PCB *pcb){
		return pcb->pid == pid;
	}
	sem_wait(&semPCB);
	t_PCB *proceso = list_remove_by_condition(PCBs, (void*) _mismoPID );
	sem_post(&semPCB);
	t_PCB *aux = proceso;
	if(proceso->estado == EJECUTANDO){
		//TODO: Sacarlo de la CPU, matarlo y enterrar el cadaver donde nadie pueda verlo

	}
	if(proceso->estado == LISTO) {
		sem_wait(&semReady);
		aux = list_remove_by_condition(colaReady, (void*) _mismoPID);
		sem_post(&semReady);
	}
	if(proceso->estado == BLOQUEADO){
		sem_wait(&semLock);
		aux = list_remove_by_condition(colaBloqueados, (void*) _mismoPID);
		sem_post(&semLock);
	}
	if (aux != proceso){
		fprintf(stderr, "Error irrecuperable al finalizar el proceso %d\n", pid);
		if(__DEBUG__){
			fprintf(stderr, "Punteros: %d, %d", aux, proceso);
		}
		exit(0);
	}
	free(proceso->path);
	free(proceso);
	return proceso;
}

int nuevoPid(){
	sem_wait(&PID.sem);
	int pidAsignado = PID.pid++; //Aumento el pid en uno y lo guardo en una variable local
	sem_post(&PID.sem);
	return pidAsignado;
}

void mostrarProcesos(){
	t_PCB* proceso;
	fprintf(stdout, "|%-30s||%-30s||%-30s|\n", "PID", "ESTADO", "PATH");
	int i;
	for(i=0;i<list_size(PCBs);i++){
		proceso = list_get(PCBs,i);
		fprintf(stdout, "|%-30d||%-30s||%-30s|\n", proceso->pid, estados[proceso->estado], proceso->path);
	}
}

void inicializarListas() {
	sem_init(&PID.sem, 0, 1);
	sem_init(&semPCB,0,1);
	sem_init(&semReady,0,1);
	sem_init(&semLock,0,1);

	PID.pid = 1;
	PCBs = list_create();
	colaReady = list_create();
	colaBloqueados = list_create();
}
