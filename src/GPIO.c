#include <wiringPi.h>
#include "GPIO.h"
#include <stddef.h>
#include <stdio.h>

#define mouse_data_pin 0  /*BCM.17 -> wiring Pi.0 -> GPIO.0*/
#define mouse_clock_pin 7 /*BCM.4 -> wiring Pi.7 -> GPIO.7*/
#define keyboard_data_pin 
#define keyboard_clock_pin
#define host_pin 1

int G_EnableBitProfile = 1;

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

void WriteBitDev2Host(int data_pin, int clock_pin, char bit_temp)
{
    write(data_pin, bit_temp == 0? LOW: HIGH);
    delayMicroseconds(20);
    write(clock_pin, LOW);
    delayMicroseconds(40);
    write(clock_pin, HIGH);
    delayMicroseconds(20);
}

char ReadBitHost2Dev(int data_pin, int clock_pin)
{
    delayMicroseconds(20);
    write(clock_pin, LOW);
    delayMicroseconds(40);
    write(clock_pin, HIGH);
    delayMicroseconds(20);
    return read(data_pin);
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

char GetOddParity(unsigned char cmd)
{
//    byte_temp ^= byte_temp >> 4;
//    byte_temp ^= byte_temp >> 2;
//    byte_temp ^= byte_temp >> 1;
//
//    return (~byte_temp) & 1;
    char D0, D1, D2, D3, D4, D5, D6, D7,parity;
    D0 = cmd & 0x01;
    D1 = cmd & 0x02;
    D2 = cmd & 0x04;
    D3 = cmd & 0x08;
    D4 = cmd & 0x10;
    D5 = cmd & 0x20;
    D6 = cmd & 0x40;
    D7 = cmd & 0x80;
    parity = D0 ^ D1 ^ D2 ^ D3 ^ D4 ^ D5 ^ D6 ^ D7;
    return parity;
}

void PinInit(void)
{
    wiringPiSetup();
    write(mouse_data_pin, HIGH);
	write(mouse_clock_pin, HIGH);
    write(keyboard_data_pin, HIGH);
	write(keyboard_clock_pin, HIGH);
 }

int GetHostStatus()
{
    return read(host_pin);
}

int SendByteDev2Host(int device, char byte_temp)
{
    printf("            0x%02x <%s\n", byte_temp, device? "keyboard": "mouse");
    fflush(stdout);

	int clock_pin = device ? keyboard_clock_pin : mouse_clock_pin;
	int data_pin = device ? keyboard_data_pin : mouse_data_pin;

    do
    {
        WaitHigh(clock_pin);                 //step1
        delayMicroseconds(50);               //step2
    } while (read(clock_pin) == LOW); //step3
    if (read(data_pin) != HIGH)       //step4
    {
        printf("gpio: Ignored host data\n");
        fflush(stdout);
        G_EnableDebug = 0;
        return 1;
    }
    else
    {
        delay(20); //step5

        if (G_EnableBitProfile) printf("gpio: write start bit;\n");
        /*START BIT, step6*/
        WriteBitDev2Host(data_pin, clock_pin, LOW);

        /*DATA BITS, step7*/
        if (G_EnableBitProfile) printf("gpio: write bit 1;\n");
        WriteBitDev2Host(data_pin, clock_pin, byte_temp & 1);
        if (G_EnableBitProfile) printf("gpio: write bit 2;\n");
        WriteBitDev2Host(data_pin, clock_pin, (byte_temp & 0b10) >> 1);
        if (G_EnableBitProfile) printf("gpio: write bit 3;\n");
        WriteBitDev2Host(data_pin, clock_pin, (byte_temp & 0b100) >> 2);
        if (G_EnableBitProfile)  printf("gpio: write bit 4;\n");
        WriteBitDev2Host(data_pin, clock_pin, (byte_temp & 0b1000) >> 3);
        if (G_EnableBitProfile) printf("gpio: write bit 5;\n");
        WriteBitDev2Host(data_pin, clock_pin, (byte_temp & 0b10000) >> 4);
        if (G_EnableBitProfile) printf("gpio: write bit 6;\n");
        WriteBitDev2Host(data_pin, clock_pin, (byte_temp & 0b100000) >> 5);
        if (G_EnableBitProfile) printf("gpio: write bit 7;\n");
        WriteBitDev2Host(data_pin, clock_pin, (byte_temp & 0b1000000) >> 6);
        if (G_EnableBitProfile) printf("gpio: write bit 8;\n");
        WriteBitDev2Host(data_pin, clock_pin, (byte_temp & 0b10000000) >> 7);

        /*PARITY BIT, step8*/
        if (G_EnableBitProfile) printf("gpio: write parity\n");
        WriteBitDev2Host(data_pin, clock_pin, GetOddParity(byte_temp));

        /*STOP BIT, step9*/
        if (G_EnableBitProfile) printf("gpio: write stop bit;\n");
        WriteBitDev2Host(data_pin, clock_pin, HIGH);
        
        delayMicroseconds(50); //step10
    }
}   

void SendBytesDev2Host(int device, char *p_byte_temp, int bytes_length)
{
    for (int i = 0; i < bytes_length; i++)
    {
        SendByteDev2Host(device, *p_byte_temp);
        p_byte_temp += 1;
    }
}

int CheckHostHasMessage(int device, char *host_temp)
{
    if (read(clock_pin) == HIGH)
    {
        return 0;
    }
    else
    {
        printf("gpio: prepare to read host\n");
        fflush(stdout);

		int clock_pin = device ? keyboard_clock_pin : mouse_clock_pin;
		int data_pin = device ? keyboard_data_pin : mouse_data_pin;
        
        WaitHigh(clock_pin); //step1

        //if (read(DataLine) == LOW) //step2
        //{
            delayMicroseconds(40);
//            if (read(DataLine) != LOW)
//            {
//                printf("gpio: Host not prepared;\n");
//                fflush(stdout);
//                return 0;
//            }
            /*DATA BITS, step3*/
            if (G_EnableBitProfile) printf("gpio: read bit 1;\n");
            HostBuffer = ReadBitHost2Dev(data_pin, clock_pin);
            if (G_EnableBitProfile) printf("gpio: read bit 2;\n");
            HostBuffer += ReadBitHost2Dev(data_pin, clock_pin) << 1;
            if (G_EnableBitProfile) printf("gpio: read bit 3;\n");
            HostBuffer += ReadBitHost2Dev(data_pin, clock_pin) << 2;
            if (G_EnableBitProfile) printf("gpio: read bit 4;\n");
            HostBuffer += ReadBitHost2Dev(data_pin, clock_pin) << 3;
            if (G_EnableBitProfile) printf("gpio: read bit 5;\n");
            HostBuffer += ReadBitHost2Dev(data_pin, clock_pin) << 4;
            if (G_EnableBitProfile) printf("gpio: read bit 6;\n");
            HostBuffer += ReadBitHost2Dev(data_pin, clock_pin) << 5;
            if (G_EnableBitProfile) printf("gpio: read bit 7;\n");
            HostBuffer += ReadBitHost2Dev(data_pin, clock_pin) << 6;
            if (G_EnableBitProfile) printf("gpio: read bit 8;\n");
            HostBuffer += ReadBitHost2Dev(data_pin, clock_pin) << 7;
            /*PARITY BIT, step4*/
            if (G_EnableBitProfile) printf("gpio: read bit parity;\n");
            char parity_temp = ReadBitHost2Dev(data_pin, clock_pin);

            //StopBit
            if (G_EnableBitProfile) printf("gpio: read stop bit;\n");
            ReadBitHost2Dev(data_pin, clock_pin);

            if (G_EnableBitProfile) printf("gpio: write act bit;\n");
            WriteBitDev2Host(LOW);
            write(data_pin, HIGH);

            if (parity_temp != GetOddParity(HostBuffer)){
                printf("gpio: parity error, %x?\n", HostBuffer);
                fflush(stdout);
                delayMicroseconds(45);
                return 0;
            }
            
            printf("host> 0x%x\n", HostBuffer);
            fflush(stdout);
            *host_temp = HostBuffer;
            
            delayMicroseconds(45);
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
