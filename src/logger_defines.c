
//#include "clogger.h"
#include "logger_defines.h"
#include __MALORGITH_CLOGGER_INCLUDE

__MALORGITH_NAMESPACE_OPEN

int const kCloggerBufferSize = { CLOGGER_BUFFER_SIZE };
int const kCloggerIdMaxLen = { CLOGGER_ID_MAX_LEN };
int const kCloggerMaxMessageSize = { CLOGGER_MAX_MESSAGE_SIZE };
int const kCloggerMaxNumHandlers = { CLOGGER_MAX_NUM_HANDLERS };
int const kCloggerMaxNumIds = { CLOGGER_MAX_NUM_IDS };

int const kCloggerEmergency = { LOGGER_EMERGENCY };
int const kCloggerAlert = { LOGGER_ALERT };
int const kCloggerCritical = { LOGGER_CRITICAL };
int const kCloggerError = { LOGGER_ERROR };
int const kCloggerWarn = { LOGGER_WARN };
int const kCloggerNotice = { LOGGER_NOTICE };
int const kCloggerInfo = { LOGGER_INFO };
int const kCloggerDebug = { LOGGER_DEBUG };

int const kCloggerMaxFormatSize = { CLOGGER_MAX_FORMAT_SIZE };

__MALORGITH_NAMESPACE_CLOSE
