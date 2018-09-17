#include "input.h"
#include "mouse.h"
#include "GPIO.h"
#include <stdio.h>
#include <pthread.h>
#include <wiringPi.h>
#include <sys/time.h>

int g_init = 0;
int test_thread;

long long int get_timestamp()
{
	struct timeval timer_usec;
	long long int timestamp_usec;
	if (!gettimeofday(&timer_usec, NULL)) {
		timestamp_usec = ((long long int) timer_usec.tv_sec) * 1000000ll +
			(long long int) timer_usec.tv_usec;
		return timestamp_usec;
	}
	return -1;
}

void *_test (void *message)
{
        pinMode(26, INPUT);
        pinMode(27, INPUT);
        pinMode(28, INPUT);
        pinMode(29, INPUT);
        
		char pin26, pin27, pin28, pin29;
		char old26, old27, old28, old29;
		long long int base_timestamp;
        
        do
        {
            pin26 = digitalRead(26);
            pin27 = digitalRead(27);
            pin28 = digitalRead(28);
            pin29 = digitalRead(29);
            
            if (pin26 != old26 || pin27 != old27 || pin28 != old28 || pin29 != old29)
            {
				old26 = pin26;
				old27 = pin27;
				old28 = pin28;
				old29 = pin29;

                printf("%s%s%s%s  %f ms\n", 
					pin26? " ]": "[ ",
					pin27? "   ]": "  [ ",
					pin28? " |    ]": " |   [ ",
					pin29? "   ]": "  [ ",
					(get_timestamp() - base_timestamp) / 1000);
                fflush(stdout);
            }
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