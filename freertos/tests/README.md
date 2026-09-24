# Host parser tests

The weather and AT response parsers have no STM32 or FreeRTOS dependency. On a
POSIX host with a C compiler, run both suites from the repository root:

```sh
sh freertos/tests/run_tests.sh
```

Set `CC` to select a compiler, for example `CC=clang`. The script builds in a
temporary directory and leaves no binaries in the repository.

To compile the weather test manually from this directory:

```sh
cc -std=c99 -Wall -Wextra -Werror \
  weather_parser_test.c ../User/esp_at/weather_parser.c \
  -o weather_parser_test
./weather_parser_test
```

The cases cover valid positive and negative values, range boundaries, missing
and truncated fields, malformed integers, an API error response, null arguments
and the rule that failed parsing must not overwrite previous valid values.

The AT response capture test verifies `OK` and `ERROR` framing, truncated and
invalid terminals, null and zero-capacity buffers, embedded terminal words and
recognition of a trailing terminal after response-buffer overflow:

```sh
cc -std=c99 -Wall -Wextra -Werror \
  at_response_test.c ../User/esp_at/at_response.c \
  -o at_response_test
./at_response_test
```

GitHub Actions runs the same script when parser code, tests or the workflow
changes.
