#include "input.h"
#include "mouse.h"

int main(int argc, char *argv[])
{
    INPUT_EVENT_STRUCT *event_queue;
    int event_queue_length;

    start_listening();

    while (1)
    {
		while (0) // ���5v��Դ����
		{
			//�ȴ�5v
			mouse_init();
		}

        get_events(&event_queue, &event_queue_length);

		handle_mouse_messages(event_queue, event_queue_length);
    }
}