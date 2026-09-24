#!/usr/bin/env sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/weather-clock-tests.XXXXXX")
compiler=${CC:-cc}

cleanup() {
    rm -rf "$build_dir"
}
trap cleanup EXIT HUP INT TERM

"$compiler" -std=c99 -Wall -Wextra -Werror \
    "$repo_root/freertos/tests/weather_parser_test.c" \
    "$repo_root/freertos/User/esp_at/weather_parser.c" \
    -o "$build_dir/weather_parser_test"
"$build_dir/weather_parser_test"

"$compiler" -std=c99 -Wall -Wextra -Werror \
    "$repo_root/freertos/tests/at_response_test.c" \
    "$repo_root/freertos/User/esp_at/at_response.c" \
    -o "$build_dir/at_response_test"
"$build_dir/at_response_test"
