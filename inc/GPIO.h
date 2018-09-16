#ifndef _GPIO_H
#define _GPIO_H

void PinInit(void);
int SendByteDev2Host(char byte_temp);
void SendBytesDev2Host(char *p_byte_temp, int bytes_length);
int CheckHostHasMessage(char *host_temp);
int GetHostStatus(); 

#endif