#include "define.h"
#include "UART.h"
#include "SPI.h"
#include "nrf24l01.h"


static void prvSetupHardware( void );
/* tasks */
static void ledTask(void *pvParameters);
static void rfTask(void *pvParameters);


/*-----------------------------------------------------------*/
static xSemaphoreHandle xEventSemaphore = NULL;

/*-----------------------------------------------------------*/

int main(void) {
	prvSetupHardware();
	
	vSemaphoreCreateBinary( xEventSemaphore );
	
	printf("Now OS is running\n\r");
	
	xTaskCreate( 	ledTask,
					( signed char * ) "LD",
					configMINIMAL_STACK_SIZE,
					NULL,
					mainQUEUE_SEND_TASK_PRIORITY,
					NULL );
	xTaskCreate( 	rfTask,
					( signed char * ) "RF",
					configMINIMAL_STACK_SIZE * 4,
					NULL,
					mainQUEUE_SEND_TASK_PRIORITY,
					NULL );
	
	vTaskStartScheduler();

	return 1;
}

static void prvSetupHardware( void )
{	
	NVIC_SetPriorityGrouping( 0 );

	/* Init LED */
	STM32vldiscovery_LEDInit(LED3);
	STM32vldiscovery_LEDInit(LED4);
	STM32vldiscovery_LEDOff(LED3);
	STM32vldiscovery_LEDOff(LED4);
	
	/* Init button */
	STM32vldiscovery_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	
	/* UART */
	uart_init();
	
}

static void ledTask(void *pvParameters){

	while(1) {
		STM32vldiscovery_LEDOff(LED4);
		vTaskDelay(500);
		STM32vldiscovery_LEDOn(LED4);
		vTaskDelay(500);

	}
}


void configRF_txAddress (uint8_t * add){
	printf("configRF_txAddress\n\r");
	nrf24l01_set_tx_addr(add,5);
	delay_ms(1);
}

void RF_sendData(uint8_t * data, uint8_t len){
	uint8_t i = 0;
	for (i = 0; i < len; i++){
		nrf24l01_write_tx_payload(data + i, 1, 0);
		while (!(nrf24l01_irq_pin_active() && nrf24l01_irq_tx_ds_active()));
	}
}


uint8_t RF_sendByte(uint8_t byte){
	uint8_t reply = 0;
	uint8_t count = 0;

	nrf24l01_write_tx_payload(&byte, 1, true);
	while (!(nrf24l01_irq_pin_active() && nrf24l01_irq_tx_ds_active()));
	nrf24l01_irq_clear_all();
	nrf24l01_set_as_rx(true);

	delay_ms(1);
	for (count = 0; count < 250; count++) {

		if ((nrf24l01_irq_pin_active() && nrf24l01_irq_rx_dr_active())) {
			nrf24l01_read_rx_payload(&reply, 1); //get the payload into data
			break;
		}
		else {
			printf(".");
		}

	}

	nrf24l01_irq_clear_all(); //clear interrupts again


	delay_ms(10); //wait for receiver to come from standby to RX
	nrf24l01_set_as_tx(); //resume normal operation as a TX

	return reply;
}

uint8_t* RF_sendArray(uint8_t* data, uint8_t len){
	uint8_t i = 0;
	static uint8_t reply[32] = {0};

	for (i = 0; i < len; i++)
		reply[i] = RF_sendByte(data[i]);

	return reply;
}


static void rfTask(void *pvParameters){
	uint8_t data = 32;
	uint8_t new_data = 21;
	uint8_t destIP[5] = { 192, 168, 1, 1, 111 };
	
	SPI_Init_For_RF();
	nrf24l01_initialize_debug(false, 1, false);
	printf("Local host now run\n\r");
	configRF_txAddress(destIP);
	while (1) {

		//data = uart_getc();

		data ++;
		new_data = RF_sendByte(data);


		printf("\n\rData send: %d -> %d\n\r", data, new_data);
		if (new_data != data)
			printf("Erororrororororr\n\r");
		
		vTaskDelay(1000);
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
