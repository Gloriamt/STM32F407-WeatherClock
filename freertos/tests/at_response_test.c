#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../User/esp_at/at_response.h"

static at_response_terminal_t feed(at_response_capture_t *capture,
                                   const char *text)
{
    at_response_terminal_t terminal = AT_RESPONSE_PENDING;

    while (*text != 0 && terminal == AT_RESPONSE_PENDING)
        terminal = AtResponse_Push(capture, (uint8_t)*text++);
    return terminal;
}

static void test_ok_and_error(void)
{
    at_response_capture_t capture;
    char buffer[64];

    AtResponse_Init(&capture, buffer, sizeof(buffer));
    assert(feed(&capture, "AT\r\n\r\nOK\r\n") == AT_RESPONSE_OK);
    assert(!capture.overflowed);
    assert(strcmp(buffer, "AT\r\n\r\nOK\r\n") == 0);

    AtResponse_Init(&capture, buffer, sizeof(buffer));
    assert(feed(&capture, "AT+BAD\r\n\r\nERROR\r\n") == AT_RESPONSE_ERROR);
    assert(!capture.overflowed);
}

static void test_overflow_still_finds_terminal(void)
{
    at_response_capture_t capture;
    char buffer[8];

    AtResponse_Init(&capture, buffer, sizeof(buffer));
    assert(feed(&capture, "0123456789abcdef\r\nOK\r\n") == AT_RESPONSE_OK);
    assert(capture.overflowed);
    assert(buffer[sizeof(buffer) - 1U] == 0);
    assert(strcmp(buffer, "0123456") == 0);

    AtResponse_Init(&capture, buffer, sizeof(buffer));
    assert(feed(&capture, "0123456789abcdef\r\nERROR\r\n") ==
           AT_RESPONSE_ERROR);
    assert(capture.overflowed);
}

static void test_embedded_text_is_not_a_terminal(void)
{
    at_response_capture_t capture;
    char buffer[64];

    AtResponse_Init(&capture, buffer, sizeof(buffer));
    assert(feed(&capture, "{\"text\":\"OK ERROR\"}") ==
           AT_RESPONSE_PENDING);
    assert(!capture.overflowed);
}

static void test_truncated_response_has_no_terminal(void)
{
    at_response_capture_t capture;
    char buffer[32];

    AtResponse_Init(&capture, buffer, sizeof(buffer));
    assert(feed(&capture, "+HTTPCLIENT:12,{\"x\":1}") ==
           AT_RESPONSE_PENDING);
    assert(!capture.overflowed);
}

int main(void)
{
    test_ok_and_error();
    test_overflow_still_finds_terminal();
    test_embedded_text_is_not_a_terminal();
    test_truncated_response_has_no_terminal();
    puts("at_response_test: PASS");
    return 0;
}
