
#include "handlers/graylog_handler.h"

#include <errno.h>  // to get error from send()
#include <netdb.h>  // used by getaddrinfo()
#include <stdlib.h>
#include <string.h> // memset()
#include <unistd.h>

#include "logger_defines.h"

#define MAX_HOSTNAME_LEN 100
#define MAX_GRAYLOG_URL_LEN 80
#define MAX_PORT_LEN 8
#define GRAYLOG_MAX_MESSAGE_LENGTH CLOGGER_MAX_MESSAGE_SIZE + 150

__MALORGITH_NAMESPACE_OPEN

const int kGraylogTCP = 0;
const int kGraylogUDP = 1;

// global variables
static char const* kGraylogMsgFormat =
    "{"
        "\"version\":\"1.1\", "
        "\"short_message\":\"%s\", "
        "\"host\":\"%s\", "
        "\"level\":%d, "
        "\"timestamp\":%lu, "
        "\"_id\":\"%s\""
    "}";

typedef struct {
    /*! the hostname of the system the logger is running on */
    char hostname_[MAX_HOSTNAME_LEN];
    /*! the socket that will be used to send messages */
    int socket_;
    /*! addrinfo that should be used for the connection to graylog */
    struct addrinfo* addrinfo_;
    /*! pointer to the first addrinfo received; needed to free the info */
    struct addrinfo* addrinfo_start_;
} grayloghandler_data;

static const size_t kDataSize = { sizeof(grayloghandler_data) };

// private function declarations
static int _graylog_handler_close(log_handler *handler);
static int _graylog_handler_open(log_handler *handler);
static int _graylog_handler_isOpen(const log_handler *handler);
static int _graylog_handler_write(log_handler *handler, const t_loggermsg* msg);

// private function definitions
int _graylog_handler_close(log_handler *handler) {

    // TODO check below aborts if handler data is NULL; is that what we want?
    if (lgh_checks(handler, kDataSize, "Graylog handler"))
        return 1;

    int t_nRtn = 0;
    grayloghandler_data *data = (grayloghandler_data*)handler->handler_data_;
    if (data->socket_ < 0) {
        lgu_warn_msg("Graylog handler asked to close socket that isn't valid");
        t_nRtn++;
    }
    else {
        if (close(data->socket_)) {
            // failed to close socket
            lgu_warn_msg_int("Graylog handler failed to close socket with error: %d", errno);
            // TODO anything else we can do?
            t_nRtn++;
        }
        else {
            data->socket_ = -1;
        }
    }

    if (((grayloghandler_data*)handler->handler_data_)->addrinfo_start_ != NULL) {
        // free addrinfo using the original pointer
        freeaddrinfo(((grayloghandler_data*)handler->handler_data_)->addrinfo_start_);
        ((grayloghandler_data*)handler->handler_data_)->addrinfo_start_ = NULL;
        ((grayloghandler_data*)handler->handler_data_)->addrinfo_ = NULL;
    }

    free(data);
    handler->handler_data_ = NULL;

    return t_nRtn;
}

int _graylog_handler_open(log_handler *handler) {

    /*
     * TODO
     * Should we attempt to verify the size of handler_data_ in
     * these handler functions? Something like 'sizeof(*(handler->handler_data_))'
     * could possibly be used to verify that the size of the data matches the
     * expected size of this handler's data struct.
     */

    if (handler == NULL) {
        lgu_warn_msg("Graylog handler asked to open NULL handler");
        return 1;
    }
    else if (handler->handler_data_ == NULL) {
        lgu_warn_msg("Graylog handler has no data saved");
        return 1;
    }
    else if (((grayloghandler_data*)handler->handler_data_)->addrinfo_ == NULL) {
        lgu_warn_msg("Graylog handler has no address info saved");
        return 1;
    }

    struct addrinfo *addrinfo_ptr = ((grayloghandler_data*)handler->handler_data_)->addrinfo_;
    // create a socket for the address
    int socket_num = socket(addrinfo_ptr->ai_family, addrinfo_ptr->ai_socktype, addrinfo_ptr->ai_protocol);

    if (socket_num == -1) {
        lgu_warn_msg("Graylog handler failed to open socket");
        return 1;
    }

    // try to connect using the socket
    if (connect(socket_num, addrinfo_ptr->ai_addr, addrinfo_ptr->ai_addrlen) == -1) {
        // failed to connect
        lgu_warn_msg("Graylog handler failed to connect to opened socket");
        close(socket_num);
        return 1;
    }

    // socket opened and we connected
    ((grayloghandler_data*)handler->handler_data_)->socket_ = socket_num;

    // no longer need the addrinfo
    freeaddrinfo(((grayloghandler_data*)handler->handler_data_)->addrinfo_start_);
    ((grayloghandler_data*)handler->handler_data_)->addrinfo_start_ = NULL;
    ((grayloghandler_data*)handler->handler_data_)->addrinfo_ = NULL;

    return 0;
}

int _graylog_handler_isOpen(const log_handler *handler) {
    if (lgh_checks(handler, kDataSize, "Graylog handler"))
        return 1;
    // return 1 so passes if() checks
    // TODO is 0 a valid socket? I doubt it...
    if (((grayloghandler_data*)handler->handler_data_)->socket_ >= 0) return 1;
    else return 0;
}

int _graylog_handler_write(log_handler *handler, const t_loggermsg* msg) {

    if (lgh_checks(handler, kDataSize, "Graylog handler"))
        return 1;
    else if (msg == NULL) {
        lgu_warn_msg("Graylog handler given NULL message to log");
        return 1;
    }

    grayloghandler_data *data = (grayloghandler_data*)handler->handler_data_;

    // make sure the socket has been opened
    if (data->socket_ < 0) {
        lgu_warn_msg("Graylog handler asked to write to socket that hasn't been opened");
        return 1;
    }

    char char_msg[GRAYLOG_MAX_MESSAGE_LENGTH];
    static char const* err_msg = "couldn't format message for Graylog";
    if (lgu_wsnprintf(
        char_msg,
        GRAYLOG_MAX_MESSAGE_LENGTH,
        err_msg,
        kGraylogMsgFormat,
        msg,
        data->hostname_,
        msg->log_level_,
        msg->timestamp_,
        msg->id_
    )) {
        return 1;
    }

    // send() and write() are equivalent, except send() supports flags; when flags == 0, send() is the same as write()
    if (send(data->socket_, msg, strlen(char_msg) , 0 ) == -1) {
        lgu_warn_msg_int("Graylog handler failed to send a message with error number: %d", errno);
        return 2;
    }
    // TODO even though the message above is null-terminated, we have to send another
    if (send(data->socket_, "\0", sizeof(char), 0) == -1) {
        static char const* t_sErr2 = "Graylog handler failed to send null terminator after msg with error number: %d";
        lgu_warn_msg_int(t_sErr2, errno);
        return 3;
    }

    return 0;
}

// public function definitions
int create_graylog_handler(log_handler *handler, char const* graylog_url, int port, int protocol) {

    if (handler == NULL) {
        lgu_warn_msg("can't create Graylog handler in NULL ptr");
        return 1;
    }

    struct addrinfo addrinfo_hints;

    // 0-out the hints struct
    memset(&addrinfo_hints, 0, sizeof(struct addrinfo));
    // set the values in addrinfo_hints
    addrinfo_hints.ai_family = AF_UNSPEC;        // accept IPv4 or IPv6
    addrinfo_hints.ai_flags = 0;

    if (protocol == kGraylogTCP) {
        // TCP
        addrinfo_hints.ai_socktype = SOCK_STREAM;
        addrinfo_hints.ai_protocol = IPPROTO_TCP;
    }
    else if (protocol == kGraylogUDP) {
        // UDP
        addrinfo_hints.ai_socktype = SOCK_DGRAM;
        addrinfo_hints.ai_protocol = IPPROTO_UDP;
    }
    else {
        fprintf(stderr, "Can't create Graylog handler because an unknown protocol was given.\n");
        return 1;
    }

    // the port ('service' arg to getaddrinfo) must be a string
    char port_str[MAX_PORT_LEN];
    {
        static char const* err_msg = "Graylog handler failed to convert given port to a string";
        lgu_wsnprintf(port_str, (sizeof(char) * MAX_PORT_LEN), err_msg, "%d", port);
    }

    struct addrinfo *result = NULL;
    int addrinfo_rtn = getaddrinfo(graylog_url, port_str, &addrinfo_hints, &result);
    if (addrinfo_rtn) {
        lgu_warn_msg("Graylog handler failed to get address info");

        // attempt to get a failure string
        static const int msg_size = 100;
        char err_msg[msg_size];
        static char const* t_sErr2 = "failed to get addressinfo error";
        if (!lgu_wsnprintf(err_msg, msg_size, t_sErr2, "%s", gai_strerror(addrinfo_rtn))) {
            // output the error string
            lgu_warn_msg(err_msg);
        }

        if (result != NULL) {
            freeaddrinfo(result);
        }
        return 1;
    }

    // iterate over the list of address structures
    int socket_num;
    struct addrinfo *result_itr = NULL;
    for (result_itr = result; result_itr != NULL; result_itr = result_itr->ai_next) {

        // create a socket for the address
        socket_num = socket(result_itr->ai_family, result_itr->ai_socktype, result_itr->ai_protocol);

        if (socket_num == -1)
            continue;   // couldn't create a socket; continue looping

        // try to connect using the socket
        if (connect(socket_num, result_itr->ai_addr, result_itr->ai_addrlen) != -1)
            break; // success

        // failed to connect above; close the socket
        close(socket_num);
    }

    if (result_itr == NULL) {
        lgu_warn_msg("Graylog handler failed to bind to socket");
        freeaddrinfo(result);
        return 1;
    }

    // result_itr != NULL, so we broke from the for loop; the socket is open
    grayloghandler_data *data = (grayloghandler_data*)malloc(kDataSize);
    data->socket_ = -1;
    data->addrinfo_ = NULL;
    data->addrinfo_start_ = NULL;

    // save the addrinfo that worked
    data->addrinfo_ = result_itr;
    data->addrinfo_start_ = result;

    // close the socket that was opened
    if (close(socket_num)) {
        lgu_warn_msg("Graylog handler failed to close successful socket test to Graylog");
        // TODO how do we handle this? for now we'll fail, I guess
        freeaddrinfo(result);
        free(data);
    }

    // get the hostname of the machine the logger is running on
    // sets the UNQUALIFIED hostname of the system; need more steps to get FQDN
    if (gethostname(data->hostname_, MAX_HOSTNAME_LEN)) {
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
        data
    };

    memcpy(handler, &t_structHandler, sizeof(log_handler));

    return 0;
}

__MALORGITH_NAMESPACE_CLOSE
