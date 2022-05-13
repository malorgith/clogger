
# clogger

This library aims to provide a fast, lightweight, threaded logger for C and C++ applications.

An emphasis is placed on reducing the amount of time the calling thread spends when
logging a message.

# WARNING
This library is still under active development. **Implementation details might change with little
or no notice until the first major release.**

# Known Issues
* Does not support multiple handlers of the same type
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
* *(OPTIONAL)* Create an ID that will be included in log messages
    * `logger_create_id(<string_identifier>)`
* Send messages to the logger
    * `logger_log_msg(<int_msg_log_level>, <string_msg_format>, <msg_format_args>...)`
    * `logger_log_msg_id(<int_msg_log_level>, <logger_id>, <string_msg_format>, <msg_format_args>...)`
* Stop the log thread and free memory when done
    * `logger_free()`

Example code can be found in `src/examples`.

# TODO
* Create version-specific symlinks when installing the shared library
* Embed version/build info into compiled library
* Install appropriate CMake files with build output
* Add locks to the `logger_id` objects so we can retrieve their values safely in the logging thread
* Implement `logger_formatter` objects better and allow users to modify their properties
* Associate a `logger_formatter` with each handler that's created
* Add support for storing a `(const void*)` in `log_handler` objects for handler-specific data
* Associate `log_handler` objects (by a reference) to `logger_id` objects; use this to allow
messages to be sent to specific handlers based on the `logger_id` used
* *(MAYBE)* Support adding user-defined handlers to logger

