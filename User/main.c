#include "define.h"

static void prvSetupHardware( void );
/* tasks */
static void ledTask(void *pvParameters);
static void ledTask2(void *pvParameters);
static void buttonTask(void *pvParameters);


/*-----------------------------------------------------------*/
static xSemaphoreHandle xEventSemaphore = NULL;

/*-----------------------------------------------------------*/

int main(void) {
	prvSetupHardware();
	
	vSemaphoreCreateBinary( xEventSemaphore );
	
	xTaskCreate( 	ledTask2,
					( signed char * ) "LE",
					configMINIMAL_STACK_SIZE,
					NULL,
					mainQUEUE_SEND_TASK_PRIORITY + 1,
					NULL );

	xTaskCreate( 	ledTask,
					( signed char * ) "LD",
					configMINIMAL_STACK_SIZE,
					NULL,
					mainQUEUE_SEND_TASK_PRIORITY,
					NULL );
	xTaskCreate( 	timerTask,
					( signed char * ) "TI",
					configMINIMAL_STACK_SIZE * 3,
					NULL,
					mainQUEUE_SEND_TASK_PRIORITY,
					NULL );
	xTaskCreate( 	buttonTask,
					( signed char * ) "BU",
					configMINIMAL_STACK_SIZE * 3,
					NULL,
					mainQUEUE_SEND_TASK_PRIORITY,
					NULL );
	
	vTaskStartScheduler();

	return 1;
}

static void prvSetupHardware( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	NVIC_SetPriorityGrouping( 0 );

	STM32vldiscovery_LEDInit(LED3);
	STM32vldiscovery_LEDInit(LED4);

	STM32vldiscovery_LEDOff(LED3);
	STM32vldiscovery_LEDOff(LED4);


	STM32vldiscovery_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	/* UART */

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);


	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;

	GPIO_Init(GPIOA,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;

	GPIO_Init(GPIOA,&GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate=9600;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;
	USART_InitStructure.USART_Parity=USART_Parity_No;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;

	USART_Init(USART1,&USART_InitStructure);

	USART_Cmd(USART1,ENABLE);	

}

static void ledTask(void *pvParameters){

	while(1) {
		STM32vldiscovery_LEDOff(LED4);
		vTaskDelay(500);
//		STM32vldiscovery_LEDOn(LED4);
//		vTaskDelay(500);

	}
}

static void ledTask2(void *pvParameters){
	while(1) {
//		STM32vldiscovery_LEDOff(LED3);
//		vTaskDelay(200);
//		STM32vldiscovery_LEDOn(LED3);
//		vTaskDelay(200);
		vTaskDelay(500);
		STM32vldiscovery_LEDOn(LED4);
		
	} 
}

static void buttonTask(void *pvParameters){
	while(1){
		if (STM32vldiscovery_PBGetState(BUTTON_USER) == SET){
			printf("buttons \n\r");
			STM32vldiscovery_LEDToggle(LED3);
		}
		vTaskDelay(100);
	}
}


/*----------------------------------------------------------*
 * 				DONOT CARE									*
 *----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	static uint32_t ulCount = 0;

	/* The RTOS tick hook function is enabled by setting configUSE_TICK_HOOK to
	1 in FreeRTOSConfig.h.

	"Give" the semaphore on every 500th tick interrupt. */
	ulCount++;
	if( ulCount >= 500UL )
	{
		/* This function is called from an interrupt context (the RTOS tick
		interrupt),	so only ISR safe API functions can be used (those that end
		in "FromISR()".

		xHigherPriorityTaskWoken was initialised to pdFALSE, and will be set to
		pdTRUE by xSemaphoreGiveFromISR() if giving the semaphore unblocked a
		task that has equal or higher priority than the interrupted task.
		http://www.freertos.org/a00124.html */
		xSemaphoreGiveFromISR( xEventSemaphore, &xHigherPriorityTaskWoken );
		ulCount = 0UL;
	}

	/* If xHigherPriorityTaskWoken is pdTRUE then a context switch should
	normally be performed before leaving the interrupt (because during the
	execution of the interrupt a task of equal or higher priority than the
	running task was unblocked).  The syntax required to context switch from
	an interrupt is port dependent, so check the documentation of the port you
	are using.  http://www.freertos.org/a00090.html

	In this case, the function is running in the context of the tick interrupt,
	which will automatically check for the higher priority task to run anyway,
	so no further action is required. */
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The malloc failed hook is enabled by setting
	configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

	Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software 
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  pxCurrentTCB can be
	inspected in the debugger if the task name passed into this function is
	corrupt. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
	FreeRTOSConfig.h.

	This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amount of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}
