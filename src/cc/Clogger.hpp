#ifndef MALORGITH_CLOGGER_CC_H_
#define MALORGITH_CLOGGER_CC_H_

#pragma once

#include "clogger.hpp"

#include <exception>
#include <memory>
#include <sstream>
#include <string>

namespace malorgith {
namespace clogger {

class Clogger {
  public:
    /*!
     * @brief Exception class.
     */
    class CloggerException : public ::std::exception {
      public:
        CloggerException() {}
        CloggerException(::std::string const& err_msg) : err_msg_(err_msg) {}
        ~CloggerException() {}

        char const* what() const throw() { return this->err_msg_.c_str(); }

      protected:
        ::std::string err_msg_;
    };

    /*!
     * @brief Class used to signal the end of a log message.
     */
    class CloggerEnd {
      public:
        CloggerEnd() {}
        ~CloggerEnd() {}
    };

    /*!
     * @brief Class used to log messages of different levels.
     */
    class CloggerEndpoint {
      public:
        /*!
         * @brief Constructor.
         */
        CloggerEndpoint(int log_level, ::std::stringstream* stream, logid_t* logid);

        /*!
         * @brief Destructor.
         */
        ~CloggerEndpoint();

        ::std::stringstream* getStream() const { return this->stream_; }
        int getLogLevel() const { return this->log_level_; }
        logid_t getLogID() const;

      protected:
        /*! pointer to the stringstream that will be updated */
        ::std::stringstream *stream_;
        /*! this endpoint's log level */
        int log_level_;
        /*! the logid for the endpoint */
        logid_t* logid_;
    };

    static Clogger const kLogger;
    static CloggerEnd const kEnd;

    /*!
     * @brief Create a new Clogger object with the specified ID.
     */
    static Clogger createLogger(char const* id_name);

    /*!
     * @brief Create a new Clogger object that logs to specific handlers.
     */
    static Clogger createLogger(char const* id_name, loghandler_t log_handlers);

    /*!
     * @brief Copy constructor.
     */
    Clogger(Clogger const&);

    /*!
     * @brief Move constructor.
     */
    Clogger(Clogger&&) noexcept;

    /*!
     * @brief Destructor.
     */
    ~Clogger();

    friend ::std::ostream& operator<<(::std::ostream& os, Clogger const& log);
    Clogger& operator=(Clogger const&) noexcept;
    Clogger& operator=(Clogger&&) noexcept;

    int log(int log_level, char const* msg, ...);

    logid_t getLogID() const;

    CloggerEndpoint const kEmergency;
    CloggerEndpoint const kAlert;
    CloggerEndpoint const kCritical;
    CloggerEndpoint const kError;
    CloggerEndpoint const kWarn;
    CloggerEndpoint const kNotice;
    CloggerEndpoint const kInfo;
    CloggerEndpoint const kDebug;

  protected:
    /*!
     * @brief Standard constructor.
     */
    Clogger(logid_t logid);

    /*!
     * @brief Perform basic checks and initialize the object.
     *
     * @throws CloggerException if a sanity check fails
     */
    static void _init(char const* id_str);

    /*! the logid_t for the object */
    logid_t log_id_;

    ::std::stringstream emergency_storage_;
    ::std::stringstream alert_storage_;
    ::std::stringstream critical_storage_;
    ::std::stringstream error_storage_;
    ::std::stringstream warn_storage_;
    ::std::stringstream notice_storage_;
    ::std::stringstream info_storage_;
    ::std::stringstream debug_storage_;

    ::std::shared_ptr<bool> can_delete_;

};

template <class T> void to_log(
    T const& value,
    ::std::string& stream
);

template <typename T> void to_log(
    T const* value,
    ::std::string& stream
);

template <class T>
Clogger::CloggerEndpoint const& operator<<(
    Clogger::CloggerEndpoint const& log,
    T const& value
) {
    ::std::string log_val;
    to_log(value, log_val);
    *log.getStream() << log_val;
    return log;
}

template <typename T>
Clogger::CloggerEndpoint const& operator<<(
    Clogger::CloggerEndpoint const& log,
    T* value
) {
    ::std::string log_val;
    to_log(value, log_val);
    *log.getStream() << log_val;
    return log;
}

// specialize operator<< for CloggerEnd
template <>
Clogger::CloggerEndpoint const& operator<<(
    Clogger::CloggerEndpoint const& log,
    Clogger::CloggerEnd const& value
);

}  // namespace clogger
}  // namespace malorgith

#endif // MALORGITH_CLOGGER_CC_H_
