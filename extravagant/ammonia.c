//
//  main.c
//  ammonia
//
//  Created by bedtime on 11/19/23.
//

#define __AmmoniaImpl

#include <stdio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <libproc.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <dispatch/dispatch.h>
#include <mach-o/dyld_images.h>
#include <EndpointSecurity/EndpointSecurity.h>
#include "ammonia.h"

kern_return_t (*_thread_convert_thread_state)(thread_act_t thread, int direction, thread_state_flavor_t flavor, thread_state_t in_state, mach_msg_type_number_t in_stateCnt, thread_state_t out_state, mach_msg_type_number_t *out_stateCnt);

#define wait_time 2048

#define PTR_SIZE sizeof(void*)

#define kr(value) if (value != KERN_SUCCESS)\
{\
    printf("%s\n-    line %d\n", mach_error_string(value), __LINE__);\
} else {\
    printf("[*] line %d success..\n", __LINE__);\
}

// --
// Helpers
// --

void AmmoniaPrepareForUsage(void)
{
    void *rosettaLibrary = dlopen("/usr/lib/libRosetta.dylib", RTLD_NOW);
    IsCurrentProcessTranslated = (RosettaIsCurrentProcessTranslated)dlsym(rosettaLibrary, "rosetta_is_current_process_translated");
    IsProcessTranslated = (RosettaIsProcessTranslated)dlsym(rosettaLibrary, "rosetta_is_process_translated");
    IsTranslationAvailable = (RosettaIsTranslationAvailable)dlsym(rosettaLibrary, "rosetta_is_translation_available");
    ThreadCreateRunning = (RosettaThreadCreateRunning)dlsym(rosettaLibrary, "rosetta_thread_create_running");
    ThreadGetRip = (RosettaThreadGetRip)dlsym(rosettaLibrary, "rosetta_thread_get_rip");

    if (!IsCurrentProcessTranslated || !IsProcessTranslated || !IsTranslationAvailable || !ThreadCreateRunning || !ThreadGetRip)
    {
        fprintf(stderr, "Error: Unable to get function pointers from libRosetta. Maybe this isnt an arm64 host.\n");
        dlclose(rosettaLibrary);
    }
}

// --
// Fruit
// --

#ifdef __x86_64__
// Shellcode for the mach thread.
static const unsigned char mach_thread_code[] =
{
    0x55,                                           //push      rbp
    0x48, 0x89, 0xe5,                               //mov       rbp, rsp
    0x48, 0x89, 0xef,                               //mov       rdi, rbp
    0xff, 0xd0,                                     //call      rax
    0x48, 0xc7, 0xc0, 0x09, 0x03, 0x00, 0x00,       //mov       rax, 777
    0xe9, 0xfb, 0xff, 0xff, 0xff                    //jmp       -5
};

// Shellcode for the posix thread.
static const unsigned char posix_thread_code[] =
{
    0x55,                                           //push      rbp
    0x48, 0x89, 0xe5,                               //mov       rbp, rsp
    0x48, 0x8b, 0x07,                               //mov       rax, [rdi]
    0x48, 0x8b, 0x7f, 0xf8,                         //mov       rdi, [rdi - 8]
    0xbe, 0x01, 0x00, 0x00, 0x00,                   //mov       esi, 1
    0xff, 0xd0,                                     //call      rax
    0xc9,                                           //leave
    0xc3                                            //ret
};

#define MACH_CODE_SIZE sizeof(mach_thread_code)
#define POSIX_CODE_SIZE sizeof(posix_thread_code)
#define STACK_SIZE 1024

#else

static char shell_code[] =
"\xFF\xC3\x00\xD1"                 // sub        sp, sp, #0x30
"\xFD\x7B\x02\xA9"                 // stp        x29, x30, [sp, #0x20]
"\xFD\x83\x00\x91"                 // add        x29, sp, #0x20
"\xA0\xC3\x1F\xB8"                 // stur       w0, [x29, #-0x4]
"\xE1\x0B\x00\xF9"                 // str        x1, [sp, #0x10]
"\xE0\x23\x00\x91"                 // add        x0, sp, #0x8
"\x08\x00\x80\xD2"                 // mov        x8, #0
"\xE8\x07\x00\xF9"                 // str        x8, [sp, #0x8]
"\xE1\x03\x08\xAA"                 // mov        x1, x8
"\xE2\x01\x00\x10"                 // adr        x2, #0x3C
"\xE2\x23\xC1\xDA"                 // paciza     x2
"\xE3\x03\x08\xAA"                 // mov        x3, x8
"\x49\x01\x00\x10"                 // adr        x9, #0x28 ; pthread_create_from_mach_thread
"\x29\x01\x40\xF9"                 // ldr        x9, [x9]
"\x20\x01\x3F\xD6"                 // blr        x9
"\xA0\x4C\x8C\xD2"                 // movz       x0, #0x6265
"\x20\x2C\xAF\xF2"                 // movk       x0, #0x7961, lsl #16
"\x09\x00\x00\x10"                 // adr        x9, #0
"\x20\x01\x1F\xD6"                 // br         x9
"\xFD\x7B\x42\xA9"                 // ldp        x29, x30, [sp, #0x20]
"\xFF\xC3\x00\x91"                 // add        sp, sp, #0x30
"\xC0\x03\x5F\xD6"                 // ret
"\x00\x00\x00\x00\x00\x00\x00\x00" //
"\x7F\x23\x03\xD5"                 // pacibsp
"\xFF\xC3\x00\xD1"                 // sub        sp, sp, #0x30
"\xFD\x7B\x02\xA9"                 // stp        x29, x30, [sp, #0x20]
"\xFD\x83\x00\x91"                 // add        x29, sp, #0x20
"\xA0\xC3\x1F\xB8"                 // stur       w0, [x29, #-0x4]
"\xE1\x0B\x00\xF9"                 // str        x1, [sp, #0x10]
"\x21\x00\x80\xD2"                 // mov        x1, #1
"\x60\x01\x00\x10"                 // adr        x0, #0x2c ; payload_path
"\x09\x01\x00\x10"                 // adr        x9, #0x20 ; dlopen
"\x29\x01\x40\xF9"                 // ldr        x9, [x9]
"\x20\x01\x3F\xD6"                 // blr        x9
"\x09\x00\x80\x52"                 // mov        w9, #0
"\xE0\x03\x09\xAA"                 // mov        x0, x9
"\xFD\x7B\x42\xA9"                 // ldp        x29, x30, [sp, #0x20]
"\xFF\xC3\x00\x91"                 // add        sp, sp, #0x30
"\xFF\x0F\x5F\xD6"                 // retab
"\x00\x00\x00\x00\x00\x00\x00\x00" //
"\x00\x00\x00\x00\x00\x00\x00\x00" // empty space for payload_path
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00";

#define MACH_CODE_SIZE sizeof(mach_thread_code)
#define STACK_SIZE 0x100000

#endif

pid_t audit_token_to_pid(audit_token_t atoken);
int pthread_create_from_mach_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

void AmmoniaInjectX86(int process_id, const char * dylib_path)
{

#ifdef __x86_64__
    //Function addresses.
    const static void* pthread_create_from_mach_thread_address = (const void*)pthread_create_from_mach_thread;
    const static void* dlopen_address = (const void*)dlopen;
    vm_size_t path_length = strlen(dylib_path);
    
    //Obtain the task port.
    task_t task;
    kr(task_for_pid(mach_task_self_, process_id, &task));
    
    //Allocate the two instruction pointers.
    mach_vm_address_t mach_code_mem = 0;
    kr(mach_vm_allocate(task, &mach_code_mem, MACH_CODE_SIZE, VM_FLAGS_ANYWHERE));
    
    mach_vm_address_t posix_code_mem = 0;
    kr(mach_vm_allocate(task, &posix_code_mem, MACH_CODE_SIZE, VM_FLAGS_ANYWHERE));
    
    //Allocate the path variable and the stack.
    mach_vm_address_t stack_mem = 0;
    kr(mach_vm_allocate(task, &stack_mem, STACK_SIZE, VM_FLAGS_ANYWHERE));
    
    mach_vm_address_t path_mem = 0;
    kr(mach_vm_allocate(task, &path_mem, path_length, VM_FLAGS_ANYWHERE));
    
    //Allocate the pthread parameter array.
    mach_vm_address_t posix_param_mem = 0;
    kr(mach_vm_allocate(task, &posix_param_mem, (PTR_SIZE * 2), VM_FLAGS_ANYWHERE));

    //Write the path into memory.
    kr(mach_vm_write(task, path_mem, (vm_offset_t)dylib_path, (int)path_length));
    
    //Write the parameter array contents into memory. This array will be the pthread's parameter.
    
    //The address of dlopen() is the first parameter.
    kr(mach_vm_write(task, posix_param_mem, (vm_offset_t)&dlopen_address, PTR_SIZE));
    
    //The pointer to the dylib path is the second parameter.
    kr(mach_vm_write(task, (posix_param_mem - PTR_SIZE), (vm_offset_t)&path_mem, PTR_SIZE));
    
    //Write to both instructions, and mark them as readable, writable, and executable.
    
    //Do it for the mach thread instruction.
    kr(mach_vm_write(task, mach_code_mem, (vm_offset_t)&mach_thread_code, MACH_CODE_SIZE));
    kr(mach_vm_protect(task, mach_code_mem, MACH_CODE_SIZE, FALSE, VM_PROT_ALL));
    
    //Do it for the pthread instruction.
    kr(mach_vm_write(task, posix_code_mem, (vm_offset_t)&posix_thread_code, POSIX_CODE_SIZE));
    kr(mach_vm_protect(task, posix_code_mem, POSIX_CODE_SIZE, FALSE, VM_PROT_ALL));
    
    //The state and state count for launching the thread and reading its registers.
    mach_msg_type_number_t state_count = x86_THREAD_STATE64_COUNT;
    mach_msg_type_number_t state = x86_THREAD_STATE64;
    
    //Set all the registers to 0 so we can avoid setting extra registers to 0.
    x86_thread_state64_t regs;
    bzero(&regs, sizeof(regs));
    
    // Set the mach thread instruction pointer.
    regs.__rip = (__uint64_t)mach_code_mem;
    
    // Since the stack "grows" downwards, this is a usable stack pointer.
    regs.__rsp = (__uint64_t)(stack_mem + STACK_SIZE);
    
    // Set the function address, the 3rd parameter, and the 4th parameter.
    regs.__rax = (__uint64_t)pthread_create_from_mach_thread_address;
    regs.__rdx = (__uint64_t)posix_code_mem;
    regs.__rcx = (__uint64_t)posix_param_mem;

    // Initialize the thread.
    thread_act_t thread;
    
    if (IsCurrentProcessTranslated())
    {
        // run the rosetta injection
        kr(ThreadCreateRunning(task, state, (thread_state_t)(&regs), state_count, &thread));
    } else
    {
        // run the native real intel injection
        kr(thread_create_running(task, state, (thread_state_t)(&regs), state_count, &thread));
    }
    
    usleep(wait_time);
    
    // Terminate the thread.
    kr(thread_suspend(thread));
    kr(thread_terminate(thread));
    
    // Clean up.
    kr(mach_vm_deallocate(task, stack_mem, STACK_SIZE));
    kr(mach_vm_deallocate(task, mach_code_mem, MACH_CODE_SIZE));
#else
    printf("Unsupported.\n");
#endif
    return;
}

void AmmoniaInjectArm(int process_id, const char * dylib_path)
{
#ifdef __x86_64__
    printf("Unsupported.\n");
#else
    mach_port_t task = 0;
    thread_act_t thread = 0;
    mach_vm_address_t code = 0;
    mach_vm_address_t stack = 0;
    vm_size_t stack_size = 16 * 1024;
    uint64_t stack_contents = 0x00000000CAFEBABE;

    kr(task_for_pid(mach_task_self(), process_id, &task));
    kr(mach_vm_allocate(task, &stack, stack_size, VM_FLAGS_ANYWHERE));
    kr(mach_vm_write(task, stack, (vm_address_t) &stack_contents, sizeof(uint64_t)));
    kr(vm_protect(task, stack, stack_size, 1, VM_PROT_READ | VM_PROT_WRITE));
    kr(mach_vm_allocate(task, &code, sizeof(shell_code), VM_FLAGS_ANYWHERE));

    uint64_t pcfmt_address = (uint64_t) ptrauth_strip(dlsym(RTLD_DEFAULT, "pthread_create_from_mach_thread"), ptrauth_key_function_pointer);
    uint64_t dlopen_address = (uint64_t) ptrauth_strip(dlsym(RTLD_DEFAULT, "dlopen"), ptrauth_key_function_pointer);

    memcpy(shell_code + 88, &pcfmt_address, sizeof(uint64_t));
    memcpy(shell_code + 160, &dlopen_address, sizeof(uint64_t));
    memcpy(shell_code + 168, dylib_path, strlen(dylib_path));

    kr(mach_vm_write(task, code, (vm_address_t) shell_code, sizeof(shell_code)))
    
    kr(vm_protect(task, code, sizeof(shell_code), 0, VM_PROT_EXECUTE | VM_PROT_READ));
    
    void *handle = dlopen("/usr/lib/system/libsystem_kernel.dylib", RTLD_GLOBAL | RTLD_LAZY);
    if (handle) 
    {
        _thread_convert_thread_state = dlsym(handle, "thread_convert_thread_state");
        dlclose(handle);
    }
    
    if (!_thread_convert_thread_state)
    {
        fprintf(stderr, "could not load symbol: thread_convert_thread_state\n");
    }

    struct arm_unified_thread_state thread_state = {};
    struct arm_unified_thread_state machine_thread_state = {};

    thread_state_flavor_t thread_flavor = ARM_UNIFIED_THREAD_STATE;
    mach_msg_type_number_t thread_flavor_count = ARM_UNIFIED_THREAD_STATE_COUNT;
    mach_msg_type_number_t machine_thread_flavor_count = ARM_UNIFIED_THREAD_STATE_COUNT;

    thread_state.ash.flavor = ARM_THREAD_STATE64;
    thread_state.ash.count = ARM_THREAD_STATE64_COUNT;

    __darwin_arm_thread_state64_set_pc_fptr(thread_state.ts_64, ptrauth_sign_unauthenticated((void *) code, ptrauth_key_asia, 0));
    __darwin_arm_thread_state64_set_sp(thread_state.ts_64, stack + (stack_size / 2));

    kr(thread_create(task, &thread));

    _thread_convert_thread_state(thread, 2, thread_flavor, (thread_state_t) &thread_state, thread_flavor_count, (thread_state_t) &machine_thread_state, &machine_thread_flavor_count);

    thread_set_state(thread, thread_flavor, (thread_state_t)&machine_thread_state, machine_thread_flavor_count);


    thread_resume(thread);

    
    usleep(wait_time);
    
    // Terminate the thread.
    kr(thread_suspend(thread));
    kr(thread_terminate(thread));
   
    // The code below was commented out for making the target process
    // cpu go crazy.
    //
    //
    //    usleep(10000);
    //
    //    for (int i = 0; i < 10; ++i)
    //    {
    //        kr(thread_get_state(thread, thread_flavor, (thread_state_t)&thread_state, &thread_flavor_count));
    //        if (thread_state.ts_64.__x[0] == 0x79616265)
    //        {
    //            break;
    //        }
    //        usleep(20000);
    //    }
#endif
    
    return;
}

