
#include "clogger.h"

#include <pthread.h>
#include <unistd.h>

typedef struct {
    int m_nThreadNum;
    int m_nMessages;
} stressTestStruct;

static void *stress_test_logger(void *p_pData) {

    stressTestStruct *my_params = (stressTestStruct*) p_pData;
    int t_nThread = (*my_params).m_nThreadNum;
    char tmp_id[50];
    logger_id id = 0;
    int snprintf_rtn = snprintf(tmp_id, 50, "Thread %d", t_nThread);
    if ((snprintf_rtn >= 50) || (snprintf_rtn < 0)) {
        fprintf(stderr, "Failed to create string for thread ID.\n");
    }
    else {
        id = logger_create_id(tmp_id);
    }
    int t_nMaxMessages = (*my_params).m_nMessages;

    for (int t_nCount = 0; t_nCount < t_nMaxMessages; t_nCount++) {
        char msg[80];
        sprintf(msg, "Message Number %d", t_nCount);
        // TODO adjust sleep time or make variable?
        struct timespec t_sleeptime = { 0, (long)10000000 };
        nanosleep(&t_sleeptime, NULL);

        if (!logger_log_msg_id(LOGGER_INFO, id, msg)) {
            printf("Thread %d: Got my first failure at message number %d\n", t_nThread, t_nCount);
            fflush(stdout);
        }
    }

    sleep(1);
    return NULL;
}

static bool logger_run_stress_test(int p_nThreads, int p_nMessages) {

    if (!logger_is_running()) {
        fprintf(stderr, "Must initialize the logger and add handlers before running the stress test.\n");
        return false;
    }

    printf("\nLogger Stress Test Starting\n");
    printf("===========================\n\n");

    pthread_t t_logTestThreads[p_nThreads];
    stressTestStruct t_Structs[p_nThreads];
    for (int t_nCount = 0; t_nCount < p_nThreads; t_nCount++) {

        stressTestStruct *my_struct = &t_Structs[t_nCount];
        my_struct->m_nThreadNum = t_nCount;
        my_struct->m_nMessages = p_nMessages;

        pthread_create(&t_logTestThreads[t_nCount], NULL, stress_test_logger, my_struct);
    }

    for (int t_nCount = 0; t_nCount < p_nThreads; t_nCount++) {
        pthread_join(t_logTestThreads[t_nCount], NULL);
    }

    printf("\nLogger Stress Test Completed\n");
    printf("============================\n\n");

    return true;
}

int main(__attribute__((unused))int argc, __attribute__((unused))char** argv) {
    logger_init(LOGGER_DEBUG);
    logger_create_console_handler(stdout);
    logger_run_stress_test(5, 25);
    logger_free();
    return 0;
}
