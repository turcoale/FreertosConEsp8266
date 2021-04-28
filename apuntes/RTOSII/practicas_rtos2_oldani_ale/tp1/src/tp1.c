/* Copyright 2017-2018, Eric Pernia
 * All rights reserved.
 *
 * This file is part of sAPI Library.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*==================[inlcusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

// sAPI header
#include "sapi.h"

// QM Pool
#include "qmpool.h"

#include <string.h>
#include <ctype.h>
#include <queue.h>

/*==================[definiciones y macros]==================================*/

//DEBUG_PRINT_ENABLE;

#define STX 0x55
#define ETX 0xAA

#define TIMEOUT_SERIE 5	//ms
#define CAPACIDAD_COLA_TXP 1024

typedef void (*CompletionHandler) ( void *); //Transmisión Proactiva

typedef struct Transmision_proactiva_struct {
	uint8_t * pBuffer;
	uint32_t largo;
	CompletionHandler ch;
}Transmision_proactiva;


/*==================[definiciones de datos internos]=========================*/

enum protocolo {
	pos_STX = 0,
	pos_OP,
	pos_T,
	pos_DATOS,
};

enum estados_fsm {
	estado_STX,
	estado_OP,
	estado_T,
	estado_Datos,
	estado_ETX,
};

enum OP {
	OP_MAYUCULIZAR = 0,
	OP_MINUSCULIZAR,
};

enum tamanio_pool
{
	eTAMANIO_CHICO = 16,
	eTAMANIO_MEDIANO = 128,
	eTAMANIO_GRANDE = 256
};

enum capacidad_pool
{
	eCAPACIDAD_CHICA = 10,
	eCAPACIDAD_MEDIANO = 20,
	eCAPACIDAD_GRANDE = 30
};

typedef struct
{
	uint8_t enable_timeout;
	uint16_t time;
	uint16_t indexRx;
}CTRL_SERIAL_PORT;

CTRL_SERIAL_PORT ctrlSerialPort;

QMPool mem_pool_chico, mem_pool_mediano, mem_pool_grande;
static uint8_t memoria_para_pool_chico[eTAMANIO_CHICO * eCAPACIDAD_GRANDE];
static uint8_t memoria_para_pool_mediano[eTAMANIO_MEDIANO * eCAPACIDAD_MEDIANO];
static uint8_t memoria_para_pool_grande[eTAMANIO_GRANDE * eCAPACIDAD_CHICA];


/*==================[definiciones de datos externos]=========================*/


/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

// Prototipo de funcion de la tarea
static void fsm_procesar_datos (char c);
static void enviar_paquete( char * paquete );
void myTask( void* taskParmPtr );
void tasktimeoutTrama(void* taskParmPtr);
void taskconvertir(void* taskParmPtr);
QMPool* obtener_pool_segun_tamanio(uint8_t size);

void uartUsbReceiveCallback( void *noUsado )
{
	char c = uartRxRead( UART_USB );
	fsm_procesar_datos(c);
	//printf( "Recibimos <<%c>> por UART\r\n", c );
}

void uartUsbSendCallback(void *noUsado )
{

}

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main(void)
{
	// ---------- CONFIGURACIONES ------------------------------
	// Inicializar y configurar la plataforma
	boardConfig();

	/* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
	uartConfig(UART_USB, 115200);
	// Seteo un callback al evento de recepcion y habilito su interrupcion
	uartCallbackSet(UART_USB, UART_RECEIVE, uartUsbReceiveCallback, NULL);
	// Seteo un callback al evento de transmisor libre y habilito su interrupcion
	uartCallbackSet(UART_USB, UART_TRANSMITER_FREE, uartUsbSendCallback, NULL);
	// Habilito todas las interrupciones de UART_USB
	uartInterrupt(UART_USB, true);

	//FIXME: ver esto como implementarlo
	//circularBuffer_Init( &cola_tx_proactiva, memoria_para_cola_txp, CAPACIDAD_COLA_TXP, sizeof(char));


	QMPool_init(&mem_pool_chico,
			memoria_para_pool_chico,
			sizeof(memoria_para_pool_chico),
			eTAMANIO_CHICO);

	QMPool_init(&mem_pool_mediano,
			memoria_para_pool_mediano,
			sizeof(memoria_para_pool_mediano),
			eTAMANIO_MEDIANO);

	QMPool_init(&mem_pool_grande,
			memoria_para_pool_grande,
			sizeof(memoria_para_pool_grande),
			eTAMANIO_GRANDE);


	// Inicializo el control de puerto serie
	ctrlSerialPort.enable_timeout = 0;
	ctrlSerialPort.time = 0;
	ctrlSerialPort.indexRx = 0;


	// Crear tarea en freeRTOS

	xTaskCreate(
				myTask,                     // Funcion de la tarea a ejecutar
				(const char *)"myTask",     // Nombre de la tarea como String amigable para el usuario
				configMINIMAL_STACK_SIZE*2, // Cantidad de stack de la tarea
				0,                          // Parametros de tarea
				tskIDLE_PRIORITY+1,         // Prioridad de la tarea
				0                           // Puntero a la tarea creada en el sistema
		);

	xTaskCreate(
			tasktimeoutTrama,                     // Funcion de la tarea a ejecutar
			(const char *)"timeoutTrama",     // Nombre de la tarea como String amigable para el usuario
			configMINIMAL_STACK_SIZE*3, // Cantidad de stack de la tarea
			0,                          // Parametros de tarea
			tskIDLE_PRIORITY+1,         // Prioridad de la tarea
			0                           // Puntero a la tarea creada en el sistema
	);

	xTaskCreate(
			taskconvertir,                     // Funcion de la tarea a ejecutar
			(const char *)"convertir",     // Nombre de la tarea como String amigable para el usuario
			configMINIMAL_STACK_SIZE*3, // Cantidad de stack de la tarea
			0,                          // Parametros de tarea
			tskIDLE_PRIORITY+1,         // Prioridad de la tarea
			0                           // Puntero a la tarea creada en el sistema
	);

	//Inicializo colas

	// Led para dar señal de vida
	gpioWrite( LED3, ON );

	// Iniciar scheduler
	vTaskStartScheduler();

	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE ) {
		// Si cae en este while 1 significa que no pudo iniciar el scheduler
	}

	// NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
	// directamenteno sobre un microcontroladore y no es llamado por ningun
	// Sistema Operativo, como en el caso de un programa para PC.
	return 0;
}

/*==================[definiciones de funciones internas]=====================*/


/*==================[definiciones de funciones externas]=====================*/

// Implementacion de funcion de la tarea
void myTask( void* taskParmPtr )
{
	// ---------- INICIALIZACION ------------------------------

	// ---------- REPETIR POR SIEMPRE --------------------------
	while(TRUE) {
		//Solicito un bloque de memoria
		// Intercambia el estado del LEDB
		gpioToggle( LEDB );
		vTaskDelay( 500 / portTICK_RATE_MS );
	}
}

void tasktimeoutTrama(void* taskParmPtr)
{
	while(TRUE) {
		if(ctrlSerialPort.enable_timeout)
		{
			ctrlSerialPort.time++;
			if(ctrlSerialPort.time > TIMEOUT_SERIE)
			{
				ctrlSerialPort.indexRx = 0;
				ctrlSerialPort.time = 0;
				ctrlSerialPort.enable_timeout = 0;
			}
		}
		vTaskDelay( 100 / portTICK_RATE_MS );
	}
}

void taskconvertir(void* taskParmPtr)
{
	char * bloque = NULL;
	uint8_t largo_paquete;
	uint8_t operacion;

	while(TRUE) {
   //FIXME: Ver esta cola como manejarla
		// xQueueReceive ( queCapital, &bloque, portMAX_DELAY);

    largo_paquete = bloque[pos_T];
    capital(bloque, largo_paquete, bloque[pos_OP]);

    //FIXME: Ver esto
//    xQueueSend( queTransmision, &bloquem portMAX_DELAY);
	}

}

QMPool* obtener_pool_segun_tamanio(uint8_t size)
{
	if(size < eTAMANIO_CHICO)
	{
		return &mem_pool_chico;
	}else if( size < eTAMANIO_MEDIANO)
	{
		return &mem_pool_mediano;
	}else
	{
		return &mem_pool_grande;
	}
	return NULL;
}

static void fsm_procesar_datos (char c) {
	//TODO: FALTA LO DE LOS POOLS
	static char * buffer = NULL;
	static uint8_t buffer_pos;
	static uint8_t estados_fsm = estado_STX;
	static char header[3];
	QMPool *pool;

	switch (estados_fsm) {
	case estado_STX:
		if (c == STX){
			header [pos_STX] = c;
			estados_fsm = estado_OP;
			buffer_pos = 0;
		}
		break;

	case estado_OP:
		header [pos_OP] = c;
		estados_fsm = estado_T;
		break;

	case estado_T:
		//TODO: ver que hacer según tamaño
		pool = obtener_pool_segun_tamanio(c + sizeof(header));
		buffer = QMPool_get(pool,0,true);

		if (buffer != NULL) {
			header[pos_T] = c;
			estados_fsm = estado_Datos;
			buffer_pos = 0;
		} else {
			estados_fsm = estado_STX;
			//TODO: Requerimiento opcional para ganar unos puntines
		}
		break;

	case estado_Datos:
		//TODO: ver que hacer acá
		buffer[buffer_pos++] = c;
		if(buffer_pos >= header[pos_T])
		{
			estados_fsm = estado_ETX;
		}
		break;

	case estado_ETX:
		if (c == ETX){
			buffer [buffer_pos] = c;
			enviar_paquete( buffer);
			//TODO: ver que hacer

		} else {
			//TODO: Paquete invalido (mandar mensaje)
			//TODO: Liberar paquete
		}
		estados_fsm = estado_STX;
		break;


	default:

		break;
	}

}

//Funcion para pasar de may a min o de min a may.
void capital( char *buff, uint8_t size, int operacion) {

	for (uint8_t i = pos_DATOS; i < size; i++ ) {
		if ( isalpha( buff[i] ) ){
			switch (operacion) {
			case OP_MAYUCULIZAR:
				if ( islower( buff[i])) {
					buff[i] = toupper( buff[i] );
				}

				break;

			case OP_MINUSCULIZAR:
				if ( isupper( buff[i])) {
					buff[i] = tolower( buff[i] );
				}
				break;

			default:
				break;

			}
		}
	}
}

static void enviar_paquete (char * paquete ){
	xQueueHandle cola_destino;

	//FIXME: Ver esto
//	cola_destino = queCapital;

	if( cola_destino != 0) {
		xQueueSend( cola_destino, paquete, portMAX_DELAY);
	}

}

/*==================[fin del archivo]========================================*/
