#include <unistd.h>      // 替换 mach_time.h
#include <pthread.h>     // 线程支持

#define STUCK_THRESHOLD 1500

void reset_keys() {
    // 注意：0x1A2B3C4D 需替换为实际地址
    volatile int* input_reg = (int*)0x000040F8;
    for(int i = 0; i < 20; i++) {
        if(input_reg[i] > STUCK_THRESHOLD) {
            input_reg[i] = 0;
        }
    }
}

void* worker_thread(void* arg) {
    while(1) {
        reset_keys();
        usleep(100000);  // 100ms
    }
    return NULL;
}

// 入口函数（库加载时自动执行）
__attribute__((constructor))
void entry() {
    pthread_t tid;
    pthread_create(&tid, NULL, worker_thread, NULL);
}
