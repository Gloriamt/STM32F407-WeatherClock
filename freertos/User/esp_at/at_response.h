#ifndef AT_RESPONSE_H
#define AT_RESPONSE_H

#include <stdint.h>

#define AT_RESPONSE_TAIL_SIZE  9U

typedef enum
{
    AT_RESPONSE_PENDING = 0,
    AT_RESPONSE_OK,
    AT_RESPONSE_ERROR
} at_response_terminal_t;

typedef struct
{
    char *buffer;
    uint16_t capacity;
    uint16_t length;
    char tail[AT_RESPONSE_TAIL_SIZE];
    uint8_t tail_length;
    uint8_t overflowed;
} at_response_capture_t;

void AtResponse_Init(at_response_capture_t *capture, char *buffer,
                     uint16_t capacity);
at_response_terminal_t AtResponse_Push(at_response_capture_t *capture,
                                       uint8_t byte);

#endif
