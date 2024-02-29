
#include <gtest/gtest.h>

#include "cc/Clogger.hpp"
#include "logger_id.h"

namespace malorgith {
namespace clogger {
namespace {

Clogger testFunc() {
    return Clogger::createLogger("testFunc()");
}

void testFunc2(Clogger& src) {
    src = Clogger::createLogger("testFunc2()");
}

}  // namespace

TEST(CloggerSuite, move_test_1) {
    logger_init(kCloggerDebug);
    Clogger test(Clogger::createLogger("test"));
    EXPECT_FALSE(test.getLogID() == kCloggerDefaultId);
    // check to ensure the ID still exists after the destructor was called
    char id_dest[kCloggerIdMaxLen];
    EXPECT_FALSE(lgi_get_id(test.getLogID(), id_dest) == kCloggerHandlerErr);
    EXPECT_TRUE(test.kError.getLogID() != Clogger::kLogger.kError.getLogID());
    logger_free();
}

TEST(CloggerSuite, move_test_2) {
    logger_init(kCloggerDebug);
    Clogger test = Clogger::createLogger("test");
    EXPECT_FALSE(test.getLogID() == kCloggerDefaultId);
    // check to ensure the ID still exists after the destructor was called
    char id_dest[kCloggerIdMaxLen];
    EXPECT_FALSE(lgi_get_id(test.getLogID(), id_dest) == kCloggerHandlerErr);
    EXPECT_TRUE(test.kError.getLogID() != Clogger::kLogger.kError.getLogID());
    logger_free();
}

TEST(CloggerSuite, copy_test_1) {
    logger_init(kCloggerDebug);
    {
        Clogger test = Clogger::kLogger;  // copy default obj
        logid_t test2_logid = kCloggerDefaultId;
        EXPECT_TRUE(test.getLogID() == kCloggerDefaultId);
        {
            Clogger test2 = Clogger::createLogger("test2");  // move from function to obj
            test2_logid = test2.getLogID();
            EXPECT_FALSE(test2_logid == kCloggerDefaultId);
            test = test2;  // copy from obj to obj
            char id_dest[kCloggerIdMaxLen];
            EXPECT_FALSE(lgi_get_id(test.getLogID(), id_dest) == kCloggerHandlerErr);
            EXPECT_TRUE(test.getLogID() == test2_logid);
            EXPECT_TRUE(test.kError.getLogID() == test2.kError.getLogID());
        }
        // make sure the ID wasn't deleted
        char id_dest[kCloggerIdMaxLen];
        EXPECT_FALSE(lgi_get_id(test.getLogID(), id_dest) == kCloggerHandlerErr);
        EXPECT_TRUE(test.getLogID() == test2_logid);
    }
    logger_free();
}

TEST(CloggerSuite, copy_test_4) {
    logger_init(kCloggerDebug);
    Clogger test = Clogger::kLogger; // copy default obj
    EXPECT_TRUE(test.getLogID() == kCloggerDefaultId);
    test = testFunc();  // move from function
    // check to ensure the ID still exists after the destructor was called
    char id_dest[kCloggerIdMaxLen];
    EXPECT_FALSE(lgi_get_id(test.getLogID(), id_dest) == kCloggerHandlerErr);
    EXPECT_FALSE(test.getLogID() == kCloggerDefaultId);
    logger_free();
}

TEST(CloggerSuite, copy_test_4_2) {
    logger_init(kCloggerDebug);
//  Clogger test = Clogger::kLogger;
//  EXPECT_TRUE(test.getLogID() == kCloggerDefaultId);
//  test = testFunc();
    Clogger test = testFunc();
    // check to ensure the ID still exists after the destructor was called
    char id_dest[kCloggerIdMaxLen];
    EXPECT_FALSE(lgi_get_id(test.getLogID(), id_dest) == kCloggerHandlerErr);
    EXPECT_FALSE(test.getLogID() == kCloggerDefaultId);
    logger_free();
}

TEST(CloggerSuite, copy_test_5) {
    logger_init(kCloggerDebug);
    Clogger test = Clogger::kLogger;
    EXPECT_TRUE(test.getLogID() == kCloggerDefaultId);
    testFunc2(test);
    // check to ensure the ID still exists after the destructor was called
    char id_dest[kCloggerIdMaxLen];
    EXPECT_FALSE(lgi_get_id(test.getLogID(), id_dest) == kCloggerHandlerErr);
    EXPECT_FALSE(test.getLogID() == kCloggerDefaultId);
    logger_free();
}

}  // namespace clogger
}  // namespace malorgith
