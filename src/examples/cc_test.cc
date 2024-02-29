
#include <stdint.h>

#include "cc/Clogger.hpp"

#include <exception>
#include <sstream>
#include <string>

#include <iostream>

namespace {

using ::malorgith::clogger::logger_log_msg;
using ::malorgith::clogger::logger_log_msg_id;
using ::malorgith::clogger::loghandler_t;
using ::malorgith::clogger::logid_t;

using ::malorgith::clogger::Clogger;

}  // namespace

/*
 * Create a simple class that can be used to demonstrate how to log custom objects.
 */
class DemoClass {
  public:
    DemoClass() : value_(42) {}
    ~DemoClass() {}
    int getValue() const { return this->value_; }
  protected:
    int value_;
};

// functions must be declared inside the ::malorgith::clogger namespace
namespace malorgith {
namespace clogger {

// define how to convert the object into a string
template <> void to_log<DemoClass>(DemoClass const& value, ::std::string& log) {
    ::std::stringstream bar;
    bar << "DemoClass: " << ::std::to_string(value.getValue());
    log = bar.str();
}

// declare the function to handle sending the object to the log
template Clogger::CloggerEndpoint const& operator<< <DemoClass> (
    Clogger::CloggerEndpoint const& log,
    DemoClass const& value
);

}  // namespace clogger
}  // namespace malorgith

int main() {
    // initialize logger and start logging thread
    if (::malorgith::clogger::logger_init(::malorgith::clogger::kCloggerInfo)) {
        ::std::cerr << "Failed to initialize logger." << ::std::endl;
        return 1;
    }

    // create a file handler
    loghandler_t log_file_handler = ::malorgith::clogger::logger_create_file_handler("./logs/", "simple_test.log");
    if (log_file_handler == ::malorgith::clogger::kCloggerHandlerErr) {
        ::std::cerr << "Failed to open log file for writing." << ::std::endl;
        ::malorgith::clogger::logger_free();
        return 1;
    }

    // (optional) create a second handler that goes to stdout
    loghandler_t log_stdout_handler = ::malorgith::clogger::logger_create_console_handler(stdout);
    if (log_stdout_handler == ::malorgith::clogger::kCloggerHandlerErr) {
        ::std::cerr << "Failed to create stdout console handler." << ::std::endl;
        ::malorgith::clogger::logger_free();
        return 1;
    }

    // (optional) create a logid_t to identify the calling thread/function
    logid_t stdout_id = ::malorgith::clogger::logger_create_id_w_handlers("stdout", log_stdout_handler);
    if (stdout_id == ::malorgith::clogger::kCloggerMaxNumIds) {
        ::std::cerr << "Failed to create stdout logid_t." << ::std::endl;
        ::malorgith::clogger::logger_free();
        return 1;
    }

    // log a message
    if(logger_log_msg(::malorgith::clogger::kCloggerNotice, "Logging a message")) {
        ::std::cerr << "Failed to add first message to logger." << ::std::endl;
    }

    // log a message using a logid_t
    if (logger_log_msg_id(
        ::malorgith::clogger::kCloggerInfo,
        stdout_id,
        "Logging a message using a logid_t"
    )) {
        ::std::cerr << "Failed to add second message to logger." << ::std::endl;
    }

    // log a message to the default Clogger object
    Clogger::kLogger.kInfo << "log message using object" << Clogger::kEnd;

    // create a new Clogger object
    Clogger example_obj = Clogger::createLogger("example");

    // log a message to the new object
    example_obj.kInfo << "log message to new object" << Clogger::kEnd;

    // use printf format with object
    example_obj.log(::malorgith::clogger::kCloggerNotice, "this %s message %d", "notice", 1);

    // log a custom object
    DemoClass demo_obj;
    Clogger::kLogger.kInfo << demo_obj << Clogger::kEnd;

    // stop the log thread, close open handlers, and free memory when done
    if (::malorgith::clogger::logger_free()) {
        ::std::cerr << "Failed to stop the logger." << ::std::endl;
    }

    return 0;
}
