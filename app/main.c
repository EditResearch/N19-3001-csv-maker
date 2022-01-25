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
#define DB_PC_STATUS 288
#define DB_GLASS_STRUCT_SIZE 288

#define DEFAULT_CSV_PATH "./"
#define CSV_NAME "Klebezelle"
#define CSV_SEPARATOR ';'


/********************* data types definitions *****************/

/*
** Enum with work cycle states
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
** Enum with glass models
*/
typedef enum
{
   T7 = 7
   , ID_BUZZ = 1
}GlassModel;


/*
** structure with state bits as part of PC/PLC communication interface
*/
typedef struct
{
    uint8_t success : 1;
    uint8_t failed : 1;
}PCInterface;


/*
** DTL structure for recording date and time
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
** Structure for holding results states from primer control of the glass
*/
typedef struct
{
    uint8_t zone1:1;
    uint8_t zone2:1;
    uint8_t zone3:1;
    uint8_t zone4:1;
}PrimerDetectionZones;


/*
** Enum with Metralight inspection results state
*/
typedef enum
{
    MetralightOK = 0x10
    , MetralightNOK = 0x20
    , MetralightError = 255
}MetralightStatus;


/*
** Structure with validation information of barrel
*/
typedef struct
{
    char batchNumber[17];
    char serialNumber[17];
    uint16_t expiration_year;
    uint8_t expiration_month;
    uint8_t expiration : 1;
}BarrelInfo;


/*
** Structure with information from glass production
*/
typedef struct
{
    char jobNr[11];
    char vehicleNumber[14];
    char rearWindow[19];
    uint8_t vehicleModel;
    uint32_t id;
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
    int32_t mixerTubeLife;
    float ambientHumidity;
    float ambientTemperature;
}Glass;


/*
** Constants for csv header
*/
char * csv_header[][2] =
{
    {"JobNummer", ""}
    , {"AuftragsNr", ""}
    , {"ScheibenType", ""}
    , {"FahrzeugModell", ""}
    , {"ScheibenNr", ""}
    , {"TS_PrimerAuftrag", "Datum / Uhrzeit"}
    , {"PrimerDetektiert", "true/false/NaN"}
    , {"PrimerDetektiertZones", "true/false/NaN"}
    , {"TS_PrimerAbgetrocknet", "Datum / Uhrzeit"}
    , {"TI_PrimerAufgebrachtBisKleberaupe", "s"}
    , {"Lagerfach", ""}
    , {"TS_LetzteSpuelungMischer", "Datum / Uhrzeit"}
    , {"TS_KleberaupeStart", "Datum / Uhrzeit"}
    , {"TS_KleberaupeFertig", "Datum / Uhrzeit"}
    , {"Kleberaubenerkennung", "true/false/NaN"}
    , {"Metralight-Ergebnis umgangen", "true/false/NaN"}
    , {"MetralightZone1", "true/false/NaN"}
    , {"MetralightZone2", "true/false/NaN"}
    , {"MetralightZone3", "true/false/NaN"}
    , {"MetralightZone4", "true/false/NaN"}
    , {"MetralightZone5", "true/false/NaN"}
    , {"MetralightZone6", "true/false/NaN"}
    , {"MetralightZone7", "true/false/NaN"}
    , {"MetralightZone8", "true/false/NaN"}
    , {"MetralightZone9", "true/false/NaN"}
    , {"MetralightZone10", "true/false/NaN"}
    , {"MetralightZone11", "true/false/NaN"}
    , {"MetralightZone12", "true/false/NaN"}
    , {"TI_KleberaupeFertigBisScheibeEndnommen", "s"}
    , {"KomponenteA_Unabgelaufen", "true/false"}
    , {"KomponenteA_BatchId", ""}
    , {"KomponenteA_SerienNr", ""}
    , {"KomponenteA_Menge", "ml"}
    , {"AppizierdueseTempMin", "°C"}
    , {"AppizierdueseTempAktuell", "°C"}
    , {"AppizierdueseTempMax", "°C"}
    , {"KomponenteA_TempMin", "°C"}
    , {"KomponenteA_TempAktuell", "°C"}
    , {"KomponenteA_TempMax", "°C"}
    , {"KomponenteB_Unabgelaufen", "true/false"}
    , {"KomponenteB_BatchId", ""}
    , {"KomponenteB_SerienNr", ""}
    , {"KomponenteB_Menge", "ml"}
    , {"MischungsverhaeltnisKomponenten", ""}
    , {"MischerrohrLebensdauerVerbleibend", "s"}
    , {"RoboterZyklusOhneFehler", "true/false"}
    , {"DosiereinheitOhneFehler", "true/false"}
    , {"DrehtischOhneFehler", "true/false"}
    , {"KleberaupenauftragOhneFehler", "true/false"}
};


/********************* csv_maker functions ***************/

#define read_bit(array, byte, bit) \
    array[byte] & (1 << bit) ? 1 : 0


/*
** function for swaping bytes of 16-bits variable
*/
uint16_t
swap_endian_int16(uint16_t n)
{
    return ((n>>8) | (n<<8));
}


/*
** function for swaping bytes of 32-bits variables
*/
uint32_t
swap_endian_int32(uint32_t n)
{
    return (((n) >> 24)
     | (((n) & 0x00FF0000) >> 8)
     | (((n) & 0x0000FF00) << 8)
     | ((n) << 24));
}


/*
** function for swaping bytes of 32-bits float variable
*/
float
swap_endian_float(float n)
{
    return (float) swap_endian_int32((uint32_t) n);
}


/*
** generic macro for swaping bytes of 16-bits or 32-bits
** variable
*/
#define swap_endian(n)                 \
    _Generic(                          \
        (n)                            \
        , int16_t: swap_endian_int16   \
        , uint16_t: swap_endian_int16  \
        , int32_t: swap_endian_int32   \
        , uint32_t: swap_endian_int32  \
        , float: swap_endian_float)    \
            (n)


/*
** Conversion of constatn defining vehicle model into
** string representation
*/
char *
vehicle_model_to_string(
  GlassModel model)
{
  switch(model)
  {
    case T7:
      return "T7";
    case ID_BUZZ:
      return "ID.BUZZ";
    default:
      return "NaN";
  }
}


/*
** Conversion DTL structure (structured time format) into seconds
*/
uint64_t
dtl_to_seconds(DTL dtl)
{
  return ((uint64_t)(((((dtl.YEAR > 0) ? dtl.YEAR-1970 : 0))*31556925.216)
  + (dtl.MONTH*30.4368499*86400)
  + (dtl.DAY*86400)
  + (dtl.HOUR*3600)
  + (dtl.MINUTE*60)
  + dtl.SECOND));
}


/*
** Function which returns current time in string representation
** in given format.
** Returned string is allocated in bss memory location and must not be
** freed
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
** This function check if the given path in file system is valid
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
** Function for generation of output csv file name with address for
** storing based on given prefix and current time
** Returned string is allocated in bss memory location and must not be
** freed
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
** Function for reading DTL structure from given byte_array and
** given memory address
*/
DTL
read_dtl(
    size_t base
    , char byte_array[])
{
    return (DTL)
        {.YEAR = swap_endian(*(uint16_t*) (byte_array+base))
         , .MONTH = byte_array[base+2]
         , . DAY = byte_array[base+3]
         , .WEEKDAY = byte_array[base+4]
         , .HOUR = byte_array[base+5]
         , .MINUTE = byte_array[base+6]
         , .SECOND = byte_array[base+7]
         , .NANOSECOND = swap_endian(*(uint32_t*) (byte_array+8))};
}


/*
** Function for reading BarrelInfo structure from given byte_array and
** given memory address
*/
BarrelInfo
read_barrel_info(
    size_t base
    , char byte_array[])
{
    BarrelInfo barrel;

    memcpy(barrel.batchNumber, byte_array+base+2, 17);
    memcpy(barrel.serialNumber, byte_array+base+20, 17);
    barrel.expiration_year = swap_endian(*(uint16_t*) (byte_array+base+36));
    barrel.expiration_month = byte_array[base+38];
    barrel.expiration = read_bit(byte_array, 39, 0);

    return barrel;
}


/*
** Function for parsing Glass structure from byte array. Returned Glass
** structure is allocated in bss memory location and must not be freed
*/
Glass *
read_glass_structure(
    size_t size
    , char byte_array[size])
{
    static Glass glass;

    memcpy(glass.jobNr, (byte_array+2), 10);
    memcpy(glass.vehicleNumber, (byte_array+28), 13);
    memcpy(glass.rearWindow, (byte_array+44), 18);
    glass.vehicleModel = byte_array[62];
    glass.id = swap_endian(*(uint32_t*)(byte_array+64));
    glass.drawerIndex = swap_endian(*(uint16_t*)(byte_array+148));
    memcpy(glass.metralightZone, (byte_array+196), 12);
    glass.primerApplicationTime = read_dtl(74, byte_array);
    glass.primerFlashoffTime = read_dtl(86, byte_array);
    glass.timeSinceLastDispense = read_dtl(134, byte_array);
    glass.glueStartApplicationTime = read_dtl(98, byte_array);
    glass.glueEndApplicationTime = read_dtl(110, byte_array);
    glass.assemblyTime = read_dtl(122, byte_array);
    glass.A = read_barrel_info(208, byte_array);
    glass.B = read_barrel_info(248, byte_array);
    glass.aAppliedGlueAmount = swap_endian(*((float*) (byte_array+180)));
    glass.bAppliedGlueAmount = swap_endian(*(float*) (byte_array+184));
    glass.pistolTemperatureMin = swap_endian(*(int16_t*) (byte_array+152));
    glass.pistolTempDuringApp = swap_endian(*(float*) (byte_array+172));
    glass.pistolTemperatureMax = swap_endian(*(int16_t*) (byte_array+150));
    glass.aPotTemperatureMin = swap_endian(*(int16_t*) (byte_array+156));
    glass.aPotTempDuringApp = swap_endian(*(float*) (byte_array+176));
    glass.aPotTemperatureMax = swap_endian(*(int16_t*) (byte_array+154));
    glass.aApplicationRatio = swap_endian(*(float*) (byte_array+164));
    glass.bApplicationRatio = swap_endian(*(float*) (byte_array+168));
    glass.mixerTubeLife = swap_endian(*(int32_t*) (byte_array+158));
    glass.ambientHumidity = swap_endian(*(float*) (byte_array+188));
    glass.ambientTemperature = swap_endian(*(float*) (byte_array+192));
    glass.primerAppEnable = read_bit(byte_array, 163, 3);
    glass.primerInspectionEnable = read_bit(byte_array, 163, 2);
    glass.primerInspectionResult = read_bit(byte_array, 162, 1);
    glass.metralightEn = read_bit(byte_array, 162, 3);
    glass.glueApplicationResult = read_bit(byte_array, 162, 2);
    glass.glueInspectionBypass = read_bit(byte_array, 163, 4);
    glass.robotCompleteSuccess = read_bit(byte_array, 162, 5);
    glass.dispenseCompleteSuccess = read_bit(byte_array, 162, 4);
    glass.rotaryUniteCompleteSucces = read_bit(byte_array, 162, 7);
    glass.addhesiveProcessComplete = read_bit(byte_array, 163, 0);
    glass.zones =
        (PrimerDetectionZones)
            {.zone1 = byte_array[146] && (1 << 0)
            , .zone2 = byte_array[146] && (1 << 1)
            , .zone3 = byte_array[146] && (1 << 2)
            , .zone4 = byte_array[146] && (1 << 3)};

    return &glass;
}


/*
** Function for writing part of csv header into csv file
*/
void
print_csv_header(
  FILE * csv
  , int item_index)
{
    for(size_t i = 0
      ; i < (sizeof(csv_header) / sizeof(csv_header[0]))
      ; i++)
    {
        if(i==0)
            fprintf(csv, "%s", csv_header[i][item_index]);
        else
            fprintf(csv, ";%s", csv_header[i][item_index]);
    }
}


/*
** Function for writing whole csv header into csv file
*/
void
store_csv_header(FILE * csv)
{
    print_csv_header(csv, 0);
    fprintf(csv, "\n");
    print_csv_header(csv, 1);
}


/*
** This function prints DTL structure into given file
*/
#define print_dtl(file, dtl)          \
  fprintf(                            \
    file                              \
    , "%d-%02d-%02d %02d:%02d:%02d;"  \
    , dtl.YEAR                        \
    , dtl.MONTH                       \
    , dtl.DAY                         \
    , dtl.HOUR                        \
    , dtl.MINUTE                      \
    , dtl.SECOND)


/*
** Function for writing new csv line into csv file from given Glass structure
*/
void
store_csv_line(
  FILE * csv
  , Glass * glass
  , bool write_header)
{
    if(write_header == true)
      store_csv_header(csv);

    fprintf(csv, "\n%s;", glass->jobNr);
    fprintf(csv, "%s;", glass->vehicleNumber);
    fprintf(csv, "%s;", glass->rearWindow);
    fprintf(csv, "%s;", vehicle_model_to_string(glass->vehicleModel));
    fprintf(csv, "%d;", glass->id);

    if(glass->primerAppEnable == true)
      print_dtl(
        csv
        , glass->primerApplicationTime);
    else
      fprintf(csv, "NaN;");

    if(glass->primerInspectionEnable == true)
      fprintf(
        csv
        , "%s;"
        , glass->primerInspectionResult ? "true" : "false");
    else
      fprintf(csv, "NaN;");

    if(glass->primerInspectionEnable == true)
      fprintf(
        csv
        , "%s-%s-%s-%s;"
        , (glass->zones.zone1 ? "true" : "false")
        , (glass->zones.zone2 ? "true" : "false")
        , (glass->zones.zone3 ? "true" : "false")
        , (glass->zones.zone4 ? "true" : "false"));
    else
      fprintf(csv, "NaN;");

    if(glass->primerAppEnable == true)
      print_dtl(
        csv
        , glass->primerFlashoffTime);
    else
      fprintf(csv, "NaN;");

    // interval from primer application to start of glue application
    if(glass->primerAppEnable == true)
      fprintf(
        csv
        , "%lld;"
        , dtl_to_seconds(glass->glueStartApplicationTime)
          - dtl_to_seconds(glass->primerApplicationTime));
    else
      fprintf(csv, "NaN;");

    fprintf(csv, "%d;", glass->drawerIndex);
    print_dtl(csv, glass->timeSinceLastDispense);
    print_dtl(csv, glass->glueStartApplicationTime);
    print_dtl(csv, glass->glueEndApplicationTime);

    if(glass->metralightEn == true)
      fprintf(
        csv
        , "%s;"
        , glass->glueApplicationResult ? "true" : "false");
    else
      fprintf(csv, "NaN;");

    if(glass->metralightEn == true)
      fprintf(
        csv
        , "%s;"
        , glass->glueInspectionBypass ? "true" : "false");
    else
      fprintf(csv, "NaN;");

    if(glass->metralightEn == true)
    {
      for(int i = 0; i < 12; i++)
      {
        fprintf(
          csv
          , "%s;"
          , glass->metralightZone[i] == MetralightOK ? "true" : "false");
      }
    }
    else
    {
      for(int i = 0; i < 12; i++)
      {
        fprintf(
          csv
          , "NaN;");
      }
    }

    fprintf(
      csv
      , "%lld;"
      , dtl_to_seconds(glass->assemblyTime)
        - dtl_to_seconds(glass->glueEndApplicationTime));

    fprintf(
      csv
      , "%s;"
      , glass->A.expiration ? "true" : "false");

    fprintf(
      csv
      , "%s;"
      , glass->A.batchNumber);

    fprintf(
      csv
      , "%s;"
      , glass->A.serialNumber);

    fprintf(
      csv
      , "%f;"
      , glass->aAppliedGlueAmount);

    fprintf(
      csv
      , "%d;"
      , glass->pistolTemperatureMin);

    fprintf(
      csv
      , "%f;"
      , glass->pistolTempDuringApp);

    fprintf(
      csv
      , "%d;"
      , glass->pistolTemperatureMax);

    fprintf(
      csv
      , "%d;"
      , glass->aPotTemperatureMin);

    fprintf(
      csv
      , "%f;"
      , glass->aPotTempDuringApp);

    fprintf(
      csv
      , "%d;"
      , glass->aPotTemperatureMax);

    fprintf(
      csv
      , "%s;"
      , glass->B.expiration ? "true" : "false");

    fprintf(
      csv
      , "%s;"
      , glass->B.batchNumber);

    fprintf(
      csv
      , "%s;"
      , glass->B.serialNumber);

    fprintf(
      csv
      , "%f;"
      , glass->bAppliedGlueAmount);

    fprintf(
      csv
      , "%f:%f;"
      , glass->aApplicationRatio
      , glass->bApplicationRatio);

    fprintf(
      csv
      , "%d;"
      , glass->mixerTubeLife);

    fprintf(
      csv
      , "%s;"
      , glass->robotCompleteSuccess ? "true" : "false");

    fprintf(
      csv
      , "%s;"
      , glass->dispenseCompleteSuccess ? "true" : "false");

    fprintf(
      csv
      , "%s;"
      , glass->rotaryUniteCompleteSucces ? "true" : "false");

    fprintf(
      csv
      , "%s"
      , glass->addhesiveProcessComplete ? "true" : "false");
}


/*
** State function for connection to the PLC.
** This function cyclic trying to connect to PLC until it successfully connect
*/
State
connection(S7Object plc)
{
    if(Cli_ConnectTo(
        plc
        , IP_ADDRESS
        , RACK, SLOT) == 0)
    {
        fprintf(stdout, "PLC successfully connected.\n");
        return StateReadStatus;
    }
    else
        return StateConnection;
}


/*
** State function for reading request status bit from PLC
** This function cyclic reads request bit until it is not true
** If something wrong with connection it tries to reconnect
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
        return StateDisconnect;
}


/*
** State function for reading Glass structure from PLC and writing
** new csv line into csv file
*/
State
write_csv_line(
    S7Object plc
    , char path[])
{
  char db[DB_GLASS_STRUCT_SIZE];

  if(Cli_DBRead(plc, DB_INDEX, 0, DB_GLASS_STRUCT_SIZE, db) == 0)
  {
    Glass * glass =
      read_glass_structure(DB_GLASS_STRUCT_SIZE, db);

    char * file_path =
      generate_csv_name(
        time(NULL)
        ,  path
        , CSV_NAME);

    bool write_csv_header =
      access(file_path, F_OK) != 0;

      FILE * csv =
        fopen(
            file_path
          , "a");

      if(csv != NULL)
      {
        if(write_csv_header == true)
          fprintf(stdout, "Creating new csv file.\n");

        store_csv_line(
          csv
          , glass
          , write_csv_header);

        fclose(csv);
        fprintf(stdout, "Csv line stored.\n");

        return StateSuccess;
      }
      else
        fprintf(stderr, "Error during openg csv file!\n");
  }
  else
      fprintf(stderr, "Error during reading PLC datablock!\n");

    return StateFailure;
}


/*
** State function for settings of success state bit in PLC
*/
State
success(S7Object plc)
{
    PCInterface pc_interface =
        {.success = true
        , .failed = false};

    if(Cli_DBWrite(
        plc
        , DB_INDEX
        , DB_PC_STATUS
        , 1
        , &pc_interface) == 0)
        return StateFinish;
    else
        return StateDisconnect;
}


/*
** State function for waiting for reset request bit in PLC
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

            if(Cli_DBWrite(plc, DB_INDEX, DB_PC_STATUS, 1, &pc_interface) == 0)
            {
                fprintf(stdout, "Request finished.\n");
                return StateReadStatus;
            }
        }
        else
            return StateFinish;
    }

    return StateDisconnect;
}


/*
** State function for settings of failure state bit in PLC
*/
State
failure(S7Object plc)
{
    PCInterface pc_interface =
        {.success = false
        , .failed = true};

    if(Cli_DBWrite(plc, DB_INDEX, DB_PC_STATUS, 1, &pc_interface) == 0)
        return StateFinish;
    else
        return StateDisconnect;
}


/*
** State function for disconnection of PLC connection
*/
State
disconnect(S7Object plc)
{
    Cli_Disconnect(plc);

    return StateConnection;
}


/*
** Function where is main work cycle for communication with PLC
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

            case StateSuccess:
                state = success(plc);
                break;

            case StateDisconnect:
                state = disconnect(plc);
                break;

            default:
                state = StateDisconnect;
                break;
        }

        fflush(stdout);
        sleep(0.5);
    }

    Cli_Destroy(&plc);
}


int
main(int argc, char ** argv)
{
    fprintf(stdout, "Connecting to plc...\n");
    fflush(stdout);

    if(argc > 1)
        run(argv[1]);
    else
        run(DEFAULT_CSV_PATH);

    return EXIT_SUCCESS;
}
