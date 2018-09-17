#include "mouse.h"
#include "GPIO.h"
#include "input.h"
#include <stdio.h>
#include <string.h>

char g_last_6_host_commands[6] = {0};
PS2_MOUSE_MODE g_mode;
int g_initialized;
int g_enabled;
int g_wait_for_params;

const char EXTENDED_MOUSE_TEST_COMMANDS[] = {0xF3, 0xC8, 0xF3, 0x64, 0xF3, 0x50};
const char EXTENDED_MOUSE2_TEST_COMMANDS[] = {0xF3, 0xC8, 0xF3, 0xC8, 0xF3, 0x50};

void push_history_host_command(char byte)
{
    char buffer[5];
    memcpy(buffer, g_last_6_host_commands + 1, 5);
    memcpy(g_last_6_host_commands, buffer, 5);
    g_last_6_host_commands[5] = byte;
}

void handle_host_message(PS2_MOUSE_HOST_COMMANDS host_command)
{
    if (host_command == RESET)
    {
        SendByteDev2Host(MOUSE, ACK);
        mouse_init();
    }
    else if (host_command == RESEND)
    {
        SendByteDev2Host(MOUSE, ACK);
        mouse_init();
    }
    else if (host_command == GET_DEVICE_ID)
    {
        if (strncmp(g_last_6_host_commands, EXTENDED_MOUSE_TEST_COMMANDS, 6))
        {
            g_mode = EXTENDED_MOUSE;
            SendByteDev2Host(MOUSE, ACK);
            SendByteDev2Host(MOUSE, EXTENDED_MOUSE);
        }
        else if (strncmp(g_last_6_host_commands, EXTENDED_MOUSE2_TEST_COMMANDS, 6))
        {
            g_mode = EXTENDED_MOUSE2;
            SendByteDev2Host(MOUSE, ACK);
            SendByteDev2Host(MOUSE, EXTENDED_MOUSE2);
        }
        else
        {
            SendByteDev2Host(MOUSE, ACK);
            SendByteDev2Host(MOUSE, GENERIC_MOUSE);
        }
    }
    else if (host_command == STAUTS_REPORT)
    {
        SendByteDev2Host(MOUSE, ACK);
        SendByteDev2Host(MOUSE, g_mode);
        SendByteDev2Host(MOUSE, 0x02);
        SendByteDev2Host(MOUSE, 0x64);
    }
    else if (host_command == ENABLE_DATA_REPORTING)
    {
        SendByteDev2Host(MOUSE, ACK);
        g_enabled = 1;
    }
    else if (host_command == SET_RESOLUTION || host_command == SET_SAMPLE_RATE)
    {
        SendByteDev2Host(MOUSE, ACK);
        g_wait_for_params = 1;
    }
    else if (g_wait_for_params)
    {
        SendByteDev2Host(MOUSE, ACK);
    }
    else if (host_command == 0)
    {
        return;
    }
    else
    {
        SendByteDev2Host(MOUSE, ACK);
    }
    push_history_host_command(host_command);
}

void mouse_init()
{
    g_mode = GENERIC_MOUSE;
    g_enabled = 0;
    SendByteDev2Host(MOUSE, BAT_SUCCESS);
    SendByteDev2Host(MOUSE, g_mode);
    g_initialized = 1;
}

void handle_mouse_messages(INPUT_EVENT_STRUCT *event_queue, int event_num)
{
    PS2_MOUSE_EXTENSION_PACK_STRUCT message;
    char host_byte;
    while (CheckHostHasMessage(MOUSE, &host_byte))
    {
        handle_host_message((PS2_MOUSE_HOST_COMMANDS)host_byte);
    }
    for (INPUT_EVENT_STRUCT *event_ptr = event_queue; event_ptr - event_queue < event_num; event_ptr++)
    {
        if (event_ptr->event_type == EVENT_MOUSE_BUTTON_PRESS || event_ptr->event_type == EVENT_MOUSE_BUTTON_RELEASE || event_ptr->event_type == EVENT_MOUSE_MOTION)
        {
            message.status = 0b00001000 | (event_ptr->motion_X < 0 ? 0b00011000 : 0) | (event_ptr->motion_Y < 0 ? 0b00101000 : 0);
            if (event_ptr->event_type == EVENT_MOUSE_BUTTON_PRESS)
                message.status |= (event_ptr->mouse_button == BUTTON_MIDDLE ? 0b00000100 : 0) | (event_ptr->mouse_button == BUTTON_RIGHT ? 0b0010 : 0) | (event_ptr->mouse_button == BUTTON_LEFT ? 0b0001 : 0);

            message.movement_x = (int)event_ptr->motion_X;
            message.movement_y = (int)event_ptr->motion_Y;
            if (event_ptr->event_type == EVENT_MOUSE_BUTTON_PRESS)
                message.movement_z = (event_ptr->mouse_button == BUTTON_FORWARD ? 1 : event_ptr->mouse_button == BUTTON_BACK ? -1 : 0);

			if (g_mode == GENERIC_MOUSE && g_initialized && g_enabled)
			{
				SendBytesDev2Host((char*)&message, 2);
			}
			else if (g_mode != GENERIC_MOUSE && g_initialized && g_enabled)
			{
				SendBytesDev2Host((char*)&message, 3);
			}
        }
    }
}