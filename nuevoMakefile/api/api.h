/*
 * api.h
 *
 *  Created on: 30/8/2015
 *      Author: utnso
 */

#ifndef API_H_
#define API_H_

#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/config.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <semaphore.h>
#include <netdb.h>

//Tamaño del buffer
#define BUFFERSIZE 50

// Logger del commons
t_log* logger;

/*
 * @NAME: conectarCliente
 * @DESC: conecta a cliente y retorna el socket
 * */
int conectarCliente (char* ip, char* puerto);

/*
 * @NAME: CerrarSocket
 * @DESC: cierra el socket y escribe en el archivo log
 * */
void CerrarSocket(int socket);

/*
 * @NAME: ErrorFatal
 * @DESC: Escribe el error en el archivo log y termina la ejecucion del proceso
 * */
void ErrorFatal(const char* mensaje, ...);

/*
 * @NAME: ObtenerComandoMSJ
 * @DESC: Hay que obtener el comando dado el buffer. El comando está dado por el primer caracter, que tiene que ser un número.
 * */
int ObtenerComandoMSJ(char* buffer);

/*
 * @NAME: PosicionDeBufferAInt
 * @DESC: convierte un char (de la ubicacion buffer[posicion]), que representa un numero, a int
 * */
int PosicionDeBufferAInt(char* buffer, int posicion);

/*
 * @NAME: ChartToInt
 * @DESC: recibe un caracter que represente un numero y devuelve el int equivalente
 * */
int ChartToInt(char x);

/*
 * @NAME: RecibirDatos
 * @DESC: recibe TODOS los datos del socket y lo almacena en buffer. Retorna el tam del buffer
 * */
long unsigned RecibirDatos(int socket, char **buffer);

/*
 * @NAME: DigitosNombreArchivo
 * @DESC: Posicionado al comienzo de la rafaga, devuelve el dato correspondiente al primer dato del mensaje. El puntero
 * queda al comienzo del proximo dato del mensaje, por lo que al llamarla de nuevo devuelve el segundo dato(si corresponde)
 *
 * */
char* DigitosNombreArchivo(char *buffer,int *posicion);

/*
 * @NAME: EnviarDatos
 * @DESC: Hace el send de los datos del buffer al socket, YO es el char* que para nosotros (Emi y Esme) era ID
 * */
long unsigned EnviarDatos(int socket, char *buffer, long unsigned tamanioBuffer, char* YO);

/*
 * @NAME: obtenerSubBuffer
 * @DESC: Esta funcion recibe un nombre y devuelve ese nombre de acuerdo al protocolo. Ej: carlos ------> 16carlos
 * */
char* obtenerSubBuffer(char *nombre);

/*
 * @NAME: cuentaDigitos
 * @DESC: Esta funcion recibe un valor y devuelve la cantidad de digitos de ese valor: cuentaDigitos(100) ==> 3
 * Obs: Se podria convertir ese valor a un string y hacerle un strlen pero ya habiamos creado esta funcion para ese entonces xD.
 * */
int cuentaDigitos(int valor);



/*
 * @NAME: obtenerSubBufferDeContenido
 * @DESC: Igual que obtenerSubBuffer pero esta soporta \0 en el medio del contenido pero hay q pasarle un parametro
 * 		  Le tengo q pasar un string y un tamanio que seria el tamaño maximo de ese string,
 *		  El nombre debe tener el resto del contenido con \0, Ej: nombre="hola\0\0\0....\0" la cantidad de \0 son (tamanio-strlen(hola))
 *		  PD: Puse el strlen como ejemplo pero no se puede usar xDXDXDXDxXxXdXXdXDXd
 *		  Ej: nombre= AhiEstaLaYetaDeCeci\0,\0aTocarMaderaTodos  tamanio=256  ==> salida= 3256AhiEstaLaYetaDeCeci\0,\0aTocarMaderaTodos\0\0\0\0...\0
 * */
char* obtenerSubBufferDeContenido(char *nombre,int tamanio);


/*
 * @NAME: Error
 * @DESC: Hace un print del error (mensaje) y lo escribe en el archivo log
 * */
void Error(const char* mensaje, ...);
/*
 * @NAME: imprimirContenido
 * @DESC: Esta funcion imprime el contenido de un string malvado. (String malvado = string que tiene \0 en el medio, no solo en el final)
 * */
void imprimirContenido(char* contenido, long unsigned tamanio);

/*
 * @NAME: strlenHastaUnChar
 * @DESC: Esta funcion cuenta el tamanio del string hasta encontrer unChar, funciona aunque el string tenga \0 en el medio
 * */
int strlenHastaUnChar(char* string, char unChar);

#endif /* API_H_ */
