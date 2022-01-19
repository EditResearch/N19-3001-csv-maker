#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <snap7.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>


/********************** configuration ************************/
#define IP_ADDRESS "192.168.2.1"
#define RACK 0
#define SLOT 1
#define DB_INDEX 18
#define DB_GLASS_STRUCT_SIZE 288

#define DEFAULT_CSV_PATH "./"
#define CSV_NAME "Klebezelle"
#define CSV_SEPARATOR ';'


/********************* data types definitions *****************/

/*
**
*/
typedef enum
{
    StateConnection
    , StateReadStatus
    , StatusWriteCsvLine
    , StateFinish
    , StateSuccess
    , StateFailure
    , StateDisconnect
}State;


/*
**
*/
typedef enum
{
   T7=7
    , ID_BUZZ = 1
}GlassModel;


/*
**
*/
typedef struct
{
    uint8_t success : 1;
    uint8_t failed : 1;
}PCInterface;


/*
**
*/
typedef struct
{
    uint16_t YEAR;
    uint8_t MONTH;
    uint8_t DAY;
    uint8_t WEEKDAY;
    uint8_t HOUR;
    uint8_t MINUTE;
    uint8_t SECOND;
    uint32_t NANOSECOND;
}DTL;


/*
**
*/
typedef struct
{
    uint8_t zone1:1;
    uint8_t zone2:1;
    uint8_t zone3:1;
    uint8_t zone4:1;
}PrimerDetectionZones;


/*
**
*/
typedef enum
{
    MetralightOK = 0x10
    , MetralightNOK = 0x20
    , MetralightError = 255
}MetralightStatus;


/*
**
*/
typedef struct
{
    char batchNumber[17];
    char serialSumber[17];
    uint16_t expiration_year;
    uint8_t expiration_month;
    uint8_t expiration :1;
}BarrelInfo; 


/*
**
*/
typedef struct
{
    char jobNr[11];
    char vehicleNumber[13];
    char rearWindow[19];
    uint8_t vehicleModel;
    uint32_t window;   
    uint8_t primerAppEnable : 1;
    uint8_t primerInspectionEnable : 1;
    uint8_t primerInspectionResult : 1;
    uint8_t  metralightEn : 1;
    uint8_t glueApplicationResult : 1;
    uint8_t glueInspectionBypass : 1;
    uint8_t robotCompleteSuccess :1;
    uint8_t dispenseCompleteSuccess   : 1;
    uint8_t rotaryUniteCompleteSucces : 1;
    uint8_t addhesiveProcessComplete  : 1;
    PrimerDetectionZones zones;
    uint16_t drawerIndex;
    DTL primerApplicationTime;
    DTL primerFlashoffTime;
    DTL timeSinceLastDispense;
    DTL glueStartApplicationTime;
    DTL glueEndApplicationTime;
    DTL assemblyTime;
    MetralightStatus metralightZone[12];
    BarrelInfo A;
    BarrelInfo B;
    float aAppliedGlueAmount;
    int pistolTemperatureMin;
    float pistolTempDuringApp;
    int pistolTemperatureMax;
    int aPotTemperatureMin;
    float aPotTempDuringApp;
    int aPotTemperatureMax;
    float bAppliedGlueAmount;
    float aApplicationRatio;
    float bApplicationRatio;
    float mixerTubeLife;
    float ambientHumidity;
    float ambientTemperature;
}Glass;


/********************* csv_maker functions *****************/



/*
** přečtení aktuálního času a uložení do řetězce
*/
char *
time_string(const char * format)
{
	static char time_string[24];

	time_t my_time;
	struct tm* time_info;

	time(&my_time);
	time_info = localtime(&my_time);
	strftime(time_string, 23, format, time_info);

	return time_string;
}


/*
** kontrola zda je cesta pro uložení souboru validní
*/
bool 
is_path_valid(char * path)
{
	if(access(path, F_OK) != 0)
	{
		if(errno == ENOENT 
            || errno == ENOTDIR)
			return false;
	}

	return true;
}


/*
**
*/
char * 
generate_csv_name(
    time_t t
    , char * csv_path
    , char * csv_name)
{
    static char f[513];
 
  	struct tm tm = *localtime(&t);

    snprintf(
        f
        , 512
        , "%s/%s-%d-%02d-%02d.csv"
        , csv_path
        , csv_name
        , tm.tm_year + 1900
        , tm.tm_mon + 1
        , tm.tm_mday);

    return f;
}


/*
**
*/
Glass *
read_glass_structure(
    size_t size
    , char byte_array[size])
{
    static Glass glass;

    return &glass;
}


/*
**
*/
void
store_csv_line(FILE * csv, Glass * glass)
{
    //TODO: code
}


/*
**
*/
State
connection(S7Object plc)
{
    if(Cli_ConnectTo(
        plc
        , IP_ADDRESS
        , RACK, SLOT) == 0)
        return StateReadStatus;
    else
        return StateConnection;
}


/*
**
*/
State 
read_status(S7Object plc)
{
    uint8_t store;

    if(Cli_DBRead(plc, DB_INDEX, 0, 1, &store) == 0)
    {
        if(store == true)
            return StatusWriteCsvLine;
        else
            return StateReadStatus;
    }
    else
        return StateConnection;
}


/*
**
*/
State
write_csv_line(
    S7Object plc
    , char path[])
{   
    FILE * csv = 
        fopen(
            generate_csv_name(time(NULL)
                , path
                , CSV_NAME)
            , "a");

    if(csv != NULL)
    {
        char db[DB_GLASS_STRUCT_SIZE];

        if(Cli_DBRead(plc, DB_INDEX, 0, 1, db) == 0)
        {
            Glass * glass = 
                read_glass_structure(DB_GLASS_STRUCT_SIZE, db);
            
            store_csv_line(csv, glass);
            
            return StateSuccess;
        }    
        else
        {
            //todo error debug
        }
            
        fclose(csv);
    }
    else
    {
        //todo error debug
    }
        
    return StateFailure;
}


/*
**
*/
State
success(S7Object plc)
{
    PCInterface pc_interface = 
        {.success = true
        , .failed = false}; 

    if(Cli_DBWrite(plc, DB_INDEX, 0, 1, &pc_interface) == 0)
        return StateFinish;
    else
        return StateDisconnect;
}


/*
**
*/
State
finish(S7Object plc)
{
    uint8_t store;

    if(Cli_DBRead(plc, DB_INDEX, 0, 1, &store) == 0)
    {
        if(store == false)
        {
            PCInterface pc_interface = 
                {.success = false
                , .failed = false}; 

            if(Cli_DBWrite(plc, DB_INDEX, 0, 1, &pc_interface) == 0)   
                return StateReadStatus;    
        }
        else
            return StateFinish;
    }
        
    return StateDisconnect;
}


/*
**
*/
State
failure(S7Object plc)
{
    PCInterface pc_interface = 
        {.success = false
        , .failed = true}; 

    if(Cli_DBWrite(plc, DB_INDEX, 0, 1, &pc_interface) == 0)
        return StateFinish;
    else
        return StateDisconnect;
}


/*
**
*/
State
disconnect(S7Object plc)
{
    Cli_Disconnect(plc);

    return StateConnection;
}


/*
**
*/
void
run(char * path)
{
    S7Object plc = Cli_Create();
    State state = StateConnection; 

    while(true)
    {
        switch(state)
        {
            case StateConnection:
                state = connection(plc);
                break;

            case StateReadStatus:
                state = read_status(plc);
                break;
        
            case StatusWriteCsvLine:
                state = write_csv_line(plc, path);
                break;

            case StateFinish:
                state = finish(plc);
                break;

            case StateFailure:
                state = failure(plc);
                break;

            default:
                state = StateDisconnect;
        }

        sleep(0.5);
    }

    Cli_Destroy(&plc);
}


int
main(int argc, char ** argv)
{
    fprintf(stdout, "Connecting to plc...\n");

    if(argc > 1)
        run(argv[1]);
    else
        run(DEFAULT_CSV_PATH);

    return EXIT_SUCCESS;
}



