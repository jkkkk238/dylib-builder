#include <stdio.h>
#include <unistd.h>
#include <mach-o/dyld.h>
#include <string.h>

// 移除Substrate依赖，使用函数指针钩子
typedef void (*InputUpdate_Func)(void* instance);
static InputUpdate_Func orig_InputUpdate = NULL;

// 函数指针类型定义
typedef void (*MSHookFunction_type)(void *symbol, void *replace, void **result);
static MSHookFunction_type MSHookFunction_ptr = NULL;

#define KEY_TIMEOUT 1500

void fixed_InputUpdate(void* instance) {
    if (orig_InputUpdate) orig_InputUpdate(instance);
    
    void* inputQueue = *(void**)((uintptr_t)instance + 0x38);
    if (!inputQueue) return;
    
    for (int i = 0; i < 32; i++) {
        uintptr_t keyEntry = (uintptr_t)inputQueue + 0x40 + i * 0x18;
        int* keyState = (int*)(keyEntry + 0x10);
        
        if (*keyState > KEY_TIMEOUT) {
            *keyState = 0;
            printf("[InputFix] Reset stuck key at %lu\n", keyEntry);
        }
    }
}

__attribute__((constructor)) static void init() {
    // 动态加载Substrate (如果存在)
    void *handle = dlopen("/Library/Frameworks/CydiaSubstrate.framework/CydiaSubstrate", RTLD_LAZY);
    if (handle) {
        MSHookFunction_ptr = (MSHookFunction_type)dlsym(handle, "MSHookFunction");
    }
    
    uintptr_t base = _dyld_get_image_vmaddr_slide(0);
    unsigned char pattern[] = {0xFD, 0x7B, 0xBF, 0xA9, 0xFD, 0x03, 0x00, 0x91};
    
    for (uintptr_t p = base; p < base + 0x1000000; p += 4) {
        if (memcmp((void*)p, pattern, 8) == 0) {
            printf("[InputFix] Found target at 0x%lx\n", p);
            
            if (MSHookFunction_ptr) {
                // 使用Substrate钩子
                MSHookFunction_ptr((void*)p, (void*)fixed_InputUpdate, (void**)&orig_InputUpdate);
                printf("[InputFix] Hooked with Substrate\n");
            } else {
                // 回退方案：直接覆盖函数指针
                orig_InputUpdate = *(InputUpdate_Func*)p;
                *(void**)p = fixed_InputUpdate;
                printf("[InputFix] Hooked with function pointer override\n");
            }
            break;
        }
    }
}
