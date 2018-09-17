#ifndef _GPIO_H
#define _GPIO_H

extern int G_EnableBitProfile;

#define KEYBOARD 1
#define MOUSE 0

void PinInit(void);
int SendByteDev2Host(int device, char byte_temp);
void SendBytesDev2Host(int device, char *p_byte_temp, int bytes_length);
int CheckHostHasMessage(int device, char *host_temp);
int GetHostStatus(); 

#endif