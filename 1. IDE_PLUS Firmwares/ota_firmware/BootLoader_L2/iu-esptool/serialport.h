/***************************************************************************
 ***
 ***    serialport.h
 ***    - include file for serialport.c
 ***
 **/


#ifndef SERIALPORT_H
#define SERIALPORT_H

int serialport_open(const char *dev, unsigned int baudrate);
int serialport_close(void);
void serialport_set_dtr(unsigned char val);
void serialport_set_rts(unsigned char val);
void serialport_send_break();
void serialport_set_timeout(unsigned int timeout);
unsigned serialport_get_timeout();
void serialport_drain(void);
void serialport_flush(void);
unsigned serialport_read(unsigned char* data, unsigned int size);
unsigned serialport_write(const unsigned char* data, unsigned int size);

int serialport_send_slip(unsigned char *data, unsigned int size);
int serialport_receive_slip(unsigned char *data, unsigned int size);
int serialport_send_C0(void);
int serialport_receive_C0(void);

#endif
