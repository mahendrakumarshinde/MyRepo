#ifndef IUMESSAGE_H_INCLUDED
#define IUMESSAGE_H_INCLUDED


/**
 * A data format specification for communication between the MCU, ESP chip and IU servers.
 * Added in Firmware v1.1.3
 */

typedef int16_t q15_t;      // Since we cannot include arm_math.h for ESP firmware.

namespace IUMessageFormat 
{
    /**
     * MCU --> ESP
     * Defines raw FFT values to be sent to server for processing. Also meant to store
     * timestamp when the values were copied from Feature buffer. Note that this timestamp
     * does NOT equal the time when this data was collected by the sensor.  
    */
    const int maxBlockSize = 4096;      // current max block size = 4096. Change this if block size is increased in future. 
    struct rawDataPacket {
        double timestamp;
        char axis;
        q15_t txRawValues[maxBlockSize];        
    };

    /**
     * ESP --> server
     * Defines the payload format which is sent as a HTTP POST to server.
     * The compiler pads the struct with extra bytes by default.
     * *****************        IMPORTANT       ************************************************************** 
     * We can't disable byte padding by the compiler because :
     * #pragma pack(1) does not work on the STM32 compiler. See https://stackoverflow.com/questions/31664452/pragma-packpush-1-crashes-in-gcc-4-4-7-possible-compiler-bug
     * __attribute__((packed, aligned(1))) works on the STM32 compiler, but does not work for ESP. 
     * Therefore, extra padding bytes have to be added manually.
     * Also note that the order of elements defined for this struct below matters. If you change the order, 
     * the compiler's padding operations might cause the structure to change in size which will cause errors in decoding the payload at the server.
     * 
     * Apparently, the compiler pads according to 8 byte boundaries. This compiler is weird.
     * sizeof rawDataHTTPPayload = 8248 bytes
     * *******************************************************************************************************
     */
    struct rawDataHTTPPayload {
        q15_t rawValues[maxBlockSize];
        char macId[24];                 //should be 18 but 6 extra padding bytes added for padding 8-byte alignment
        char firmwareVersion[8];
        double timestamp;
        int blockSize;
        int samplingRate;
        char axis;
        char padding[7];                // padding bytes, not used
    };
}
#endif //IUMESSAGE_H_INCLUDED
