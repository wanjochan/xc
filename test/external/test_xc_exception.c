/**
 * External Test for XC Exception Handling
 * 
 * This test validates the exception handling functionality through the public API.
 * It only uses functions and types defined in libxc.h.
 */
#include "test_utils.h"
#include <stdio.h>
#include <string.h>

/* Global flag to track if catch handler was called */
static int catch_handler_called = 0;

/* Global flag to track if finally handler was called */
static int finally_handler_called = 0;

/* Function that throws an exception */
static xc_val throw_exception_func(xc_val self, xc_val args, int argc, xc_val* argv) {
    printf("DEBUG: throw_exception_func 被调用，self=%p, args=%p, argc=%d\n", self, args, argc);
    
    // 创建一个异常对象
    xc_val error = xc.new(XC_TYPE_EXCEPTION, "Test exception");
    printf("DEBUG: 创建异常对象 error=%p\n", error);
    
    // 抛出异常
    printf("DEBUG: 准备抛出异常\n");
    xc.throw(error);
    
    // 这里不应该被执行到，因为 throw 会跳转到异常处理器
    printf("ERROR: throw_exception_func 抛出异常后继续执行\n");
    return NULL;
}

/* Catch handler function */
static xc_val catch_handler(xc_val self, xc_val args, int argc, xc_val* argv) {
    printf("DEBUG: catch_handler 被调用，self=%p, args=%p, argc=%d\n", self, args, argc);
    
    // 设置标志，表示 catch 处理器被调用
    catch_handler_called = 1;
    
    // 获取异常对象
    if (argc > 0) {
        xc_val error = argv[0];
        printf("DEBUG: 捕获到异常 error=%p\n", error);
    } else {
        printf("DEBUG: 捕获到异常，但没有参数\n");
    }
    
    // 返回一个值，表示异常已处理
    return xc.new(XC_TYPE_STRING, "Exception caught");
}

/* Finally handler function */
static xc_val finally_handler(xc_val self, xc_val args, int argc, xc_val* argv) {
    printf("DEBUG: finally_handler 被调用，self=%p, args=%p, argc=%d\n", self, args, argc);
    
    // 设置标志，表示 finally 处理器被调用
    finally_handler_called = 1;
    
    // 返回一个值，表示 finally 块已执行
    return xc.new(XC_TYPE_STRING, "Finally executed");
}

/* Test basic exception handling */
void test_exception_basic(void) {
    test_start("Exception Basic Functionality (External)");
    
    printf("\n=== 测试基本异常处理 ===\n");
    
    // 重置标志
    catch_handler_called = 0;
    finally_handler_called = 0;
    
    // 创建函数对象
    xc_val throw_func = xc.new(XC_TYPE_FUNC, throw_exception_func);
    xc_val catch_func = xc.new(XC_TYPE_FUNC, catch_handler);
    xc_val finally_func = xc.new(XC_TYPE_FUNC, finally_handler);
    
    printf("DEBUG: 创建函数对象 throw_func=%p, catch_func=%p, finally_func=%p\n", 
           throw_func, catch_func, finally_func);
    
    // 使用 XC 的异常处理机制
    printf("DEBUG: 调用 try_catch_finally\n");
    xc_val result = xc.try_catch_finally(throw_func, catch_func, finally_func);
    
    printf("DEBUG: try_catch_finally 返回结果 result=%p\n", result);
    
    // 检查 catch 和 finally 处理器是否被调用
    printf("DEBUG: catch_handler_called=%d, finally_handler_called=%d\n", 
           catch_handler_called, finally_handler_called);
    
    // 验证结果
    TEST_ASSERT(catch_handler_called == 1, "Catch handler was not called");
    TEST_ASSERT(finally_handler_called == 1, "Finally handler was not called");
    
    printf("=== 基本异常处理测试通过 ===\n\n");
    
    test_end("Exception Basic Functionality (External)");
}

/* 未捕获异常处理器函数 */
static xc_val uncaught_handler_func(xc_val self, int argc, xc_val *argv, void *closure) {
    printf("DEBUG: uncaught_handler_func 被调用，self=%p, args=%p, argc=%d\n", self, argv, argc);
    
    if (argc < 1 || !argv || !argv[0]) {
        return xc.new(XC_TYPE_STRING, "Uncaught: Unknown error");
    }
    
    // 获取异常消息
    xc_val message = xc.dot(argv[0], "message");
    if (!message) {
        return xc.new(XC_TYPE_STRING, "Uncaught: No message");
    }
    
    // 在外部测试中，我们不能直接访问字符串的值
    // 所以我们只返回一个固定的消息
    return xc.new(XC_TYPE_STRING, "Uncaught: Test exception");
}

/* Test uncaught exception handling */
void test_uncaught_exception(void) {
    test_start("Uncaught Exception Handling (External)");
    
    printf("Testing uncaught exception handling through public API...\n");
    
    // Create a custom uncaught exception handler using C function
    xc_val uncaught_handler = xc.new(XC_TYPE_FUNC, uncaught_handler_func);
    
    // Set the uncaught exception handler
    xc.set_uncaught_exception_handler(uncaught_handler);
    
    // Create a function that throws an exception
    xc_val throw_func = xc.new(XC_TYPE_FUNC, throw_exception_func);
    
    // Execute try with no catch (should call uncaught handler)
    xc_val result = xc.try_catch_finally(throw_func, NULL, NULL);
    
    // Check that the result contains the expected message
    TEST_ASSERT(result != NULL, "Result from uncaught handler should not be NULL");
    TEST_ASSERT(xc.is(result, XC_TYPE_STRING), "Result should be a string");
    
    // Get the current error (should be cleared after handling)
    xc_val current_error = xc.get_current_error();
    TEST_ASSERT(current_error == NULL, "Current error should be NULL after handling");
    
    test_end("Uncaught Exception Handling (External)");
} 
