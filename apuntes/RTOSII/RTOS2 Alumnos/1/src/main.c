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

/*==================[definiciones y macros]==================================*/
QMPool mem_pool_1; /* Estructura de control del Pool */
static uint8_t memoria_para_pool_1[512]; /* Espacio de almacenamiento para el Pool */
/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

DEBUG_PRINT_ENABLE;

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

// Prototipo de funcion de la tarea
void myTask( void* taskParmPtr );

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main(void)
{
	// ---------- CONFIGURACIONES ------------------------------
	// Inicializar y configurar la plataforma
	boardConfig();

	// UART for debug messages
	debugPrintConfigUart( UART_USB, 115200 );
	debugPrintlnString( "Memory Pool con freeRTOS y sAPI." );

	// Led para dar señal de vida
	gpioWrite( LED3, ON );

	//Inicialización del Pool
	QMPool_init(&mem_pool_1,
			memoria_para_pool_1,
			sizeof(memoria_para_pool_1),
			10U);  /* Bloques de 10 bytes cada uno */

	// Crear tarea en freeRTOS
	xTaskCreate(
			myTask,                     // Funcion de la tarea a ejecutar
			(const char *)"myTask",     // Nombre de la tarea como String amigable para el usuario
			configMINIMAL_STACK_SIZE*2, // Cantidad de stack de la tarea
			0,                          // Parametros de tarea
			tskIDLE_PRIORITY+1,         // Prioridad de la tarea
			0                           // Puntero a la tarea creada en el sistema
	);

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
	char msg[50+1];
	sprintf( msg, "Numero de elementos: %d", QMPool_getMin(&mem_pool_1) );
	debugPrintlnString( msg );

	// ---------- REPETIR POR SIEMPRE --------------------------
	while(TRUE) {
		//Solicito un bloque de memoria
		char * block1 = QMPool_get(&mem_pool_1, 0U); /* asserts on empty pool */
		// Intercambia el estado del LEDB
		gpioToggle( LEDB );
		strncpy(block1, "Blink!", 10U);
		debugPrintlnString( block1 );
		//Libero el bloque de memoria
		QMPool_put(&mem_pool_1, block1);
		// Envia la tarea al estado bloqueado durante 500ms
		vTaskDelay( 500 / portTICK_RATE_MS );
	}
}

/*==================[fin del archivo]========================================*/
