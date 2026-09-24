#include "at_response.h"
#include <string.h>

static uint8_t tail_matches(const at_response_capture_t *capture,
                            const char *terminal)
{
    uint8_t terminal_length = (uint8_t)strlen(terminal);

    if (capture->tail_length < terminal_length)
        return 0U;

    return (memcmp(capture->tail + capture->tail_length - terminal_length,
                   terminal, terminal_length) == 0) ? 1U : 0U;
}

void AtResponse_Init(at_response_capture_t *capture, char *buffer,
                     uint16_t capacity)
{
    if (capture == 0)
        return;

    capture->buffer = buffer;
    capture->capacity = capacity;
    capture->length = 0U;
    capture->tail_length = 0U;
    capture->overflowed = 0U;
    if (buffer != 0 && capacity > 0U)
        buffer[0] = 0;
}

at_response_terminal_t AtResponse_Push(at_response_capture_t *capture,
                                       uint8_t byte)
{
    if (capture == 0)
        return AT_RESPONSE_PENDING;

    if (capture->buffer != 0 && capture->capacity > 0U &&
        capture->length < capture->capacity - 1U)
    {
        capture->buffer[capture->length++] = (char)byte;
        capture->buffer[capture->length] = 0;
    }
    else
    {
        capture->overflowed = 1U;
    }

    if (capture->tail_length < AT_RESPONSE_TAIL_SIZE)
    {
        capture->tail[capture->tail_length++] = (char)byte;
    }
    else
    {
        memmove(capture->tail, capture->tail + 1,
                AT_RESPONSE_TAIL_SIZE - 1U);
        capture->tail[AT_RESPONSE_TAIL_SIZE - 1U] = (char)byte;
    }

    if (tail_matches(capture, "\r\nERROR\r\n"))
        return AT_RESPONSE_ERROR;
    if (tail_matches(capture, "\r\nOK\r\n"))
        return AT_RESPONSE_OK;
    return AT_RESPONSE_PENDING;
}
