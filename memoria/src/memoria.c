/*o
 ============================================================================
 Name        : memoria.c
 Author      : SO didn't C that coming
 Version     : 1.0
 Copyright   : SO didn't C that coming - UTN FRBA 2015
 Description : Trabajo Practivo Sistemas Operativos 2C 2015
 Testing	 :
 ============================================================================
 */

#include "memoria.h"


int main(void) {
	//Si el tercer parametro es true graba en archivo y muestra en pantalla sino solo graba en archivo
	logger = log_create(NOMBRE_ARCHIVO_LOG, "cpu", false, LOG_LEVEL_TRACE);

	// Levantamos el archivo de configuracion.
	LevantarConfig();

	iniciarTLB();

	iniciarListamProc();

	iniciarMemoriaPrincipal();

	int iThreadSeniales = pthread_create(&hSeniales, NULL,
				(void*) Seniales, NULL );
	if (iThreadSeniales) {
		fprintf(stderr,
			"Error al crear hilo - pthread_create() return code: %d\n",
			iThreadSeniales);
	}

	//Me Conecto a Swap
	if(conexionASwap()){
		//Hilo orquestador conexiones
		int iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL,
				(void*) HiloOrquestadorDeConexiones, NULL );
		if (iThreadOrquestador) {
			fprintf(stderr,
				"Error al crear hilo - pthread_create() return code: %d\n",
				iThreadOrquestador);
			exit(EXIT_FAILURE);
		}
		pthread_join(hOrquestadorConexiones, NULL );
		pthread_join(hSeniales, NULL );
	} else {
		printf("Error, no se pudo conectar a SWAP.\n");
		return -1;
	}
	return 0;
}

void Manejador(int signum){
	switch (signum){
	case SIGUSR1:
	   	printf("He recibido la señal SIGUSR1\n");
	   	//Vaciar la TLB
	   	break;
    case SIGUSR2:
	    printf("He recibido la señal SIGUSR2\n");
	    //Limpiar la MP y actualizar las tablas y bajar a swap los marcos modificados
	    break;
    case SIGPOLL:
	    printf("He recibido la señal SIGPOLL");
	    //Volcado de la memoria en un archivo
	    break;
    default:
    	printf("Fin de ejecucion\n");
    	exit(EXIT_SUCCESS);
    }
}

void Seniales(){
	if (signal(SIGUSR1, Manejador) == SIG_ERR) {
		perror("error en la señal SIGUSR1");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGUSR2, Manejador) == SIG_ERR) {
		perror("error en la señal SIGUSR2");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGPOLL, Manejador) == SIG_ERR) {
		perror("error en SIGTERM");
		exit(EXIT_FAILURE);
	}
	while (1)
		pause();
}

void iniciarListamProc(){
	lista_mProc = list_create();
}

void iniciarMemoriaPrincipal(){
	int i;
	printf("Cantidad Marcos Seteados en MP:%d\n",g_Cantidad_Marcos);
	a_Memoria = malloc(sizeof(t_mp)*g_Cantidad_Marcos);
	for(i=0;i<g_Cantidad_Marcos;i++){
		a_Memoria[i].marco = i;
		a_Memoria[i].bitModificado = 0;
		a_Memoria[i].bitUso = 0;
		a_Memoria[i].contenido = string_new();
	}
}

void iniciarTLB(){
	if(!strcmp(g_TLB_Habilitada,"SI")){
		lista_tlb = list_create();
		printf("TLB Habilitada - Entradas:%d\n",g_Entradas_TLB);
		int i;
		for(i=0;i<g_Entradas_TLB;i++){
			t_tlb* tlb = malloc(sizeof(t_tlb));
			tlb->marco = -1;
			tlb->pagina = -1;
			list_add(lista_tlb,tlb);
		}
	} else {
		printf("TLB No Habilitada\n");
	}
}

void HiloOrquestadorDeConexiones() {

	int socket_host;
	struct sockaddr_in client_addr;
	struct sockaddr_in my_addr;
	int yes = 1;
	socklen_t size_addr = 0;

	socket_host = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_host == -1)
		ErrorFatal(
				"No se pudo inicializar el socket que escucha a los clientes");

	if (setsockopt(socket_host, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		ErrorFatal("Error al hacer el 'setsockopt'");
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(g_Puerto);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8 * sizeof(char));

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		ErrorFatal("Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		ErrorFatal(
				"Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	//Traza("El socket está listo para recibir conexiones. Numero de socket: %d, puerto: %d", socket_host, g_Puerto);
	log_trace(logger,
			"SOCKET LISTO PARA RECIBIR CONEXIONES. Numero de socket: %d, puerto: %d",
			socket_host, g_Puerto);

	while (g_Ejecutando) {
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		if ((socket_client = accept(socket_host,(struct sockaddr *) &client_addr, &size_addr)) != -1) {
			//Traza("Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, socket_client);
			log_trace(logger,
					"NUEVA CONEXION ENTRANTE. Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d",
					inet_ntoa(client_addr.sin_addr), client_addr.sin_port,
					socket_client);
			// Aca hay que crear un nuevo hilo, que será el encargado de atender al cliente
			pthread_t hNuevoCliente;
			pthread_create(&hNuevoCliente, NULL, (void*) AtiendeCliente,
					(void *) socket_client);
		} else {
			Error("ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
		}
	}
	CerrarSocket(socket_host);
}

void procesarBuffer(char* buffer, long unsigned tamanioBuffer){

	FILE * archivo = fopen("texto.txt","w");

	fwrite(buffer,1,tamanioBuffer,archivo);

	fclose(archivo);

}

void procesarBuffer2(char* buffer, long unsigned tamanioBuffer){

	FILE * archivo = fopen("texto2.txt","w");

	fwrite(buffer,1,tamanioBuffer,archivo);

	fclose(archivo);

}


void enviarArchivo(){
	FILE * archivo = fopen("texto.txt","r");

	char * contenido;

	fseek(archivo,0L,SEEK_END);

	long unsigned tamanioA = ftell(archivo);

	contenido = malloc(tamanioA+1);
	memset(contenido,0,tamanioA+1);

	rewind(archivo);

	fread(contenido,1,tamanioA,archivo);

	EnviarDatos(socket_Swap,contenido,tamanioA);

	free(contenido);

}

void enviarArchivo2(int socket){
	FILE * archivo = fopen("texto2.txt","r");

	char * contenido;

	fseek(archivo,0L,SEEK_END);

	long unsigned tamanioA = ftell(archivo);

	contenido = malloc(tamanioA+1);
	memset(contenido,0,tamanioA+1);

	rewind(archivo);

	fread(contenido,1,tamanioA,archivo);

	EnviarDatos(socket,contenido,tamanioA);

	free(contenido);

}

void envioDeInfoIniciarASwap(int pid,int cantidadPaginas){

	//Hay q verificar la respuesta de Swap para ver si pudo o no Reservar el espacio solicitado

	char * bufferASwap=string_new();

	string_append(&bufferASwap,"31");
	string_append(&bufferASwap,obtenerSubBuffer(string_itoa(pid)));
	string_append(&bufferASwap,obtenerSubBuffer(string_itoa(cantidadPaginas)));

	printf("Buffer Enviado a SWAP (Iniciar): %s\n",bufferASwap);
	//EnviarDatos(socket_Swap,bufferASwap,strlen(bufferASwap));

}

void implementoIniciarCpu(char *buffer){

	//2 1 111 112

	int posActual=2,pid,cantidadPaginas=0,i=0;
	char *bufferAux;
	t_mProc *mProc = malloc (sizeof(t_mProc));
	t_pagina *pagina;

	//Id Proceso
	bufferAux= DigitosNombreArchivo(buffer,&posActual);
	pid = atoi(bufferAux);
	mProc->pid=pid;
	free(bufferAux);

	//Cantidad de Paginas
	bufferAux= DigitosNombreArchivo(buffer,&posActual);
	cantidadPaginas=atoi(bufferAux);

	mProc->paginas=list_create();
	for(i=0;i<cantidadPaginas;i++){
		pagina=malloc(sizeof(t_pagina));
		pagina->bitMP=-1;
		pagina->marco=-1;
		pagina->pagina=-1;
		list_add(mProc->paginas,pagina);

	}

	//Envio a Swap info necesaria para que reserve el espacio solicitado
	envioDeInfoIniciarASwap(pid,cantidadPaginas);

	//Agrego nuevo proceso a la lista
	list_add(lista_mProc,mProc);






}

int buscarPaginaEnTLB(int pid,int nroPagina,int *marco){

	t_tlb *telebe;
	int j=0;

	for(j=0;j<g_Entradas_TLB;j++){
		telebe=list_get(lista_tlb,j);

		if(telebe->pid == pid && telebe->pagina==nroPagina){
			*marco = telebe->marco;
			return 1;
		}
	}

	return 0;


}

char* buscarEnMemoriaPrincipal(int marco){

	//Aca falta mas cosas pero no me acuerdo que ajajj

	a_Memoria[marco].bitUso=1;

	return a_Memoria[marco].contenido;

}

void enviarContenidoACpu(int socket,int pid,int nroPagina,char* contenido){
	// No se bien por ahora si hace falta el nroPagina pero por las dudas lo mando
	//contenido = cicilianiYeta    pid = 4 nroPagina=3
	//3 2 114 113 213cicilianiYeta

	char *bufferACpu=string_new();

	string_append(&bufferACpu,"32");
	string_append(&bufferACpu,obtenerSubBuffer(string_itoa(pid)));
	string_append(&bufferACpu,obtenerSubBuffer(string_itoa(nroPagina)));
	string_append(&bufferACpu,obtenerSubBuffer(contenido));

	printf("Buffer Enviado a CPU: %s",bufferACpu);

	//EnviarDatos(socket,bufferACpu,strlen(bufferACpu));
}

int buscarEnTablaDePaginas(int pid,int nroPagina,int *marco){

	t_mProc *mProc;
	t_pagina *pagina;
	int i=0,j=0;


	while(i<list_size(lista_mProc)){
		mProc=list_get(lista_mProc,i);

		if(mProc->pid==pid){
			j=0;
			while(j<list_size(mProc->paginas)){
				pagina=list_get(mProc->paginas,j);

				if(pagina->pagina==nroPagina && pagina->bitMP==1){
					*marco = pagina->marco;
					return 1;

				}
				j++;
			}
		}
		i++;
	}

	return 0;

}

char * pedirContenidoASwap(int pid,int nroPagina){

	//3 2 111 112

	char * buffer=string_new();
	char * bufferRespuesta=string_new();

	string_append(&buffer,"32");
	string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(nroPagina)));

	printf("Buffer a Swap (Leer): %s",buffer);
	//EnviarDatos(socket_Swap,buffer,strlen(buffer));

	// Aca cuando reciba el buffer con el Contenido me va a venir con el protocolo, tengo q trabajarlo y solo retornar el contenido
	//RecibirDatos(socket_Swap,&bufferRespuesta);

	return bufferRespuesta;

}


void actualizarMemoriaPrincipal(int pid,int nroPagina,char *contenido){

	t_mProc *mProc;
	t_pagina *pagina;
	int i=0,j=0;

	while(i<list_size(lista_mProc)){
		mProc = list_get(lista_mProc,i);

		if(mProc-> pid == pid){
			j=0;
			while(j<list_size(mProc->paginas)){
				pagina = list_get(mProc->paginas,j);

				if(pagina->pagina == nroPagina){

					// Hay q ver q otras cosas actualizo
					pagina->bitMP=1;
					string_append(&a_Memoria[pagina->marco].contenido,contenido);

				}
				j++;
			}
		}
		i++;
	}

}


void implementoLeerCpu(int socket,char *buffer){

	//2 2 111 112

	int posActual=2,pid,nroPagina=-1,marco=-1;
	char *bufferAux,*contenido;


	//Id Proceso
	bufferAux= DigitosNombreArchivo(buffer,&posActual);
	pid = atoi(bufferAux);
	free(bufferAux);

	//Numero de Pagina
	bufferAux= DigitosNombreArchivo(buffer,&posActual);
	nroPagina=atoi(bufferAux);

	if(buscarPaginaEnTLB(pid,nroPagina,&marco)){
		//Acierto de la TLB entonces quiere decir que si esta en la TLB esta si o si en la memoria princial
		contenido=buscarEnMemoriaPrincipal(marco);
		enviarContenidoACpu(socket,pid,nroPagina,contenido);

	}else{

		if(buscarEnTablaDePaginas(pid,nroPagina,&marco)){
			//Encontro la pagina en la tabla de paginas
			contenido=buscarEnMemoriaPrincipal(marco);
			enviarContenidoACpu(socket,pid,nroPagina,contenido);

		}else{
			//No encontro la pagina en la Tabla, entonces debe pedirla al Swap (si o si va a devolver el contenido el Swap)

			contenido=pedirContenidoASwap(pid,nroPagina);
			enviarContenidoACpu(socket,pid,nroPagina,contenido);
			actualizarMemoriaPrincipal(pid,nroPagina,contenido);

		}
	}



}


void implementoCPU(int socket,char* buffer){
	int operacion = ObtenerComandoMSJ(buffer+1);

	switch (operacion) {
	case INICIAR:
		implementoIniciarCpu(buffer);
		//Reservar memoria en Swap
		//Devolver un Ok a CPU
		break;
	case LEER:
		implementoLeerCpu(socket,buffer);
		//fijarse si la pagina esta en la TLB
		//si esta, tomar el marco y devolver el contenido a la cpu
		//si no esta, buscarla en la memoria principal y ver si esa pagina esta en swap
		//si no esta en swap devolverle el contenido a la cpu
		//si esta en swap pedirle ese marco al swap, cargarlo en la pagina principal teniendo en cuenta
		//el algoritmo de reemplazo, luego devolverle el contenido a la cpu
		break;
	case ESCRIBIR:



		//Fijarse si la pagina a escribir esta en la TLB, si esta ir a la memoria principal y reemplazar el marco
		//dejar marcada esa pagina como modificada en la memoria principal
		//si no esta en la tlb, ir a buscarla a la memoria principal y fijarse si ese marco esta en swap o no,
		//si no esta en swap reemplazar su contenido y marcarla como modificada
		//si esta en swap, ir a buscarla, reemplazar algun marco existente teniendo en cuenta el algoritmo de
		//reemplazo y luego reemplazar su contenido
		//decidir si le devolvemos un ok o el contenido grabado a la cpu
		break;
	case FINALIZAR:
		//limpiar los marcos reservados de swap del proceso y luego borrar la tabla de paginas y devolver
		// un ok a la cpu
		break;
	default:
		break;
	}
}

int AtiendeCliente(void * arg) {
	int socket = (int) arg;


	// Es el encabezado del mensaje. Nos dice quien envia el mensaje
	int emisor = 0;

	// Dentro del buffer se guarda el mensaje recibido por el cliente.
	char* buffer;
	buffer = malloc(BUFFERSIZE * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.

	// Cantidad de bytes recibidos.
	long unsigned bytesRecibidos;

	// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
	int desconexionCliente = 0;

	// Código de salida por defecto
	int code = 0;
	while ((!desconexionCliente) && g_Ejecutando) {
		if (buffer != NULL )
			free(buffer);
		buffer = string_new();

		//Recibimos los datos del cliente
		bytesRecibidos = RecibirDatos(socket,&buffer);

		if (bytesRecibidos > 0) {
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			emisor = ObtenerComandoMSJ(buffer);

			//Evaluamos los comandos
			switch (emisor) {
			case ES_CPU:
				implementoCPU(socket,buffer);
				break;
			case COMANDO:
				printf("Ejecutado por telnet");
				EnviarDatos(socket,"Ok",strlen("Ok"));
				break;
			default:
				procesarBuffer(buffer,bytesRecibidos);
				enviarArchivo();
				free(buffer);
				buffer = string_new();
				char *  buffer2 = string_new();
				bytesRecibidos = RecibirDatos(socket_Swap,&buffer2);
				procesarBuffer2(buffer2,bytesRecibidos);
				enviarArchivo2(socket);
				free(buffer2);
				break;
			}
		} else
			desconexionCliente = 1;
	}
	CerrarSocket(socket);
	return code;
}


int conectarSWAP() {

	//ESTRUCTURA DE SOCKETS; EN ESTE CASO CONECTA CON NODO
	//log_info(logger, "Intentando conectar a nodo\n");
	//conectar con Nodo
	struct addrinfo hints;
	struct addrinfo *serverInfo;
	int conexionOk = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP


	if (getaddrinfo(g_Ip_Swap, g_Puerto_Swap, &hints, &serverInfo) != 0) {// Carga en serverInfo los datos de la conexion
		log_info(logger,
				"ERROR: cargando datos de conexion socket_FS");
	}

	if ((socket_Swap = socket(serverInfo->ai_family, serverInfo->ai_socktype,
			serverInfo->ai_protocol)) < 0) {
		log_info(logger, "ERROR: crear socket_FS");
	}
	if (connect(socket_Swap, serverInfo->ai_addr, serverInfo->ai_addrlen)
			< 0) {
		log_info(logger, "ERROR: conectar socket_FS");
	} else {
		conexionOk = 1;
	}
	freeaddrinfo(serverInfo);	// No lo necesitamos mas
	return conexionOk;
}


int conexionASwap(){
	//int cont;
	//int bytesRecibidos,cantRafaga=1,tamanio;
	//char*buffer = string_new();
	//char*bufferR = string_new();
	//char*bufferE = string_new();
	//char*aux;

	if(conectarSWAP()){
		printf("Se conecto a SWAP\n");
		//ENVIO a FILESYSTEM(la segunda rafaga)
		//31210127.0.0.1143000
		//string_append(&buffer,"31");
		//aux=obtenerSubBuffer(g_Ip_Nodo);
		//string_append(&buffer,aux);
		//aux=obtenerSubBuffer(string_itoa(g_Puerto_Nodo));
		//string_append(&buffer,aux);
		//aux=obtenerSubBuffer(LuToChar(tamanio_archivo(g_Archivo_Bin)));
		//string_append(&buffer,aux);

		//string_append(&bufferE,"3");
		//cont = cuentaDigitos(strlen(buffer));
		//string_append(&bufferE,string_itoa(cont));
		//string_append(&bufferE,string_itoa(strlen(buffer)));

		//Primera Rafaga
		//sem_wait(&semCon);
		//EnviarDatos(socket_Fs,bufferE, strlen(bufferE));
		//sem_post(&semCon);
		//bufferR = RecibirDatos(socket_Fs,bufferR, &bytesRecibidos,&cantRafaga,&tamanio);
		//Recibo respuesta de FS
		//if(bufferR!=NULL){
			//Segunda Rafaga
			//sem_wait(&semCon);
			//EnviarDatos(socket_Fs,buffer, strlen(buffer));
			//sem_post(&semCon);
			//bufferR = RecibirDatos(socket_Fs,bufferR, &bytesRecibidos,&cantRafaga,&tamanio);
			//printf("\nNodo Conectado al Fs!");
		//} else {
			//printf("No se pudo conectar al FS\n");
		//}
		//free(buffer);
		//free(bufferR);
		//free(bufferE);
		return 1;
	}
	return 0;
}


void CerrarSocket(int socket) {
	close(socket);
	//Traza("SOCKET SE CIERRA: (%d).", socket);
	log_trace(logger, "SOCKET SE CIERRA: (%d).", socket);
}

void ErrorFatal(const char* mensaje, ...) {
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);
	printf("\nERROR FATAL--> %s \n", nuevo);
	log_error(logger, "\nERROR FATAL--> %s \n", nuevo);
	char fin;

	printf(
			"El programa se cerrara. Presione ENTER para finalizar la ejecución.");
	fin = scanf("%c", &fin);

	va_end(arguments);
	if (nuevo != NULL )
		free(nuevo);
	exit(EXIT_FAILURE);
}

int ObtenerComandoMSJ(char* buffer) {
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return PosicionDeBufferAInt(buffer, 0);
}

int PosicionDeBufferAInt(char* buffer, int posicion) {
	int logitudBuffer = 0;
	logitudBuffer = strlen(buffer);

	if (logitudBuffer <= posicion)
		return 0;
	else
		return ChartToInt(buffer[posicion]);
}

int ChartToInt(char x) {
	int numero = 0;
	char * aux = string_new();
	string_append_with_format(&aux, "%c", x);
	//char* aux = malloc(1 * sizeof(char));
	//sprintf(aux, "%c", x);
	numero = strtol(aux, (char **) NULL, 10);

	if (aux != NULL )
		free(aux);
	return numero;
}

long unsigned RecibirDatos(int socket, char **buffer) {
	long bytesRecibidos = 0,tamanioBuffer=0,bytesEnviados;
	char *bufferAux= malloc(1);
	int posicion=1;
	memset(bufferAux,0,1);

	bufferAux = realloc(bufferAux,BUFFERSIZE * sizeof(char)+1);

	memset(bufferAux, 0, BUFFERSIZE * sizeof(char)+1); //-> llenamos el bufferAux con barras ceros.

	if ((bytesRecibidos = bytesRecibidos+recv(socket, bufferAux, BUFFERSIZE, 0)) == -1) {
		Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",socket);
	}

	if(bytesRecibidos>0) {
		bytesEnviados = send(socket, "Ok", strlen("Ok"), 0);
		if(bytesEnviados<=0) return 0;
	} else return 0;

	tamanioBuffer = atoi(DigitosNombreArchivo(bufferAux,&posicion));

	free(bufferAux);

	bufferAux = malloc(tamanioBuffer+1);
	*buffer = malloc(tamanioBuffer+10);
	memset(bufferAux,0,tamanioBuffer+1);
	memset(*buffer,0,tamanioBuffer+10);

	ssize_t numBytesRecv = 0;

	long unsigned pos=0;
	do{
		numBytesRecv = recv(socket, bufferAux, tamanioBuffer, 0);
		if ( numBytesRecv < 0)
			printf("ERROR\n");
		//printf("Recibido:%lu\n",pos);
		memcpy((*buffer+pos),bufferAux,numBytesRecv);
		memset(bufferAux,0,tamanioBuffer+1);
		pos = pos + numBytesRecv;
	}while (pos < tamanioBuffer);

	log_trace(logger, "RECIBO DATOS. socket: %d. tamanio buffer:%lu", socket,tamanioBuffer);
	return tamanioBuffer;
}


char* DigitosNombreArchivo(char *buffer,int *posicion){

	char *nombreArch;
	int digito=0,i=0,j=0,algo=0,aux=0,x=0;

	digito=PosicionDeBufferAInt(buffer,*posicion);
	for(i=1;i<=digito;i++){
		algo=PosicionDeBufferAInt(buffer,*posicion+i);
		aux=aux*10+algo;
	}
	nombreArch = malloc(aux+1);
	for(j=*posicion+i;j<*posicion+i+aux;j++){
		nombreArch[x]=buffer[j];
		x++;
	}
	nombreArch[x]='\0';
	*posicion=*posicion+i+aux;
	return nombreArch;
}

long unsigned EnviarDatos(int socket, char *buffer, long unsigned tamanioBuffer) {

	int bytecount,bytesRecibidos;
	long unsigned cantEnviados=0;
	char * bufferE = string_new(),*bufferR=malloc(BUFFERSIZE);
	memset(bufferR,0,BUFFERSIZE);

	string_append(&bufferE,YO);

	string_append(&bufferE,obtenerSubBuffer(string_itoa(tamanioBuffer)));

	if ((bytecount = send(socket,bufferE,strlen(bufferE), 0)) == -1){
		Error("No puedo enviar información a al cliente. Socket: %d", socket);
		return 0;
	}

	if ((bytesRecibidos = recv(socket, bufferR, BUFFERSIZE, 0)) == -1) {
		Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",socket);
	}

	long unsigned n,bytesleft=tamanioBuffer;

	while(cantEnviados < tamanioBuffer) {
		n = send(socket, buffer+cantEnviados, bytesleft, 0);
		if (n == -1){
			Error("Fallo al enviar a Socket: %d,",socket);
			return 0;
		}
		cantEnviados += n;
		bytesleft -= n;
		//printf("Cantidad Enviada :%lu\n",n);
	}
	if(cantEnviados!=tamanioBuffer) return 0;
	log_info(logger, "ENVIO DATOS. socket: %d. Cantidad Enviada:%lu ",socket,tamanioBuffer);
	return tamanioBuffer;
}

#if 1 // METODOS CONFIGURACION //
void LevantarConfig() {
	t_config* config = config_create(PATH_CONFIG);
	// Nos fijamos si el archivo de conf. pudo ser leido y si tiene los parametros

	if (config->properties->table_current_size != 0) {

		// Puerto de escucha
		if (config_has_property(config, "PUERTO_ESCUCHA")) {
			g_Puerto = config_get_int_value(config, "PUERTO_ESCUCHA");
		} else
			Error("No se pudo leer el parametro PUERTO_ESCUCHA");
		if (config_has_property(config, "IP_SWAP")) {
			g_Ip_Swap = config_get_string_value(config, "IP_SWAP");
		} else
			Error("No se pudo leer el parametro IP_SWAP");
		if (config_has_property(config, "PUERTO_SWAP")) {
			g_Puerto_Swap = config_get_string_value(config, "PUERTO_SWAP");
		} else
			Error("No se pudo leer el parametro PUERTO_SWAP");
		if (config_has_property(config, "MAXIMO_MARCOS_POR_PROCESO")) {
			g_Maximo_Marcos_Por_Proceso = config_get_int_value(config, "MAXIMO_MARCOS_POR_PROCESO");
		} else
			Error("No se pudo leer el parametro MAXIMO_MARCOS_POR_PROCESO");
		if (config_has_property(config, "CANTIDAD_MARCOS")) {
			g_Cantidad_Marcos = config_get_int_value(config, "CANTIDAD_MARCOS");
		} else
			Error("No se pudo leer el parametro CANTIDAD_MARCOS");
		if (config_has_property(config, "TAMANIO_MARCO")) {
			g_Tamanio_Marco = config_get_int_value(config, "TAMANIO_MARCO");
		} else
			Error("No se pudo leer el parametro TAMANIO_MARCO");
		if (config_has_property(config, "ENTRADAS_TLB")) {
			g_Entradas_TLB = config_get_int_value(config, "ENTRADAS_TLB");
		} else
			Error("No se pudo leer el parametro ENTRADAS_TLB");
		if (config_has_property(config, "TLB_HABILITADA")) {
			g_TLB_Habilitada = config_get_string_value(config, "TLB_HABILITADA");
		} else
			Error("No se pudo leer el parametro TLB_HABILITADA");
		if (config_has_property(config, "RETARDO_MEMORIA")) {
			g_Retardo_Memoria = config_get_int_value(config, "RETARDO_MEMORIA");
		} else
			Error("No se pudo leer el parametro RETARDO_MEMORIA");
	} else {
		ErrorFatal("No se pudo abrir el archivo de configuracion");
	}
	if (config != NULL ) {
		free(config);
	}
}

#endif

#if 1 // METODOS MANEJO DE ERRORES //
void Error(const char* mensaje, ...) {
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);

	fprintf(stderr, "\nERROR: %s\n", nuevo);
	log_error(logger, "%s", nuevo);

	va_end(arguments);
	if (nuevo != NULL )
		free(nuevo);
}
#endif

char* obtenerSubBuffer(char *nombre){
	// Esta funcion recibe un nombre y devuelve ese nombre de acuerdo al protocolo. Ej: carlos ------> 16carlos
	char *aux=string_new();
	int tamanioNombre=0;
	float tam=0;
	int cont=0;

	tamanioNombre=strlen(nombre);
	tam=tamanioNombre;
	while(tam>=1){
		tam=tam/10;
		cont++;
	}
	string_append(&aux,string_itoa(cont));
	string_append(&aux,string_itoa(tamanioNombre));
	string_append(&aux,nombre);

	return aux;
}

