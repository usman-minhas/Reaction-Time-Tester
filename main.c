/****** 



1.The Fist Extern button (named extBtn1)  connected to PC1 (canot use PB1, for 429i-DISCO ,pb1 is used by LCD), ExtBtn1_Config()  //
		2014: canot use pin PB1, for 429i-DISCO ,pb1 is used by LCD. if use this pin, always interrupt by itself
					can not use pin PA1, used by gyro. if use this pin, never interrupt
					pd1----WILL ACT AS PC13, To trigger the RTC timestamp event
					....ONLY PC1 CAN BE USED TO FIRE EXTI1 !!!!
2. the Second external button (extBtn2) may connect to pin PD2.  ExtBtn2_Config() --The pin PB2 on the board have trouble.
    when connect extern button to the pin PB2, the voltage at this pin is not 3V as it is supposed to be, it is 0.3V, why?
		so changed to use pin PD2.
	
	 
		PA2: NOT OK. (USED BY LCD??)
		PB2: ??????
		PC2: ok, BUT sometimes (every 5 times around), press pc2 will trigger exti1, which is configured to use PC1. (is it because of using internal pull up pin config?)
		      however, press PC1 does not affect exti 2. sometimes press PC2 will also affect time stamp (PC13)
		PD2: OK,     
		PE2:  OK  (PE3, PE4 PE5 , seems has no other AF function, according to the table in manual for discovery board)
		PF2: NOT OK. (although PF2 is used by SDRAM, it affects LCD. press it, LCD will flick and displayed chars change to garbage)
		PG2: OK
		
	3. RTC and RTC_Alarm have been configured.
 
*/


/* Includes ------------------------------------------------------------------*/
#include "main.h"

//#include <stdio.h>
#include <math.h>


/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup GPIO_EXTI
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)



/* Private macro -------------------------------------------------------------*/

 

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef  I2c3_Handle;

RTC_HandleTypeDef RTCHandle;
RTC_DateTypeDef RTC_DateStructure;
RTC_TimeTypeDef RTC_TimeStructure;



HAL_StatusTypeDef Hal_status;  //HAL_ERROR, HAL_TIMEOUT, HAL_OK, of HAL_BUSY 


//memory location to write to in the device
__IO uint16_t memLocation = 0x000A; //pick any location within range
uint8_t hours, minutes, seconds, year, month, day, weekday;
uint8_t newHours = 0, newMinutes = 0, newSeconds = 0, newYear = 0, newMonth = 1, newDay = 1;

char lcd_buffer[14];



void RTC_Config(void);
void RTC_AlarmAConfig(void);



void ExtBtn1_Config(void);  //for the first External button
void ExtBtn2_Config(void);


//Variables
uint8_t showingPrev = 0;
uint8_t settingTime = 0;
uint8_t valToSet = 1;
uint32_t timePressed = 0;

//Function Declarations
void displayTime(int row);
void storeTime(void);
void displayPrevTimes(void);
void clearLCD(void);
void displayDate(int row);
char* convertMonth(uint8_t month);
char* convertDay(uint8_t day);
void displayValToBeSet(void);
void editVal(void);
void setDateAndTime(void);
void increaseYear(void);
void increaseMonth(void);
void increaseDay(void);
void increaseHour(void);
void increaseMinute(void);
void increaseSecond(void);
uint8_t daysInMonth(void);
void storeNewDateAndTime(void);
void setNewMonth(void);
void setNewWeekDay(void);
	
 

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  //the following variables are for testging I2C_EEPROM
	
	uint8_t data1 =0x67,  data2=0x68;
	uint8_t readData=0x00;
	char AA[34]= "efghijklmnopqstuvefghijklmnopqstuv";
	uint8_t * bufferdata=(uint8_t *)AA;	
	int i;
	uint8_t readMatch=1;
	uint32_t EE_status;

	
	/* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();
	
	 /* Configure the system clock to 180 MHz */
  SystemClock_Config();
	
	
	//Init Systic interrupt so can use HAL_Delay() function 
	HAL_InitTick(0x0000); // set systick's priority to the highest.
                        
  
	
	//Configure LED3 and LED4 ======================================
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);


	//configure the USER button as exti mode. 
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);   // BSP_functions in stm32f429i_discovery.c
	


	//Use this line, so do not need extiline0_config()
	//configure external push-buttons and interrupts
	
		
	ExtBtn1_Config();
	ExtBtn2_Config();
	
	
	
	
//-----------Init LCD 
	BSP_LCD_Init();
	//BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);   //LCD_FRAME_BUFFER, defined as 0xD0000000 in _discovery_lcd.h
														// the LayerIndex may be 0 and 1. if is 2, then the LCD is dark.
	//BSP_LCD_SelectLayer(uint32_t LayerIndex);
	BSP_LCD_SelectLayer(0);
	//BSP_LCD_SetLayerVisible(0, ENABLE); //do not need this line.
	BSP_LCD_Clear(LCD_COLOR_WHITE);  //need this line, otherwise, the screen is dark	
	BSP_LCD_DisplayOn();
 
	BSP_LCD_SetFont(&Font20);  //the default font,  LCD_DEFAULT_FONT, which is defined in _lcd.h, is Font24
	
	
	/**	LCD_DisplayString(5, 2, (uint8_t *)"MT3TA4 LAB 3");
		LCD_DisplayString(6, 2, (uint8_t *)"Testing EEPROM....");
		
		HAL_Delay(1000);   //display for 1 second
		
		//BSP_LCD_Clear(LCD_COLOR_WHITE);
		BSP_LCD_ClearStringLine(5);
		BSP_LCD_ClearStringLine(6);
		BSP_LCD_ClearStringLine(7);
		
		
		
	//	LCD_DisplayInt(7, 6, 123456789);
	//	LCD_DisplayFloat(8, 6, 12.3456789, 4);*/


	
	//configure real-time clock
	RTC_Config();
		
	RTC_AlarmAConfig();
	//RTC_AlarmA_IT_Disable(&RTCHandle);
	//RTC_AlarmA_IT_Enable(&RTCHandle);
	
	
	
//Init I2C for EEPROM		
	I2C_Init(&I2c3_Handle);



/*********************Testing I2C EEPROM------------------
	EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation , data1);
	if (EE_status==HAL_OK)
			LCD_DisplayString(0, 0, (uint8_t *)"w data1 OK");
	else
			LCD_DisplayString(0, 0, (uint8_t *)"w data1 failed");

	EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1 , data2);
	if (EE_status==HAL_OK)
			LCD_DisplayString(1, 0, (uint8_t *)"w data2 OK");
	else
			LCD_DisplayString(1, 0, (uint8_t *)"w data2 failed");
	
	
	
	
	readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
	if (data1 == readData) {
			LCD_DisplayString(3, 0, (uint8_t *)"r data1 success");
	}else{
			LCD_DisplayString(3, 0, (uint8_t *)"r data1 mismatch");
	}	
	LCD_DisplayInt(3, 14, readData);
	
	readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1);
	if (data2 == readData) {
			LCD_DisplayString(4, 0, (uint8_t *)"r data2 success");
	}else{
			LCD_DisplayString(4, 0, (uint8_t *)"r data2 mismatch");
	}	
	LCD_DisplayInt(4, 14, readData);
	

	EE_status=I2C_BufferWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation, bufferdata, 34);
	if (EE_status==HAL_OK)
		LCD_DisplayString(6, 0, (uint8_t *)"w buffer OK");
	else
		LCD_DisplayString(6, 0, (uint8_t *)"W buffer failed");

	for (i=0;i<=33;i++) { 
			readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+i);
			HAL_Delay(5);   // just for display effect. for EEPROM read, do not need dalay		
		//BUT :  if here delay longer time, the floowing display will have trouble,???
	
			BSP_LCD_DisplayChar(COLUMN(i%16),LINE(8+ 2*(int)(i/16)), (char) readData);	
			BSP_LCD_DisplayChar(COLUMN(i%16),LINE(9+ 2*(int)(i/16)),  bufferdata[i]);
			if (bufferdata[i]!=readData)
					readMatch=0;
	}

	if (readMatch==0)
		LCD_DisplayString(15, 0, (uint8_t *)"r buffer mismatch");
	else 
		LCD_DisplayString(15, 0, (uint8_t *)"r buffer success");

******************************testing I2C EEPROM*****************************/
	
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	
	
	/**HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation , RTC_TimeStructure.Hours);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 1, RTC_TimeStructure.Minutes);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 2, RTC_TimeStructure.Seconds);
	hours=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
	minutes=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 1);
	seconds=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 2);
	
	LCD_DisplayString(7, 0, (uint8_t *) "0 :0 :0 ");
	LCD_DisplayInt(7, 1 - floor(log10(hours)), hours);
	LCD_DisplayInt(7, 4 - floor(log10(minutes)), minutes);
	LCD_DisplayInt(7, 7 - floor(log10(seconds)), seconds);**/
	

	
	
	LCD_DisplayString(0, 0, (uint8_t *) "Time: (HH:MM:SS)");

  /* Infinite loop */

while (1)
  {			

		//LCD_DisplayInt(15, 0, n);
		//n++;
  } //end of while


}  

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
 
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */




/**
 * Use this function to configure the GPIO to handle input from
 * external pushbuttons and configure them so that you will handle
 * them through external interrupts.
 */
void ExtBtn1_Config(void)     // for GPIO C pin 1
// can only use PA0, PB0... to PA4, PB4 .... because only  only  EXTI0, ...EXTI4,on which the 
	//mentioned pins are mapped to, are connected INDIVIDUALLY to NVIC. the others are grouped! 
		//see stm32f4xx.h, there is EXTI0_IRQn...EXTI4_IRQn, EXTI15_10_IRQn defined
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_1;
	//GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void ExtBtn2_Config(void){  //**********PD2.***********
	
	
 //Enable GPIOD Clock
	GPIO_InitTypeDef   GPIO_InitStructure;
	__HAL_RCC_GPIOD_CLK_ENABLE();
	
	//Configure PD2 as IT Falling
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_2);
	
	//Enable interrupt
	HAL_NVIC_SetPriority(EXTI2_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	
	
	
}


void RTC_Config(void) {
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	/****************
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RTC_AlarmTypeDef RTC_AlarmStructure;
	****************/
	
	//1: Enable the RTC domain access (enable wirte access to the RTC )
			//1.1: Enable the Power Controller (PWR) APB1 interface clock:
        __HAL_RCC_PWR_CLK_ENABLE(); 
			//1.2:  Enable access to RTC domain 
				HAL_PWR_EnableBkUpAccess();
			//1.3: Select the RTC clock source
				__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);  //RCC_RTCCLKSOURCE_LSI is defined in hal_rcc.h
	       // according to P9 of AN3371 Application Note, LSI's accuracy is not suitable for RTC application!!!! 
					//can not use LSE!!!---LSE is not available, at leaset not available for stm32f407 board.
				//****"Without parts at X3, C16, C27, and removing SB15 and SB16, the LSE is not going to tick or come ready"*****.
			//1.4: Enable RTC Clock
			__HAL_RCC_RTC_ENABLE();   //enable RTC
			
	
			//1.5  Enable LSI
			__HAL_RCC_LSI_ENABLE();   //need to enable the LSI !!!
																//defined in _rcc.c
			while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY)==RESET) {}    //defind in rcc.c
	
			// for the above steps, please see the CubeHal UM1725, p616, section "Backup Domain Access" 	
				
				
				
	//2.  Configure the RTC Prescaler (Asynchronous and Synchronous) and RTC hour 
        
				RTCHandle.Instance = RTC;
				RTCHandle.Init.HourFormat = RTC_HOURFORMAT_24;
				//RTC time base frequency =LSE/((AsynchPreDiv+1)*(SynchPreDiv+1))=1Hz
				//see the AN3371 Application Note: if LSE=32.768K, PreDiv_A=127, Prediv_S=255
				//    if LSI=32K, PreDiv_A=127, Prediv_S=249
				//also in the note: LSI accuracy is not suitable for calendar application!!!!!! 
				RTCHandle.Init.AsynchPrediv = 127; //if using LSE: Asyn=127, Asyn=255: 
				RTCHandle.Init.SynchPrediv = 249;  //if using LSI(32Khz): Asyn=127, Asyn=249: 
				// but the UM1725 says: to set the Asyn Prescaler a higher value can mimimize power comsumption
				
				RTCHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
				RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
				RTCHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
				
				//HAL_RTC_Init(); 
				if(HAL_RTC_Init(&RTCHandle) != HAL_OK)
				{
						LCD_DisplayString(1, 0, (uint8_t *)"RTC Init Error!");
				}
	
	//3. init the time and date
				RTC_DateStructure.Year = 15;
				RTC_DateStructure.Month = RTC_MONTH_JANUARY;
				RTC_DateStructure.Date = 18; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x18 for the 18th.
				RTC_DateStructure.WeekDay = RTC_WEEKDAY_FRIDAY; //???  if the real weekday is not correct for the given date, still set as 
																												//what is specified here.
				
				if(HAL_RTC_SetDate(&RTCHandle,&RTC_DateStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better 
															//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(2, 0, (uint8_t *)"Date Init Error!");
				} 
  
  
				RTC_TimeStructure.Hours = 23;  
				RTC_TimeStructure.Minutes = 59; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x19
				RTC_TimeStructure.Seconds = 29;
				RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
				RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
				RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;//?????/
				
				if(HAL_RTC_SetTime(&RTCHandle,&RTC_TimeStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better
																																					//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(3, 0, (uint8_t *)"TIME Init Error!");
				}
  
			//Writes a data in a RTC Backup data Register0   --why need this line?
			//HAL_RTCEx_BKUPWrite(&RTCHandle,RTC_BKP_DR0,0x32F2);   

	/*
			//The RTC Resynchronization mode is write protected, use the
			//__HAL_RTC_WRITEPROTECTION_DISABLE() befor calling this function.
			__HAL_RTC_WRITEPROTECTION_DISABLE(&RTCHandle);
			//wait for RTC APB registers synchronisation
			HAL_RTC_WaitForSynchro(&RTCHandle);
			__HAL_RTC_WRITEPROTECTION_ENABLE(&RTCHandle);				
	 */
				
				
			__HAL_RTC_TAMPER1_DISABLE(&RTCHandle);
			__HAL_RTC_TAMPER2_DISABLE(&RTCHandle);	
				//Optionally, a tamper event can cause a timestamp to be recorded. ---P802 of RM0090
				//Timestamp on tamper event
				//With TAMPTS set to ‘1 , any tamper event causes a timestamp to occur. In this case, either
				//the TSF bit or the TSOVF bit are set in RTC_ISR, in the same manner as if a normal
				//timestamp event occurs. The affected tamper flag register (TAMP1F, TAMP2F) is set at the
				//same time that TSF or TSOVF is set. ---P802, about Tamper detection
				//-------that is why need to disable this two tamper interrupts. Before disable these two, when program start, there is always a timestamp interrupt.
				//----also, these two disable function can not be put in the TSConfig().---put there will make  the program freezed when start. the possible reason is
				//-----one the RTC is configured, changing the control register again need to lock and unlock RTC and disable write protection.---See Alarm disable/Enable 
				//---function.
				
			HAL_RTC_WaitForSynchro(&RTCHandle);	
			//To read the calendar through the shadow registers after Calendar initialization,
			//		calendar update or after wake-up from low power modes the software must first clear
			//the RSF flag. The software must then wait until it is set again before reading the
			//calendar, which means that the calendar registers have been correctly copied into the
			//RTC_TR and RTC_DR shadow registers.The HAL_RTC_WaitForSynchro() function
			//implements the above software sequence (RSF clear and RSF check).	
}


void RTC_AlarmAConfig(void)
{
	RTC_AlarmTypeDef RTC_Alarm_Structure;

	RTC_Alarm_Structure.Alarm = RTC_ALARM_A;
  RTC_Alarm_Structure.AlarmMask = RTC_ALARMMASK_ALL;
				// See reference manual. especially 
				//p11-12 of AN3371 Application Note.
				// this mask mean alarm occurs every second.
				//if MaskAll, the other 3 fieds of the AlarmStructure do not need to be initiated
				//the other three fieds are: RTC_AlarmTime(for when to occur), 
				//RTC_AlarmDateWeekDaySel (to use RTC_AlarmDateWeekDaySel_Date or RTC_AlarmDateWeekDaySel_WeekDay
				//RTC_AlarmDateWeekDay (0-31, or RTC_Weekday_Monday, RTC_Weekday_Tuesday...., depends on the value of AlarmDateWeekDaySel)	
	//RTC_Alarm_Structure.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
  //RTC_Alarm_Structure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  //RTC_Alarm_Structure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
		   //RTC_ALARMSUBSECONDMASK_ALL --> All Alarm SS fields are masked. 
        //There is no comparison on sub seconds for Alarm 
			
  //RTC_Alarm_Structure.AlarmTime.Hours = 0x02;
  //RTC_Alarm_Structure.AlarmTime.Minutes = 0x20;
  //RTC_Alarm_Structure.AlarmTime.Seconds = 0x30;
  //RTC_Alarm_Structure.AlarmTime.SubSeconds = 0x56;
  
  if(HAL_RTC_SetAlarm_IT(&RTCHandle,&RTC_Alarm_Structure,RTC_FORMAT_BCD) != HAL_OK)
  {
			LCD_DisplayString(4, 0, (uint8_t *)"Alarm setup Error!");
  }
  
	//Enable the RTC Alarm interrupt
//	__HAL_RTC_ALARM_ENABLE_IT(&RTCHandle,RTC_IT_ALRA);   //already in function HAL_RTC_SetAlarm_IT()
	
	//Enable the RTC ALARMA peripheral.
//	__HAL_RTC_ALARMA_ENABLE(&RTCHandle);  //already in function HAL_RTC_SetAlarm_IT()
	
	__HAL_RTC_ALARM_CLEAR_FLAG(&RTCHandle, RTC_FLAG_ALRAF); //need it? !!!!, without it, sometimes(SOMETIMES, when first time to use the alarm interrupt)
																			//the interrupt handler will not work!!! 		
	

	
		//need to set/enable the NVIC for RTC_Alarm_IRQn!!!!
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);   
	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x00, 0);  //not important
	
				
	
}


HAL_StatusTypeDef  RTC_AlarmA_IT_Disable(RTC_HandleTypeDef *hrtc) 
{ 
 	// Process Locked  
	__HAL_LOCK(hrtc);
  
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_DISABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  
}


HAL_StatusTypeDef  RTC_AlarmA_IT_Enable(RTC_HandleTypeDef *hrtc) 
{	
	// Process Locked  
	__HAL_LOCK(hrtc);	
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_ENABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_ENABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  

}







void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		while (*ptr!=NULL)
    {
				BSP_LCD_DisplayChar(COLUMN(ColumnNumber),LINE(LineNumber), *ptr); //new version of this function need Xpos first. so COLUMN() is the first para.
				ColumnNumber++;
			 //to avoid wrapping on the same line and replacing chars 
				if ((ColumnNumber+1)*(((sFONT *)BSP_LCD_GetFont())->Width)>=BSP_LCD_GetXSize() ){
					ColumnNumber=0;
					LineNumber++;
				}
					
				ptr++;
		}
}

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		
		sprintf(lcd_buffer,"%.*f",DigitAfterDecimalPoint, Number);  //6 digits after decimal point, this is also the default setting for Keil uVision 4.74 environment.
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}



/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
  if(GPIO_Pin == KEY_BUTTON_PIN)  //GPIO_PIN_0
  {

		//Only work if not in set time mode
		if(!settingTime){
			
			//Check if held for more than 1 second
			timePressed = HAL_GetTick();
			while(BSP_PB_GetState(BUTTON_KEY)){}
			
			if((HAL_GetTick() - timePressed) > 1000)
			{
				//If held display current time and date
				clearLCD();
				showingPrev = 0;
				LCD_DisplayString(7, 0, (uint8_t *) "Current Time:");
				displayTime(8);
				LCD_DisplayString(9, 0, (uint8_t *) "Current Date:");
				displayDate(10);
			}
			else
			{
				//Store time if pressed
				storeTime();
				clearLCD();
				showingPrev = 0;
				LCD_DisplayString(7, 0, (uint8_t *) "Time Stored:");
				displayTime(8);
				displayDate(9);
			}
		}
		
  }
	
	
	if(GPIO_Pin == GPIO_PIN_1) //External button 1
  {
		
		if(!settingTime){//If not in set time mode
			if(!showingPrev)
			{
				//display previous 2 times
				displayPrevTimes();
				showingPrev = 1;
			}
			else
			{
				//If showing previous two, stop showing
				clearLCD();
				showingPrev = 0;
			}
		}
		else{
			//If setting time, update value
			clearLCD();
			editVal();
		}
		
	}  //end of PIN_1

	
	if(GPIO_Pin == GPIO_PIN_2)//External button 2
  {
		
		//If not in set time mode, enter set time mode
		if(!settingTime){
			showingPrev = 0;
			settingTime = 1;
			clearLCD();
			displayValToBeSet();
		}
		else{
			//Change value to set if in set time mode
			valToSet++;
			clearLCD();
			displayValToBeSet();
		}
		
	} //end of if PIN_2	
	
	
}


void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
 //update time
	displayTime(1);
}


void storeNewDateAndTime(void){//Store the new date and time
	
	//Store Date
	RTC_DateStructure.Year = newYear;
	setNewMonth();
	RTC_DateStructure.Date = newDay;
	setNewWeekDay();
	
	//Store Time
	RTC_TimeStructure.Hours = newHours;
	RTC_TimeStructure.Minutes = newMinutes;
	RTC_TimeStructure.Seconds = newSeconds;
	
	//Update RTC
	HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
}


void setNewWeekDay(void){//Determines weekday and sets
	
	//Determines what day it was on that date
	uint8_t weekday = (newDay += newMonth < 3 ? newYear-- : newYear - 2, 23*newMonth/9 + newDay + 4 + newYear/4- newYear/100 + newYear/400)%7;
	
	switch (weekday){//Stores depending on result
		case 0:{
			RTC_DateStructure.WeekDay = RTC_WEEKDAY_SUNDAY;
			break;
		}
		case 1:{
			RTC_DateStructure.WeekDay = RTC_WEEKDAY_MONDAY;
			break;
		}
		case 2:{
			RTC_DateStructure.WeekDay = RTC_WEEKDAY_TUESDAY;
			break;
		}
		case 3:{
			RTC_DateStructure.WeekDay = RTC_WEEKDAY_WEDNESDAY;
			break;
		}
		case 4:{
			RTC_DateStructure.WeekDay = RTC_WEEKDAY_THURSDAY;
			break;
		}
		case 5:{
			RTC_DateStructure.WeekDay = RTC_WEEKDAY_FRIDAY;
			break;
		}
		case 6:{
			RTC_DateStructure.WeekDay = RTC_WEEKDAY_SATURDAY;
			break;
		}
	}
	
}

void setNewMonth(void){//Sets the new month using the RTC values
	
	switch (newMonth){
		case 1:{
			RTC_DateStructure.Month = RTC_MONTH_JANUARY;
			break;
		}
		case 2:{
			RTC_DateStructure.Month = RTC_MONTH_FEBRUARY;
			break;
		}
		case 3:{
			RTC_DateStructure.Month = RTC_MONTH_MARCH;
			break;
		}
		case 4:{
			RTC_DateStructure.Month = RTC_MONTH_APRIL;
			break;
		}
		case 5:{
			RTC_DateStructure.Month = RTC_MONTH_MAY;
			break;
		}
		case 6:{
			RTC_DateStructure.Month = RTC_MONTH_JUNE;
			break;
		}
		case 7:{
			RTC_DateStructure.Month = RTC_MONTH_JULY;
			break;
		}
		case 8:{
			RTC_DateStructure.Month = RTC_MONTH_AUGUST;
			break;
		}
		case 9:{
			RTC_DateStructure.Month = RTC_MONTH_SEPTEMBER;
			break;
		}
		case 10:{
			RTC_DateStructure.Month = RTC_MONTH_OCTOBER;
			break;
		}
		case 11:{
			RTC_DateStructure.Month = RTC_MONTH_NOVEMBER;
			break;
		}
		case 12:{
			RTC_DateStructure.Month = RTC_MONTH_DECEMBER;
			break;
		}
	}
}
void editVal(void){//Edits the value that is currently selected
	
	switch (valToSet){ //Increases the value currently displayed
		case 1:{
			increaseYear();
			break;
		}
		case 2:{
			increaseMonth();
			break;
		}
		case 3:{
			increaseDay();
			break;
		}
		case 4:{
			increaseHour();
			break;
		}
		case 5:{
			increaseMinute();
			break;
		}
		case 6:{
			increaseSecond();
			break;
		}
	}

}

void increaseYear(void){//Increase the year
	
	if(newYear < 99){//RTC max 99, if greater start back at 0
		newYear++;
	}
	else{
		newYear = 0;
	}
	
	displayValToBeSet();
}

void increaseMonth(void){//Increase Month
	
	if(newMonth < 12){//Update month, if over 12 return to 1
		newMonth++;
	}
	else{
		newMonth = 1;
	}
	
	displayValToBeSet();
}

void increaseDay(void){//Increase Day
	
	if(newDay < daysInMonth()){//Increase day as long as less than days in month, otherwise return to 1
		newDay++;
	}
	else{
		newDay = 1;
	}
	
	displayValToBeSet();
}

void increaseHour(void){//Increase hour
	
	if(newHours < 23){//Increase hour as long as less than 24
		newHours++;
	}
	else{
		newHours = 0;
	}
	
	displayValToBeSet();
}

void increaseMinute(void){//Increase Minute
	
	if(newMinutes < 59){//Increase minute as long as less than 60
		newMinutes++;
	}
	else{
		newMinutes = 0;
	}
	
	displayValToBeSet();
}

void increaseSecond(void){//Increase second
	
	if(newSeconds < 59){//Increase second as long as less than 60
		newSeconds++;
	}
	else{
		newSeconds = 0;
	}
	
	displayValToBeSet();
}

uint8_t daysInMonth(){//Determines the days in a month (including leap years)
	
	//Months with 31 Days
	if(newMonth == 1 || newMonth == 3 || newMonth == 5 || newMonth == 7 || newMonth == 8 ||  newMonth == 10 ||  newMonth == 12)
	{
		return 31;
	}
	//Months with 30 Days
	else if(newMonth == 4 ||  newMonth == 6 || newMonth == 9 || newMonth == 11){
		return 30;
	}
	//February and check for leap year
	else{
		if((newYear%400==0) || ((newYear%4==0) && (newYear%100!=0))){
			return 29;
		}
		else{
			return 28;
		}
	}

}

void displayValToBeSet(void){//Display the value currenrlty being edited
	
		switch (valToSet){
		case 1:{
			LCD_DisplayString(6, 0, (uint8_t *) "Year:");
			LCD_DisplayInt(7, 0, 2000 + newYear);
			break;
		}
		case 2:{
			LCD_DisplayString(6, 0, (uint8_t *) "Month:");
			LCD_DisplayString(7, 0, (uint8_t *) convertMonth(newMonth));
			break;
		}
		case 3:{
			LCD_DisplayString(6, 0, (uint8_t *) "Day:");
			LCD_DisplayInt(7, 0, newDay);
			break;
		}
		case 4:{
			LCD_DisplayString(6, 0, (uint8_t *) "Hour:");
			LCD_DisplayInt(7, 0, newHours);
			break;
		}
		case 5:{
			LCD_DisplayString(6, 0, (uint8_t *) "Minute:");
			LCD_DisplayInt(7, 0, newMinutes);
			break;
		}
		case 6:{
			LCD_DisplayString(6, 0, (uint8_t *) "Seconds:");
			LCD_DisplayInt(7, 0, newSeconds);
			break;
		}
		default:{//If surpasses last value, set date and time and reset values
			storeNewDateAndTime();
			settingTime = 0;
			valToSet = 1;
			newDay = 1;
			newHours = 0;
			newMinutes = 0;
			newMonth = 1;
			newSeconds = 0;
			newYear = 0;
			break;
		}
	}
}


void displayTime(int row){//Displays the time
	
	//Get Time
	HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	
	//Display Time
	LCD_DisplayString(row, 0, (uint8_t *) "0 :0 :0 ");
	LCD_DisplayInt(row, 1 - floor(log10(RTC_TimeStructure.Hours)), RTC_TimeStructure.Hours);
	LCD_DisplayInt(row, 4 - floor(log10(RTC_TimeStructure.Minutes)), RTC_TimeStructure.Minutes);
	LCD_DisplayInt(row, 7 - floor(log10(RTC_TimeStructure.Seconds)), RTC_TimeStructure.Seconds);
}

void displayDate(int row){//Displays the date
	
	//Get Date
	HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	
	//Display Date
	LCD_DisplayString(row, 0, (uint8_t *) convertDay(RTC_DateStructure.WeekDay));
	LCD_DisplayString(row, 3, (uint8_t *) " ");
	LCD_DisplayString(row, 4, (uint8_t *) convertMonth(RTC_DateStructure.Month));
	LCD_DisplayString(row, 7, (uint8_t *) " ");
	LCD_DisplayInt(row, 8, RTC_DateStructure.Date);
	LCD_DisplayString(row, 10, (uint8_t *) ", ");
	LCD_DisplayInt(row, 12, RTC_DateStructure.Year + 2000);
	
}

char* convertDay(uint8_t day){//COnverts the weekday int to a string
	
	switch (day){
		case 1:
			return "Mon";
			break;
		case 2:
			return "Tue";
			break;
		case 3:
			return "Wed";
			break;
		case 4:
			return "Thu";
			break;
		case 5:
			return "Fri";
			break;
		case 6:
			return "Sat";
			break;
		case 7:
			return "Sun";
			break;
	}
	
}

char* convertMonth(uint8_t month){//Converts the month int to a string
	
	switch (month){
		case 1:
			return "Jan";
			break;
		case 2:
			return "Feb";
			break;
		case 3:
			return "Mar";
			break;
		case 4:
			return "Apr";
			break;
		case 5:
			return "May";
			break;
		case 6:
			return "Jun";
			break;
		case 7:
			return "Jul";
			break;
		case 8:
			return "Aug";
			break;
		case 9:
			return "Sep";
			break;
		case 10:
			return "Oct";
			break;
		case 11:
			return "Nov";
			break;
		case 12:
			return "Dec";
			break;
		
	}
	
}

void storeTime(void){//Stores previous 2 times along with dates
	
	
	//Get Current time
	HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	
	//Move last time to next memory location
	hours=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
	minutes=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 1);
	seconds=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 2);
	year=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 3);
	month=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 4);
	day=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 5);
	weekday=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 6);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 7, hours);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 8, minutes);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 9, seconds);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 10, year);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 11, month);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 12, day);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 13, weekday);
	
	//Write new time
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation, RTC_TimeStructure.Hours);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 1, RTC_TimeStructure.Minutes);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 2, RTC_TimeStructure.Seconds);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 3, RTC_DateStructure.Year);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 4, RTC_DateStructure.Month);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 5, RTC_DateStructure.Date);
	I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 6, RTC_DateStructure.WeekDay);
	
}

void displayPrevTimes(void){//Displays previous two times with dates
	
	clearLCD();
	
	//Get Previous Time 1
	hours=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
	minutes=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 1);
	seconds=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 2);
	year=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 3);
	month=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 4);
	day=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 5);
	weekday=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 6);
	
	LCD_DisplayString(6, 0, (uint8_t *) "Last Time:");
	LCD_DisplayString(7, 0, (uint8_t *) "0 :0 :0 ");
	LCD_DisplayInt(7, 1 - floor(log10(hours)), hours);
	LCD_DisplayInt(7, 4 - floor(log10(minutes)), minutes);
	LCD_DisplayInt(7, 7 - floor(log10(seconds)), seconds);
	LCD_DisplayString(8, 0, (uint8_t *) convertDay(weekday));
	LCD_DisplayString(8, 3, (uint8_t *) " ");
	LCD_DisplayString(8, 4, (uint8_t *) convertMonth(month));
	LCD_DisplayString(8, 7, (uint8_t *) " ");
	LCD_DisplayInt(8, 8, day);
	LCD_DisplayString(8, 10, (uint8_t *) ", ");
	LCD_DisplayInt(8, 12, year + 2000);
	
	//Get Previous Time 2
	hours=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 7);
	minutes=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 8);
	seconds=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 9);
	year=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 10);
	month=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 11);
	day=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 12);
	weekday=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation + 13);
	
	LCD_DisplayString(10, 0, (uint8_t *) "2nd Last Time:");
	LCD_DisplayString(11, 0, (uint8_t *) "0 :0 :0 ");
	LCD_DisplayInt(11, 1 - floor(log10(hours)), hours);
	LCD_DisplayInt(11, 4 - floor(log10(minutes)), minutes);
	LCD_DisplayInt(11, 7 - floor(log10(seconds)), seconds);
	LCD_DisplayString(12, 0, (uint8_t *) convertDay(weekday));
	LCD_DisplayString(12, 3, (uint8_t *) " ");
	LCD_DisplayString(12, 4, (uint8_t *) convertMonth(month));
	LCD_DisplayString(12, 7, (uint8_t *) " ");
	LCD_DisplayInt(12, 8, day);
	LCD_DisplayString(12, 10, (uint8_t *) ", ");
	LCD_DisplayInt(12, 12, year + 2000);
}

void clearLCD(void){//Clears LCD passed current time display
	
	for(uint8_t n = 2; n <= 15; n++){
		BSP_LCD_ClearStringLine(n);
	}
	
}




/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
}





#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
