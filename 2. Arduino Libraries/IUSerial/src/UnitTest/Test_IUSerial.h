#ifndef TEST_IUSERIAL_H_INCLUDED
#define TEST_IUSERIAL_H_INCLUDED


#include <ArduinoUnit.h>
#include "UnitTestUtilities.h"
#include "../IUSerial.h"


test(IUSerial_buffer_reading)
{
    DummyHardwareSerial dhs;
    char testBuffer[200];

    IUSerial serial(&dhs, testBuffer, 200, IUSerial::LEGACY_PROTOCOL,
                    115200, ';', 100);

    // Buffer initially empty
    assertFalse(serial.hasNewMessage());
    assertEqual(serial.getCurrentBufferLength(), 0);

    // Beginning the HardwareSerial
    serial.begin();
    assertTrue(dhs.begun());
    assertEqual(dhs.getBaudRate(), 115200);

    // Buffer reading
    dhs.setRxBuffer("Hello", 5);
    serial.readToBuffer();
    assertEqual(dhs.available(), 0);
    assertEqual(strncmp(serial.getBuffer(), "Hello", 5), 0);
    assertEqual(serial.getCurrentBufferLength(), 5);

    dhs.setRxBuffer(" World!", 6);
    serial.readToBuffer();
    assertEqual(dhs.available(), 0);
    assertEqual(strncmp(serial.getBuffer(), "Hello World!", 11), 0);
    assertEqual(serial.getCurrentBufferLength(), 11);

    // Resetting
    serial.resetBuffer();
    assertEqual(serial.getCurrentBufferLength(), 0);

    // Read timeout
    assertEqual(serial.getCurrentBufferLength(), 0);
    dhs.setRxBuffer("Hello", 5);
    serial.readToBuffer();
    assertEqual(dhs.available(), 0);
    assertEqual(strncmp(serial.getBuffer(), "Hello", 5), 0);
    assertEqual(serial.getCurrentBufferLength(), 5);
    delay(120);

    dhs.setRxBuffer(" World!", 6);
    serial.readToBuffer();
    assertEqual(dhs.available(), 0);
    assertEqual(strncmp(serial.getBuffer(), " World!", 6), 0);
    assertEqual(serial.getCurrentBufferLength(), 6);
}

test(IUSerial_legacy_protocol)
{
    DummyHardwareSerial dhs;
    char testBuffer[200];
    IUSerial serial(&dhs, testBuffer, 200, IUSerial::LEGACY_PROTOCOL,
                    115200, ';', 100);
    serial.begin();

    // String termination
    dhs.setRxBuffer("Hello", 5);
    assertFalse(serial.readToBuffer());
    assertFalse(serial.hasNewMessage());
    assertEqual(strncmp(serial.getBuffer(), "Hello", 5), 0);
    assertEqual(serial.getCurrentBufferLength(), 5);

    dhs.setRxBuffer(" World!;", 8);
    assertTrue(serial.readToBuffer());
    assertTrue(serial.hasNewMessage());
    // Replacing ';' by string termination char 0
    assertEqual(strncmp(serial.getBuffer(), "Hello World!", 13), 0);
    assertEqual(serial.getCurrentBufferLength(), 13);

    // Resetting
    serial.resetBuffer();
    assertFalse(serial.hasNewMessage());
    assertEqual(serial.getCurrentBufferLength(), 0);


    // Reading several message at once
    dhs.setRxBuffer("message0;message1;", 18);
    // 1st message read
    assertTrue(serial.readToBuffer());
    assertTrue(serial.hasNewMessage());
    assertEqual(strncmp(serial.getBuffer(), "message0", 9), 0);
    assertEqual(serial.getCurrentBufferLength(), 9);
    // 2nd message read
    serial.resetBuffer();
    assertTrue(serial.readToBuffer());
    assertTrue(serial.hasNewMessage());
    assertEqual(strncmp(serial.getBuffer(), "message1", 9), 0);
    assertEqual(serial.getCurrentBufferLength(), 9);

    // Message sending, reading and decoding
    dhs.flush();
    serial.resetBuffer();
    // Sending
    serial.port->write("This is a test;");
    // Reading
    dhs.putTxIntoRx();
    assertTrue(serial.readToBuffer());
    assertTrue(serial.hasNewMessage());
    // Decoding
    assertEqual(strncmp(serial.getBuffer(), "This is a test", 15), 0);
    assertEqual(serial.getCurrentBufferLength(), 15);

}

test(IUSerial_ms_protocol)
{
    DummyHardwareSerial dhs;
    char testBuffer[200];

    IUSerial serial(&dhs, testBuffer, 200, IUSerial::MS_PROTOCOL,
                    115200, ';', 100);
    serial.begin();


    // Command sending, reading and decoding without message
    dhs.flush();
    serial.resetBuffer();
    // Sending
    serial.sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
    // Reading
    dhs.putTxIntoRx();
    assertTrue(serial.readToBuffer());
    assertTrue(serial.hasNewMessage());
    // Decoding
    assertEqual(serial.getMspCommand(), MSPCommand::WIFI_WAKE_UP);
    assertEqual(serial.getCurrentBufferLength(), 0);


    // Command sending, reading and decoding with message
    dhs.flush();
    serial.resetBuffer();
    // Sending
    serial.sendMSPCommand(MSPCommand::RECEIVE_BLE_MAC, "01:02:03:04:05:06");
    // Reading
    dhs.putTxIntoRx();
    assertTrue(serial.readToBuffer());
    assertTrue(serial.hasNewMessage());
    // Decoding
    assertEqual(serial.getMspCommand(), MSPCommand::RECEIVE_BLE_MAC);
    assertEqual(serial.getCurrentBufferLength(), 17);
    assertEqual(strncmp(serial.getBuffer(), "01:02:03:04:05:06", 17), 0);
}

#endif // TEST_IUSERIAL_H_INCLUDED
