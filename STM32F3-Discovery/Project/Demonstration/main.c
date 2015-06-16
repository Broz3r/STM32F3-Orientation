/*#############################################################
File Name       : main.c
Author          : Paul Mougin
First author		: Grant Phillips
Date Modified   : 16/06/2015
Compiler        : Keil ARM-MDK (uVision V4.70.0.0)
Tested On       : STM32F3-Discovery

Description     : Program to use the compass and gyroscope to 
									make an IMU.
									Data are send throw a serial port to a PC to 
									display values of angles and positions.

Requirements    : * STM32F3-Discovery Board
									* Make sure the VCP_F3.c file is in the 
                    same folder than the project
              
Circuit         : * Mini-USB cable from the User USB port on the
										STM32F3-Discovery to a USB port on a PC
									
See the STM32F3-Discovery User Manual (UM1570) for the block 
diagram of the STM32F303VCT6 processor (p13), a summary of
the GPIO connections (p21-29) and the schematic diagram (p31-34)

##############################################################*/

/* #includes -------------------------------------------------*/
#include "main.h"																								//main library to include for device drivers, peripheral drivers, etc.
#include "VCP_F3.c"																							//for the UART4 functions on the STM32F3-Discovery
#include "imu_devs.h"																						//lib for captors

/* #defines --------------------------------------------------*/

/* #function prototypes --------------------------------------*/

void init(void);
void ReadOrientation(float *pHeading, float *pRoll, float *pPitch);

/* #private macro ---------------------------------------------*/
#define ABS(x)         (x < 0) ? (-x) : x

#define PI                         (float)     3.14159265f
#define alpha											 (float)		0.5
	
#define DELAY											 (int)			250								/*!< loop delay in ms */


/* #global variables -----------------------------------------*/
GPIO_InitTypeDef        GPIO_InitStructure;											//structure used for setting up a GPIO port
RCC_ClocksTypeDef 			RCC_Clocks;															//structure used for setting up the SysTick Interrupt

/* #private variable -----------------------------------------*/
float MagBuffer[3] = {0.0f}, AccBuffer[3] = {0.0f}, Buffer[3] = {0.0f};
float Heading, Roll, Pitch;	
float fAccBuffer[3] = {0.0}; //Filtred value for the AccBuffer
float fMagBuffer[3] = {0.0}; //Filtred value for the MagBuffer
float fGyroBuffer[3] = {0.0}; //Filtred value for the GyroBuffer
float zeroAngRate[3];




// Unused global variables that have to be included to ensure correct compiling */
// ###### DO NOT CHANGE ######
// ===============================================================================
__IO uint32_t TimingDelay = 0;																	//used with the Delay function
__IO uint8_t DataReady = 0;
__IO uint32_t USBConnectTimeOut = 100;
__IO uint32_t UserButtonPressed = 0;
__IO uint8_t PrevXferComplete = 1;
// ===============================================================================



int main(void)
{
	char buf[40];																									//string variable of 40 characters
	
	/* Set the SysTick Interrupt to occur every 1ms) */
	RCC_GetClocksFreq(&RCC_Clocks);
	if (SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000))
		while(1);																										//will end up in this infinite loop if there was an error with Systick_Config
	
	VCP_ResetPort();																							//pull the USB D+ line low to disconnect the USB device from the PC
	Delay(500);																										//allow a few milliseconds before reconnecting
	VCP_Init();																										//initialize the STM32F3-Discovery as a Virtual COM Port - device gets re-detected by the PC
	
	/* Wait for the user to press the BLUE USER button
	to start the program.  This gives the user chance
	to first connect to the Virtual COM Port from an
	application like Termite */
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI); 
	while(UserButtonPressed != 1);																//wait until the button is pressed

	/* Initialize LEDs and User Button available on STM32F3-Discovery board */
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);
  STM_EVAL_LEDInit(LED7);
  STM_EVAL_LEDInit(LED8);
  STM_EVAL_LEDInit(LED9);
  STM_EVAL_LEDInit(LED10);
	
	/* Demo Gyroscope */
  init();
	
	while (1)
	{
		/* Wait for data ready */
		while(DataReady != 0x05)
		{}
		DataReady = 0x00;
		
		/* LEDs Off */
		STM_EVAL_LEDOff(LED3);
		STM_EVAL_LEDOff(LED6);
		STM_EVAL_LEDOff(LED7);
		STM_EVAL_LEDOff(LED4);
		STM_EVAL_LEDOff(LED10);
		STM_EVAL_LEDOff(LED8);
		STM_EVAL_LEDOff(LED9);
		STM_EVAL_LEDOff(LED5);
		
		/* Read Gyro Angular data */
		ReadOrientation(&Heading, &Roll, &Pitch);
		
		sprintf(buf, "X: %11.3f   Y: %11.3f   Z: %11.3f\n", Roll, Pitch, Heading);		//print characetrs to the buf string										
		VCP_PutStr(buf);																													//write a string variable to the VCP
		Delay(DELAY);
	}
}




/*********************************************************************************************
Function name   : init
Author 					: Paul Mougin
Date Modified   : 16/06/2015
Compiler        : Keil ARM-MDK (uVision V4.70.0.0)

Description			: Initializes the LSM303DLHC & L3GD20 modules

Special Note(s) : NONE

Parameters			: NONE
Return value		: NONE
*********************************************************************************************/
void init(void)
{
	InitAccAndMag();		//initialize the compass
	InitGyro();			//initialize the gyroscope
}



/*********************************************************************************************
Function name   : ReadOrientation
Author 					: Grant Phillips
Modified by			:	Paul Mougin
Date Modified   : 16/06/2015
Compiler        : Keil ARM-MDK (uVision V4.70.0.0)

Description			: Reads the heading value, roll angle and pitch angle

Special Note(s) : All the calculations in this function was done by ST Microelectronics

Parameters			: *pHeading		-	reference to variable for the heading value forced to 0
									*pRoll			-	reference to variable for the roll angle (-180.0 - 180.0)
									*pPitch			-	reference to variable for the pitch angle (-90.0 - 90.0)
Return value		: NONE
*********************************************************************************************/
void ReadOrientation(float *pHeading, float *pRoll, float *pPitch)
{
	uint8_t i = 0;
	float MagBuffer[3] = {0.0f}, AccBuffer[3] = {0.0f};
	float sinRoll, cosRoll, sinPitch, cosPitch = 0.0f, RollAng = 0.0f, PitchAng = 0.0f;
	float fTiltedX, fTiltedY = 0.0f;
	float HeadingValue = 0.0f;
	
	ReadMagnetometer(fMagBuffer);
  ReadAccelerometer(fAccBuffer);

//	//Low Pass Filter
//		//AccBuffer
//	for (i=0; i<3; i++)
//		fAccBuffer[i] = AccBuffer[i] * alpha + (fAccBuffer[i] * (1.0 - alpha));
//		//MagBuffer
//	for (i=0; i<3; i++)
//		fMagBuffer[i] = MagBuffer[i] * alpha + (fMagBuffer[i] * (1.0 - alpha));

  RollAng = atan2f(fAccBuffer[1], fAccBuffer[2]);
  PitchAng = -atan2f(fAccBuffer[0], sqrt(fAccBuffer[1]*fAccBuffer[1] + fAccBuffer[2]*fAccBuffer[2]));
      
  sinRoll = sinf(RollAng);
  cosRoll = cosf(RollAng);
  sinPitch = sinf(PitchAng);
  cosPitch = cosf(PitchAng);
	
  fTiltedX = fMagBuffer[0]*cosRoll+fMagBuffer[2]*sinPitch;
  fTiltedY = fMagBuffer[0]*sinRoll*sinPitch + fMagBuffer[1]*cosRoll - fMagBuffer[2]*sinRoll*cosPitch;
      
	HeadingValue = atan2f((float)fTiltedY,(float)fTiltedX);
	
	 // We cannot correct for tilt over 40 degrees with this alg, if the board is tilted as such, return 0.
  if(RollAng > 0.78f || RollAng < -0.78f || PitchAng > 0.78f || PitchAng < -0.78f)
  {
    HeadingValue = 0;
  }
 
	
	*pHeading	=	HeadingValue*180/PI;
	*pRoll = RollAng*180/PI;
	*pPitch = PitchAng*180/PI;
}

// -------------------------------------------------------------------------------

// Function to insert a timing delay of nTime
// ###### DO NOT CHANGE ######
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

// Function to Decrement the TimingDelay variable.
// ###### DO NOT CHANGE ######
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}



