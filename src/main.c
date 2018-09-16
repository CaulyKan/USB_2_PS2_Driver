#include "input.h"
#include "mouse.h"
#include "GPIO.h"
#include <stdio.h>

int g_init = 0;

int main(int argc, char *argv[])
{
    INPUT_EVENT_STRUCT *event_queue;
    int event_queue_length;
    
    printf("main: pin init\n");
    fflush(stdout);
    
    PinInit();    

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