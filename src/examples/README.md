
# Code Examples

The examples here are intended to demonstrate use of the library and provide binaries
that can be used for debugging and memory checking. The build option `CLOGGER_BUILD_EXAMPLES` must
be enabled before each file's respective build options are allowed.

# simple_test.c
Demonstrates the basic features of the library and how to start logging.
* Build option: `CLOGGER_BUILD_EXAMPLE_SIMPLE`
* Binary name: `clogger_example_simple`

# stress_test.c
Creates multiple threads that each create a `logger_id` and send multiple messages
to the logger. Intended to test the performance of the logger.
* Build option: `CLOGGER_BUILD_EXAMPLE_STRESS`
* Binary name: `clogger_example_stress`

# feature_test.c
A more feature-complete example than `simple_test.c`, this file aims to demonstrate
all the major features of the library. Requires environment variables to be set to
test certain features.
* Build option: `CLOGGER_BUILD_EXAMPLE_FEATURE`
* Binary name: `clogger_example_feature`

# TODO
* Allow user to control number of threads and number of messages by passing CLI options
in `stress_test.c`
* Allow toggling features and/or pass options through CLI for `feature_test.c`
