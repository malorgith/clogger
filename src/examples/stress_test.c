
#include "clogger.h"

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#ifdef __cplusplus
using namespace ::malorgith::clogger;
#endif  // __cplusplus

typedef struct {
    int thread_num;
    int messages_to_log;
} stressTestStruct;

static void *stress_test_logger(void *thread_data) {

    stressTestStruct *my_params = (stressTestStruct*) thread_data;
    int local_thread_num = (*my_params).thread_num;
    char tmp_id[50];
    logid_t id = 0;
    int snprintf_rtn = snprintf(tmp_id, 50, "Thread %d", local_thread_num);
    if ((snprintf_rtn >= 50) || (snprintf_rtn < 0)) {
        fprintf(stderr, "Failed to create string for thread ID.\n");
    }
    else {
        id = logger_create_id(tmp_id);
        if (id == kCloggerMaxNumIds) {
            fprintf(stderr, "Thread %d failed to get a logid_t\n", local_thread_num);
            return NULL;
        }
    }
    int max_messages = (*my_params).messages_to_log;

    for (int count = 0; count < max_messages; count++) {
        char msg[80];
        sprintf(msg, "Message Number %d", count);
        // TODO adjust sleep time or make variable?
        struct timespec sleep_time = { 0, (long)10000000 };
        nanosleep(&sleep_time, NULL);

        if (logger_log_msg_id(kCloggerInfo, id, msg)) {
            printf("Thread %d: Got my first failure at message number %d\n", local_thread_num, count);
            fflush(stdout);
            sleep(1);
            return NULL;
        }
    }

    sleep(1);
    return NULL;
}

static bool logger_run_stress_test(int num_threads, int num_messages) {

    if (!logger_is_running()) {
        fprintf(stderr, "Must initialize the logger and add handlers before running the stress test.\n");
        return false;
    }

    printf("\nLogger Stress Test Starting\n");
    printf("===========================\n\n");

    pthread_t test_threads[num_threads];
    stressTestStruct test_structs[num_threads];
    for (int count = 0; count < num_threads; count++) {

        stressTestStruct *my_struct = &test_structs[count];
        my_struct->thread_num = count;
        my_struct->messages_to_log = num_messages;

        pthread_create(&test_threads[count], NULL, stress_test_logger, my_struct);
    }

    for (int count = 0; count < num_threads; count++) {
        pthread_join(test_threads[count], NULL);
    }

    printf("\nLogger Stress Test Completed\n");
    printf("============================\n\n");

    return true;
}

int main(__attribute__((unused))int argc, __attribute__((unused))char** argv) {

    if(logger_init(kCloggerDebug)) {
        fprintf(stderr, "Failed to initialize logger.\n");
        return 1;
    }
    loghandler_t console_handler = logger_create_console_handler(stdout);
    if (console_handler == kCloggerHandlerErr) {
        fprintf(stderr, "Failed to add console handler to logger.\n");
        logger_free();
        return 1;
    }
    logger_run_stress_test(5, 25);

    if (logger_free()) {
        fprintf(stderr, "Failed to stop logger.\n");
        return 1;
    }

    return 0;
}
