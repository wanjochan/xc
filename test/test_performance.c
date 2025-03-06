#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>  // For sleep

// 故意的内存泄漏
void memory_leak_test() {
    printf("Running memory leak test...\n");
    
    // 分配内存但不释放
    char *leak1 = malloc(1024);
    sprintf(leak1, "This is a deliberate memory leak of 1KB");
    
    // 分配更大的内存但不释放
    char *leak2 = malloc(1024 * 1024);
    memset(leak2, 'X', 1024 * 1024);
    
    printf("Created two memory leaks: 1KB and 1MB\n");
}

// CPU密集型操作
void cpu_intensive_test() {
    printf("Running CPU intensive test...\n");
    
    // 计算大量斐波那契数
    long long fib[100];
    fib[0] = 0;
    fib[1] = 1;
    
    for (int i = 2; i < 100; i++) {
        fib[i] = fib[i-1] + fib[i-2];
    }
    
    // 进行一些无用但CPU密集的计算
    double result = 0.0;
    for (int i = 0; i < 10000000; i++) {
        result += i * 1.0 / (i + 1);
    }
    
    printf("CPU intensive calculation result: %f\n", result);
}

// 正常的内存使用
void normal_memory_test() {
    printf("Running normal memory test...\n");
    
    // 分配内存并正确释放
    char *buffer = malloc(1024 * 1024);
    if (buffer) {
        memset(buffer, 'Y', 1024 * 1024);
        printf("Allocated and initialized 1MB of memory\n");
        free(buffer);
        printf("Freed 1MB of memory\n");
    }
}

int main(int argc, char *argv[]) {
    printf("XC Performance Test Program\n");
    printf("==========================\n\n");
    
    // 记录开始时间
    clock_t start = clock();
    
    // 运行测试
    normal_memory_test();
    cpu_intensive_test();
    memory_leak_test();
    
    // 记录结束时间并计算运行时间
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    printf("\nAll tests completed in %.2f seconds\n", cpu_time_used);
    
    // 等待一段时间，以便leaks命令可以检测到内存泄漏
    printf("Waiting for 3 seconds to allow memory leak detection...\n");
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        sleep(1);
    }
    printf("\nDone!\n");
    
    return 0;
} 