#ifndef SEGMENTEDMESSAGE_H
#define SEGMENTEDMESSAGE_H

#define MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE 50  // 0 <= segmentCount <= MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE, theoretical upper bound is 127 (java does not have unsigned 8-bit int) but device runs out of stack 
#define PAYLOAD_LENGTH 16  // BLE Rx-Tx characteristic buffers are 20 bytes => PAYLOAD_LENGTH = 20 bytes - (M/m : 1byte), (messageID: 1byte), (segmentIndex/Count: 1byte), (';' : 1byte)
#define MESSAGE_LENGTH 800 // MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE * PAYLOAD_LENGTH, theoretical upper bound is 127*16 = 2032, but device runs out of stack 
#define TIMEOUT_OFFSET 10000 // milliseconds

enum SEGMENTED_MESSAGE_STATE {
    MESSAGE_SUCCESSFUL,
    MESSAGE_SEGMENTS_MISSING,
    MESSAGE_HASH_VERIFICATION_FAILED,
    MESSAGE_TIMED_OUT
};

struct SegmentedMessage
{
    /* data */
    int messageID = -1;  // default -1, -1 indicates message is inactive, 0 <= messageID < MAX_SEGMENTED_MESSAGES indicates message is alive (ongoing segment tranmission)
    int segmentCount = 0;  // allow message which has (MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE) segments of (PAYLOAD_LENGTH) bytes
    bool receivedSegments[MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE]; 
    char dataSegments[MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE][PAYLOAD_LENGTH+1]; 
    int startTimestamp = 0;  
    int timeout = 0; // 
    int messageState = -1;  // used for sending response
    bool messageReady = false;
    bool messageConsumed = false;
    char message[MESSAGE_LENGTH+1] = "";  
    char receivedHash[PAYLOAD_LENGTH+1] = "";
    char computedHash[PAYLOAD_LENGTH+1] = "";
};


#endif // SEGMENTEDMESSAGE_H
    