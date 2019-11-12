/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f7xx_hal.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "devDesc.h"
#include "_BSP_/SystemClock.h"

#include "modEventLog/modEventLog.h"
#include "modEventLog/EventSPI.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
cJournal        Journal;                                                         // Журнал событий EventLog

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
//void MX_FREERTOS_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/


/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
void BAKUP_SRAM_Init(void);
/* USER CODE END 0 */

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //cJournal Journal;
    cJournal::t_journal_event_id currentEventID;
    cJournal::t_journal_time currentTime;
    cJournal::t_journal_date currentDate;
    float DataToSave = 0;
    cJournal::t_jornal_representation_type Report_type;
    cJournal::t_jornal_period ReportPeriod;
    cJournal::t_jornal_statistics Statistics = {0};
    cJournal::t_record Events[1750];
    uint32_t EvetRiseTimes = 1500;
    uint32_t tickstart = 0;
    uint32_t Timeout = 0;
    uint32_t Tick = 0;
    uint32_t i = 0;
    uint8_t StartStatCalc = 0;
    uint8_t WaitForStat = 0;
    uint8_t GetStat = 0;
    uint8_t GetRecords = 0;
    uint16_t index = 0;
    uint8_t AreaReturn = 0;
    uint8_t AreaData[cJournal::EEPROM_FreeAreaSize];
    uint32_t AreaI;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main(void)
{

    for(volatile unsigned long startPause = 0; startPause < 1000000; startPause++){}

    /* USER CODE BEGIN 1 */
    unsigned char ucTmp;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    currentTime.milliseconds    = 0;
    currentTime.seconds         = 0;
    currentTime.minutes         = 0;
    currentTime.hours           = 0;
    currentDate.day             = 1;
    currentDate.month           = 6;
    currentDate.year            = 41;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    /* USER CODE END 1 */

    /* MCU Configuration----------------------------------------------------------*/

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
    /* USER CODE BEGIN 2 */

    /* USER CODE END 2 */


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //// Инициализация EventLog
    MX_SPI4_Init();
    BAKUP_SRAM_Init(); // не нужен, т.к. инициализируется в БД тоже
    Journal.modEventLog_Init(BKPSRAM_BASE + 0x100);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        // Обработка логики работы журнала событий EventLog
        Journal.modEventLog_Handler();

    ///+++++++++++++++++++++journal TEST++++++++++++++++++++++++++++++++++++++++++
        if (Tick < EvetRiseTimes)
        {
                /* Timeout management */
            if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
            {
                currentTime.milliseconds    = 500;
                currentTime.seconds = 0;

                //currentDate.day = 28;

                currentTime.minutes++;
                if (currentTime.minutes > 59)
                {
                    currentTime.hours++;
                    currentTime.minutes = 0;
                }
                if (currentTime.hours > 23)
                {
                    currentDate.day++;
                    currentTime.hours = 0;
                }

                currentDate.month           = 6;
                currentDate.year            = 41;



                currentEventID = Journal.JRN_EV_CRASH_OVERCURRENT_PROTECTION;
                DataToSave = Tick;

                Journal.modEventLog_EventSave(currentEventID, currentTime, currentDate, DataToSave);

                currentEventID = Journal.JRN_EV_WORK_ACT_DI_3_STATE;
                //currentEventID = Journal.JRN_EV_CRASH_OVERCURRENT_PROTECTION;
    //            currentTime.milliseconds    = (Tick/10) % 1000;
    //            currentTime.seconds         = 1;
    //            currentTime.minutes         = Tick % 60;
    //            currentTime.hours           = Tick % 24;
    //            currentDate.day             = 29;
    //            currentDate.month           = 6;
    //            currentDate.year            = 41;
                if (DataToSave)
                  DataToSave = 0;
                else
                  DataToSave = 1;

                Journal.modEventLog_EventSave(currentEventID, currentTime, currentDate, DataToSave);

                currentEventID = Journal.JRN_EV_WORK_STATE_MOVE_TO_SAFE_POSITION;
                //currentEventID = Journal.JRN_EV_CRASH_OVERCURRENT_PROTECTION;
    //            currentTime.milliseconds    = (Tick/10) % 1000;
    //            currentTime.seconds         = 1;
    //            currentTime.minutes         = Tick % 60;
    //            currentTime.hours           = Tick % 24;
    //            currentDate.day             = 1;
    //            currentDate.month           = 7;
    //            currentDate.year            = 41;
                DataToSave = 100 + Tick;

                Journal.modEventLog_EventSave(currentEventID, currentTime, currentDate, DataToSave);

                tickstart = HAL_GetTick();
                Timeout = 350; //500  ms
                Tick++;
        ////////////////////////////////////////////////////////////////////////////////
            }
        }
        /**
         *     uint8_t StartStatCalc = 0;
        uint8_t WaitForStat = 0;
        uint8_t GetStat = 0;
        uint8_t GetRecords = 0;
         * @return
         */
        if (Tick == EvetRiseTimes)
        {
            StartStatCalc = 1;
            Tick++;
        }

        if (StartStatCalc)
        {
            /*            currentTime.milliseconds    = 5;
                currentTime.seconds         = 1;
                currentTime.minutes         = 1 + Tick + 7;
                currentTime.hours           = 7;
                currentDate.day             = 29;
                currentDate.month           = 6;
                currentDate.year            = 41;*/
    ////****************************************************************************
            ReportPeriod.period_begin_date.year         = 41;
            ReportPeriod.period_begin_date.month        = 6;
            ReportPeriod.period_begin_date.day          = 28;
            ReportPeriod.period_begin_time.hours        = 6;
            ReportPeriod.period_begin_time.minutes      = 59;
            ReportPeriod.period_begin_time.seconds      = 59;
            ReportPeriod.period_begin_time.milliseconds = 999;
    ///*****************************************************************************
            ReportPeriod.period_end_date.year           = 41;
            ReportPeriod.period_end_date.month          = 6;
            ReportPeriod.period_end_date.day            = 1;
            ReportPeriod.period_end_time.hours          = 1;
            ReportPeriod.period_end_time.minutes        = 30;
            ReportPeriod.period_end_time.seconds        = 30;
            ReportPeriod.period_end_time.milliseconds   = 0;

            //Report_type = Journal.REPRESENT_PERIOD_ALL;
            //Report_type = Journal.REPRESENT_PERIOD_CRASH;
            //Report_type = Journal.REPRESENT_PERIOD_WORK_ACT;
            //Report_type = Journal.REPRESENT_PERIOD_WORK_STATE;
            //Report_type = Journal.REPRESENT_PERIOD_WORK_ACT_CRASH;
            //Report_type = Journal.REPRESENT_PERIOD_WORK_ACT_WORK_STATE;
            Report_type = Journal.REPRESENT_PERIOD_WORK_STATE_CRASH;
    ///////-------------------------------------------------------------------------
            Journal.modEventLog_CalcStatistics(Report_type, ReportPeriod);
            StartStatCalc = 0;
            WaitForStat = 1;
        }

        if (WaitForStat)
        {
            if (Journal.modEventLog_CheckJournalReady() == Journal.StatisticsReady)
            {
                Journal.modEventLog_GetStatistics(&Statistics);
                GetStat = 1;
            }
    //        WaitForStat = 0;
        }

        if (GetStat)
        {
            if (Statistics.events_amount_all)
            {
                Journal.modEventLog_EventRead(&index, &Events[i]);
                i++;
            }
            GetStat = 0;
        }

    ///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    }

    /* USER CODE END 3 */
}


/* USER CODE BEGIN 4 */

void BAKUP_SRAM_Init(void)
{
    //Enable access to the backup domain
    HAL_PWR_EnableBkUpAccess();

    //Enable the PWR clock
    __HAL_RCC_PWR_CLK_ENABLE();

    //Enable backup SRAM Clock
    __HAL_RCC_BKPSRAM_CLK_ENABLE();
}

/* USER CODE END 4 */


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
