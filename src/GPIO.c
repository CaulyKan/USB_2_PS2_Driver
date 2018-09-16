#include <wiringPi.h>
#include "GPIO.h"
#include <stddef.h>
#include <stdio.h>

#define DataLine 0  /*BCM.17 -> wiring Pi.0 -> GPIO.0*/
#define ClockLine 7 /*BCM.4 -> wiring Pi.7 -> GPIO.7*/
#define HostLine 1

int PinModes[40];

unsigned char HostBuffer;

char read(int pin)
{
    if (PinModes[pin] != INPUT)
    {
        pinMode(pin, INPUT);
        PinModes[pin] = INPUT;
    }
    char result = digitalRead(pin);
    return result;
}

void write(int pin, int state)
{
        if (PinModes[pin] != OUTPUT)
    {
        pinMode(pin, OUTPUT);
        PinModes[pin] = OUTPUT;
    }
    digitalWrite(pin, state);
}

void WriteBitDev2Host(char bit_temp)
{
    write(DataLine, bit_temp);
    delayMicroseconds(20);
    write(ClockLine, LOW);
    delayMicroseconds(40);
    write(ClockLine, HIGH);
    delayMicroseconds(20);
}

char ReadBitHost2Dev(void)
{
    delayMicroseconds(20);
    write(ClockLine, LOW);
    delayMicroseconds(40);
    write(ClockLine, HIGH);
    delayMicroseconds(20);
    return read(DataLine);
}

void WaitHigh(int gpio)
{
    while (read(gpio) == LOW)
        ;
}

void WaitLow(int gpio)
{
    while (read(gpio) == HIGH)
        ;
}

char GetOddParity(unsigned char byte_temp)
{
    byte_temp ^= byte_temp >> 4;
    byte_temp ^= byte_temp >> 2;
    byte_temp ^= byte_temp >> 1;

    return (~byte_temp) & 1;
}

void PinInit(void)
{
    wiringPiSetup();
    write(ClockLine, HIGH);
    char result = read(ClockLine);
 }

int GetHostStatus()
{
    return read(HostLine);
}

int SendByteDev2Host(char byte_temp)
{
    printf("            0x%02x <device\n", byte_temp);
    fflush(stdout);

    do
    {
        WaitHigh(ClockLine);                 //step1
        delayMicroseconds(50);               //step2
    } while (read(ClockLine) == LOW); //step3
    if (read(DataLine) != HIGH)       //step4
    {
        printf("Ignored host data\n");
        fflush(stdout);
        return 1;
    }
    else
    {
        delay(20); //step5

        /*START BIT, step6*/
        WriteBitDev2Host(LOW);

        /*DATA BITS, step7*/
        WriteBitDev2Host(byte_temp & 1);
        WriteBitDev2Host((byte_temp & 0b10) >> 1);
        WriteBitDev2Host((byte_temp & 0b100) >> 2);
        WriteBitDev2Host((byte_temp & 0b1000) >> 3);
        WriteBitDev2Host((byte_temp & 0b10000) >> 4);
        WriteBitDev2Host((byte_temp & 0b100000) >> 5);
        WriteBitDev2Host((byte_temp & 0b1000000) >> 6);
        WriteBitDev2Host((byte_temp & 0b10000000) >> 7);

        /*PARITY BIT, step8*/
        WriteBitDev2Host(GetOddParity(byte_temp));

        /*STOP BIT, step9*/
        WriteBitDev2Host(HIGH);
        
        delay(30); //step10
    }
}

void SendBytesDev2Host(char *p_byte_temp, int bytes_length)
{
    for (int i = 0; i < bytes_length; i++)
    {
        SendByteDev2Host(*p_byte_temp);
        p_byte_temp += 1;
    }
}

int CheckHostHasMessage(char *host_temp)
{
    if (read(ClockLine) == HIGH)
    {
        return 0;
    }
    else
    {
        WaitHigh(ClockLine); //step1

        //if (read(DataLine) == LOW) //step2
        //{
            delayMicroseconds(20);
            if (read(DataLine) != LOW)
            {
                printf("gpio: Host not prepaed;\n");
                fflush(stdout);
                return 0;
            }
ReadBitHost2Dev();
            /*DATA BITS, step3*/
            HostBuffer = ReadBitHost2Dev();
            HostBuffer += ReadBitHost2Dev() << 1;
            HostBuffer += ReadBitHost2Dev() << 2;
            HostBuffer += ReadBitHost2Dev() << 3;
            HostBuffer += ReadBitHost2Dev() << 4;
            HostBuffer += ReadBitHost2Dev() << 5;
            HostBuffer += ReadBitHost2Dev() << 6;
            HostBuffer += ReadBitHost2Dev() << 7;
            /*PARITY BIT, step4*/
            char parity_temp = ReadBitHost2Dev();


            if (parity_temp != GetOddParity(HostBuffer)){
                printf("gpio: parity error, %d?\n", HostBuffer);
                fflush(stdout);
                return 0;
            }
                
            //TEST CODE
            printf("host> 0x%x\n", HostBuffer);
            fflush(stdout);
            *host_temp = HostBuffer;

            //StopBit
            ReadBitHost2Dev();

            WriteBitDev2Host(LOW);
            write(DataLine, HIGH);
            
            delayMicroseconds(25);
            return 1;

//            /*STOP BIT, step5*/
//            if (WriteBitDev2Host(LOW))
//            {
//                return 1; /*Send Abort*/
//            }
//            else
//            {
//                if (read(DataLine) == LOW) //step6
//                {
//                    while (read(DataLine) != HIGH)
//                    {
//                        write(ClockLine, read(ClockLine));
//                    }
//                    return 1;
//                }
//                else
//                {
//                    /*Output Acknowledge bit, step7*/
//                    WriteBitDev2Host(LOW);
//                    WriteBitDev2Host(HIGH);
//
//                    /*Check parity bit, step8*/
//                    if (parity_temp != GetOddParity(HostBuffer))
//                    {
//                        return 1;
//                    }
//                    else
//                    {
//                        delayMicroseconds(45); //step9
//                        return 0;
//                    }
//                }
//            }
            
    }
}
