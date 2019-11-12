/* 
 * File:   modEventLog.cpp
 * Author: s.nevmyvaka
 * 
 * Created on 28 of may 2019
 */
#include "modEventLog.h"
#include "EventSPI.h"
#include <string.h>


/**
 * Инициализация начальных значений параметров
 * @param bakup_sram_base_addr - начальный адрес для помещения управляющей структуры журналов (в области BAKUP SRAM)
 * (0x4002 4000 - 0x4002 4FFF)
 */
void cJournal::modEventLog_Init (unsigned long bakup_sram_base_addr)
{
    t_jornal_period initPeriod;
    //unsigned long debugBuff;
    unsigned char i;
    
    JournalObject[JOURNAL_WORK_ACTION].jornal_lenght = EEPROM_JornalLenght_WA;
    JournalObject[JOURNAL_WORK_ACTION].data_lenght = EEPROM_DataLenght_WA;
    JournalObject[JOURNAL_WORK_ACTION].eeprom_start_address = EEPROM_JournalAdress_WA;

    JournalObject[JOURNAL_WORK_STATE].jornal_lenght = EEPROM_JornalLenght_WS;
    JournalObject[JOURNAL_WORK_STATE].data_lenght = EEPROM_DataLenght_WS;
    JournalObject[JOURNAL_WORK_STATE].eeprom_start_address = EEPROM_JournalAdress_WS;

    JournalObject[JOURNAL_CRASH].jornal_lenght = EEPROM_JornalLenght_Cr;
    JournalObject[JOURNAL_CRASH].data_lenght = EEPROM_DataLenght_Cr;
    JournalObject[JOURNAL_CRASH].eeprom_start_address = EEPROM_JournalAdress_Cr;

    HandlerState = HANDLER_WAIT;
    firstIndexRead[JOURNAL_WORK_ACTION] = 0;
    firstIndexRead[JOURNAL_WORK_STATE] = 0;
    firstIndexRead[JOURNAL_CRASH] = 0;
    CheckJournalCounter = HAL_GetTick();
    
    if ((bakup_sram_base_addr >= BKPSRAM_Adress) && (bakup_sram_base_addr < BKPSRAM_Adress + BKPSRAM_Size - sizeof(t_jornal_sram_bakup)))
    {
        // add journal struct
        BKPSRAM_Journal = (t_jornal_sram_bakup *)bakup_sram_base_addr;

        if (BKPSRAM_Journal->BKPSRAM_JournalStatus.eeprom_start_address != bakup_sram_base_addr)
        {
            //debugBuff = sizeof(t_jornal_sram_bakup);
            memset(BKPSRAM_Journal, 0, sizeof(t_jornal_sram_bakup));
            BKPSRAM_Journal->BKPSRAM_JournalStatus.journal_current_address = 0;
            BKPSRAM_Journal->BKPSRAM_JournalStatus.data_current_records = 0;
            BKPSRAM_Journal->BKPSRAM_JournalStatus.eeprom_start_address = bakup_sram_base_addr;
            BKPSRAM_Journal->BKPSRAM_JournalStatus.jornal_lenght = BKPSRAM_JornalLenght;         
            BKPSRAM_Journal->BKPSRAM_JournalStatus.data_lenght = BKPSRAM_DataLenght;         
            BKPSRAM_Journal->NumRecordsToWrite = 0;
            BKPSRAM_Journal->AttepmtsCounterWrite = 0;
            BKPSRAM_Journal->JournalError = JOURNAL_STATE_OK;

            initPeriod = {{19, 5, 14}, {23, 59, 30, 500}, {19, 5, 14}, {23, 59, 30, 500}};
            
            for (i = 0; i < JOURNAL_NUMBER; i++)
            {
                JournalObject[i].journal_current_address = 0;
                JournalObject[i].data_current_records = 0;
                JournalObject[i].period = initPeriod;
            }

        }
        else
        {
            for (i = 0; i < JOURNAL_NUMBER; i++)
            {
                JournalObject[i].journal_current_address = BKPSRAM_Journal->bkp_journal_current_address[i];
                JournalObject[i].data_current_records = BKPSRAM_Journal->bkp_data_current_records[i];
                JournalObject[i].period = BKPSRAM_Journal->bkp_period[i];
            }
        }
    }
    else
    {
        BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SRAM;//error
    }
}


/**
 * Стартует рассчет статистики и подготовку данных журналов для чтения за заданный период
 * @param statistics_type - вид представления данных согласно: t_jornal_representation_type
 * @param period - диапазон дат для выборки 
 */
void cJournal::modEventLog_CalcStatistics (cJournal::t_jornal_representation_type image_type, cJournal::t_jornal_period period)
{
    t_jornal_period comparePeriod;
    
    // обнулить статистику
    memset(&JornalStatistics, 0, sizeof(t_jornal_statistics)); 

    JornalStatistics.statistics_type = image_type;
                
    //если даты перепутаны, ставим на место
    if (modEventLog_DateTimeCompare (period)) 
    {
        JornalStatistics.ordered_period = period;
    }
    else
    {
        comparePeriod.period_begin_date = JornalStatistics.ordered_period.period_end_date;
        comparePeriod.period_begin_time = JornalStatistics.ordered_period.period_end_time;
        comparePeriod.period_end_date = JornalStatistics.ordered_period.period_begin_date;
        comparePeriod.period_end_time = JornalStatistics.ordered_period.period_begin_time;
        JornalStatistics.ordered_period = comparePeriod;
    }
                
    firstIndexRead[JOURNAL_WORK_ACTION] = 0;
    firstIndexRead[JOURNAL_WORK_STATE] = 0;
    firstIndexRead[JOURNAL_CRASH] = 0;
    
    JornalStatistics.events_all_ok = 0;
                                                    
    switch (JornalStatistics.statistics_type)
    {
        case REPRESENT_PERIOD_ALL :
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_ACTION] = 1;
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_STATE] = 1;
            JornalStatistics.need_calc_statistics[JOURNAL_CRASH] = 1;
            break;
        case REPRESENT_PERIOD_WORK_ACT :
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_ACTION] = 1;
            break;
        case REPRESENT_PERIOD_WORK_STATE :
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_STATE] = 1;
            break;
        case REPRESENT_PERIOD_CRASH :
            JornalStatistics.need_calc_statistics[JOURNAL_CRASH] = 1;
            break;
        case REPRESENT_PERIOD_WORK_ACT_WORK_STATE :
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_ACTION] = 1;
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_STATE] = 1;
            break;
        case REPRESENT_PERIOD_WORK_ACT_CRASH :
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_ACTION] = 1;
            JornalStatistics.need_calc_statistics[JOURNAL_CRASH] = 1;
            break;
        case REPRESENT_PERIOD_WORK_STATE_CRASH :
            JornalStatistics.need_calc_statistics[JOURNAL_WORK_STATE] = 1;
            JornalStatistics.need_calc_statistics[JOURNAL_CRASH] = 1;
            break;
        default :
            break;
    }
}


/**
 * Используется для проверки завершения расчета статистики и подготовки данных журналов к чтению 
 * или получения текущего состояния модуля журналов
 * @return - флаг завершения расчета статистики statistics_calculated
 *          или текущая ошибка модуля журналов JournalError
 */
unsigned char cJournal::modEventLog_CheckJournalReady (void)
{
    if (BKPSRAM_Journal->JournalError)
    {
        return BKPSRAM_Journal->JournalError;
    }
    else
    {
        return JornalStatistics.statistics_calculate_state;
    }
}

/**
 * Записывает рассчитанные данные статистики по переданному указателю н структуру
 * @param statistics - структура вывода данных
 */
void cJournal::modEventLog_GetStatistics (cJournal::t_jornal_statistics *statistics)
{
    *statistics = JornalStatistics;
}

/**
 * Сохраняет данные о событии (ID, время/дата) в журнал 
 * @param event_ID - ID события из t_journal_event_id
 * @param time - время возникновения
 * @param date - дата возникновения
 * @param crash_data - данные для сохранения (необязательно)
 */
void cJournal::modEventLog_EventSave (cJournal::t_journal_event_id event_ID, cJournal::t_journal_time time,  cJournal::t_journal_date date, float crash_data)
{
    t_record event_record;
    unsigned char i;

    // записать в BACKUP_SRAM
    event_record.event_id = event_ID;
    event_record.date = date;
    event_record.time = time;
    event_record.saved_data = crash_data;
    
    modEventLog_SaveRecordSRAM(event_record);

    for (i = 0; i < JOURNAL_NUMBER; i++)
    {
        // бэкап текущих данных журналов на случай сброса 
        BKPSRAM_Journal->bkp_journal_current_address[i] = JournalObject[i].journal_current_address;
        BKPSRAM_Journal->bkp_data_current_records[i] = JournalObject[i].data_current_records;
        BKPSRAM_Journal->bkp_period[i] = JournalObject[i].period;     
    }
}

/**
 * Читает первую представленную после расчета статистики запись о событии, затем следующую и так далее пока не закончатся записи.
 * Когда событий не останется, будет передавать нули.  
 * @param index - порядковый номер текущего читаемого события, инкрементируется самой функцией. Можно задать ненулевое значение для пропуска начальных записей
 * @param event - структура для получения данных
 */
unsigned char cJournal::modEventLog_EventRead (unsigned short *firstIndex, cJournal::t_record *event)
{
    unsigned char returnValue = 0;
    t_jornal_period comparePeriod;
    unsigned long findAdress;
    unsigned char i;
    
    // будем жонглировать записями и тогда встает ограничение на чтение: читать можно только от первого и по порядку
    /*
     1. читаем первые записи по указателям статистики
     2. сравниваем даты
     3. передаем самое раннее событие
     4. сдвигаем указатель прочитанного журнала на следующее событие
     */
    for (i = 0; i < JOURNAL_NUMBER; i++)
    {
        if ((JornalStatistics.first_data_address[i] != 0xFFFF) && (JornalStatistics.events_amount[i] > 0))
        {
            if (firstIndexRead[i] != 7777)
            {
                if (*firstIndex > JornalStatistics.events_amount_all)
                    *firstIndex = JornalStatistics.events_amount_all;

                findAdress = JornalStatistics.first_data_address[i] - *firstIndex / 3;
                findAdress = (JournalObject[i].journal_current_address + JournalObject[i].jornal_lenght - JournalObject[i].data_current_records) % JournalObject[i].jornal_lenght + JornalStatistics.first_data_address[i];
                firstIndexRead[i] = 7777;
            }
            else
            {
                *firstIndex++;
                findAdress = JornalStatistics.first_data_address[i];
                findAdress = (JournalObject[i].journal_current_address + JournalObject[i].jornal_lenght - JournalObject[i].data_current_records) % JournalObject[i].jornal_lenght + JornalStatistics.first_data_address[i];
            }

            if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, findAdress, &CompareRecords[i]))
            {
                BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
            }
            
//            if (CompareRecords[i].event_type == 0xFF)
//            {
//                if ((JornalStatistics.first_data_address[CompareRecords[i].event_type] != 0xFFFF) && (JornalStatistics.events_amount[CompareRecords[i].event_type] > 0))
//                {
//                    JornalStatistics.events_amount[CompareRecords[i].event_type]--;
//                    JornalStatistics.first_data_address[CompareRecords[i].event_type]--;
//                    
//                    if (JornalStatistics.events_amount_all)
//                        JornalStatistics.events_amount_all--;
//                    
//                    if ((JornalStatistics.first_data_address[CompareRecords[i].event_type] == 0xFFFF) && (JornalStatistics.events_amount[CompareRecords[i].event_type] > 0))
//                        JornalStatistics.first_data_address[CompareRecords[i].event_type] = JournalObject[CompareRecords[i].event_type].jornal_lenght - 1;
//                }
//            }
        }
        else
        {
            memset(&CompareRecords[i], 0, sizeof(t_record)); 
        }
    }

    comparePeriod.period_begin_date = CompareRecords[JOURNAL_WORK_ACTION].date;
    comparePeriod.period_begin_time = CompareRecords[JOURNAL_WORK_ACTION].time;
    comparePeriod.period_end_date = CompareRecords[JOURNAL_WORK_STATE].date;
    comparePeriod.period_end_time = CompareRecords[JOURNAL_WORK_STATE].time;

    if (modEventLog_DateTimeCompare (comparePeriod)) 
    {
        comparePeriod.period_begin_date = CompareRecords[JOURNAL_WORK_ACTION].date;
        comparePeriod.period_begin_time = CompareRecords[JOURNAL_WORK_ACTION].time;
        comparePeriod.period_end_date = CompareRecords[JOURNAL_CRASH].date;
        comparePeriod.period_end_time = CompareRecords[JOURNAL_CRASH].time;

        if (modEventLog_DateTimeCompare (comparePeriod)) 
        {
            *event = CompareRecords[JOURNAL_WORK_ACTION];
        }
        else
        {
            *event = CompareRecords[JOURNAL_CRASH];
        }
    }
    else
    {
        comparePeriod.period_begin_date = CompareRecords[JOURNAL_WORK_STATE].date;
        comparePeriod.period_begin_time = CompareRecords[JOURNAL_WORK_STATE].time;
        comparePeriod.period_end_date = CompareRecords[JOURNAL_CRASH].date;
        comparePeriod.period_end_time = CompareRecords[JOURNAL_CRASH].time;

        if (modEventLog_DateTimeCompare (comparePeriod)) 
        {
            *event = CompareRecords[JOURNAL_WORK_STATE];
        }
        else
        {
            *event = CompareRecords[JOURNAL_CRASH];
        }
    }
    
    event->event_type = modEventLog_DetermineJournal(event->event_id);
    if ((JornalStatistics.first_data_address[event->event_type] != 0xFFFF) && (JornalStatistics.events_amount[event->event_type] > 0))
    {
        JornalStatistics.events_amount[event->event_type]--;
        JornalStatistics.first_data_address[event->event_type]--;
        
        if (JornalStatistics.events_amount_all)
            JornalStatistics.events_amount_all--;
        
        if ((JornalStatistics.first_data_address[event->event_type] == 0xFFFF) && (JornalStatistics.events_amount[event->event_type] > 0))
            JornalStatistics.first_data_address[event->event_type] = JournalObject[event->event_type].jornal_lenght - 1;
    }
    
    return returnValue;
}

/**
 * Обработчик записи/чтения журналов, расчета статистики и подготовки данных
 */
void cJournal::modEventLog_Handler (void)
{
    unsigned long adressToRead;
    unsigned long firstAdress;
    unsigned long lastAdress;
    unsigned long firstBorder;
    unsigned long lastBorder;
    unsigned long adressShift;
    unsigned char recordFinded;
    t_jornal_period comparePeriod;
    unsigned char writeBuff[RecordSize];
    unsigned char readBuff[RecordSize];
    unsigned char i = 0;
    unsigned char adressSRAM;
    unsigned long adressToWrite;
            
    switch (HandlerState)
    {
///********************************************************************************************************///
        case HANDLER_WAIT:  // ожидание событий
///********************************************************************************************************///
            NextHandlerState = HANDLER_WAIT;
            
            if (JornalStatistics.need_calc_statistics[JOURNAL_WORK_ACTION] 
                    || JornalStatistics.need_calc_statistics[JOURNAL_WORK_STATE] 
                    || JornalStatistics.need_calc_statistics[JOURNAL_CRASH])
            {
                NextHandlerState = HANDLER_CALC_STATISTICS;
            }

            if (BKPSRAM_Journal->NumRecordsToWrite)            // если появилась новая запись в BKP_SRAM, записать на EEPROM
            {
                NextHandlerState = HANDLER_WRITE_EEPROM;
                BKPSRAM_Journal->AttepmtsCounterWrite = 3;
            }
            
            if ((HAL_GetTick() - CheckJournalCounter) >=  CheckJournalTimeout)
            {
                CheckJournalCounter = HAL_GetTick();
                NextHandlerState = HANDLER_CHECK_JOURNALS;
            }
            
            break;
///********************************************************************************************************///
        case HANDLER_WRITE_EEPROM:  // запись 
///********************************************************************************************************///
                // decrement_address_one = (address + Length - 1) % Length
            adressSRAM = (BKPSRAM_Journal->BKPSRAM_JournalStatus.journal_current_address + BKPSRAM_Journal->BKPSRAM_JournalStatus.jornal_lenght - BKPSRAM_Journal->NumRecordsToWrite) % BKPSRAM_Journal->BKPSRAM_JournalStatus.jornal_lenght;
            
            modEventLog_ReadRecordSRAM(adressSRAM, event_record_write);
            
             // определиться с типом журнала по event_ID
            event_record_write->event_type = modEventLog_DetermineJournal(event_record_write->event_id);
            
            writeBuff[0] = event_record_write->event_id;
            writeBuff[1] = event_record_write->date.year;
            writeBuff[2] = event_record_write->date.month;
            writeBuff[3] = event_record_write->date.day;
            writeBuff[4] = event_record_write->time.hours;
            writeBuff[5] = event_record_write->time.minutes;
            writeBuff[6] = event_record_write->time.seconds;
            writeBuff[7] = (unsigned char)(event_record_write->time.milliseconds >> 8);
            writeBuff[8] = (unsigned char) event_record_write->time.milliseconds;
            writeBuff[9] =  (unsigned char)((unsigned long)event_record_write->saved_data >> 24);
            writeBuff[10] = (unsigned char)((unsigned long)event_record_write->saved_data >> 16);
            writeBuff[11] = (unsigned char)((unsigned long)event_record_write->saved_data >> 8);
            writeBuff[12] = (unsigned char) event_record_write->saved_data;
            writeBuff[13] = modEventLog_HashCode(writeBuff, 13);
   
            //adressToWrite = journalForWrite->eeprom_start_address + journalForWrite->journal_current_address * EEPROM_PageSize / RecordsPerPage + (journalForWrite->journal_current_address % RecordsPerPage) * RecordSize;
            adressToWrite = JournalObject[event_record_write->event_type].eeprom_start_address 
                          + (JournalObject[event_record_write->event_type].journal_current_address / RecordsPerPage) * EEPROM_PageSize 
                          + (JournalObject[event_record_write->event_type].journal_current_address % RecordsPerPage) * RecordSize;
            
            BKPSRAM_Journal->AttepmtsCounterWrite = BKPSRAM_Journal->AttepmtsCounterWrite - 1;
            
            if (modEventLog_WriteData(adressToWrite, writeBuff, RecordSize))//записать наконец в EEPROM
            {
                BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
            }
                
            if (modEventLog_ReadData (adressToWrite, readBuff, RecordSize))//считать 
            {
                BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
            }
            
            if (!(writeBuff[RecordSize - 1] ^ readBuff[RecordSize - 1]))// если все записалось  - Хеш сошелся
            {
                modEventLog_JournalAddNewItem (&JournalObject[event_record_write->event_type], event_record_write->time, event_record_write->date);
                BKPSRAM_Journal->NumRecordsToWrite = BKPSRAM_Journal->NumRecordsToWrite - 1;
                BKPSRAM_Journal->AttepmtsCounterWrite = 0;
                //NextHandlerState = HANDLER_WAIT;
            }
            else if (BKPSRAM_Journal->AttepmtsCounterWrite)// если не записалось повторить еще 2 раза
            {
                //NextHandlerState = HANDLER_WAIT;                
            }
            else
            {
                BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_EEPROM_DATA;// CRC error !!!
                //NextHandlerState = HANDLER_WAIT;
            }
            
            NextHandlerState = HANDLER_WAIT;
            break;
///********************************************************************************************************///
        case HANDLER_REPRESENT_J_RECORDS:   // 
///********************************************************************************************************///            
            // если здесь, то нужно найти первые и последние записи согласно флагов
            // установить курсоры и давать задание на чтение события
            // потом сравнивать с требуемыми данными
            for (i = 0; i < JOURNAL_NUMBER; i++) ///перейти на итеративный механизм с перескоками в WAIT ??? ага
            {
                // для 
                if (JornalStatistics.need_find_event_first[i])
                {
                    //считать запись посередине и дальше середину середины, пока не найдется (двоичный поиск вобщем)
                    recordFinded = 0;
                    BKPSRAM_Journal->ReadCounter[i] = 0;
                    // выплняем пока не будет удовлетворено условие, что левая запись списка больше(меньше) заданного времени, а правая заись списка меньше(больше) заданного времени 
                    // уникальность записей обеспечивается наличием в них миллисекунд // принимается, что проверки на наличие требуемой даты в диапазоне списка пройдены успешно
                    do
                    {
                        if (BKPSRAM_Journal->ReadCounter[i] == 0)
                        {
                            firstBorder = firstAdress = ((JournalObject[i].jornal_lenght + JournalObject[i].journal_current_address) - JournalObject[i].data_current_records) % JournalObject[i].jornal_lenght;
                            lastBorder = lastAdress = (JournalObject[i].jornal_lenght + JournalObject[i].journal_current_address - 1) % JournalObject[i].jornal_lenght;
                        }
                        BKPSRAM_Journal->ReadCounter[i]++;

                        if ((lastBorder < firstBorder) && (lastAdress < firstAdress))
                        {
                            adressShift = JournalObject[i].jornal_lenght;
                        }
                        else
                        {
                            adressShift = 0;
                        }

                        adressToRead = (firstAdress + ((lastAdress + adressShift) - firstAdress) / 2) % JournalObject[i].jornal_lenght;
                        
                        if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, adressToRead, &CompareRecords[RECORD_MIDDLE]))
                        {
                            BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
                        }
                        
                        comparePeriod.period_begin_date = JornalStatistics.ordered_period.period_begin_date;
                        comparePeriod.period_begin_time = JornalStatistics.ordered_period.period_begin_time;
                        comparePeriod.period_end_date = CompareRecords[RECORD_MIDDLE].date;
                        comparePeriod.period_end_time = CompareRecords[RECORD_MIDDLE].time;

                        if (modEventLog_DateTimeCompare (comparePeriod)) 
                        {
                            firstAdress = adressToRead + 1;
                            if (firstAdress >= JournalObject[i].jornal_lenght)
                            {
                                firstAdress = 0;
                            }

                            CompareRecords[RECORD_LEFT].date = CompareRecords[RECORD_MIDDLE].date;
                            CompareRecords[RECORD_LEFT].time = CompareRecords[RECORD_MIDDLE].time;

                            if (firstAdress > (lastAdress + adressShift))
                            {
                                if (adressToRead != lastBorder)
                                    adressToRead++;

                                if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, adressToRead, &CompareRecords[RECORD_MIDDLE]))
                                {
                                    BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
                                }

                                comparePeriod.period_begin_date = JornalStatistics.ordered_period.period_begin_date;
                                comparePeriod.period_begin_time = JornalStatistics.ordered_period.period_begin_time;
                                comparePeriod.period_end_date = CompareRecords[RECORD_MIDDLE].date;
                                comparePeriod.period_end_time = CompareRecords[RECORD_MIDDLE].time;

                                if (modEventLog_DateTimeCompare (comparePeriod)) 
                                {   
                                        JornalStatistics.really_period[i].period_begin_date = CompareRecords[RECORD_MIDDLE].date;
                                        JornalStatistics.really_period[i].period_begin_time = CompareRecords[RECORD_MIDDLE].time;
                                        JornalStatistics.first_data_address[i] = adressToRead;
                                        //JornalStatistics.events_amount[i] = (adressToRead + 1) - (JournalObject[i].journal_current_address - JournalObject[i].data_current_records);
                                        JornalStatistics.events_amount[i] = ((JournalObject[i].jornal_lenght - firstBorder) + (adressToRead + 1)) % JournalObject[i].jornal_lenght;
                                        if (JornalStatistics.events_amount[i] > JournalObject[i].data_lenght)
                                            JornalStatistics.events_amount[i] = JournalObject[i].data_lenght;
                                        JornalStatistics.really_period_all.period_end_date = JornalStatistics.really_period[i].period_end_date;
                                        JornalStatistics.really_period_all.period_end_time = JornalStatistics.really_period[i].period_end_time;

                                }
                                else
                                {  
                                        JornalStatistics.really_period[i].period_begin_date = CompareRecords[RECORD_LEFT].date;
                                        JornalStatistics.really_period[i].period_begin_time = CompareRecords[RECORD_LEFT].time;
                                        JornalStatistics.first_data_address[i] = adressToRead - 1;
                                        //JornalStatistics.events_amount[i] = adressToRead - (JournalObject[i].journal_current_address - JournalObject[i].data_current_records);
                                        JornalStatistics.events_amount[i] = ((JournalObject[i].jornal_lenght - firstBorder) + adressToRead) % JournalObject[i].jornal_lenght;
                                        if (JornalStatistics.events_amount[i] > JournalObject[i].data_lenght)
                                            JornalStatistics.events_amount[i] = JournalObject[i].data_lenght;
                                        JornalStatistics.really_period_all.period_end_date = JornalStatistics.really_period[i].period_end_date;
                                        JornalStatistics.really_period_all.period_end_time = JornalStatistics.really_period[i].period_end_time;
                                }
                                recordFinded = 1;
                            }
                        }
                        else
                        {  
                            lastAdress = adressToRead - 1;
                            if (lastAdress == 0xFFFFFFFF)
                            {
                                lastAdress = JournalObject[i].jornal_lenght;
                            }

                            CompareRecords[RECORD_RIGHT].date = CompareRecords[RECORD_MIDDLE].date;
                            CompareRecords[RECORD_RIGHT].time = CompareRecords[RECORD_MIDDLE].time;
                            
                            if ((lastAdress + adressShift) < firstAdress)
                            {
                                if (adressToRead != firstBorder)
                                    adressToRead--;
                                
                                if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, adressToRead, &CompareRecords[RECORD_MIDDLE]))
                                {
                                    BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
                                }

                                JornalStatistics.really_period[i].period_begin_date = CompareRecords[RECORD_MIDDLE].date;
                                JornalStatistics.really_period[i].period_begin_time = CompareRecords[RECORD_MIDDLE].time;
                                JornalStatistics.first_data_address[i] = adressToRead;
                                //JornalStatistics.events_amount[i] = (adressToRead + 1) - (JournalObject[i].journal_current_address - JournalObject[i].data_current_records);
                                JornalStatistics.events_amount[i] = ((JournalObject[i].jornal_lenght - firstBorder) + (adressToRead + 1)) % JournalObject[i].jornal_lenght;
                                if (JornalStatistics.events_amount[i] > JournalObject[i].data_lenght)
                                    JornalStatistics.events_amount[i] = JournalObject[i].data_lenght;
                                JornalStatistics.really_period_all.period_end_date = JornalStatistics.really_period[i].period_end_date;
                                JornalStatistics.really_period_all.period_end_time = JornalStatistics.really_period[i].period_end_time;
                                recordFinded = 1;
                            }
                        }
                    } while (recordFinded == 0);
                    
                    JornalStatistics.need_find_event_first[i] = 0;
                }

                ///-----------------------------------------------------------------------------------------------------------------------------
                if (JornalStatistics.need_find_event_last[i])
                {
                    //считать запись посередине и дальше середину середины, пока не найдется (двоичный поиск вобщем)
                    recordFinded = 0;
                    BKPSRAM_Journal->ReadCounter[i] = 0;
                    // выплняем пока не будет удовлетворено условие, что левая запись списка больше(меньше) заданного времени, а правая заись списка меньше(больше) заданного времени 
                    // уникальность записей обеспечивается наличием в них миллисекунд // принимается, что проверки на наличие требуемой даты в диапазоне списка пройдены успешно
                    do
                    {
                        if (BKPSRAM_Journal->ReadCounter[i] == 0)
                        {
                            firstAdress = ((JournalObject[i].jornal_lenght + JournalObject[i].journal_current_address) - JournalObject[i].data_current_records) % JournalObject[i].jornal_lenght;
                            lastAdress = (JournalObject[i].jornal_lenght + JournalObject[i].journal_current_address - 1) % JournalObject[i].jornal_lenght;
                        }
                        BKPSRAM_Journal->ReadCounter[i]++;

                        if ((lastBorder < firstBorder) && (lastAdress < firstAdress))
                        {
                            adressShift = JournalObject[i].jornal_lenght;
                        }
                        else
                        {
                            adressShift = 0;
                        }
                        
                        adressToRead = (firstAdress + ((lastAdress + adressShift) - firstAdress) / 2) % JournalObject[i].jornal_lenght;

                        if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, adressToRead, &CompareRecords[RECORD_MIDDLE]))
                        {
                            BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
                        }
                        
                        comparePeriod.period_begin_date = JornalStatistics.ordered_period.period_end_date;
                        comparePeriod.period_begin_time = JornalStatistics.ordered_period.period_end_time;
                        comparePeriod.period_end_date = CompareRecords[RECORD_MIDDLE].date;
                        comparePeriod.period_end_time = CompareRecords[RECORD_MIDDLE].time;

                        if (modEventLog_DateTimeCompare (comparePeriod)) 
                        {   
                            firstAdress = adressToRead + 1;
                            if (firstAdress >= JournalObject[i].jornal_lenght)
                            {
                                firstAdress = 0;
                            }

                            CompareRecords[RECORD_LEFT].date = CompareRecords[RECORD_MIDDLE].date;
                            CompareRecords[RECORD_LEFT].time = CompareRecords[RECORD_MIDDLE].time;

                            if (firstAdress > (lastAdress + adressShift))
                            {
                                if (adressToRead != lastBorder)
                                    adressToRead++;

                                if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, adressToRead, &CompareRecords[RECORD_MIDDLE]))
                                {
                                    BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
                                }

                                JornalStatistics.really_period[i].period_end_date = CompareRecords[RECORD_MIDDLE].date;
                                JornalStatistics.really_period[i].period_end_time = CompareRecords[RECORD_MIDDLE].time;
                                JornalStatistics.really_period_all.period_end_date = JornalStatistics.really_period[i].period_end_date;
                                JornalStatistics.really_period_all.period_end_time = JornalStatistics.really_period[i].period_end_time;

                                if (JornalStatistics.events_amount[i] == 0)
                                {
                                    JornalStatistics.events_amount[i] = JournalObject[i].data_current_records - ((JournalObject[i].jornal_lenght - firstBorder) + (adressToRead + 1)) % JournalObject[i].jornal_lenght;
                                }
                                else
                                {
                                    JornalStatistics.events_amount[i] = JornalStatistics.events_amount[i] - ((JournalObject[i].jornal_lenght - firstBorder) + (adressToRead + 1)) % JournalObject[i].jornal_lenght;
                                }
                                if (JornalStatistics.events_amount[i] > JournalObject[i].data_lenght)
                                    JornalStatistics.events_amount[i] = JournalObject[i].data_lenght;
                                
                                if (JornalStatistics.first_data_address[i] == 0)
                                {
                                    JornalStatistics.first_data_address[i] = JournalObject[i].data_current_records - 1;
                                }

                                recordFinded = 1;
                            }
                        }
                        else
                        {  
                            lastAdress = adressToRead - 1;
                            if (lastAdress == 0xFFFFFFFF)
                            {
                                lastAdress = JournalObject[i].jornal_lenght;
                            }

                            CompareRecords[RECORD_RIGHT].date = CompareRecords[RECORD_MIDDLE].date;
                            CompareRecords[RECORD_RIGHT].time = CompareRecords[RECORD_MIDDLE].time;

                            if ((lastAdress + adressShift) < firstAdress)
                            {
                                if (adressToRead != firstBorder)
                                    adressToRead--;

                                if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, adressToRead, &CompareRecords[RECORD_MIDDLE]))
                                {
                                    BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
                                }

                                comparePeriod.period_begin_date = JornalStatistics.ordered_period.period_end_date;
                                comparePeriod.period_begin_time = JornalStatistics.ordered_period.period_end_time;
                                comparePeriod.period_end_date = CompareRecords[RECORD_MIDDLE].date;
                                comparePeriod.period_end_time = CompareRecords[RECORD_MIDDLE].time;

                                if (modEventLog_DateTimeCompare (comparePeriod)) 
                                {   
                                    JornalStatistics.really_period[i].period_end_date = CompareRecords[RECORD_RIGHT].date;
                                    JornalStatistics.really_period[i].period_end_time = CompareRecords[RECORD_RIGHT].time;
                                    JornalStatistics.really_period_all.period_end_date = JornalStatistics.really_period[i].period_end_date;
                                    JornalStatistics.really_period_all.period_end_time = JornalStatistics.really_period[i].period_end_time;

                                    if (JornalStatistics.events_amount[i] == 0)
                                    {
                                        JornalStatistics.events_amount[i] = JournalObject[i].data_current_records - ((JournalObject[i].data_lenght + (adressToRead + 1)) - firstBorder) % JournalObject[i].data_lenght;
                                    }
                                    else
                                    {
                                        JornalStatistics.events_amount[i] = JornalStatistics.events_amount[i] - ((JournalObject[i].data_lenght + (adressToRead + 1)) - firstBorder) % JournalObject[i].data_lenght;
                                    }
                                    if (JornalStatistics.events_amount[i] > JournalObject[i].data_lenght)
                                        JornalStatistics.events_amount[i] = JournalObject[i].data_lenght;
                                    
                                    if (JornalStatistics.first_data_address[i] == 0)
                                    {
                                        JornalStatistics.first_data_address[i] = JournalObject[i].data_current_records;
                                                              
                                        if (JornalStatistics.first_data_address[i] == JournalObject[i].data_lenght)
                                        {
                                            JornalStatistics.first_data_address[i] = JournalObject[i].data_current_records - 1;
                                        }

                                    }
                                }
                                else
                                { 
                                    JornalStatistics.really_period[i].period_end_date = CompareRecords[RECORD_MIDDLE].date;
                                    JornalStatistics.really_period[i].period_end_time = CompareRecords[RECORD_MIDDLE].time;
                                    JornalStatistics.really_period_all.period_end_date = JornalStatistics.really_period[i].period_end_date;
                                    JornalStatistics.really_period_all.period_end_time = JornalStatistics.really_period[i].period_end_time;

                                    if (JornalStatistics.events_amount[i] == 0)
                                    {
                                        JornalStatistics.events_amount[i] = JournalObject[i].data_current_records - ((JournalObject[i].data_lenght + (adressToRead + 1)) - firstBorder) % JournalObject[i].data_lenght;
                                    }
                                    else
                                    {
                                        JornalStatistics.events_amount[i] = JornalStatistics.events_amount[i] - ((JournalObject[i].data_lenght + (adressToRead + 1)) - firstBorder) % JournalObject[i].data_lenght;
                                    }
                                    if (JornalStatistics.events_amount[i] > JournalObject[i].data_lenght)
                                        JornalStatistics.events_amount[i] = JournalObject[i].data_lenght;

                                    if (JornalStatistics.first_data_address[i] == 0)
                                    {
                                        JornalStatistics.first_data_address[i] = JournalObject[i].data_current_records - 1;
                                    }
                                }
                                recordFinded = 1;
                            }
                        }
                    } while (recordFinded == 0);
                    
                    JornalStatistics.need_find_event_last[i] = 0;
                }
            }
            
                // общая дата реального периода,  
            for (i = 0; i < JOURNAL_NUMBER; i++)
            {
                //-----------------------начальная дата периода------------------------------------
                comparePeriod.period_begin_date = JornalStatistics.really_period[i].period_begin_date;
                comparePeriod.period_begin_time = JornalStatistics.really_period[i].period_begin_time;
                comparePeriod.period_end_date = JornalStatistics.really_period_all.period_begin_date;
                comparePeriod.period_end_time = JornalStatistics.really_period_all.period_begin_time;
                if (modEventLog_DateTimeCompare (comparePeriod)) 
                {   // в журнале хранятся события попадающие в границы запрошенного периода
                    JornalStatistics.really_period_all.period_begin_date = comparePeriod.period_begin_date;
                    JornalStatistics.really_period_all.period_begin_time = comparePeriod.period_begin_time;
                }
                
                if ((JornalStatistics.really_period[i].period_end_date.day != 0) && (JornalStatistics.really_period[i].period_end_date.month != 0))
                {
                    //-------------------------конечная дата периода------------------------------------
                    comparePeriod.period_begin_date = JornalStatistics.really_period_all.period_end_date;
                    comparePeriod.period_begin_time = JornalStatistics.really_period_all.period_end_time;
                    comparePeriod.period_end_date = JornalStatistics.really_period[i].period_end_date;
                    comparePeriod.period_end_time = JornalStatistics.really_period[i].period_end_time;
                    if (modEventLog_DateTimeCompare (comparePeriod)) 
                    {   // в журнале хранятся события попадающие в границы запрошенного периода
                        JornalStatistics.really_period_all.period_end_date = comparePeriod.period_end_date;
                        JornalStatistics.really_period_all.period_end_time = comparePeriod.period_end_time;
                    }
                }
                
                if (JornalStatistics.events_all_ok == 0)
                {
                    JornalStatistics.events_amount_all = JornalStatistics.events_amount_all + JornalStatistics.events_amount[i];
                }
            }
            
            JornalStatistics.events_all_ok = 0;
            JornalStatistics.statistics_calculate_state = StatisticsReady;
            NextHandlerState = HANDLER_WAIT;
            break;
///********************************************************************************************************///
        case HANDLER_CALC_STATISTICS:   // расчет статистики 
///********************************************************************************************************///
            // проверить текущие даты, чтоб не лезть на флешку лишний раз
            // если начальная дата запрашиваемого диапазона "больше" либо равна журнальной, то берем журнальную
            for (i = 0; i < JOURNAL_NUMBER; i++) 
            {
                if (JornalStatistics.need_calc_statistics[i])   //calc stat
                {
                    if (JournalObject[i].data_current_records > 0)
                    {
                        //-----------------------начальная дата периода------------------------------------
                        comparePeriod.period_begin_date = JornalStatistics.ordered_period.period_begin_date;
                        comparePeriod.period_begin_time = JornalStatistics.ordered_period.period_begin_time;
                        comparePeriod.period_end_date = JournalObject[i].period.period_begin_date;
                        comparePeriod.period_end_time = JournalObject[i].period.period_begin_time;

                        if (modEventLog_DateTimeCompare (comparePeriod)) 
                        {   // в журнале хранятся события попадающие в границы запрошенного периода
                            JornalStatistics.really_period[i].period_begin_date = comparePeriod.period_end_date;
                            JornalStatistics.really_period[i].period_begin_time = comparePeriod.period_end_time;
                            JornalStatistics.need_find_event_first[i] = 0;
                        }
                        else
                        {   // проверка на наличие даты внутри диапазона журнала
                            comparePeriod.period_begin_date = JournalObject[i].period.period_end_date;
                            comparePeriod.period_begin_time = JournalObject[i].period.period_end_time;
                            comparePeriod.period_end_date = JornalStatistics.ordered_period.period_begin_date;
                            comparePeriod.period_end_time = JornalStatistics.ordered_period.period_begin_time;

                            if (modEventLog_DateTimeCompare (comparePeriod)) 
                            {   // заданный период за границами существующих данных, ничего не даем
                                JornalStatistics.need_find_event_first[i] = 0;
                                JornalStatistics.first_data_address[i] = 0xFFFF;
                                JornalStatistics.events_amount[i] = 0;
                            }
                            else
                            {   // нужно найти адрес *первого* события, попадающего в запрошенный период 
                                JornalStatistics.need_find_event_first[i] = 1;
                            }
                        }

                        if (JornalStatistics.first_data_address[i] != 0xFFFF)
                        {
                            //-------------------------конечная дата периода------------------------------------
                            // если конечная дата диапазона "меньше" либо равна журнальной - берем журнальную
                            comparePeriod.period_begin_date = JournalObject[i].period.period_end_date;
                            comparePeriod.period_begin_time = JournalObject[i].period.period_end_time;
                            comparePeriod.period_end_date = JornalStatistics.ordered_period.period_end_date;
                            comparePeriod.period_end_time = JornalStatistics.ordered_period.period_end_time;

                            if (modEventLog_DateTimeCompare (comparePeriod)) 
                            {   // в журнале хранятся события попадающие в границы запрошенного периода
                                JornalStatistics.really_period[i].period_end_date = comparePeriod.period_begin_date;
                                JornalStatistics.really_period[i].period_end_time = comparePeriod.period_begin_time;
                                JornalStatistics.need_find_event_last[i] = 0;
                                JornalStatistics.really_period_all.period_end_date = comparePeriod.period_begin_date;
                                JornalStatistics.really_period_all.period_end_time = comparePeriod.period_begin_time;
                            }
                            else
                            {   
                                comparePeriod.period_begin_date = JornalStatistics.ordered_period.period_end_date;
                                comparePeriod.period_begin_time = JornalStatistics.ordered_period.period_end_time;
                                comparePeriod.period_end_date = JournalObject[i].period.period_begin_date;
                                comparePeriod.period_end_time = JournalObject[i].period.period_begin_time;

                                if (modEventLog_DateTimeCompare (comparePeriod)) 
                                {   // заданный период за границами существующих данных, ничего не даем
                                    JornalStatistics.need_find_event_last[i] = 0;
                                    JornalStatistics.first_data_address[i] = 0xFFFF;
                                    JornalStatistics.events_amount[i] = 0;
                                }
                                else
                                {   // нужно найти адрес *последнего* события, попадающего в запрошенный период 
                                    JornalStatistics.need_find_event_last[i] = 1;
                                }
                            }
                        }
                        else
                        {
                            JornalStatistics.need_find_event_last[i] = 0;
                        }

                        if ((JornalStatistics.need_find_event_first[i] == 0) && (JornalStatistics.need_find_event_last[i] == 0))
                        {
                            if (JornalStatistics.first_data_address[i] != 0xFFFF)
                            {
                                JornalStatistics.first_data_address[i] = JournalObject[i].data_current_records - 1;
                                JornalStatistics.events_amount[i] = JournalObject[i].data_current_records;
                            }
                        }
                    }
                    else
                    {
                        JornalStatistics.really_period[i].period_begin_date = {0, 0, 0};
                        JornalStatistics.really_period[i].period_begin_time = {0, 0, 0, 0};
                        JornalStatistics.need_find_event_first[i] = 0;
                        JornalStatistics.really_period[i].period_end_date = {0, 0, 0};
                        JornalStatistics.really_period[i].period_end_time = {0, 0, 0, 0};
                        JornalStatistics.need_find_event_last[i] = 0;
                        JornalStatistics.first_data_address[i] = 0xFFFF;
                        JornalStatistics.events_amount[i] = 0;
                    }   
                    
                    JornalStatistics.need_calc_statistics[i] = 0;
                }
                else
                {
                    JornalStatistics.really_period[i].period_begin_date = {0, 0, 0};
                    JornalStatistics.really_period[i].period_begin_time = {0, 0, 0, 0};
                    JornalStatistics.need_find_event_first[i] = 0;
                    JornalStatistics.really_period[i].period_end_date = {0, 0, 0};
                    JornalStatistics.really_period[i].period_end_time = {0, 0, 0, 0};
                    JornalStatistics.need_find_event_last[i] = 0;
                    JornalStatistics.first_data_address[i] = 0xFFFF;
                    JornalStatistics.events_amount[i] = 0;
                }
            }
                        
            // общая дата реального периода, если не нужно искать ее на флеш 
            for (i = 0; i < JOURNAL_NUMBER; i++)
            {
                if ((JornalStatistics.need_find_event_first[JOURNAL_WORK_ACTION] == 0) && (JornalStatistics.need_find_event_first[JOURNAL_WORK_STATE] == 0) && (JornalStatistics.need_find_event_first[JOURNAL_CRASH] == 0))
                {
                    //-----------------------начальная дата периода------------------------------------
                    comparePeriod.period_begin_date = JornalStatistics.really_period[i].period_begin_date;
                    comparePeriod.period_begin_time = JornalStatistics.really_period[i].period_begin_time;
                    comparePeriod.period_end_date = JornalStatistics.really_period_all.period_begin_date;
                    comparePeriod.period_end_time = JornalStatistics.really_period_all.period_begin_time;

                    if (modEventLog_DateTimeCompare (comparePeriod)) 
                    {   // в журнале хранятся события попадающие в границы запрошенного периода
                        JornalStatistics.really_period_all.period_begin_date = comparePeriod.period_begin_date;
                        JornalStatistics.really_period_all.period_begin_time = comparePeriod.period_begin_time;
                    }
                }

                if ((JornalStatistics.need_find_event_last[JOURNAL_WORK_ACTION] == 0) && (JornalStatistics.need_find_event_last[JOURNAL_WORK_STATE] == 0) && (JornalStatistics.need_find_event_last[JOURNAL_CRASH] == 0))
                {
                    if ((JornalStatistics.really_period[i].period_end_date.day != 0) && (JornalStatistics.really_period[i].period_end_date.month != 0))
                    {
                        //-------------------------конечная дата периода------------------------------------
                        comparePeriod.period_begin_date = JornalStatistics.really_period_all.period_end_date;
                        comparePeriod.period_begin_time = JornalStatistics.really_period_all.period_end_time;
                        comparePeriod.period_end_date = JornalStatistics.really_period[i].period_end_date;
                        comparePeriod.period_end_time = JornalStatistics.really_period[i].period_end_time;

                        if (modEventLog_DateTimeCompare (comparePeriod)) 
                        {   // в журнале хранятся события попадающие в границы запрошенного периода
                            JornalStatistics.really_period_all.period_end_date = comparePeriod.period_end_date;
                            JornalStatistics.really_period_all.period_end_time = comparePeriod.period_end_time;
                        }
                    }
                }

                if ((JornalStatistics.need_find_event_first[JOURNAL_WORK_ACTION] == 0) && (JornalStatistics.need_find_event_first[JOURNAL_WORK_STATE] == 0) && (JornalStatistics.need_find_event_first[JOURNAL_CRASH] == 0))
                {
                    if ((JornalStatistics.need_find_event_last[JOURNAL_WORK_ACTION] == 0) && (JornalStatistics.need_find_event_last[JOURNAL_WORK_STATE] == 0) && (JornalStatistics.need_find_event_last[JOURNAL_CRASH] == 0))
                    {
                        JornalStatistics.events_amount_all = JornalStatistics.events_amount_all + JornalStatistics.events_amount[i];
                        JornalStatistics.events_all_ok = 1;
                    }
                }
            }       

            NextHandlerState = HANDLER_REPRESENT_J_RECORDS; 
            break;
///********************************************************************************************************///        
        case HANDLER_CHECK_JOURNALS:    // периодическая проверка журналов 
///********************************************************************************************************///            
            for (i = 0; i < JOURNAL_NUMBER; i++) 
            {   // актуализуется последняя дата журнала
                if (JournalObject[i].data_current_records == JournalObject[i].data_lenght)
                {
                    adressToRead = ((JournalObject[i].jornal_lenght + JournalObject[i].journal_current_address) - JournalObject[i].data_current_records) % JournalObject[i].jornal_lenght;

                    if (modEventLog_RecordRead(JournalObject[i].eeprom_start_address, adressToRead, event_record_read))
                    {
                        BKPSRAM_Journal->JournalError = JOURNAL_STATE_ERROR_SPI;//error
                    }

                    JournalObject[i].period.period_end_date = event_record_read->date;
                    JournalObject[i].period.period_end_time = event_record_read->time;
                }
                // бэкап текущих данных журнала на случай сброса 
                BKPSRAM_Journal->bkp_journal_current_address[i] = JournalObject[i].journal_current_address;
                BKPSRAM_Journal->bkp_data_current_records[i] = JournalObject[i].data_current_records;
                BKPSRAM_Journal->bkp_period[i] = JournalObject[i].period;     
            }
            
            NextHandlerState = HANDLER_WAIT;
            break;
        default :   //error
            break;
    }
    
    HandlerState = NextHandlerState;
}

/**
 * Читает запись о событии исходя из начального адреса журнала на EEPROM и прядкового номера записи в журнале
 * @param eepromStartAddress - начальный адрес журнала на EEPROM
 * @param JournalAdress - порядковый номер записи в журнале
 * @param record - указатеьл на структуру для считываемых данных
 * @return - статус обмена по SPI 
 */
unsigned char cJournal::modEventLog_RecordRead(unsigned long eepromStartAddress, unsigned short JournalAdress, cJournal::t_record *record)
{
    unsigned char readBuff[RecordSize];
    unsigned char returnValue;
    unsigned char readComplete = 0;
    unsigned char attepmtsCounter = 3;
    unsigned long eepromAddress;
    
    do 
    {
        attepmtsCounter = attepmtsCounter - 1;

        eepromAddress = eepromStartAddress + (JournalAdress / RecordsPerPage) * EEPROM_PageSize + (JournalAdress % RecordsPerPage) * RecordSize;
        
        if (modEventLog_ReadData(eepromAddress, readBuff, RecordSize))//считать 
        {
            returnValue = JOURNAL_STATE_ERROR_SPI;//error
        }

        record->event_id      = (t_journal_event_id)readBuff[0];
        record->date.year     = readBuff[1];
        record->date.month    = readBuff[2];
        record->date.day      = readBuff[3];
        record->time.hours    = readBuff[4];
        record->time.minutes  = readBuff[5];
        record->time.seconds  = readBuff[6];
        record->time.milliseconds = ((unsigned short)readBuff[7] << 8) | readBuff[8];
        record->saved_data    = (float)(((unsigned long)readBuff[9] << 24) | ((unsigned long)readBuff[10] << 16) | ((unsigned long)readBuff[11] << 8) | readBuff[12]);
        record->crc = modEventLog_HashCode(readBuff, 13);

        if (record->crc == readBuff[RecordSize - 1])
        {
            //BKPSRAM_Journal->NumRecordsToRead = BKPSRAM_Journal->NumRecordsToRead - 1;
            readComplete = 1;
            attepmtsCounter = 0;
            //NextHandlerState = HANDLER_WAIT;
            returnValue = JOURNAL_STATE_OK;
        }
        else
        {
            returnValue = JOURNAL_STATE_ERROR_EEPROM_DATA;// CRC error !!!
            //NextHandlerState = HANDLER_WAIT;
        }

    }while ((readComplete == 0) && (attepmtsCounter != 0));

    return returnValue;
}

/**
 * Сравнивает две даты (год, месяц, день, час, мин, сек, милисек) и определяет которая из них в будущем 
 * @param period - структура для передачи дат для сравнения
 * @return: 1 - дата period_begin в будущем, дата period_end в прошлом
 *          0 - дата period_end в будущем, дата period_begin в пршлом
 */
unsigned char cJournal::modEventLog_DateTimeCompare (t_jornal_period period)
{
    unsigned long beginTime_ms;
    unsigned long endTime_ms;
    unsigned char returnValue = 0;

    
//    if ((period.period_begin_date.month == 0) && (period.period_begin_date.day == 0))
//    {
//        returnValue = 0;
//    }
//    else
//    {
//        if (period.period_begin_date.year < period.period_end_date.year)
//        {
//            returnValue = 0;
//        }
//        else if (period.period_begin_date.year > period.period_end_date.year)
//        {
//            returnValue = 1;
//        }
//        else
//        {
//            if (period.period_begin_date.month < period.period_end_date.month)
//            {
//                returnValue = 0;
//            }
//            else if (period.period_begin_date.month > period.period_end_date.month)
//            {
//                returnValue = 1;
//            }
//            else
//            {
//                if (period.period_begin_date.day < period.period_end_date.day)
//                {
//                    returnValue = 0;
//                }
//                else if (period.period_begin_date.day > period.period_end_date.day)
//                {
//                    returnValue = 1;
//                }
//                else
//                {
//                    beginTime_ms = ((period.period_begin_time.hours * 60 + period.period_begin_time.minutes) * 60 + period.period_begin_time.seconds) * 1000 + period.period_begin_time.milliseconds;
//                    endTime_ms   = ((period.period_end_time.hours * 60 + period.period_end_time.minutes) * 60 + period.period_end_time.seconds) * 1000 + period.period_end_time.milliseconds;
//
//                    if (beginTime_ms >= endTime_ms)
//                    {
//                        returnValue = 1;
//                    }
//                    else
//                    {
//                        returnValue = 0;
//                    }
//                }
//            }
//        }
//    }
    
    if ((period.period_begin_date.month != 0) && (period.period_begin_date.day != 0))
    {
        if (period.period_begin_date.year < period.period_end_date.year)
        {
            returnValue = 0;
        }
        else if (period.period_begin_date.year > period.period_end_date.year)
        {
            returnValue = 1;
        }
        else
        {
            if (period.period_begin_date.month < period.period_end_date.month)
            {
                returnValue = 0;
            }
            else if (period.period_begin_date.month > period.period_end_date.month)
            {
                returnValue = 1;
            }
            else
            {
                if (period.period_begin_date.day < period.period_end_date.day)
                {
                    returnValue = 0;
                }
                else if (period.period_begin_date.day > period.period_end_date.day)
                {
                    returnValue = 1;
                }
                else
                {
                    beginTime_ms = ((period.period_begin_time.hours * 60 + period.period_begin_time.minutes) * 60 + period.period_begin_time.seconds) * 1000 + period.period_begin_time.milliseconds;
                    endTime_ms   = ((period.period_end_time.hours * 60 + period.period_end_time.minutes) * 60 + period.period_end_time.seconds) * 1000 + period.period_end_time.milliseconds;

                    if (beginTime_ms >= endTime_ms)
                    {
                        returnValue = 1;
                    }
                    else
                    {
                        returnValue = 0;
                    }
                }
            }
        }
    }
    else
    {
        returnValue = 0;
    }

    return returnValue;
}

/**
 * Расчитывает хеш-сумму буфера для определения повреждены данные или нет
 * @param buffer - указатель на буфер данных
 * @param buff_size - размер буфера
 * @return - расчитанная хеш-сумма 
 */
unsigned char cJournal::modEventLog_HashCode (unsigned char *buffer, unsigned char buff_size)
{
    unsigned short hashsum = 0;
    unsigned char i;
    
    if (buffer != NULL)
    {
        for (i = 0; i < buff_size; i++)
        {
            hashsum = hashsum + buffer[i] * 211;
            hashsum = (hashsum ^ (hashsum >> 8)) & 0xFF;
        }
    }
    return hashsum;
}

/**
 * Определяет журнал, которому принадлежит событие по ID события
 * @param event_id - ID события
 * @return - ID журнала
 */
cJournal::t_journal_name cJournal::modEventLog_DetermineJournal (cJournal::t_journal_event_id event_id)
{
    cJournal::t_journal_name returnJourmalType;
    
        // определиться с типом журнала по event_ID
    if (event_id < JRN_EV_WORK_ACT_EVENTS_NUMBER)
    {
        returnJourmalType = t_journal_name::JOURNAL_WORK_ACTION;
    }
    else if (event_id < JRN_EV_WORK_STATE_EVENTS_NUMBER)
    {
        returnJourmalType = t_journal_name::JOURNAL_WORK_STATE;
    }
    else if (event_id < JRN_EV_CRASH_EVENTS_NUMBER)
    {
        returnJourmalType = t_journal_name::JOURNAL_CRASH;
    }
    else
    {
        returnJourmalType = t_journal_name::JOURNAL_TYPE_ERROR;
    }

    return returnJourmalType;
}

/**
 * Инкрементирует индексы журнала для нового события, добавляет дату/время события как начальную дату/время периода журнала
 * @param journal - указатель на структуру журнала
 * @param time - время события
 * @param date - дата события
 */
void cJournal::modEventLog_JournalAddNewItem (cJournal::t_jornal_struct *journal, t_journal_time time,  t_journal_date date)
{
    if (journal != NULL)
    {
        if (journal->data_current_records == 0)
        {
            journal->period.period_end_date = date;
            journal->period.period_end_time = time;
        }

        journal->period.period_begin_date = date;
        journal->period.period_begin_time = time;
        
        if (journal->data_current_records < journal->data_lenght)
        {
            journal->data_current_records++;
        }

        journal->journal_current_address = (journal->journal_current_address + 1) % journal->jornal_lenght;//  increment_address_one = (address + 1) % Length
        
    }
}

/**
 * Переносит данные о событии в буфер ожидания записи на EEPROM
 * @param event_record - запись о событии
 */
void cJournal::modEventLog_SaveRecordSRAM (cJournal::t_record event_record)
{
    if (BKPSRAM_Journal->NumRecordsToWrite < BKPSRAM_DataLenght)
    {
        BKPSRAM_Journal->NumRecordsToWrite++;
        
        BKPSRAM_Journal->BKPSRAM_Buffer[BKPSRAM_Journal->BKPSRAM_JournalStatus.journal_current_address] = event_record;
        
        modEventLog_JournalAddNewItem(&(BKPSRAM_Journal->BKPSRAM_JournalStatus), event_record.time, event_record.date);
    }
}

/**
 * Читает запись о событии из буфера ожидания записи
 * @param index - порядковый номер записи
 * @param event_record - указатель на структуру 
 */
void cJournal::modEventLog_ReadRecordSRAM (unsigned short index, cJournal::t_record *event_record)
{
    *event_record = BKPSRAM_Journal->BKPSRAM_Buffer[index];
}

/////////////////////////////////////////////////Area/////////////////////////////////////////////////
/**
 * Запись на EEPROM массива данных в свободную от журналов область
 * @param pData - указатель на буфер с данными
 * @param size - размер записи (мах  EEPROM_FreeAreaSize = 8415)
 * @return - статус обмена по SPI 
 */    
unsigned char cJournal::modEventLog_AreaWrite (unsigned char *pData, unsigned long size)
{
    unsigned char returnValue;
    unsigned char *currentDataPntr;
    unsigned long writeDataCounter;
    unsigned long adressToWrite;
    unsigned char pageCounter;
    unsigned char hashSummR;
    unsigned char hashSummW;
    unsigned char writeComplete = 0;
    unsigned char attepmtsCounter; 
    //unsigned char writeBuff[EEPROM_PageSize];

    if ((size <= EEPROM_FreeAreaSize) && (pData != NULL))
    {
        currentDataPntr = pData;
        writeDataCounter = size;
        pageCounter = 0;
        
        while (writeDataCounter > 0)
        {
            attepmtsCounter = 3; 
            writeComplete = 0;
            
            if (writeDataCounter >= (EEPROM_PageSize - 1))
            {
                do{
                    attepmtsCounter = attepmtsCounter - 1;
                    
                    hashSummW = modEventLog_HashCode(currentDataPntr, (EEPROM_PageSize - 1));

                    adressToWrite = EEPROM_FreeAreaAdress + pageCounter * EEPROM_PageSize;
                    if (modEventLog_WriteData(adressToWrite, currentDataPntr, (EEPROM_PageSize - 1)))
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }
                    if (modEventLog_WriteData((adressToWrite + (EEPROM_PageSize - 1)), &hashSummW, 1))//дописать хеш 
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }
                    if (modEventLog_ReadData((adressToWrite + (EEPROM_PageSize - 1)), &hashSummR, 1))//считать 
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }
                    
                    if (hashSummW == hashSummR)
                    {
                        writeComplete = 1;
                        attepmtsCounter = 0;
                        returnValue = JOURNAL_STATE_OK;

                        pageCounter = pageCounter + 1;
                        currentDataPntr = currentDataPntr + (EEPROM_PageSize - 1);
                        writeDataCounter = writeDataCounter - (EEPROM_PageSize - 1);
                    }
                    else
                    {
                        if (attepmtsCounter == 0)
                            return JOURNAL_STATE_ERROR_EEPROM_DATA;// CRC error !!!
                    }
                    
                }while ((writeComplete == 0) && (attepmtsCounter != 0));
                
            }
            else
            {
                do{
                    attepmtsCounter = attepmtsCounter - 1;

                    hashSummW = modEventLog_HashCode(currentDataPntr, writeDataCounter);

                    adressToWrite = EEPROM_FreeAreaAdress + pageCounter * EEPROM_PageSize;
                    if (modEventLog_WriteData(adressToWrite, currentDataPntr, writeDataCounter))
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }
                    if (modEventLog_WriteData((adressToWrite + writeDataCounter), &hashSummW, 1))//считать 
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }
                    if (modEventLog_ReadData((adressToWrite + writeDataCounter), &hashSummR, 1))//считать 
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }

                    if (hashSummW == hashSummR)
                    {
                        writeComplete = 1;
                        attepmtsCounter = 0;
                        returnValue = JOURNAL_STATE_OK;

                        BKPSRAM_Journal->AreaDataAmount = size;
                        writeDataCounter = 0;
                    }
                    else
                    {
                        if (attepmtsCounter == 0)
                            return JOURNAL_STATE_ERROR_EEPROM_DATA;// CRC error !!!
                    }

                }while ((writeComplete == 0) && (attepmtsCounter != 0));
            }
        }        
    }
    else
    {
        returnValue = JOURNAL_STATE_OUT_OF_BOUNDS;// out of bounds
    }

    return returnValue;
}

/**
 * Чтение из EEPROM массива данных из свободной от журналов области
 * @param pData - указатель на буфер
 * @param size  - количество байт (мах  EEPROM_FreeAreaSize = 8415)
 * @return - статус обмена по SPI 
 */
unsigned char cJournal::modEventLog_AreaRead (unsigned char *pData, unsigned long size)
{
    unsigned char returnValue;
    unsigned char *currentDataPntr;
    unsigned long readDataCounter;
    unsigned long adressToRead;
    unsigned char pageCounter;
    unsigned char hashSummR;
    unsigned char hashSummW;
    unsigned char readComplete = 0;
    unsigned char attepmtsCounter; 
//    unsigned char readBuff[EEPROM_PageSize];

    if ((size <= EEPROM_FreeAreaSize) && (pData != NULL))
    {
        currentDataPntr = pData;
        readDataCounter = size;
        pageCounter = 0;
        
        while (readDataCounter > 0)
        {
            attepmtsCounter = 3; 
            readComplete = 0;
            
            if (readDataCounter >= (EEPROM_PageSize - 1))
            {
                do{
                    attepmtsCounter = attepmtsCounter - 1;
                    
                    adressToRead = EEPROM_FreeAreaAdress + pageCounter * EEPROM_PageSize;
                    if (modEventLog_ReadData(adressToRead, currentDataPntr, (EEPROM_PageSize - 1)))
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }
                    if (modEventLog_ReadData((adressToRead + (EEPROM_PageSize - 1)), &hashSummR, 1))//считать 
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }
                    
                    hashSummW = modEventLog_HashCode(currentDataPntr, (EEPROM_PageSize - 1));
                    
                    if (hashSummW == hashSummR)
                    {
                        readComplete = 1;
                        attepmtsCounter = 0;
                        returnValue = JOURNAL_STATE_OK;

                        pageCounter = pageCounter + 1;
                        currentDataPntr = currentDataPntr + (EEPROM_PageSize - 1);
                        readDataCounter = readDataCounter - (EEPROM_PageSize - 1);
                    }
                    else
                    {
                        if (attepmtsCounter == 0)
                            return JOURNAL_STATE_ERROR_EEPROM_DATA;// CRC error !!!
                    }
                    
                }while ((readComplete == 0) && (attepmtsCounter != 0));
                
            }
            else
            {
                do{
                    attepmtsCounter = attepmtsCounter - 1;

                    adressToRead = EEPROM_FreeAreaAdress + pageCounter * EEPROM_PageSize;
                    if (modEventLog_ReadData(adressToRead, currentDataPntr, (readDataCounter + 1)))
                    {
                        return JOURNAL_STATE_ERROR_SPI;
                    }

                    hashSummR = *(currentDataPntr +  readDataCounter);
                    hashSummW = modEventLog_HashCode(currentDataPntr, readDataCounter);
                    
                    if (hashSummW == hashSummR)
                    {
                        readComplete = 1;
                        attepmtsCounter = 0;
                        returnValue = JOURNAL_STATE_OK;

                        readDataCounter = 0;
                    }
                    else
                    {
                        if (attepmtsCounter == 0)
                            return JOURNAL_STATE_ERROR_EEPROM_DATA;// CRC error !!!
                    }
                }while ((readComplete == 0) && (attepmtsCounter != 0));
            }
        }        
    }
    else
    {
        returnValue = JOURNAL_STATE_OUT_OF_BOUNDS;// out of bounds
    }

    return returnValue;
}
    
///////////////////////////////////////////////////////SPI///////////////////////////////////////////
/**
 * Запись на EEPROM буфера данных
 * @param adress - начальный адрес для записи
 * @param pData - указатель на буфер с данными
 * @param size - размер записи (мах 255)
 * @return - статус обмена по SPI 
 */    
unsigned char cJournal::modEventLog_WriteData (unsigned long adress, unsigned char *pData, unsigned short size)
{
    unsigned char TxAddr[4];
    unsigned char SPI_Status;
    unsigned char EEPROM_Status;
    unsigned char EEPROM_Instruction;
    unsigned long timeout = 5; // 5 ms
    unsigned char attempts = 0;
    unsigned char i;
    unsigned char MemoryOccupied = 0;
//    unsigned long eraseAdress = 0;
//    unsigned long writeIndex = 0;
    
    do{
        //status
        EEPROM_Instruction = EPPROM_INSTRUCTION_READ_SR; //RDSR Read Status Register
        // прочитать регистр статуса 
        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
        attempts = 10;
        do{
            SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
        } while ((SPI_Status != HAL_OK) && attempts--);
        if (0 == attempts) // error occured in HAL
            return SPI_Status;
        
        attempts = 10;
        while ((HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) && attempts--);
        if (0 == attempts) // error occured on SPI
            return HAL_SPI_GetState(&hspi4);
        
        SPI_Status = HAL_SPI_Receive(&hspi4, &EEPROM_Status, 1, 20); // receive 1 byte
        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()

        if ((EEPROM_Status & EEPROM_WIP_FLAG) == SET)
            HAL_Delay(timeout);
    } while ((EEPROM_Status & EEPROM_WIP_FLAG) == SET); // Write in progress

    // if data present in memory read and erase 
    ///-------------------------------------------------------------------------
    
    modEventLog_ReadData (adress, SectorEraseBuff, size);
    
    for (i = 0; i < size; i++) 
    {
        if (SectorEraseBuff[i] != 0xFF)
        {
            MemoryOccupied = 1;
        }
    }
    
    if (MemoryOccupied == 1)
    {
        modEventLog_SectorErase (adress & 0xFFFFF000);
    }
    
    modEventLog_ReadData (adress, SectorEraseBuff, size);
        
    ///-------------------------------------------------------------------------
    EEPROM_Instruction = EPPROM_INSTRUCTION_WRITE_EN; //WREN Write Enable
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
    SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()

    timeout = 1; // 1 ms
    HAL_Delay(timeout);
    
    // write
    TxAddr[0] = EPPROM_INSTRUCTION_WRITE; // WRITE Write to Memory Array
    TxAddr[1] = adress >> 16;
    TxAddr[2] = adress >> 8;
    TxAddr[3] = adress;
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()

    // передать команду и адрес 
    SPI_Status = HAL_SPI_Transmit(&hspi4, TxAddr, 4, 10);
    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
    SPI_Status = HAL_SPI_Transmit(&hspi4, pData, size, 10);
    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()

//    if (MemoryOccupied == 0)
//    {
//        ///-------------------------------------------------------------------------
//        EEPROM_Instruction = EPPROM_INSTRUCTION_WRITE_EN; //WREN Write Enable
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//        SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//
//        timeout = 1; // 1 ms
//        HAL_Delay(timeout);
//        
//        // write
//        TxAddr[0] = EPPROM_INSTRUCTION_WRITE; // WRITE Write to Memory Array
//        TxAddr[1] = adress >> 16;
//        TxAddr[2] = adress >> 8;
//        TxAddr[3] = adress;
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//
//        // передать команду и адрес 
//        SPI_Status = HAL_SPI_Transmit(&hspi4, TxAddr, 4, 10);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//        SPI_Status = HAL_SPI_Transmit(&hspi4, pData, size, 10);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//    }
//    else
//    {
//        eraseAdress = adress & 0xFFF;
//        modEventLog_ReadData (eraseAdress, SectorEraseBuff, FLASH_SectorSize);
//        modEventLog_SectorErase (eraseAdress);
//        
//        writeIndex = adress & 0xFFFFF000;
//
//        for (i = 0; i < size; i++) 
//        {
//            SectorEraseBuff[writeIndex] = *pData;
//            pData++;
//        }
//        
//        ///-------------------------------------------------------------------------
//        EEPROM_Instruction = EPPROM_INSTRUCTION_WRITE_EN; //WREN Write Enable
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//        SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//
//        timeout = 1; // 1 ms
//        HAL_Delay(timeout);
//        
//        // write
//        TxAddr[0] = EPPROM_INSTRUCTION_WRITE; // WRITE Write to Memory Array
//        TxAddr[1] = eraseAdress >> 16;
//        TxAddr[2] = eraseAdress >> 8;
//        TxAddr[3] = eraseAdress;
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//
//        // передать команду и адрес 
//        SPI_Status = HAL_SPI_Transmit(&hspi4, TxAddr, 4, 10);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//        SPI_Status = HAL_SPI_Transmit(&hspi4, pData, FLASH_SectorSize, 10);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//    }
    
    return SPI_Status;
}

/**
 * Чтение из EEPROM буфера данных
 * @param adress - начальный адрес
 * @param pData - указатель на буфер
 * @param size  - количество байт
 * @return - статус обмена по SPI 
 */
unsigned char cJournal::modEventLog_ReadData (unsigned long adress, unsigned char *pData, unsigned short size)
{
    unsigned char TxAddr[4];
    unsigned char SPI_Status;
    unsigned char EEPROM_Status;
    unsigned char EEPROM_Instruction;
    unsigned long timeout = 5; // 5 ms
    unsigned char attempts = 0;

    do{
        //status
        EEPROM_Instruction = EPPROM_INSTRUCTION_READ_SR; //RDSR Read Status Register
        // прочитать регистр статуса 

        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
        attempts = 10;
        do{
            SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
        } while ((SPI_Status != HAL_OK) && attempts--);
        if (0 == attempts) // error occured in HAL
            return SPI_Status;
        
        attempts = 10;
        while ((HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) && attempts--);
        if (0 == attempts) // error occured on SPI
            return HAL_SPI_GetState(&hspi4);

        SPI_Status = HAL_SPI_Receive(&hspi4, &EEPROM_Status, 1, 20);
        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
        
        if ((EEPROM_Status & EEPROM_WIP_FLAG) == SET)
            HAL_Delay(timeout);
    } while ((EEPROM_Status & EEPROM_WIP_FLAG) == SET); // Write in progress
    
    //read
    TxAddr[0] = EPPROM_INSTRUCTION_READ; //READ Read from Memory Array
    TxAddr[1] = adress >> 16;
    TxAddr[2] = adress >> 8;
    TxAddr[3] = adress;
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()

    // передать команду и адрес 
    SPI_Status = HAL_SPI_Transmit(&hspi4, TxAddr, 4, 10);
    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
    SPI_Status = HAL_SPI_Receive(&hspi4, pData, size, 20);
    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
    
    return SPI_Status;
}

/**
 * Стирание сектора памяти 4096
 * @param adress - адрес начала сектора
  * @return - статус обмена по SPI 
 */
unsigned char cJournal::modEventLog_SectorErase (unsigned long adress)
{
    unsigned char TxAddr[4];
    unsigned char SPI_Status;
    unsigned char EEPROM_Status;
    unsigned char EEPROM_Instruction;
    unsigned long timeout = 5; // 5 ms
    unsigned char attempts = 0;

    do{
        //status
        EEPROM_Instruction = EPPROM_INSTRUCTION_READ_SR; //RDSR Read Status Register
        // прочитать регистр статуса 

        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
        attempts = 10;
        do{
            SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
        } while ((SPI_Status != HAL_OK) && attempts--);
        if (0 == attempts) // error occured in HAL
            return SPI_Status;
        
        attempts = 10;
        while ((HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) && attempts--);
        if (0 == attempts) // error occured on SPI
            return HAL_SPI_GetState(&hspi4);

        SPI_Status = HAL_SPI_Receive(&hspi4, &EEPROM_Status, 1, 20);
        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
        
        if ((EEPROM_Status & EEPROM_WIP_FLAG) == SET)
            HAL_Delay(timeout);
    } while ((EEPROM_Status & EEPROM_WIP_FLAG) == SET); // Write in progress
    
    ///-------------------------------------------------------------------------
    EEPROM_Instruction = EPPROM_INSTRUCTION_WRITE_EN; //WREN Write Enable
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
    SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()

    timeout = 1; // 1 ms
    HAL_Delay(timeout);

    // sector erase
    TxAddr[0] = FLASH_COMM_SECTOR_ERASE; // WRITE Write to Memory Array
    TxAddr[1] = adress >> 16;
    TxAddr[2] = adress >> 8;
    TxAddr[3] = adress;

    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
    // передать команду и адрес 
    SPI_Status = HAL_SPI_Transmit(&hspi4, TxAddr, 4, 10);
    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()

    
    return SPI_Status;
}

/////////////////////////////////////////////////////////SPI///////////////////////////////////////////
///**
// * Запись на EEPROM буфера данных
// * @param adress - начальный адрес для записи
// * @param pData - указатель на буфер с данными
// * @param size - размер записи (мах 255)
// * @return - статус обмена по SPI 
// */    
//unsigned char cJournal::modEventLog_WriteData (unsigned long adress, unsigned char *pData, unsigned short size)
//{
//    unsigned char TxAddr[4];
//    unsigned char SPI_Status;
//    unsigned char EEPROM_Status;
//    unsigned char EEPROM_Instruction;
//    unsigned long timeout = 5; // 5 ms
//    unsigned char attempts = 0;
//        
//    do{
//        //status
//        EEPROM_Instruction = EPPROM_INSTRUCTION_READ_SR; //RDSR Read Status Register
//        // прочитать регистр статуса 
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//        attempts = 10;
//        do{
//            SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
//        } while ((SPI_Status != HAL_OK) && attempts--);
//        if (0 == attempts) // error occured in HAL
//            return SPI_Status;
//        
//        attempts = 10;
//        while ((HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) && attempts--);
//        if (0 == attempts) // error occured on SPI
//            return HAL_SPI_GetState(&hspi4);
//        
//        SPI_Status = HAL_SPI_Receive(&hspi4, &EEPROM_Status, 1, 20);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//
//        if ((EEPROM_Status & EEPROM_WIP_FLAG) == SET)
//            HAL_Delay(timeout);
//    } while ((EEPROM_Status & EEPROM_WIP_FLAG) == SET); // Write in progress
//
//
//      // добавить условие проверки уже включенного разрешения на запись
//    EEPROM_Instruction = EPPROM_INSTRUCTION_WRITE_EN; //WREN Write Enable
//    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//    SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
//    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//
//    timeout = 1; // 1 ms
//    HAL_Delay(timeout);
//    
//    // write
//    TxAddr[0] = EPPROM_INSTRUCTION_WRITE; // WRITE Write to Memory Array
//    TxAddr[1] = adress >> 16;
//    TxAddr[2] = adress >> 8;
//    TxAddr[3] = adress;
//    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//
//    // передать команду и адрес 
//    SPI_Status = HAL_SPI_Transmit(&hspi4, TxAddr, 4, 10);
//    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//    SPI_Status = HAL_SPI_Transmit(&hspi4, pData, size, 10);
//    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//
//    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//    
//    return SPI_Status;
//}
//
///**
// * Чтение из EEPROM буфера данных
// * @param adress - начальный адрес
// * @param pData - указатель на буфер
// * @param size  - количество байт
// * @return - статус обмена по SPI 
// */
//unsigned char cJournal::modEventLog_ReadData (unsigned long adress, unsigned char *pData, unsigned short size)
//{
//    unsigned char TxAddr[4];
//    unsigned char SPI_Status;
//    unsigned char EEPROM_Status;
//    unsigned char EEPROM_Instruction;
//    unsigned long timeout = 5; // 5 ms
//    unsigned char attempts = 0;
//
//    do{
//        //status
//        EEPROM_Instruction = EPPROM_INSTRUCTION_READ_SR; //RDSR Read Status Register
//        // прочитать регистр статуса 
//
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//        attempts = 10;
//        do{
//            SPI_Status = HAL_SPI_Transmit(&hspi4, &EEPROM_Instruction, 1, 10);
//        } while ((SPI_Status != HAL_OK) && attempts--);
//        if (0 == attempts) // error occured in HAL
//            return SPI_Status;
//        
//        attempts = 10;
//        while ((HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) && attempts--);
//        if (0 == attempts) // error occured on SPI
//            return HAL_SPI_GetState(&hspi4);
//
//        SPI_Status = HAL_SPI_Receive(&hspi4, &EEPROM_Status, 1, 20);
//        while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//
//        HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//        
//        if ((EEPROM_Status & EEPROM_WIP_FLAG) == SET)
//            HAL_Delay(timeout);
//    } while ((EEPROM_Status & EEPROM_WIP_FLAG) == SET); // Write in progress
//    
//    //read
//    TxAddr[0] = EPPROM_INSTRUCTION_READ; //READ Read from Memory Array
//    TxAddr[1] = adress >> 16;
//    TxAddr[2] = adress >> 8;
//    TxAddr[3] = adress;
//    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);  //EEPROM_CS_LOW()
//
//    // передать команду и адрес 
//    SPI_Status = HAL_SPI_Transmit(&hspi4, TxAddr, 4, 10);
//    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//    SPI_Status = HAL_SPI_Receive(&hspi4, pData, size, 20);
//    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
//
//    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);    //EEPROM_CS_HIGH()
//    
//    return SPI_Status;
//}
//


