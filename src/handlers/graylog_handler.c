
#include "graylog_handler.h"

//#include <arpa/inet.h>
#include <errno.h>  // to get error from send()
#include <netdb.h>  // used by getaddrinfo()
#include <stdlib.h>
#include <string.h> // memset()
//#include <sys/socket.h>
#include <unistd.h>

#define MAX_HOSTNAME_LEN 100
#define GRAYLOG_MAX_MESSAGE_LENGTH LOGGER_MAX_MESSAGE_SIZE + 150

// GLOBAL VARS
static int g_nSocket = { -1 };
static char* g_sHostname = { NULL };

static const char* g_sGraylogMsgFormat =
    "{"
        "\"short_message\":\"%s\", "
        "\"host\":\"%s\", "
        "\"facility\":\"test\","
        "\"level\":%d"
    "}";
// END GLOBAL VARS

// PRIVATE FUNCTION DECLARATIONS
static int _graylog_handler_close();
static int _graylog_handler_open();
static int _graylog_handler_isOpen();
static int _graylog_handler_write(const t_loggermsg* p_sMsg);
// END PRIVATE FUNCTION DECLARATIONS

// PRIVATE FUNCTION DEFINITIONS
int _graylog_handler_close() {

    // make sure the socket has been opened
    if (g_nSocket == -1) {
        fprintf(stderr, "Socket was not open.\n");
        // is this an error? the end result is that the socket isn't open...
        return 0;
    }

    if (close(g_nSocket) != 0) {
        fprintf(stderr, "Failed to close the socket. Error: %d\n", errno);
        return 1;
    }

    g_nSocket = -1; // mark that the socket is not open

    free(g_sHostname);

    return 0;
}

int _graylog_handler_open() {
    // FIXME this should be where the socket is actually opened
    return 0;
}

int _graylog_handler_isOpen() {
    // FIXME this should actually check if the socket has been opened
    // return 1 so passes if() checks
    return 1;
}

int _graylog_handler_write(const t_loggermsg* p_sMsg) {

    // make sure the socket has been opened
    if (g_nSocket == -1)
        return 1;

    char msg[GRAYLOG_MAX_MESSAGE_LENGTH];
    // TODO check the length of p_sMsg
    if (snprintf(msg, GRAYLOG_MAX_MESSAGE_LENGTH, g_sGraylogMsgFormat, p_sMsg, g_sHostname, p_sMsg->m_nLogLevel) >= GRAYLOG_MAX_MESSAGE_LENGTH) {
        fprintf(stderr, "The formatted message to be sent to Graylog exceed the maximum size allowed.\n");
        return 1;
    }
    // send() and write() are equivalent, except send() supports flags; when flags == 0, send() is the same as write()
//  printf("%s\n", msg);
    if (send(g_nSocket, msg, strlen(msg) , 0 ) == -1) {
        fprintf(stderr, "Error trying to send a message. Error number; %d\n", errno);
        return 2;
    }
    send(g_nSocket, "\0", sizeof(char), 0);

    return 0;
}
// END PRIVATE FUNCTION DEFINITIONS

int create_graylog_handler(log_handler *p_pHandler, char* p_sServer, int p_nPort, int p_nProtocol) {

    /*
     * FIXME
     * The socket shouldn't actually be opened here since the handler will
     * be created by a different thread than the one that will write to it.
     *
     * Most likely need to implement a void* inside the log_handler object.
     * Each handler function will also take a pointer to a handler. Each
     * handler can then access data that's specific to it in the functions that
     * are acting on it.
     */

    if (p_pHandler == NULL) {
        fprintf(stderr, "Can't initialize a NULL handler.\n");
        return 1;
    }

    struct addrinfo t_addrinfoHints;
    struct addrinfo *t_pResult, *t_pResultItr;
    int t_nSocket, t_nAddrInfoRtn;

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
    int t_nPortStrLen = 8;
    char t_sPort[t_nPortStrLen];
    if ((size_t) snprintf(t_sPort, (sizeof(char) * t_nPortStrLen), "%d", p_nPort) >= (sizeof(char) * t_nPortStrLen)) {
        // snprintf() couldn't write all of the bytes
        fprintf(stderr, "Failed to convert the port value given to a string.\n");
        return 1;
    }

    t_nAddrInfoRtn = getaddrinfo(p_sServer, t_sPort, &t_addrinfoHints, &t_pResult);
    if (t_nAddrInfoRtn != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(t_nAddrInfoRtn));
        return 1;
    }

    // iterate over the list of address structures
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
        fprintf(stderr, "Could not bind.\n");
        // should we free t_pResult?
        return 1;
    }

    // t_pResultItr != NULL, so we broke from the for loop; the socket is open

    // no longer need the address info that was returned since we have opened the socket
    freeaddrinfo(t_pResult);

    g_nSocket = t_nSocket;

    // get the hostname of the machine the logger is running on
    g_sHostname = (char*) malloc(sizeof(char) * MAX_HOSTNAME_LEN);
    g_sHostname[MAX_HOSTNAME_LEN - 1] = '\0';
    int t_nGetHostnameRtn = -1;
    // sets g_sHostname to the UNQUALIFIED hostname of the system; need more steps to get FQDN
    if ((t_nGetHostnameRtn = gethostname(g_sHostname, MAX_HOSTNAME_LEN)) != 0) {
        fprintf(stderr, "Failed to get the hostname of the machine the logger is running on.\n");
        fprintf(stderr, "Error number: %d\n", errno);
        if (g_sHostname != NULL)
            free(g_sHostname);
        return 1;
    }
    // we can now use t_nSocket with write() (and send()) to send messages

    log_handler t_structHandler = {
        &_graylog_handler_write,
        &_graylog_handler_close,
        &_graylog_handler_open,
        &_graylog_handler_isOpen,
        false,
        false
    };

    memcpy(p_pHandler, &t_structHandler, sizeof(log_handler));

    return 0;
}

