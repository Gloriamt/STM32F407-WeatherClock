# Host parser test

The weather parser has no STM32 or FreeRTOS dependency. With a host C compiler,
run the test from this directory, for example:

```sh
cc -std=c99 -Wall -Wextra -Werror \
  weather_parser_test.c ../User/esp_at/weather_parser.c \
  -o weather_parser_test
./weather_parser_test
```

The cases cover valid positive and negative values, range boundaries, missing
fields, malformed integers and the rule that failed parsing must not overwrite
the previous valid weather values.
