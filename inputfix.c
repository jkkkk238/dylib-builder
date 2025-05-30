#include <stdio.h>
#include <objc/runtime.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
#include <dispatch/dispatch.h>

#define KEY_TIMEOUT 1500

// 定义目标类和方法（需根据实际应用调整）
static const char *TARGET_CLASS = "UnityEngine.Input";
static const char *TARGET_METHOD = "updateInput";

// 原始方法实现
static IMP original_UpdateInput = NULL;

// 修复方法实现
void fixed_UpdateInput(id self, SEL _cmd) {
    // 调用原始实现
    if (original_UpdateInput) {
        ((void (*)(id, SEL))original_UpdateInput)(self, _cmd);
    }
    
    // 获取输入队列（需根据实际类结构调整）
    void *inputQueue = NULL;
    object_getInstanceVariable(self, "_inputQueue", (void **)&inputQueue);
    
    if (!inputQueue) return;
    
    // 遍历输入队列（ARM64 结构）
    for (int i = 0; i < 32; i++) {
        uintptr_t keyEntry = (uintptr_t)inputQueue + 0x40 + i * 0x18;
        int* keyState = (int*)(keyEntry + 0x10);
        
        if (*keyState > KEY_TIMEOUT) {
            *keyState = 0; // 重置状态
            
            // 使用系统日志（可在 Xcode 控制台查看）
            os_log(OS_LOG_DEFAULT, "[InputFix] Reset stuck key at %lu", (uintptr_t)keyEntry);
        }
    }
}

__attribute__((constructor)) static void init() {
    // 延迟执行，确保运行时已初始化
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        // 获取目标类
        Class targetClass = objc_getClass(TARGET_CLASS);
        if (!targetClass) {
            os_log(OS_LOG_DEFAULT, "[InputFix] Target class not found");
            return;
        }
        
        // 获取目标方法
        SEL targetSelector = sel_registerName(TARGET_METHOD);
        Method targetMethod = class_getInstanceMethod(targetClass, targetSelector);
        if (!targetMethod) {
            os_log(OS_LOG_DEFAULT, "[InputFix] Target method not found");
            return;
        }
        
        // 交换方法实现
        original_UpdateInput = method_getImplementation(targetMethod);
        method_setImplementation(targetMethod, (IMP)fixed_UpdateInput);
        
        os_log(OS_LOG_DEFAULT, "[InputFix] Successfully hooked input update");
    });
}
