/*#############################################################
File Name       : main.c
Author          : Grant Phillips
Date Modified   : 10/04/2014
Compiler        : Keil ARM-MDK (uVision V4.70.0.0)
Tested On       : STM32F3-Discovery

Description     : Example program that uses the STM32F3-Discovery 
									board as a Virtual COM Port (VCP) module to 
									write "Hello World!" serially to a PC's COM 
									port and then display a count from 0 to 255 
									repeatedly.  Run a program called Termite 
									(http://www.compuphase.com/software_termite.htm) 
									to use as a RS-232 terminal on the PC to see 
									what messages are sent to the PC, and to send 
									messages to the STM32F3-Discovery.      

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

/* #defines --------------------------------------------------*/

/* #function prototypes --------------------------------------*/

void init(void);
void ReadOrientation(float *pHeading, float *pRoll, float *pPitch);

/* #private macro ---------------------------------------------*/
#define ABS(x)         (x < 0) ? (-x) : x

#define L3G_Sensitivity_250dps     (float)   114.285f         /*!< gyroscope sensitivity with 250 dps full scale [LSB/dps] */
#define L3G_Sensitivity_500dps     (float)    57.1429f        /*!< gyroscope sensitivity with 500 dps full scale [LSB/dps] */
#define L3G_Sensitivity_2000dps    (float)    14.285f	      /*!< gyroscope sensitivity with 2000 dps full scale [LSB/dps] */
#define PI                         (float)     3.14159265f
#define alpha											 (float)		0.5

#define LSM_Acc_Sensitivity_2g     (float)     1.0f            /*!< accelerometer sensitivity with 2 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_4g     (float)     0.5f            /*!< accelerometer sensitivity with 4 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_8g     (float)     0.25f           /*!< accelerometer sensitivity with 8 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_16g    (float)     0.0834f         /*!< accelerometer sensitivity with 12 g full scale [LSB/mg] */


/* #global variables -----------------------------------------*/
GPIO_InitTypeDef        GPIO_InitStructure;											//structure used for setting up a GPIO port
RCC_ClocksTypeDef 			RCC_Clocks;															//structure used for setting up the SysTick Interrupt

/* #private variable -----------------------------------------*/
float MagBuffer[3] = {0.0f}, AccBuffer[3] = {0.0f}, Buffer[3] = {0.0f};
float Heading, Roll, Pitch;	
double fXg = 0, fYg = 0, fZg = 0;


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
		Delay(250);
	}
}




/*********************************************************************************************
Function name   : init
Author 					: Grant Phillips
Date Modified   : 07/05/2014
Compiler        : Keil ARM-MDK (uVision V4.70.0.0)

Description			: Initializes the LSM303DLHC module

Special Note(s) : NONE

Parameters			: NONE
Return value		: NONE
*********************************************************************************************/
void init(void)
{
	Demo_CompassConfig();		//use the original DEMO program's init function
}



/*********************************************************************************************
Function name   : ReadOrientation
Author 					: Grant Phillips
Modified by			:	Paul Mougin
Date Modified   : 07/05/2015
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
	float fNormAcc, fSinRoll, fCosRoll, fSinPitch, fCosPitch = 0.0f, RollAng = 0.0f, PitchAng = 0.0f;
	float fTiltedX, fTiltedY = 0.0f;
	float HeadingValue = 0.0f;
	
	Demo_CompassReadMag(MagBuffer);
  Demo_CompassReadAcc(AccBuffer);
      
  for(i=0;i<3;i++)
		AccBuffer[i] /= 100.0f;

	//Low Pass Filter
	fXg = AccBuffer[0] * alpha + (fXg * (1.0 - alpha));
	fYg = AccBuffer[1] * alpha + (fYg * (1.0 - alpha));
	fZg = AccBuffer[2] * alpha + (fZg * (1.0 - alpha));

  RollAng = (atan2f(-fYg, fZg)*180.0)/PI;
  PitchAng = (atan2f(fXg, sqrt(fYg*fYg + fZg*fZg))*180.0)/PI;
      
	fNormAcc = sqrt((AccBuffer[0]*AccBuffer[0])+(AccBuffer[1]*AccBuffer[1])+(AccBuffer[2]*AccBuffer[2]));
      
  fSinRoll = -AccBuffer[1]/fNormAcc;
  fCosRoll = sqrt(1.0-(fSinRoll * fSinRoll));
  fSinPitch = AccBuffer[0]/fNormAcc;
  fCosPitch = sqrt(1.0-(fSinPitch * fSinPitch));
	
  fTiltedX = MagBuffer[0]*fCosPitch+MagBuffer[2]*fSinPitch;
  fTiltedY = MagBuffer[0]*fSinRoll*fSinPitch+MagBuffer[1]*fCosRoll-MagBuffer[1]*fSinRoll*fCosPitch;
      
  HeadingValue = (float) ((atan2f((float)fTiltedY,(float)fTiltedX))*180)/PI;
 
  if (HeadingValue < 0)
  {
    HeadingValue = HeadingValue + 360;    
  }
	
	*pHeading	=	HeadingValue;
	*pRoll = RollAng;
	*pPitch = PitchAng;
}



/*********************************************************************************************
The following 3 functions were copied from the STM32F3-Discovery DEMO program 
and is the property of ST Microelectronics
*********************************************************************************************/


/**
  * @brief  Configure the Mems to compass application.
  * @param  None
  * @retval None
  */
void Demo_CompassConfig(void)
{
  LSM303DLHCMag_InitTypeDef LSM303DLHC_InitStructure;
  LSM303DLHCAcc_InitTypeDef LSM303DLHCAcc_InitStructure;
  LSM303DLHCAcc_FilterConfigTypeDef LSM303DLHCFilter_InitStructure;
  
  /* Configure MEMS magnetometer main parameters: temp, working mode, full Scale and Data rate */
  LSM303DLHC_InitStructure.Temperature_Sensor = LSM303DLHC_TEMPSENSOR_DISABLE;
  LSM303DLHC_InitStructure.MagOutput_DataRate =LSM303DLHC_ODR_30_HZ ;
  LSM303DLHC_InitStructure.MagFull_Scale = LSM303DLHC_FS_8_1_GA;
  LSM303DLHC_InitStructure.Working_Mode = LSM303DLHC_CONTINUOS_CONVERSION;
  LSM303DLHC_MagInit(&LSM303DLHC_InitStructure);
  
   /* Fill the accelerometer structure */
  LSM303DLHCAcc_InitStructure.Power_Mode = LSM303DLHC_NORMAL_MODE;
  LSM303DLHCAcc_InitStructure.AccOutput_DataRate = LSM303DLHC_ODR_50_HZ;
  LSM303DLHCAcc_InitStructure.Axes_Enable= LSM303DLHC_AXES_ENABLE;
  LSM303DLHCAcc_InitStructure.AccFull_Scale = LSM303DLHC_FULLSCALE_2G;
  LSM303DLHCAcc_InitStructure.BlockData_Update = LSM303DLHC_BlockUpdate_Continous;
  LSM303DLHCAcc_InitStructure.Endianness=LSM303DLHC_BLE_LSB;
  LSM303DLHCAcc_InitStructure.High_Resolution=LSM303DLHC_HR_ENABLE;
  /* Configure the accelerometer main parameters */
  LSM303DLHC_AccInit(&LSM303DLHCAcc_InitStructure);
  
  /* Fill the accelerometer LPF structure */
  LSM303DLHCFilter_InitStructure.HighPassFilter_Mode_Selection =LSM303DLHC_HPM_NORMAL_MODE;
  LSM303DLHCFilter_InitStructure.HighPassFilter_CutOff_Frequency = LSM303DLHC_HPFCF_16;
  LSM303DLHCFilter_InitStructure.HighPassFilter_AOI1 = LSM303DLHC_HPF_AOI1_DISABLE;
  LSM303DLHCFilter_InitStructure.HighPassFilter_AOI2 = LSM303DLHC_HPF_AOI2_DISABLE;

  /* Configure the accelerometer LPF main parameters */
  LSM303DLHC_AccFilterConfig(&LSM303DLHCFilter_InitStructure);
}



/**
* @brief Read LSM303DLHC output register, and calculate the acceleration ACC=(1/SENSITIVITY)* (out_h*256+out_l)/16 (12 bit rappresentation)
* @param pnData: pointer to float buffer where to store data
* @retval None
*/
void Demo_CompassReadAcc(float* pfData)
{
  int16_t pnRawData[3];
  uint8_t ctrlx[2];
  uint8_t buffer[6], cDivider;
  uint8_t i = 0;
  float LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_2g;
  
  /* Read the register content */
  LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx,2);
  LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_OUT_X_L_A, buffer, 6);
   
  if(ctrlx[1]&0x40)
    cDivider=64;
  else
    cDivider=16;

  /* check in the control register4 the data alignment*/
  if(!(ctrlx[0] & 0x40) || (ctrlx[1] & 0x40)) /* Little Endian Mode or FIFO mode */
  {
    for(i=0; i<3; i++)
    {
      pnRawData[i]=((int16_t)((uint16_t)buffer[2*i+1] << 8) + buffer[2*i])/cDivider;
    }
  }
  else /* Big Endian Mode */
  {
    for(i=0; i<3; i++)
      pnRawData[i]=((int16_t)((uint16_t)buffer[2*i] << 8) + buffer[2*i+1])/cDivider;
  }
  /* Read the register content */
  LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx,2);


  if(ctrlx[1]&0x40)
  {
    /* FIFO mode */
    LSM_Acc_Sensitivity = 0.25;
  }
  else
  {
    /* normal mode */
    /* switch the sensitivity value set in the CRTL4*/
    switch(ctrlx[0] & 0x30)
    {
    case LSM303DLHC_FULLSCALE_2G:
      LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_2g;
      break;
    case LSM303DLHC_FULLSCALE_4G:
      LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_4g;
      break;
    case LSM303DLHC_FULLSCALE_8G:
      LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_8g;
      break;
    case LSM303DLHC_FULLSCALE_16G:
      LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_16g;
      break;
    }
  }

  /* Obtain the mg value for the three axis */
  for(i=0; i<3; i++)
  {
    pfData[i]=(float)pnRawData[i]/LSM_Acc_Sensitivity;
  }

}



/**
  * @brief  calculate the magnetic field Magn.
* @param  pfData: pointer to the data out
  * @retval None
  */
void Demo_CompassReadMag (float* pfData)
{
  static uint8_t buffer[6] = {0};
  uint8_t CTRLB = 0;
  uint16_t Magn_Sensitivity_XY = 0, Magn_Sensitivity_Z = 0;
  uint8_t i =0;
  LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_CRB_REG_M, &CTRLB, 1);
  
  LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_H_M, buffer, 1);
  LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_L_M, buffer+1, 1);
  LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Y_H_M, buffer+2, 1);
  LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Y_L_M, buffer+3, 1);
  LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Z_H_M, buffer+4, 1);
  LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Z_L_M, buffer+5, 1);
  /* Switch the sensitivity set in the CRTLB*/
  switch(CTRLB & 0xE0)
  {
  case LSM303DLHC_FS_1_3_GA:
    Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_1_3Ga;
    Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_1_3Ga;
    break;
  case LSM303DLHC_FS_1_9_GA:
    Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_1_9Ga;
    Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_1_9Ga;
    break;
  case LSM303DLHC_FS_2_5_GA:
    Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_2_5Ga;
    Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_2_5Ga;
    break;
  case LSM303DLHC_FS_4_0_GA:
    Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_4Ga;
    Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_4Ga;
    break;
  case LSM303DLHC_FS_4_7_GA:
    Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_4_7Ga;
    Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_4_7Ga;
    break;
  case LSM303DLHC_FS_5_6_GA:
    Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_5_6Ga;
    Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_5_6Ga;
    break;
  case LSM303DLHC_FS_8_1_GA:
    Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_8_1Ga;
    Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_8_1Ga;
    break;
  }
  
  for(i=0; i<2; i++)
  {
    pfData[i]=(float)((int16_t)(((uint16_t)buffer[2*i] << 8) + buffer[2*i+1])*1000)/Magn_Sensitivity_XY;
  }
  pfData[2]=(float)((int16_t)(((uint16_t)buffer[4] << 8) + buffer[5])*1000)/Magn_Sensitivity_Z;
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

// Unused functions that have to be included to ensure correct compiling
// ###### DO NOT CHANGE ######
// =======================================================================
uint32_t L3GD20_TIMEOUT_UserCallback(void)
{
  return 0;
}

uint32_t LSM303DLHC_TIMEOUT_UserCallback(void)
{
  return 0;
}
// =======================================================================



