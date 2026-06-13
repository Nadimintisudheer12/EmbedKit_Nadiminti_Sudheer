#include <stdio.h>
#include <stdint.h>

#define SOF             0xAA
#define MAX_PAYLOAD     16

typedef enum
{
    WAIT_SOF,
    GET_CMD,
    GET_LEN,
    GET_PAYLOAD,
    GET_CHECKSUM
} State;

typedef struct
{
    State state;

    uint8_t cmd;
    uint8_t len;
    uint8_t payload[MAX_PAYLOAD];

    uint8_t checksum;
    uint8_t index;

    uint32_t timeout;
    uint32_t last_time;

} Parser;

void reset_parser(Parser *p)
{
    p->state = WAIT_SOF;
    p->index = 0;
    p->checksum = 0;
}

void init_parser(Parser *p, uint32_t timeout_ms)
{
    p->timeout = timeout_ms;
    p->last_time = 0;

    reset_parser(p);
}

int feed_byte(Parser *p, uint8_t byte, uint32_t time)
{
    /* Timeout check FIRST */
    if ((p->timeout != 0U) &&
        (p->state != WAIT_SOF))
    {
        if ((time - p->last_time) > p->timeout)
        {
            reset_parser(p);
            return -2;
        }
    }

    switch (p->state)
    {
        case WAIT_SOF:

            if (byte == SOF)
            {
                p->state = GET_CMD;
                p->last_time = time;
            }

            break;


        case GET_CMD:

            p->cmd = byte;
            p->checksum = byte;

            p->state = GET_LEN;
            p->last_time = time;

            break;


        case GET_LEN:

            p->len = byte;

            if (p->len > MAX_PAYLOAD)
            {
                reset_parser(p);
                return -1;
            }

            p->checksum ^= byte;
            p->index = 0;

            if (p->len == 0)
            {
                p->state = GET_CHECKSUM;
            }
            else
            {
                p->state = GET_PAYLOAD;
            }

            p->last_time = time;

            break;


        case GET_PAYLOAD:

            p->payload[p->index] = byte;
            p->index++;

            p->checksum ^= byte;

            if (p->index == p->len)
            {
                p->state = GET_CHECKSUM;
            }

            p->last_time = time;

            break;


        case GET_CHECKSUM:

            p->last_time = time;

            if (byte == p->checksum)
            {
                return 1;
            }
            else
            {
                reset_parser(p);
                return -1;
            }
    }

    return 0;
}


/* Print frame contents */
void print_frame(Parser *p)
{
    uint8_t i;

    printf("FRAME OK CMD=0x%02X LEN=%u PAYLOAD=[",
           p->cmd,
           p->len);

    for (i = 0; i < p->len; i++)
    {
        printf("%02X", p->payload[i]);

        if (i != (p->len - 1))
        {
            printf(" ");
        }
    }

    printf("]\n");
}


/* Feed array of bytes */
void feed_stream(Parser *p,
                 uint8_t bytes[],
                 uint32_t times[],
                 uint32_t count)
{
    uint32_t i;
    int result;

    for (i = 0; i < count; i++)
    {
        result = feed_byte(p, bytes[i], times[i]);

        printf("t=%3ums byte=0x%02X -> ",
               times[i],
               bytes[i]);

        if (result == 0)
        {
            printf("receiving...\n");
        }
        else if (result == 1)
        {
            print_frame(p);
            reset_parser(p);
        }
        else if (result == -1)
        {
            printf("CHECKSUM ERROR\n");
        }
        else if (result == -2)
        {
            printf("TIMEOUT -> parser reset\n");

            result = feed_byte(p, bytes[i], times[i]);

            printf("t=%3ums byte=0x%02X -> receiving... (re-fed)\n",
                   times[i],
                   bytes[i]);
        }
    }
}


int main(void)
{
    Parser parser;


    printf("\nTEST 1 : Valid Frame\n");

    init_parser(&parser, 50);

    uint8_t bytes1[] =
    {
        0xAA, 0x01, 0x03, 0x10, 0x20, 0x30, 0x02
    };

    uint32_t times1[] =
    {
        0, 5, 10, 15, 20, 25, 30
    };

    feed_stream(&parser, bytes1, times1, 7);


    printf("\nTEST 2 : Timeout Recovery\n");

    init_parser(&parser, 50);


    uint8_t bytes2[] =
    {
        0xAA,
        0x01,
        0x03,
        0x10,

        0xAA,
        0x05,
        0x01,
        0x7F,
        0x7B
    };

    uint32_t times2[] =
    {
        0,
        5,
        10,
        15,

        200,
        200,
        205,
        210,
        215
    };

    feed_stream(&parser, bytes2, times2, 9);

    printf("\nTEST 3 : Back-to-Back Frames\n");

    init_parser(&parser, 50);

    /* cs1 = 03 ^ 01 ^ 55 = 57 */
    /* cs2 = 04 ^ 02 ^ AA ^ BB = 17 */

    uint8_t bytes3[] =
    {
        0xAA,
        0x03,
        0x01,
        0x55,
        0x57,

        0xAA,
        0x04,
        0x02,
        0xAA,
        0xBB,
        0x17
    };

    uint32_t times3[] =
    {
        0,
        5,
        10,
        15,
        20,

        25,
        30,
        35,
        40,
        45,
        50
    };

    feed_stream(&parser, bytes3, times3, 11);

    printf("\nTEST 4 : Timeout Disabled\n");

    init_parser(&parser, 0);

    uint8_t bytes4[] =
    {
        0xAA,
        0x01,
        0x03,
        0x10,
        0xAA,
        0x05,
        0x01
    };

    uint32_t times4[] =
    {
        0,
        5,
        10,
        15,
        200,
        200,
        205
    };

    feed_stream(&parser, bytes4, times4, 7);

    return 0;
}
