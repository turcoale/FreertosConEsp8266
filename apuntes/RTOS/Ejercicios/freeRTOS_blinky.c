/* Copyright 2017-2018, Amid Ale
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

/*==================[inclusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

// sAPI header
#include "sapi.h"

/*==================[definiciones y macros]==================================*/
#define DEBOUNCE_TIME 50/portTICK_RATE_MS
#define RELEASE 1
#define PUSH 0
#define FALLING 2
#define RISING 3



/*==================[definiciones de datos internos]=========================*/
uint8_t state;
bool_t flag;
TickType_t tiempo_inicio_ciclo;
TickType_t tiempo_final_ciclo;



/*==================[definiciones de datos externos]=========================*/

DEBUG_PRINT_ENABLE;

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

// Prototipo de funcion de la tarea
void debounce( void* taskParmPtr );
void toogleLed( void* taskParmPtr );

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main(void)
{
	// ---------- CONFIGURACIONES ------------------------------
	// Inicializar y configurar la plataforma
	boardConfig();

	// UART for debug messages
	debugPrintConfigUart( UART_USB, 115200 );
	debugPrintlnString( "Blinky con freeRTOS y sAPI." );

	// Led para dar señal de vida
	gpioWrite( LED3, ON );

	// Crear tarea en freeRTOS
	/*	xTaskCreate(
			myTask,                     // Funcion de la tarea a ejecutar
			(const char *)"myTask",     // Nombre de la tarea como String amigable para el usuario
			configMINIMAL_STACK_SIZE*2, // Cantidad de stack de la tarea
			0,                          // Parametros de tarea
			tskIDLE_PRIORITY+1,         // Prioridad de la tarea
			0                           // Puntero a la tarea creada en el sistema
	);*/

	xTaskCreate( debounce, (const char *)"debounce", configMINIMAL_STACK_SIZE*2, 0, tskIDLE_PRIORITY+1, 0);
	xTaskCreate( toogleLed, (const char *)"toogleLed", configMINIMAL_STACK_SIZE*2, 0, tskIDLE_PRIORITY+1, 0);



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
void debounce( void* taskParmPtr )
{
	state = RELEASE;
	// ---------- CONFIGURACIONES ------------------------------
	// ---------- REPETIR POR SIEMPRE --------------------------
	while(TRUE) {

		if (gpioRead(TEC2)) {
			if(state == PUSH){
		    	state = FALLING;		   //Estado Intermedio
				vTaskDelay(DEBOUNCE_TIME); //Espero un tiempo
				state = RELEASE;		   //Cambio a botón liberado
				tiempo_inicio_ciclo = xTaskGetTickCount();
				flag = TRUE;


			}
		} else {
			if (state == RELEASE){
				state = RISING;				//Estado Intermedio
				vTaskDelay(DEBOUNCE_TIME);	//Espero un tiempo
				state = PUSH;				//Cambio a botón presionado
				tiempo_final_ciclo = xTaskGetTickCount();
				flag = FALSE;

			}
		}

	}
}

void toogleLed ( void* taskParmPtr )
{
	TickType_t N_ticks;
	gpioWrite(LED1,FALSE);



	while(TRUE) {
		N_ticks = tiempo_final_ciclo - tiempo_inicio_ciclo;
		if (N_ticks < 0){
			N_ticks = -1*N_ticks;
		}

		if (flag && N_ticks) {
			gpioWrite(LED1,TRUE);
			vTaskDelay(N_ticks);
			gpioWrite(LED1,FALSE);
			flag = FALSE;
			N_ticks = 0;
			tiempo_inicio_ciclo = 0;
			tiempo_final_ciclo = 0;


		}
	}
}

/*==================[fin del archivo]========================================*/
