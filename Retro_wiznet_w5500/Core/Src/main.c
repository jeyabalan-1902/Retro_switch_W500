/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "dns.h"
#include "dhcp.h"
#include "socket.h"
#include "w5500_spi.h"
#include "MQTTClient.h"
#include "wizchip_conf.h"
#include "mqtt_interface.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define DHCP_BUFF_SIZE	   2048
#define SOCKET_ID_FOR_DHCP 7
#define SOCKET_ID_FOR_DNS  6

#define SWITCH_EVENT_QUEUE_SIZE 20
#define SWITCH_TASK_STACK_SIZE 512
#define MQTT_TASK_STACK_SIZE 1024

#define SWITCH_TASK_PRIORITY 3  // Higher priority
#define MQTT_TASK_PRIORITY 2   // Lower priority
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */
osThreadId_t switchTaskHandle;
osThreadId_t mqttTaskHandle;
osMessageQueueId_t switchEventQueueHandle;


wiz_NetInfo gWIZNETINFO = {
		.mac = { 0x80, 0x34, 0x28, 0x74, 0xA5, 0xCB },//MSB - LSB
		.ip ={ 192, 168, 1, 112 },
		.sn = { 255, 255, 255, 0 },
		.gw ={ 192, 168, 1, 1 },
		.dns = { 8, 8, 8, 8 },
		.dhcp = NETINFO_STATIC };

uint8_t dhcp_buffer[DHCP_BUFF_SIZE];
uint8_t destination_ip[]={192,168,1,36};
uint16_t destination_port = 1883;

MQTTClient mqtt_client;
Network network;
MQTTPacket_connectData connect_data = MQTTPacket_connectData_initializer;
MQTTMessage msg={QOS0,1,0,1,"hello from wiznet!",18};

uint8_t sendbuff[256],receivebuff[256];

uint8_t dns_buffer[MAX_DNS_BUF_SIZE];
extern bool StateChangeFlag;
bool tempflag = false;
uint8_t dns_ip[4] = {8,8,8,8};
char* remote_host = "mqtt.onwords.in";
uint8_t remote_host_ip[4];
uint8_t chk = 1;
int result;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
void InitializeHardware(void);
void InitializeW5500(void);
void Task_SwitchHandler(void *argument);
void Task_MQTTHandler(void *argument);
static void PHYStatusCheck(void);
static void PrintPHYConf(void);
void SWITCH_Handler(void);
void OnDHCPIPAssigned(void);
void DisplayNetworkConfigurations();
void FlushQueue(osMessageQueueId_t queueHandle);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("Sys Init\r\n");
  //printf("w5500initialized\n\r");
  InitializeW5500();
  printf("w5500initialized\n\r");
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  switchEventQueueHandle = osMessageQueueNew(SWITCH_EVENT_QUEUE_SIZE, sizeof(uint8_t), NULL);
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  osThreadNew(Task_SwitchHandler, NULL, &(osThreadAttr_t){
  	  .name = "SwitchTask",
  	  .priority = (osPriority_t)SWITCH_TASK_PRIORITY,
  	  .stack_size = SWITCH_TASK_STACK_SIZE
    });

	osThreadNew(Task_MQTTHandler, NULL, &(osThreadAttr_t){
	  .name = "MQTTTask",
	  .priority = (osPriority_t)MQTT_TASK_PRIORITY,
	  .stack_size = MQTT_TASK_STACK_SIZE
	});
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPI1_RST_Pin|SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SPI1_RST_Pin SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_RST_Pin|SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SWITCH3_Pin SWITCH2_Pin SWITCH1_Pin */
  GPIO_InitStruct.Pin = SWITCH3_Pin|SWITCH2_Pin|SWITCH1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SWITCH4_Pin */
  GPIO_InitStruct.Pin = SWITCH4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(SWITCH4_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void InitializeW5500(void)
{
	printf("enters on w5500\r\n");
	W5500Init();
	wiz_PhyConf phyconf;

	phyconf.by=PHY_CONFBY_SW;
	phyconf.duplex=PHY_DUPLEX_FULL;
	phyconf.speed=PHY_SPEED_10;
	phyconf.mode=PHY_MODE_AUTONEGO;//best to go with auto-negotiation

	ctlwizchip(CW_SET_PHYCONF, (void*) &phyconf);

	PHYStatusCheck();
	PrintPHYConf();
}

void Task_SwitchHandler(void *argument) {
    uint8_t detected_switch_id = 0;
    uint8_t prev_SwitchState[] = {HAL_GPIO_ReadPin(SWITCH1_GPIO_Port, SWITCH1_Pin), HAL_GPIO_ReadPin(SWITCH2_GPIO_Port, SWITCH2_Pin),
    		HAL_GPIO_ReadPin(SWITCH3_GPIO_Port, SWITCH3_Pin),HAL_GPIO_ReadPin(SWITCH4_GPIO_Port, SWITCH4_Pin)};
	uint8_t Switch_CurrentState[4];
    while (1)
    {
    	Switch_CurrentState[0] = HAL_GPIO_ReadPin(SWITCH1_GPIO_Port, SWITCH1_Pin);
		if (Switch_CurrentState[0] != prev_SwitchState[0])
		{
			detected_switch_id = 1;
			prev_SwitchState[0] = Switch_CurrentState[0];
		}

        if (detected_switch_id != 0) {
            if (osMessageQueuePut(switchEventQueueHandle, &detected_switch_id, 0, 0) != osOK) {
            	osDelay(10);
				if (osMessageQueuePut(switchEventQueueHandle, &detected_switch_id, 0, 0) != osOK) {
					printf("Queue full after retry. Dropping switch event.\r\n");
				}
            }
            detected_switch_id = 0;
        }

        osDelay(10);
    }
}

void Task_MQTTHandler(void *argument)
{
    uint8_t mqtt_connected = 0;
    uint8_t result;
    uint8_t remote_host_ip[4];

    DHCP_init(SOCKET_ID_FOR_DHCP, dhcp_buffer);
	reg_dhcp_cbfunc(OnDHCPIPAssigned, NULL, NULL);
	printf("Obtaining IP address ...\r\n");
	//MQTT Client Part
	connect_data.willFlag = 0;
	connect_data.MQTTVersion = 4;
	connect_data.clientID.cstring = "testw5500";
	connect_data.username.cstring = "Nikhil";
	connect_data.password.cstring = "Nikhil8182";

	connect_data.keepAliveInterval = 60;//seconds
	connect_data.cleansession = 1;

	NewNetwork(&network, 1);//1 is the socket number to use
	DNS_init(SOCKET_ID_FOR_DNS, dns_buffer);

    while(StateChangeFlag == false)
   	{
   	  DHCP_run();
   	}


    printf("Resolving hostname: %s\n", remote_host);
	result = DNS_run(gWIZNETINFO.dns, (uint8_t *)remote_host, remote_host_ip);

	if (result != 1)
	{
	   printf("DNS resolution failed for hostname: %s\n", remote_host);
	   while (1);
	}

	printf("Hostname resolved: %s -> %d.%d.%d.%d\n",
		  remote_host, remote_host_ip[0], remote_host_ip[1], remote_host_ip[2], remote_host_ip[3]);

	if (ConnectNetwork(&network, remote_host_ip, destination_port) != SOCK_OK)
	{
	   printf("Failed to connect to MQTT broker.\n");
	   while (1);
	}

	printf("Connected to MQTT broker.\n");

	MQTTClientInit(&mqtt_client, &network, 1000, sendbuff, sizeof(sendbuff), receivebuff, sizeof(receivebuff));
	connect_data.keepAliveInterval = 120;

	if (MQTTConnect(&mqtt_client, &connect_data) != MQTT_SUCCESS)
	{
	   printf("MQTT connection failed.\n");
	   while (1);
	}
    mqtt_connected = 1;
    printf("MQTT connection established.\n");

    while (1) {
            if (mqtt_connected)
            {
                if (!MQTTIsConnected(&mqtt_client))
                {
                    printf("MQTT connection lost. Attempting to reconnect...\n");
                    mqtt_connected = 0;
                }
                else
                {
                	uint8_t switch_event;
					if (osMessageQueueGet(switchEventQueueHandle, &switch_event, NULL, 0) == osOK)
					{
					   char message[32];
					   snprintf(message, sizeof(message), "switch %d detected", switch_event);

					   MQTTMessage mqtt_message = {
						   .qos = QOS0,
						   .retained = 0,
						   .dup = 0,
						   .id = 0,
						   .payload = message,
						   .payloadlen = strlen(message)
					   };

					   if (MQTTPublish(&mqtt_client, "onwords/wired002/status", &mqtt_message) == MQTT_SUCCESS)
					   {
						   printf("Published: %s\n", message);
					   } else
					   {
						   printf("MQTT publish failed.\n");
					   }
					   FlushQueue(switchEventQueueHandle);
					}
					 MQTTYield(&mqtt_client, 100);
                }
            }

            if (!mqtt_connected) {

            	result = DNS_run(gWIZNETINFO.dns, (uint8_t *)remote_host, remote_host_ip);

            		if (result != 1)
            		{
            		   printf("DNS resolution failed for hostname: %s\n", remote_host);
            		   while (1);
            		}

            		printf("Hostname resolved: %s -> %d.%d.%d.%d\n",
            			  remote_host, remote_host_ip[0], remote_host_ip[1], remote_host_ip[2], remote_host_ip[3]);

            		if (ConnectNetwork(&network, remote_host_ip, destination_port) != SOCK_OK)
            		{
            		   printf("Failed to connect to MQTT broker.\n");
            		   while (1);
            		}

            		printf("Connected to MQTT broker.\n");

            		MQTTClientInit(&mqtt_client, &network, 1000, sendbuff, sizeof(sendbuff), receivebuff, sizeof(receivebuff));
            		//connect_data.keepAliveInterval = 120;

            		if (MQTTConnect(&mqtt_client, &connect_data) != MQTT_SUCCESS)
            		{
            		   printf("MQTT connection failed.\n");
            		   while (1);
            		}
            	    mqtt_connected = 1;
            	    printf("MQTT connection established.\n");
            }
            osDelay(100);
        }
}

#define PUTCHAR  int __io_putchar(int ch)
PUTCHAR
{
	HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
	return ch;
}

void PHYStatusCheck(void)
{
	uint8_t tmp;

	do
	{
		printf("\r\nChecking Ethernet Cable Presence ...");
		ctlwizchip(CW_GET_PHYLINK, (void*) &tmp);

		if(tmp == PHY_LINK_OFF)
		{
			printf("NO Cable Connected!");
			HAL_Delay(1500);
		}
	}while(tmp == PHY_LINK_OFF);

	printf("Good! Cable got connected!");

}

void PrintPHYConf(void)
{
	wiz_PhyConf phyconf;

	ctlwizchip(CW_GET_PHYCONF, (void*) &phyconf);

	if(phyconf.by==PHY_CONFBY_HW)
	{
		printf("\n\rPHY Configured by Hardware Pins");
	}
	else
	{
		printf("\n\rPHY Configured by Registers");
	}

	if(phyconf.mode==PHY_MODE_AUTONEGO)
	{
		printf("\n\rAutonegotiation Enabled");
	}
	else
	{
		printf("\n\rAutonegotiation NOT Enabled");
	}

	if(phyconf.duplex==PHY_DUPLEX_FULL)
	{
		printf("\n\rDuplex Mode: Full");
	}
	else
	{
		printf("\n\rDuplex Mode: Half");
	}

	if(phyconf.speed==PHY_SPEED_10)
	{
		printf("\n\rSpeed: 10Mbps");
	}
	else
	{
		printf("\n\rSpeed: 100Mbps");
	}
}

void DisplayNetworkConfigurations()
{
	ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);

	printf(" MAC : %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
	printf(" IP : %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
	printf(" GW : %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
	printf(" SN : %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
	printf("=======================================\r\n");
}

void OnDHCPIPAssigned(void)
{

	printf("IP Obtained: \r\n");

	getIPfromDHCP(gWIZNETINFO.ip);
	getGWfromDHCP(gWIZNETINFO.gw);
	getSNfromDHCP(gWIZNETINFO.sn);
	getDNSfromDHCP(gWIZNETINFO.dns);

	gWIZNETINFO.dhcp = NETINFO_DHCP;

	ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);

	DisplayNetworkConfigurations();
}

void FlushQueue(osMessageQueueId_t queueHandle) {
    uint32_t received_message;
    while (osMessageQueueGet(queueHandle, &received_message, NULL, 0) == osOK) {
        // Loop until the queue is empty
        printf("Flushing event: %lu\r\n", received_message);
    }
}


/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  MilliTimer_Handler(); //For MQTT

       //One second callback for DHCP Module
  	static uint16_t milli_count;
  	milli_count++;

  	if(milli_count==1000)
  	{
  	milli_count=0;
  	DHCP_time_handler();
  	DNS_time_handler();
  	}
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
