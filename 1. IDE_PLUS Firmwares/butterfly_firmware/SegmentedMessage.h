#ifndef SEGMENTEDMESSAGE_H
#define SEGMENTEDMESSAGE_H

#define MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE 30
#define PAYLOAD_LENGTH 16
#define MESSAGE_LENGTH 480 // MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE * PAYLOAD_LENGTH

struct SegmentedMessage
{
    /* data */
    int messageID; 
    int segmentCount;  // max is 127, allow message which has 30 segments of 16 bytes = 480 byte message
    bool receivedSegments[MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE]; // max is 127
    char dataSegments[MAX_NUMBER_OF_SEGMENTS_PER_MESSAGE][PAYLOAD_LENGTH+1]; // PAYLOAD_LENGTH + 1, to hold the null byte indicating c string
    int lastTimestamp;
    char message[MESSAGE_LENGTH] = "";  // max is 2032, message length = 16 * segmentCount
    char receivedHash[PAYLOAD_LENGTH] = "";
    char computedHash[PAYLOAD_LENGTH] = "";
};


#endif // SEGMENTEDMESSAGE_H
    