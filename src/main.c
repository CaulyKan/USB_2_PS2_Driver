#include "input.h"
#include "mouse.h"
#include "GPIO.h"
#include <stdio.h>
#include <pthread.h>
#include <wiringPi.h>


int g_init = 0;
int test_thread;

void *_test (void *message)
{
    G_EnableDebug = 1;
        pinMode(28, INPUT);
        pinMode(29, INPUT);
        
        char pin28, pin29;
        long double t;
        
        do
        {
            pin28 = digitalRead(28);
            pin29 = digitalRead(29);
            
            if (G_EnableDebug)
            {
                if (pin28) printf(" ]"); else printf("[ ");
                if (pin29) printf ("   ]"); else printf("  [ ");
                printf("  %f ms\n", t*5/1000);
                fflush(stdout);
            }
            
            t+=1;
            delayMicroseconds(5);
        } while(1);
}

int main(int argc, char *argv[])
{
    INPUT_EVENT_STRUCT *event_queue;
    int event_queue_length;
    
    printf("main: pin init\n");
    fflush(stdout);
    
    PinInit();    
    
    pthread_create(&test_thread, NULL, _test, (void *)"input monitoring thread");
    

    while (1)
    {
        while (!GetHostStatus())
        {
            if (g_init)
            {
                g_init = 0;
                stop_listening();
                printf("main: uninit\n");
                fflush(stdout);
            }
        }

        if (!g_init)    
        {
            g_init = 1;

            printf ("main: mouse init\n");
            fflush(stdout); 
            mouse_init();

            printf ("main: input init\n");
            fflush(stdout);
            start_listening();
        }

        get_events(&event_queue, &event_queue_length);

        handle_mouse_messages(event_queue, event_queue_length);
    }
} 