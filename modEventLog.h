/* 
 * File:   modEventLog.h
 * Author: s.nevmyvaka
 *
 * Created on 28 of may 2019
 */


#ifndef MOD_EVENTLOG_H
#define	MOD_EVENTLOG_H

#include "main.h"

#ifdef	__cplusplus
extern "C" {
#endif




//extern SPI_HandleTypeDef hspi4;

class cJournal 
{
////////////////////////////////////////////////////////////////////////////////
public:

    typedef enum {
        JOURNAL_WORK_ACTION,    // журнал рабочих действий 1/5 сек
        JOURNAL_WORK_STATE,     // журнал рабочих событий 1/1 мин
        JOURNAL_CRASH,          // журнал аварий
        JOURNAL_NUMBER,         // всего журналов
        JOURNAL_TYPE_ERROR      // ошибка определения типа журнала
    }t_journal_name;  

    typedef enum {
        JOURNAL_STATE_OK,                     // ошибок нет
        JOURNAL_STATE_ERROR_SPI,              // ошибка на линии SPI
        JOURNAL_STATE_ERROR_EEPROM_DATA,      // ошибка данных в памяти
        JOURNAL_STATE_ERROR_NULL_POINTER,     // пустой указатель
        JOURNAL_STATE_ERROR_SRAM,             // ошибка инициализации SRAM
        JOURNAL_STATE_OUT_OF_BOUNDS           // значение вне допустимых границ
    }t_j_handler_errors;

    typedef enum {
        REPRESENT_PERIOD_ALL,                   // представить статистику и данные всех журналов
        REPRESENT_PERIOD_WORK_ACT,              // представить статистику и данные журнала рабочих действий
        REPRESENT_PERIOD_WORK_STATE,            // представить статистику и данные журнала рабочих событий
        REPRESENT_PERIOD_CRASH,                 // представить статистику и данные журнала аварий
        REPRESENT_PERIOD_WORK_ACT_WORK_STATE,   // представить статистику и данные журналов рабочих действий и рабочих событий
        REPRESENT_PERIOD_WORK_ACT_CRASH,        // представить статистику и данные журналов рабочих действий и аварий
        REPRESENT_PERIOD_WORK_STATE_CRASH       // представить статистику и данные журналов рабочих событий и аварий
    }t_jornal_representation_type;

    typedef enum { 
    // рабочие события    // накопление в кольцевой буфер
    // логгирование последних 20 событий 
        JRN_EV_WORK_ACT_MODE_SET,                                   // 0 //установлен автоматический режим работы 
        JRN_EV_WORK_ACT_DI_1_STATE,                                 // 1 //пользовательский дискретный вход №1 сигнал 1 
        JRN_EV_WORK_ACT_DI_2_STATE,                                 // 2 //пользовательский дискретный вход №2 сигнал 1 
        JRN_EV_WORK_ACT_DI_3_STATE,                                 // 3 //пользовательский дискретный вход №3 сигнал 1
        JRN_EV_WORK_ACT_DI_4_STATE,                                 // 4 //пользовательский дискретный вход №4 сигнал 1
        JRN_EV_WORK_ACT_DI_5_STATE,                                 // 5 //пользовательский дискретный вход №5 сигнал 1
        JRN_EV_WORK_ACT_GATE_VALVE_GO_TO_POSITION,                  // 6 //достижение задвижкой концевых положений: открыто 
        JRN_EV_WORK_ACT_GATE_VALVE_START_MOVING,                    // 7 //пуск + направление движения задвижки 
        JRN_EV_WORK_ACT_GATE_VALVE_MOVING_STOP,                     // 8 //останов задвижки 
        JRN_EV_WORK_ACT_GATE_VALVE_MOVING_PRESSING,                 // 9 //дожим 
            JRN_EV_WORK_ACT_EVENTS_NUMBER,                              // 10 //окончание событий журнала рабочих действий 
    // логгирование событий за сутки
        JRN_EV_WORK_STATE_TURNING_ON,                               // 11 //включение    
        JRN_EV_WORK_STATE_SETTINGS_CHANGE,                          // 12 //изменение настроек 
        JRN_EV_WORK_STATE_FULL_MOTION_TIME_CALIBRATION,             // 13 //калибровка времени полного хода 
        JRN_EV_WORK_STATE_FULL_MOTION_AUTOSETTING,                  // 14 //автоподстройка полного хода 
        JRN_EV_WORK_STATE_SPLITTING_TRY,                            // 15 //попытки расклинивания 
        JRN_EV_WORK_STATE_MOVE_TO_SAFE_POSITION,                    // 16 //переход в безопасное положение 
            JRN_EV_WORK_STATE_EVENTS_NUMBER,                            // 17 //окончание событий журнала рабочих состояний
    // аварийные события 
    //** события по которым сохраняются показания   **/
        JRN_EV_CRASH_FREQUENCY_DEVIATION,                           // 18 //отклонение частоты 
        JRN_EV_CRASH_VOLTAGE_CHANGE,                                // 19 //перенапряжение 
        JRN_EV_CRASH_ASYMMETRY_VOLTAGES,                            // 20 //несимметрия напряжений 
        JRN_EV_CRASH_ASYMMETRY_CURRENTS,                            // 21 //несимметрия токов 
    //** ------------------------------------------ **//
        JRN_EV_CRASH_DRIVE_OVERHEAT,                                // 22 //перегрев двигателя
        JRN_EV_CRASH_POSITION_ERROR,                                // 23 //ошибка позиционирования
        JRN_EV_CRASH_JAMMING,                                       // 24 //заклинивание
        JRN_EV_CRASH_PHASE_ROTATION_WRONG,                          // 25 //неверное чередование фаз
        JRN_EV_CRASH_SUPPLY_WIRES_BREAK,                            // 26 //обрыв питающих фаз и привода, и системы управления
        JRN_EV_CRASH_OVERCURRENT_PROTECTION,                        // 27 //максимальная токовая защита
        JRN_EV_CRASH_OVERHEAT_STARTER,                              // 28 //перегрев пускателя
        JRN_EV_CRASH_OVERHEAT_SEMISTORS,                            // 29 //перегрев симисторов
        JRN_EV_CRASH_LOAD_BREAK,                                    // 30 //обрыв фаз нагрузки
        JRN_EV_CRASH_POWER_CIRCUIT_FAILURE,                         // 31 //неисправность силовой схемы
        JRN_EV_CRASH_LOAD_CONFIGURATION_INCORRECT,                  // 32 //неверная конфигурация нагрузки
        JRN_EV_CRASH_USB_FAILURE,                                   // 33 //неисправность USB
        JRN_EV_CRASH_ACTIVE_EXCHANGE_CHANNEL_COMMUNICATION_BREAK,   // 34 //обрыв связи (по активному каналу обмена)
        JRN_EV_CRASH_PTC_SENSOR_SHORT_CIRCUIT,                      // 35 //КЗ датчика PTC;
        JRN_EV_CRASH_EMERGENCY_STOP,                                // 36 //аварийный стоп;
        JRN_EV_CRASH_FULL_MOTION_CALIBRATE_ERROR,                   // 37 //ошибка калибровки полного хода.
        JRN_EV_CRASH_WRONG_PARAMETERS,                              // 38 //Недопустимое сочетание параметров при настройке прибора
        JRN_EV_CRASH_CLOUD_NO_CONNECTION,                           // 39 //Ошибка подключения к Cloud
            JRN_EV_CRASH_EVENTS_NUMBER,                                 // 40 //окончание событий журнала аварий 
            JRN_EV_ERROR_OUT_OF_BOUNDS                                  // 41 //error - значение вне диапазона
    } t_journal_event_id;


    typedef struct
    {
      unsigned char hours;            
      unsigned char minutes;          
      unsigned char seconds;          
      unsigned short milliseconds;      
    }t_journal_time;    //Journal Time structure definition

    typedef struct
    {
        unsigned char year;    
        unsigned char month;   
        unsigned char day;    
    }t_journal_date;    //Journal Date structure definition

    typedef struct 
    {
        t_journal_event_id event_id;    // уникальный код события 
        t_journal_time time;            // время возникновения события 
        t_journal_date date;            // дата возникновения события 
        float saved_data;               // данные для сохранения
        unsigned char crc;              // хеш сумма события 
        t_journal_name event_type;      // журнал, к которому принадлежит событие
    } t_record; // тип структуры записи рабочего события
    
    // тип структуры для задания границ периода выборки событий
    typedef struct {
        t_journal_date period_begin_date;
        t_journal_time period_begin_time;
        t_journal_date period_end_date;
        t_journal_time period_end_time;
    } t_jornal_period;

    // тип структуры хранения журнала рабочих событий
    typedef struct 
    {
        unsigned short journal_current_address;  // указатель на начало списка // на поле для записи нового события
        unsigned long jornal_lenght;            // глубина журнала // макс к-во записей
        unsigned short data_current_records;    // текущее к-во записей о событиях // отслеживание ограничения по данным
        unsigned short data_lenght;             // макс к-во доступных записей для чтения
        unsigned long  eeprom_start_address;    // начальный адрес журнала в EEPROM-памяти
        t_jornal_period period;                 // дата/время первого и последнего событий журнала
    } t_jornal_struct;

    // тип структуры вывода статистики журнала/-ов 
    typedef struct {
        unsigned char statistics_calculate_state;                    // флаг завершения расчета статистики
        unsigned char events_all_ok;                            // событий всего рассчитано
        unsigned char need_calc_statistics[JOURNAL_NUMBER];     // флаги запроса на расчет статистики для каждого журнала
        unsigned char need_find_event_first[JOURNAL_NUMBER];    // флаги на поиск начального события для каждого журнала 
        unsigned char need_find_event_last[JOURNAL_NUMBER];     // флаги на поиск последнего события для каждого журнала
        
        t_jornal_representation_type statistics_type;           // тип заказанной статистики // вариант представления данных
        t_jornal_period ordered_period;                         // период времени, за который требуется представить данные
    
        t_jornal_period really_period[JOURNAL_NUMBER];          // диапазон дат, для которого есть в наличии события для каждого журнала
        t_jornal_period really_period_all;                      // диапазон дат, для которого есть в наличии события
        unsigned short first_data_address[JOURNAL_NUMBER];      // указатель на начальное представленное событие для чтения для каждого журнала
        unsigned short events_amount[JOURNAL_NUMBER];           // количество представленных событий для каждого журнала
        unsigned short events_amount_all;                       // количество представленных событий всего
    } t_jornal_statistics;

    static const unsigned long EEPROM_FreeAreaSize = 199680;//8415;
    static const unsigned char StatisticsReady = 0xDA;

    /**
     * Инициализация начальных значений параметров
     * @param bakup_sram_base_addr - начальный адрес для помещения управляющей структуры журналов (в области BAKUP SRAM)
     * (0x4002 4000 - 0x4002 4FFF)
     */
    void modEventLog_Init (unsigned long bakup_sram_base_addr);


    /**
     * Стартует рассчет статистики и подготовку данных журналов для чтения за заданный период
     * @param statistics_type - вид представления данных согласно: t_jornal_representation_type
     * @param period - диапазон дат для выборки 
     */
    void modEventLog_CalcStatistics (t_jornal_representation_type image_type, t_jornal_period period);


    /**
     * Используется для проверки завершения расчета статистики и подготовки данных журналов к чтению 
     * или получения текущего состояния модуля журналов
     * @return - флаг завершения расчета статистики statistics_calculated
     * или текущая ошибка модуля журналов JournalError
     */
    unsigned char modEventLog_CheckJournalReady (void);

    /**
     * Записывает рассчитанные данные статистики по переданному указателю н структуру
     * @param statistics - структура вывода данных
     */
    void modEventLog_GetStatistics (t_jornal_statistics *statistics);


    /**
     * Сохраняет данные о событии (ID, время/дата) в журнал 
     * @param event_ID - ID события из t_journal_event_id
     * @param time - время возникновения
     * @param date - дата возникновения
     * @param crash_data - данные для сохранения (необязательно)
     */
    void modEventLog_EventSave (t_journal_event_id event_ID, t_journal_time time,  t_journal_date date, float crash_data = 0);

    /**
     * Читает первую представленную после расчета статистики запись о событии, затем следующую и так далее пока не закончатся записи.
     * Когда событий не останется, будет передавать нули.  
     * @param index - порядковый номер текущего читаемого события, инкрементируется самой функцией. Можно задать ненулевое значение для пропуска начальных записей
     * @param event - структура для получения данных
     */
    unsigned char modEventLog_EventRead (unsigned short *index, t_record *event);


    /**
     * Обработчик записи/чтения журналов, расчета статистики и подготовки данных
     */
    void modEventLog_Handler (void);
    
    
    /**
     * Запись на EEPROM массива данных в свободную от журналов область 
     * @param pData - указатель на буфер с данными
     * @param size - размер записи (мах  EEPROM_FreeAreaSize = 8415)
     * @return - статус обмена по SPI 
     */    
    unsigned char modEventLog_AreaWrite (unsigned char *pData, unsigned long size);
    
    /**
     * Чтение из EEPROM массива данных из свободной от журналов области 
     * @param pData - указатель на буфер
     * @param size  - количество байт (мах  EEPROM_FreeAreaSize = 8415)
     * @return - статус обмена по SPI 
     */
    unsigned char modEventLog_AreaRead (unsigned char *pData, unsigned long size);

    
////////////////////////////////////////////////////////////////////////////////
private:
    
    // смотри Расчет структуры памяти EEPROM для журналов.xlsx
    //static const unsigned char RecordSizeWork = 10;
    static const unsigned char RecordSize = 14;
    //static const unsigned char RecordsPerPageWork = 25;
    static const unsigned char RecordsPerPage = 18;
    static const unsigned short EEPROM_PageSize = 256;
    static const unsigned short EEPROM_PagesNum = 16384;//512;
    
    static const unsigned long EEPROM_JournalAdress_WA = 0; 
    static const unsigned long EEPROM_JornalLenght_WA  = 164060;//4000;
    static const unsigned short EEPROM_DataLenght_WA    = 200;
    
    static const unsigned long EEPROM_JournalAdress_WS = 2333440;//57088;
    static const unsigned long EEPROM_JornalLenght_WS  = 99216;//3600;
    static const unsigned short EEPROM_DataLenght_WS    = 1440;
    
    static const unsigned long EEPROM_JournalAdress_Cr = 3744512;//108288;
    static const unsigned long EEPROM_JornalLenght_Cr  = 2000;
    static const unsigned short EEPROM_DataLenght_Cr    = 1000;
    
    static const unsigned short BKPSRAM_JornalLenght  = 20;
    static const unsigned short BKPSRAM_DataLenght    = 20;
    static const unsigned long BKPSRAM_Adress = BKPSRAM_BASE;
    static const unsigned short BKPSRAM_Size = 4096;
    
    static const unsigned long CheckJournalTimeout = 60000; // ms
    
    static const unsigned long EEPROM_FreeAreaAdress = 3773440;//122624;
    static const unsigned long FLASH_SectorSize = 4096;
        
    enum { EEPROM_WIP_FLAG = 0x01U }; // Write In Progress (WIP) flag 
    
    enum { FLASH_COMM_SECTOR_ERASE = 0x20U }; // 
    
    typedef enum {
        EPPROM_INSTRUCTION_WRITE_SR = 1,    //0x01 WRSR Write Status Register
        EPPROM_INSTRUCTION_WRITE,           //0x02 WRITE Write to Memory Array
        EPPROM_INSTRUCTION_READ,            //0x03 READ Read from Memory Array 
        EPPROM_INSTRUCTION_WRITE_DIS,       //0x04 WRDI Write Disable
        EPPROM_INSTRUCTION_READ_SR,         //0x05 RDSR Read Status Register
        EPPROM_INSTRUCTION_WRITE_EN         //0x06 WREN Write Enable bit in Status Register
    }t_eeprom_instructions;
    
    typedef enum {
        HANDLER_WAIT,               // ожидание команды
        HANDLER_WRITE_EEPROM,       // запись 
        HANDLER_REPRESENT_J_RECORDS,// подготовка представления записей
        HANDLER_CALC_STATISTICS,    // расчет статистики
        HANDLER_CHECK_JOURNALS     // периодическая проверка журналов (дозапись из BKP_SRAM)
    }t_j_handler_states;

    typedef enum {
        RECORD_LEFT,    // событие слева от искомого 
        RECORD_MIDDLE,  // текущее прочитанное событие
        RECORD_RIGHT,   // событие справа от искомого
        RECORD_THREE    // всего сравниваемых событий
    }t_record_side; // сохраняемые записи для поиска 

    // тип структуры хранения журнала рабочих событий
    typedef struct 
    {
        unsigned char NumRecordsToWrite;        // количество событий ожидающих свою запись на EEPROM из BAKUP SRAM
        unsigned char AttepmtsCounterWrite;     // текущая попытка записи на EEPROM
        unsigned char ReadCounter[JOURNAL_NUMBER];  // текущий шаг в поиске нужного события
        // бэкап к-ва записанных данных в свободной области
        unsigned long AreaDataAmount;        
        // бэкап текущих данных журнала на случай сброса  
        unsigned short bkp_journal_current_address[JOURNAL_NUMBER]; // текущий указатель на место для записи
        unsigned short bkp_data_current_records[JOURNAL_NUMBER];    // текущее количество записей  
        t_jornal_period bkp_period[JOURNAL_NUMBER];                 // текущий диапазон дат
        
        t_j_handler_errors JournalError;    // текущая ошибка журнала
        t_jornal_struct BKPSRAM_JournalStatus;  // журнал в SRAM тоже журнал и имеет такую же структуру
        t_record BKPSRAM_Buffer[BKPSRAM_DataLenght];    // буфер событий ожидающих запись на EEPROM
    } t_jornal_sram_bakup;
    
//-----------------------------------------------------------------------------
    t_jornal_statistics JornalStatistics;           // структура хранения данных расчитанной статистики и отслеживания текущего адреса читаемого события и количества представленных событий во время чтения
    t_jornal_sram_bakup *BKPSRAM_Journal;           // указатель на структуру в BAKUP SRAM
    t_jornal_struct JournalObject[JOURNAL_NUMBER];  // структуры журналов 
    t_j_handler_states HandlerState;                // состояние автомата обработки событий
    t_j_handler_states NextHandlerState;            // следующее состояние автомата обработки событий
    t_record *event_record_write;                   // указатель на событие для записи
    t_record *event_record_read;                    // указатель на событие для чтения
    t_record CompareRecords[JOURNAL_NUMBER];        // массив из трех записей для сравнения событий
    unsigned short firstIndexRead[JOURNAL_NUMBER];  // начальный порядковый номер читаемой записи из представления
    unsigned long CheckJournalCounter;              // буфер хранения значения программного таймера периодической актуализации данных журналов
    unsigned char SectorEraseBuff[FLASH_SectorSize];            // Буфер для временного хранения данных во время стирания сектора
    
    /**
     * Читает запись о событии исходя из начального адреса журнала на EEPROM и прядкового номера записи в журнале
     * @param eepromStartAddress - начальный адрес журнала на EEPROM
     * @param JournalAdress - порядковый номер записи в журнале
     * @param record - указатеьл на структуру для считываемых данных
     * @return - статус обмена по SPI 
     */
    unsigned char modEventLog_RecordRead(unsigned long eepromStartAddress, unsigned short JournalAdress, cJournal::t_record *record);

    /**
     * Сравнивает две даты (год, месяц, день, час, мин, сек, милисек) и определяет которая из них в будущем 
     * @param period - структура для передачи дат для сравнения
     * @return: 1 - дата period_begin в будущем, дата period_end в прошлом
     *          0 - дата period_end в будущем, дата period_begin в пршлом
     */
    unsigned char modEventLog_DateTimeCompare (t_jornal_period period);
    
    /**
     * Расчитывает хеш-сумму буфера для определения повреждены данные или нет
     * @param buffer - указатель на буфер данных
     * @param buff_size - размер буфера
     * @return - расчитанная хеш-сумма 
     */
    unsigned char modEventLog_HashCode (unsigned char *buffer, unsigned char buff_size);

    /**
     * Определяет журнал, которому принадлежит событие по ID события
     * @param event_id - ID события
     * @return - ID журнала
     */
    cJournal::t_journal_name modEventLog_DetermineJournal (cJournal::t_journal_event_id event_id);

    /**
     * Инкрементирует индексы журнала для нового события, добавляет дату/время события как начальную дату/время периода журнала
     * @param journal - указатель на структуру журнала
     * @param time - время события
     * @param date - дата события
     */
    void modEventLog_JournalAddNewItem (cJournal::t_jornal_struct *journal, t_journal_time time,  t_journal_date date);

    /**
     * Переносит данные о событии в буфер ожидания записи на EEPROM
     * @param event_record - запись о событии
     */
    void modEventLog_SaveRecordSRAM (cJournal::t_record event_record);   
    
    /**
     * Читает запись о событии из буфера ожидания записи
     * @param index - порядковый номер записи
     * @param event_record - указатель на структуру 
     */
    void modEventLog_ReadRecordSRAM (unsigned short index, cJournal::t_record *event_record);
    
    /**
     * Запись на EEPROM буфера данных
     * @param adress - начальный адрес для записи
     * @param pData - указатель на буфер с данными
     * @param size - размер записи (мах 255)
     * @return - статус обмена по SPI 
     */    
    unsigned char modEventLog_WriteData (unsigned long adress, unsigned char *pData, unsigned short size);
    
    /**
     * Чтение из EEPROM буфера данных
     * @param adress - начальный адрес
     * @param pData - указатель на буфер
     * @param size  - количество байт
     * @return - статус обмена по SPI 
     */
    unsigned char modEventLog_ReadData (unsigned long adress, unsigned char *pData, unsigned short size);

    /**
     * Стирание сектора памяти 4096
     * @param adress - адрес начала сектора
      * @return - статус обмена по SPI 
     */
    unsigned char modEventLog_SectorErase (unsigned long adress);

};


#ifdef	__cplusplus
}
#endif

#endif	/* MOD_EVENTLOG_H */

