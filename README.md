
# clogger

This library aims to provide a fast, lightweight, threaded logger for C and C++ applications.

An emphasis is placed on reducing the amount of time the calling thread spends when
logging a message.

# WARNING
This library is still under active development. **Implementation details might change with little
or no notice until the first major release.**

# Known Issues
* Does not support modifying the format of the log output

# Using the library
* Initialize the logger
    * `logger_init(<int_log_level>)`
    * **NOTE** Initializing the logger will start the logging thread. If your program has any functions
that must be called before other threads start, such as `curl_global_init()`, they should be called first.
* Add a handler to the logger
    * `logger_create_file_handler(<string_file_path>, <string_file_name>)`
        * The file path can be relative or an absolute path to a directory to place the log file,
but a value must be specified. To place logs in the current directory, use `./`.
    * `logger_create_console_handler(<file_stdout OR file_stderr>)`
    * When a handler is created, a `t_handlerref` is returned. Multiple `t_handlerref` can be joined
using a bitwise OR (`|`) to create a reference that points to multiple handlers.
* *(OPTIONAL)* Create an ID that will be included in log messages
    * `logger_create_id(<string_identifier>)`
    * `logger_create_id_w_handlers(<string_identifier>, <handler_ref>)`
        * The handler reference can specify multiple handlers.
* Send messages to the logger
    * `logger_log_msg(<int_msg_log_level>, <string_msg_format>, <msg_format_args>...)`
        * Logs a message using the default ID ('main') to all available handlers.
    * `logger_log_msg_id(<int_msg_log_level>, <logger_id>, <string_msg_format>, <msg_format_args>...)`
* Stop the log thread and free memory when done
    * `logger_free()`

Example code can be found in `src/examples`.

# TODO
* Create version-specific symlinks when installing the shared library
* Embed version/build info into compiled library
* Install appropriate CMake files with build output
* Implement `logger_formatter` objects better and allow users to modify their properties
* Associate a `logger_formatter` with each handler that's created
* *(MAYBE)* Support adding user-defined handlers to logger

