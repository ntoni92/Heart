/*****************************************************************************
 *                                                                           *
 *                              GENERAL                                      *
 *                                                                           *
 *****************************************************************************/
#include "mbed.h"
#include "ble/BLE.h"

DigitalOut led1(LED1, 0);
DigitalOut led3(LED3);
DigitalIn  button(PUSH1);

/*****************************************************************************
 *                             SENSOR TYPE                                   *
 *****************************************************************************/
#define MIX2


/*****************************************************************************
 *                                                                           *
 *                              THERMOMETER                                  *
 *                                                                           *
 *****************************************************************************/

#ifdef THERMOMETER

#include "ble/services/HealthThermometerService.h"
#include "LSM6DS3/IMU.h"
 
static HealthThermometerService *thermometerService;

Serial pc(USBTX, USBRX);
SPI    spi(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS); // mosi, miso, sclk
IMU    imu(&pc, &spi);

static const char     DEVICE_NAME[]        = "THR_BLE_Gian";
static const uint16_t uuid16_list[]        = {GattService::UUID_HEALTH_THERMOMETER_SERVICE};
static float          currentTemperature   = 20;
 

/* Restart Advertising on disconnection*/
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance().gap().startAdvertising();
}
 
 
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;
 
    /* Check BLE errors */
    if (error != BLE_ERROR_NONE) {
        return;
    }
 
    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }
 
    ble.gap().onDisconnection(disconnectionCallback);
 
    /* Setup primary service. */
    thermometerService = new HealthThermometerService(ble, currentTemperature, HealthThermometerService::LOCATION_EAR);
    
    /* setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::THERMOMETER_EAR);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();    
}
 
int main(void)
{
    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
    imu.configIMU();
    
    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }
    
     // infinite loop
    while (1) {
        led1 = 1;
        wait_ms(500);
        
        currentTemperature = imu.readTempIMU();
        
        if (ble.gap().getState().connected) {    
            thermometerService->updateTemperature(currentTemperature);
        }
        ble.waitForEvent();

        led1 = 0;
        wait_ms(500);
    }
}

#endif //THERMOMETER



/*****************************************************************************
 *                                                                           *
 *                              HEARTRATE                                    *
 *                                                                           *
 *****************************************************************************/

#ifdef HEARTRATE

#include "ble/services/HeartRateService.h"

HeartRateService*  hrService;

const static char       DEVICE_NAME[] = "HRM_BLE_Gian";
static const uint16_t   uuid16_list[] = {GattService::UUID_HEART_RATE_SERVICE};
static uint8_t          hrmCounter    = 60; // init HRM to 60bps


void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance(BLE::DEFAULT_INSTANCE).gap().startAdvertising(); // restart advertising
}


void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE &ble          = params->ble;
    ble_error_t error = params->error;
    
    /* Check BLE errors */
    if (error != BLE_ERROR_NONE) {
        return;
    }
    
    /* Define diconnection action */
    ble.gap().onDisconnection(disconnectionCallback);

    /* Setup primary service. */
    hrService = new HeartRateService(ble, hrmCounter, HeartRateService::LOCATION_FINGER);

    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_HEART_RATE_SENSOR);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();
}

int main(void)
{  
    BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(bleInitComplete);

    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }

    // infinite loop
    while (1) {
        led1 = 1;
        wait_ms(500);
        
        (hrmCounter<100)? hrmCounter++ : hrmCounter=60;

        if ( ble.getGapState().connected ) {
            hrService->updateHeartRate(hrmCounter);      
        } 
        ble.waitForEvent();
        

        led1 = 0;
        wait_ms(500);
    }
}

#endif //HEARTRATE



/*****************************************************************************
 *                                                                           *
 *                              UARTBLE                                      *
 *                                                                           *
 *****************************************************************************/
#ifdef UARTBLE

#include "ble/services/UARTService.h"

UARTService* uartService;
uint8_t uartChar = 'A';

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance(BLE::DEFAULT_INSTANCE).gap().startAdvertising(); // restart advertising
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE &ble          = params->ble;
    ble_error_t error = params->error;
    
    /* Check BLE errors */
    if (error != BLE_ERROR_NONE) {
        return;
    }
    
    /* Define diconnection action */
    ble.gap().onDisconnection(disconnectionCallback);

    /* Setup primary service. */
    uartService = new UARTService(ble);

    /* setup advertising */
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)"BLE UART", sizeof("BLE UART") - 1);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));

    ble.setAdvertisingInterval(1000); /* 1000ms; in multiples of 0.625ms. */
    ble.startAdvertising();

}


int main(void)
{  
    BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(bleInitComplete);

    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }
    
    
    // infinite loop
    while (1) {
        led1 = 1;
        wait_ms(500);
        
        (uartChar<'Z')? uartChar++: uartChar='A';

        if ( ble.getGapState().connected ) {
           ble.updateCharacteristicValue(uartService->getRXCharacteristicHandle(), &uartChar, 1);  
        } 
        ble.waitForEvent();

        led1 = 0;
        wait_ms(500);
    }
}

#endif //UARTBLE



/*****************************************************************************
 *                                                                           *
 *                              MIX                                          *
 *                                                                           *
 *****************************************************************************/

#ifdef MIX

#include "ble/services/HeartRateService.h"
#include "ble/services/HealthThermometerService.h"
 

HeartRateService*           hrService;
HealthThermometerService*   thermometerService;


const static char       DEVICE_NAME[] = "MIX_BLE_Gian";
static const uint16_t   uuid16_list[] = {GattService::UUID_HEART_RATE_SERVICE,
                                         GattService::UUID_HEALTH_THERMOMETER_SERVICE};


static uint8_t   hrmCounter         = 60;
static float     currentTemperature = 20;


void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance(BLE::DEFAULT_INSTANCE).gap().startAdvertising(); // restart advertising
}


void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE &ble          = params->ble;
    ble_error_t error = params->error;
    
    /* Check BLE errors */
    if (error != BLE_ERROR_NONE) {
        return;
    }
    
    /* Define diconnection action */
    ble.gap().onDisconnection(disconnectionCallback);

    /* Setup primary service. */
    hrService = new HeartRateService(ble, hrmCounter, HeartRateService::LOCATION_FINGER);
    
    /* Setup secondary service. */
    thermometerService = new HealthThermometerService(ble, currentTemperature, HealthThermometerService::LOCATION_EAR);
    

    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::WATCH_SPORTS_WATCH);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();
}

int main(void)
{  
    BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(bleInitComplete);

    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }

    // infinite loop
    while (1) {
        led1 = 1;
        wait_ms(500);
        
        (hrmCounter<100)? hrmCounter++ : hrmCounter=60;
        (currentTemperature<60)? currentTemperature++ : currentTemperature=20;

        if ( ble.getGapState().connected ) {
            hrService->updateHeartRate(hrmCounter);
        } 
        ble.waitForEvent();
        
        
        if ( ble.getGapState().connected ) {
            thermometerService->updateTemperature(currentTemperature);
        } 
        ble.waitForEvent();
          

        led1 = 0;
        wait_ms(500);
    }
}

#endif //MIX







/*****************************************************************************
 *                                                                           *
 *                              MIX2                                         *
 *                                                                           *
 *****************************************************************************/

#ifdef MIX2


#include "LSM6DS3/BufferService.h"
#include "ble/services/HealthThermometerService.h" 
//#include "ble/services/EnvironmentalService.h"
#include "LSM6DS3/IMU.h"


Serial pc(USBTX, USBRX);
SPI    spi(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS); // mosi, miso, sclk
IMU    imu(&pc, &spi);


static HealthThermometerService*   thermometerService;
//static EnvironmentalService*   thermometerService;
BufferService*                uartService;

const static char       DEVICE_NAME[] = "ST_BlueNRG1";
static const uint16_t   uuid16_list[] = {GattService::UUID_HEALTH_THERMOMETER_SERVICE,//GattService::UUID_ENVIRONMENTAL_SERVICE,
                                         GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE};

static float     currentTemperature = 20;
//uint8_t          buff[]      = {0x00,0x00,';',0x00,0x00,';',0x00,0x00};
//uint8_t          buffer2[]   = {0x00,0x00,';',0x00,0x00,';',0x00,0x00};
//uint8_t          rawbuffer[] = {0x00,0x00,';',0x00,0x00,';',0x00,0x00};

uint8_t  size = 24;
uint8_t  buffer[24] = {};

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance(BLE::DEFAULT_INSTANCE).gap().startAdvertising(); // restart advertising
}


void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE &ble          = params->ble;
    ble_error_t error = params->error;
    
    /* Check BLE errors */
    if (error != BLE_ERROR_NONE) {
        return;
    }
    
    /* Define diconnection action */
    ble.gap().onDisconnection(disconnectionCallback);
    
    /* Setup primary service. */
    thermometerService = new HealthThermometerService(ble, currentTemperature, HealthThermometerService::LOCATION_BODY);   
    //thermometerService = new EnvironmentalService(ble);   
    
    /* Setup primary service. */  
    uartService = new BufferService(ble, buffer, size);   
    
    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::JOYSTICK);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();
}

int main(void)
{  
    int16_t raw_acc[3];
    char buff[24];
    float a = 0.061;
        
        
    BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(bleInitComplete);
    
    imu.configIMU();

    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }
    
    //while(!ble.getGapState().connected)
    //    ble.waitForEvent();

    // infinite loop
    while (1) {
                
        currentTemperature = imu.readTempIMU();
        //imu.readRawAcc(rawbuffer);  
        //buffer2[0] = rawbuffer[0];
        //buffer2[1] = rawbuffer[1];
        //buffer2[3] = rawbuffer[2];
        //buffer2[5] = rawbuffer[3];
        //buffer2[6] = rawbuffer[4];
        //buffer2[7] = rawbuffer[5];
        
        imu.readAccIMU(raw_acc);
        //sprintf(str, "%hd %hd %hd", (short)raw[0], (short)raw[1], (short)raw[2]);
        
        sprintf(buff, "%.0f %.0f %.0f    ", raw_acc[0]*a, raw_acc[1]*a, raw_acc[2]*a);

  
        if ( ble.getGapState().connected ) {
            thermometerService->updateTemperature(currentTemperature);
        } 
        ble.waitForEvent();
        
        
        if ( ble.getGapState().connected ) {
            uartService->sendBuffer((uint8_t*)buff, size);
        } 
        ble.waitForEvent();
        
        
        if ( ble.getGapState().connected ) {
            led1 = !led1;
        }
        ble.waitForEvent();
        
        
        wait_ms(100);
    }
}

#endif //MIX2


