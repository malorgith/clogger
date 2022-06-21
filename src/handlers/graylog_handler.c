
#include "graylog_handler.h"

#include <errno.h>  // to get error from send()
#include <netdb.h>  // used by getaddrinfo()
#include <stdlib.h>
#include <string.h> // memset()
#include <unistd.h>

#define MAX_HOSTNAME_LEN 100
#define MAX_GRAYLOG_URL_LEN 80
#define MAX_PORT_LEN 8
#define GRAYLOG_MAX_MESSAGE_LENGTH CLOGGER_MAX_MESSAGE_SIZE + 150

// global variables
static const char* g_sGraylogMsgFormat =
    "{"
        "\"version\":\"1.1\", "
        "\"short_message\":\"%s\", "
        "\"host\":\"%s\", "
        "\"level\":%d, "
        "\"_id\":\"%s\""
    "}";

typedef struct {
    char                m_sHostname[MAX_HOSTNAME_LEN];
    int                 m_nSocket;
    struct addrinfo*    m_pAddrinfo;
} grayloghandler_data;

static const size_t g_sizeData = { sizeof(grayloghandler_data) };

// private function declarations
static int _graylog_handler_close(log_handler *p_pHandler);
static int _graylog_handler_open(log_handler *p_pHandler);
static int _graylog_handler_isOpen(const log_handler *p_pHandler);
static int _graylog_handler_write(log_handler *p_pHandler, const t_loggermsg* p_sMsg);

// private function definitions
int _graylog_handler_close(log_handler *p_pHandler) {

    // TODO check below aborts if handler data is NULL; is that what we want?
    if (lgh_checks(p_pHandler, g_sizeData, "Graylog handler"))
        return 1;

    int t_nRtn = 0;
    grayloghandler_data *t_pData = (grayloghandler_data*)p_pHandler->m_pHandlerData;
    if (t_pData->m_nSocket < 0) {
        lgu_warn_msg("Graylog handler asked to close socket that isn't valid");
        t_nRtn++;
    }
    else {
        if (close(t_pData->m_nSocket)) {
            // failed to close socket
            lgu_warn_msg_int("Graylog handler failed to close socket with error: %d", errno);
            // TODO anything else we can do?
            t_nRtn++;
        }
        else {
            t_pData->m_nSocket = -1;
        }
    }

    if (((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_pAddrinfo != NULL) {
        free(((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_pAddrinfo);
        ((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_pAddrinfo = NULL;
    }

    return t_nRtn;
}

int _graylog_handler_open(log_handler *p_pHandler) {

    /*
     * TODO
     * Should we attempt to verify the size of m_pHandlerData in
     * these handler functions? Something like 'sizeof(*(p_pHandler->m_pHandlerData))'
     * could possibly be used to verify that the size of the data matches the
     * expected size of this handler's data struct.
     */

    if (p_pHandler == NULL) {
        lgu_warn_msg("Graylog handler asked to open NULL handler");
        return 1;
    }
    else if (p_pHandler->m_pHandlerData == NULL) {
        lgu_warn_msg("Graylog handler has no data saved");
        return 1;
    }
    else if (((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_pAddrinfo == NULL) {
        lgu_warn_msg("Graylog handler has no address info saved");
        return 1;
    }

    struct addrinfo *t_pAddrinfo = ((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_pAddrinfo;
    // create a socket for the address
    int t_nSocket = socket(t_pAddrinfo->ai_family, t_pAddrinfo->ai_socktype, t_pAddrinfo->ai_protocol);

    if (t_nSocket == -1) {
        lgu_warn_msg("Graylog handler failed to open socket");
        return 1;
    }

    // try to connect using the socket
    if (connect(t_nSocket, t_pAddrinfo->ai_addr, t_pAddrinfo->ai_addrlen) == -1) {
        // failed to connect
        lgu_warn_msg("Graylog handler failed to connect to opened socket");
        close(t_nSocket);
        return 1;
    }

    // socket opened and we connected
    ((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_nSocket = t_nSocket;

    // no longer need the addrinfo
    free(((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_pAddrinfo);
    ((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_pAddrinfo = NULL;

    return 0;
}

int _graylog_handler_isOpen(const log_handler *p_pHandler) {
    if (lgh_checks(p_pHandler, g_sizeData, "Graylog handler"))
        return 1;
    // return 1 so passes if() checks
    // TODO is 0 a valid socket? I doubt it...
    if (((grayloghandler_data*)p_pHandler->m_pHandlerData)->m_nSocket >= 0) return 1;
    else return 0;
}

int _graylog_handler_write(log_handler *p_pHandler, const t_loggermsg* p_sMsg) {

    if (lgh_checks(p_pHandler, g_sizeData, "Graylog handler"))
        return 1;
    else if (p_sMsg == NULL) {
        lgu_warn_msg("Graylog handler given NULL message to log");
        return 1;
    }

    grayloghandler_data *t_pData = (grayloghandler_data*)p_pHandler->m_pHandlerData;

    // make sure the socket has been opened
    if (t_pData->m_nSocket < 0) {
        lgu_warn_msg("Graylog handler asked to write to socket that hasn't been opened");
        return 1;
    }

    char msg[GRAYLOG_MAX_MESSAGE_LENGTH];
    static const char* t_sErrMsg = "couldn't format message for Graylog";
    if (lgu_wsnprintf(
        msg,
        GRAYLOG_MAX_MESSAGE_LENGTH,
        t_sErrMsg,
        g_sGraylogMsgFormat,
        p_sMsg,
        t_pData->m_sHostname,
        p_sMsg->m_nLogLevel,
        p_sMsg->m_sId
    )) {
        return 1;
    }

    // send() and write() are equivalent, except send() supports flags; when flags == 0, send() is the same as write()
    if (send(t_pData->m_nSocket, msg, strlen(msg) , 0 ) == -1) {
        lgu_warn_msg_int("Graylog handler failed to send a message with error number: %d", errno);
        return 2;
    }
    // TODO even though the message above is null-terminated, we have to send another
    if (send(t_pData->m_nSocket, "\0", sizeof(char), 0) == -1) {
        static const char *t_sErr2 = "Graylog handler failed to send null terminator after msg with error number: %d";
        lgu_warn_msg_int(t_sErr2, errno);
        return 3;
    }

    return 0;
}

// public function definitions
int create_graylog_handler(log_handler *p_pHandler, char* p_sServer, int p_nPort, int p_nProtocol) {

    if (p_pHandler == NULL) {
        lgu_warn_msg("can't create Graylog handler in NULL ptr");
        return 1;
    }

    struct addrinfo t_addrinfoHints;

    // 0-out the hints struct
    memset(&t_addrinfoHints, 0, sizeof(struct addrinfo));
    // set the values in t_addrinfoHints
    t_addrinfoHints.ai_family = AF_UNSPEC;        // accept IPv4 or IPv6
    t_addrinfoHints.ai_flags = 0;

    if (p_nProtocol == GRAYLOG_TCP) {
        // TCP
        t_addrinfoHints.ai_socktype = SOCK_STREAM;
        t_addrinfoHints.ai_protocol = IPPROTO_TCP;
    }
    else if (p_nProtocol == GRAYLOG_UDP) {
        // UDP
        t_addrinfoHints.ai_socktype = SOCK_DGRAM;
        t_addrinfoHints.ai_protocol = IPPROTO_UDP;
    }
    else {
        fprintf(stderr, "Can't create Graylog handler because an unknown protocol was given.\n");
        return 1;
    }

    // the port ('service' arg to getaddrinfo) must be a string
    char t_sPort[MAX_PORT_LEN];
    {
        static const char* t_sErrMsg = "Graylog handler failed to convert given port to a string";
        lgu_wsnprintf(t_sPort, (sizeof(char) * MAX_PORT_LEN), t_sErrMsg, "%d", p_nPort);
    }

    struct addrinfo *t_pResult = NULL;
    int t_nAddrInfoRtn = getaddrinfo(p_sServer, t_sPort, &t_addrinfoHints, &t_pResult);
    if (t_nAddrInfoRtn) {
        lgu_warn_msg("Graylog handler failed to get address info");

        // attempt to get a failure string
        static const int t_sMsgSize = 100;
        char t_sErrMsg[t_sMsgSize];
        static const char *t_sErr2 = "failed to get addressinfo error";
        if (!lgu_wsnprintf(t_sErrMsg, t_sMsgSize, t_sErr2, "%s", gai_strerror(t_nAddrInfoRtn))) {
            // output the error string
            lgu_warn_msg(t_sErrMsg);
        }

        if (t_pResult != NULL) {
            free(t_pResult);
        }
        return 1;
    }

    // iterate over the list of address structures
    int t_nSocket;
    struct addrinfo *t_pResultItr = NULL;
    for (t_pResultItr = t_pResult; t_pResultItr != NULL; t_pResultItr = t_pResultItr->ai_next) {

        // create a socket for the address
        t_nSocket = socket(t_pResultItr->ai_family, t_pResultItr->ai_socktype, t_pResultItr->ai_protocol);

        if (t_nSocket == -1)
            continue;   // couldn't create a socket; continue looping

        // try to connect using the socket
        if (connect(t_nSocket, t_pResultItr->ai_addr, t_pResultItr->ai_addrlen) != -1)
            break; // success

        // failed to connect above; close the socket
        close(t_nSocket);
    }

    if (t_pResultItr == NULL) {
        lgu_warn_msg("Graylog handler failed to bind to socket");
        freeaddrinfo(t_pResult);
        return 1;
    }

    // t_pResultItr != NULL, so we broke from the for loop; the socket is open
    grayloghandler_data *t_pData = (grayloghandler_data*)malloc(sizeof(grayloghandler_data));
    t_pData->m_nSocket = -1;
    t_pData->m_pAddrinfo = NULL;

    // save the addrinfo that worked
    t_pData->m_pAddrinfo = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    if (t_pData->m_pAddrinfo == NULL) {
        lgu_warn_msg("Graylog handler failed to allocate space for address info");
        free(t_pData);
        return 1;
    }
    memcpy(t_pData->m_pAddrinfo, t_pResultItr, sizeof(struct addrinfo));

    // free the original addrinfo now that we've saved the working info
    freeaddrinfo(t_pResult);

    // close the socket that was opened
    if (close(t_nSocket)) {
        lgu_warn_msg("Graylog handler failed to close success socket test to Graylog");
        // TODO how do we handle this? for now we'll fail, I guess
        free(t_pData->m_pAddrinfo);
        free(t_pData);
    }

    // get the hostname of the machine the logger is running on
    // sets the UNQUALIFIED hostname of the system; need more steps to get FQDN
    if (gethostname(t_pData->m_sHostname, MAX_HOSTNAME_LEN)) {
        lgu_warn_msg("Graylog handler failed to get hostname of the system the logger is running on");
        lgu_warn_msg_int("error number: %d", errno);
        return 1;
    }

    log_handler t_structHandler = {
        &_graylog_handler_write,
        &_graylog_handler_close,
        &_graylog_handler_open,
        &_graylog_handler_isOpen,
        false,
        false,
        t_pData
    };

    memcpy(p_pHandler, &t_structHandler, sizeof(log_handler));

    return 0;
}

