
#include "cc/Clogger.hpp"

#include <stdarg.h>

#include <exception>
#include <sstream>
#include <string>

#include "clogger.h"
#include "logger.h"
#include "logger_id.h"

namespace malorgith {
namespace clogger {

Clogger const Clogger::kLogger(kCloggerDefaultId);
Clogger::CloggerEnd const Clogger::kEnd;

::std::ostream& operator<<(::std::ostream& os, Clogger const& log) {
    char dest[CLOGGER_ID_MAX_LEN];
    lgi_get_id(log.log_id_, dest);
    os << dest;
    return os;
}

Clogger::CloggerEndpoint::CloggerEndpoint(
    int log_level,
    ::std::stringstream* stream,
    logid_t* logid
) :
    stream_(stream),
    log_level_(log_level),
    logid_(logid)
{
}

Clogger::CloggerEndpoint::~CloggerEndpoint() {
}

// define generic to_log
template <class T> void to_log(
    T const& value,
    ::std::string& stream
) {
    stream = value;
}

// specialize to_log for bool
template <> void to_log <bool> (bool const& value, ::std::string& stream) {
    stream = (value) ? "true" : "false";
}

// specialize to_log for int
template <> void to_log<int>(int const& value, ::std::string& stream) {
    stream = ::std::to_string(value);
}

// specialize to_log for unsigned long
template <> void to_log<unsigned long>(unsigned long const& value, ::std::string& stream) {
    stream = ::std::to_string(value);
}

// declare ::std::string to_log
template void to_log <::std::string> (::std::string const&, ::std::string&);

// declare int operator<<
template Clogger::CloggerEndpoint const& operator<< <int> (
    Clogger::CloggerEndpoint const& log,
    int const& value
);

// declare ::std::string operator<<
template Clogger::CloggerEndpoint const& operator<< <::std::string> (
    Clogger::CloggerEndpoint const& log,
    ::std::string const& value
);

// specialize operator<< for CloggerEnd
template <>
Clogger::CloggerEndpoint const& operator<<(
    Clogger::CloggerEndpoint const& log,
    [[maybe_unused]]Clogger::CloggerEnd const& value
) {
    ::std::stringstream* stream = log.getStream();
    if (stream) {
        // log the contents of the stream
        logger_log_msg_id(log.getLogLevel(), log.getLogID(), stream->str().c_str());
        // clear the stream contents
        stream->str("");
    }
    return log;
}

logid_t Clogger::CloggerEndpoint::getLogID() const {
    return *this->logid_;
}

template <> void to_log<char>(char const* value, ::std::string& stream) {
    stream = value;
}

// decleare const char operator<<
template Clogger::CloggerEndpoint const& operator<< <char const> (
    Clogger::CloggerEndpoint const& log,
    char const* value
);

Clogger::Clogger(
    logid_t logid
) :
    kEmergency(kCloggerEmergency, &emergency_storage_, &log_id_),
    kAlert(kCloggerAlert, &alert_storage_, &log_id_),
    kCritical(kCloggerCritical, &critical_storage_, &log_id_),
    kError(kCloggerError, &error_storage_, &log_id_),
    kWarn(kCloggerWarn, &warn_storage_, &log_id_),
    kNotice(kCloggerNotice, &notice_storage_, &log_id_),
    kInfo(kCloggerInfo, &info_storage_, &log_id_),
    kDebug(kCloggerDebug, &debug_storage_, &log_id_),
    log_id_(logid),
    can_delete_(new bool(true))
{
}

Clogger::Clogger(
    Clogger const& src
) :
    kEmergency(kCloggerEmergency, &emergency_storage_, &log_id_),
    kAlert(kCloggerAlert, &alert_storage_, &log_id_),
    kCritical(kCloggerCritical, &critical_storage_, &log_id_),
    kError(kCloggerError, &error_storage_, &log_id_),
    kWarn(kCloggerWarn, &warn_storage_, &log_id_),
    kNotice(kCloggerNotice, &notice_storage_, &log_id_),
    kInfo(kCloggerInfo, &info_storage_, &log_id_),
    kDebug(kCloggerDebug, &debug_storage_, &log_id_),
    log_id_(src.log_id_),
    can_delete_(src.can_delete_)
{
    // TODO(malorgith): copy stream contents from src into new object
}

Clogger::Clogger(
    Clogger&& src
) noexcept :
    kEmergency(kCloggerEmergency, &emergency_storage_, &log_id_),
    kAlert(kCloggerAlert, &alert_storage_, &log_id_),
    kCritical(kCloggerCritical, &critical_storage_, &log_id_),
    kError(kCloggerError, &error_storage_, &log_id_),
    kWarn(kCloggerWarn, &warn_storage_, &log_id_),
    kNotice(kCloggerNotice, &notice_storage_, &log_id_),
    kInfo(kCloggerInfo, &info_storage_, &log_id_),
    kDebug(kCloggerDebug, &debug_storage_, &log_id_),
    log_id_(src.log_id_),
    can_delete_(src.can_delete_)
{
    // move constructor; change value of src
    src.log_id_ = kCloggerDefaultId;
}

Clogger::~Clogger() {
    if (
        logger_is_running() &&
        this->can_delete_.unique() &&
        this->log_id_ != kCloggerDefaultId
    ) {
        logger_remove_id(this->log_id_);
        // TODO(malorgith): log a message if remove fails?
    }
}

Clogger Clogger::createLogger(char const* id_str) {
    Clogger::_init(id_str);
    logid_t logid = logger_create_id(id_str);
    if (logid == kCloggerMaxNumIds) {
        throw CloggerException("failed to create ID");
    }
    return Clogger(logid);
}

Clogger Clogger::createLogger(
    char const* id_str,
    loghandler_t handlers
) {
    Clogger::_init(id_str);
    logid_t logid = logger_create_id_w_handlers(id_str, handlers);
    if (logid == kCloggerMaxNumIds) {
        throw CloggerException("failed to create ID");
    }
    return Clogger(logid);
}

void Clogger::_init(char const* id_str) {
    if (!logger_is_running()) {
        // TODO(malorgith): if all constructors are protected, this isn't needed
        throw CloggerException("logger isn't running");
    }
    else if (id_str == nullptr) {
        throw CloggerException("can't create log id from null");
    }
}

logid_t Clogger::getLogID() const {
    return this->log_id_;
}

Clogger& Clogger::operator=(Clogger const& src) noexcept {
    this->log_id_ = src.log_id_;
    this->can_delete_ = src.can_delete_;
    return *this;
}

Clogger& Clogger::operator=(Clogger&& src) noexcept {
    if (this == &src) {
        return *this;
    }
    this->log_id_ = src.log_id_;
    this->can_delete_ = src.can_delete_;
    src.log_id_ = kCloggerDefaultId;
    src.can_delete_.reset();
    return *this;
}

int Clogger::log(int log_level, char const* msg, ...) {
    int rtn_val = 0;
    va_list arg_list;
    va_start(arg_list, msg);
    rtn_val = _logger_log_msg(
        log_level,
        this->log_id_,
        NULL,
        msg,
        arg_list
    );
    va_end(arg_list);
    return rtn_val;
}

}  // namespace clogger
}  // namespace malorgith
