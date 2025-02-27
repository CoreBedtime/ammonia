//
//  libinfect.c
//  libinfect
//
//  Created by bedtime on 11/19/23.
//

#include <bootstrap.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <libproc.h>
#include <spawn.h>
#include <sys/_pthread/_pthread_t.h>
#include <sys/_types/_null.h>
#include <sys/_types/_pid_t.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include "CoreSymbolication.h"
#include "ammonia.h"
#include "frida-gum.h"

void LogToFile(const char *format, ...)
{
    // Open the file in append mode
    FILE *file = fopen("/tmp/infect.log", "a");
    
    if (file == NULL) {
        // Failed to open the file
        perror("Error opening file");
        return;
    }

    // Initialize variable arguments
    va_list args;
    va_start(args, format);

    // Use vfprintf to write to the file
    vfprintf(file, format, args);

    // Clean up variable arguments
    va_end(args);

    // Close the file
    fclose(file);
}

// https://github.com/evelyneee/ellekit/blob/24033cc3cfa78cfd64908fc9f4b63f1880adeb93/ellekitc/ellekitc.c#L53
// thanks siguza
int (*SandboxCheckOld)(audit_token_t au, const char *operation, int sandbox_filter_type, ...) ;
int SandboxCheck(audit_token_t au, const char *operation, int sandbox_filter_type, ...) 
{    
    va_list a;
    va_start(a, sandbox_filter_type);
    const char *name = va_arg(a, const char *);
    const void *arg2 = va_arg(a, void *);
    const void *arg3 = va_arg(a, void *);
    const void *arg4 = va_arg(a, void *);
    const void *arg5 = va_arg(a, void *);
    const void *arg6 = va_arg(a, void *);
    const void *arg7 = va_arg(a, void *);
    const void *arg8 = va_arg(a, void *);
    const void *arg9 = va_arg(a, void *);
    const void *arg10 = va_arg(a, void *);
    if (name && operation) {
        if (strcmp(operation, "mach-lookup") == 0) {
            if (strstr(name, "ammonia")) {
                return 0; // yas ! allow mach ports
            }
        }
    }
    return SandboxCheckOld(au, operation, sandbox_filter_type, name, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);;
}

int Pid2Name(const char *proc_name) {
    int mib[4];
    size_t len;
    struct kinfo_proc *proc_list;
    size_t proc_count;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_ALL;
    mib[3] = 0;

    if (sysctl(mib, 4, NULL, &len, NULL, 0) < 0) {
        perror("sysctl 1");
        return -1;
    }

    proc_list = malloc(len);
    if (proc_list == NULL) {
        perror("malloc");
        return -1;
    }

    if (sysctl(mib, 4, proc_list, &len, NULL, 0) < 0) {
        perror("sysctl 2");
        free(proc_list);
        return -1;
    }

    proc_count = len / sizeof(struct kinfo_proc);
    for (size_t i = 0; i < proc_count; i++) {
        if (strcmp(proc_list[i].kp_proc.p_comm, proc_name) == 0) {
            int pid = proc_list[i].kp_proc.p_pid;
            free(proc_list);
            return pid;
        }
    }

    free(proc_list);
    return -1; // Process not found
}


bool WindowServerKilled = false;
void FixupWindowServerOnce() {
    static bool WindowServerKilled = false;
    if (WindowServerKilled) return;

    // Find WindowServer PID
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;
    if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0) {
        perror("sysctl error");
        return;
    }
    
    struct kinfo_proc *processes = malloc(size);
    if (sysctl(mib, 4, processes, &size, NULL, 0) < 0) {
        perror("sysctl error");
        free(processes);
        return;
    }
    
    int proc_count = (int)(size / sizeof(struct kinfo_proc));
    pid_t windowServerPID = -1;
    
    for (int i = 0; i < proc_count; i++) {
        if (strcmp(processes[i].kp_proc.p_comm, "WindowServer") == 0) {
            windowServerPID = processes[i].kp_proc.p_pid;
            break;
        }
    }
    free(processes);
    
    if (windowServerPID == -1) {
        LogToFile("WindowServer process not found.\n");
        return;
    }

    // Check for the symbol in WindowServer
    CSSymbolicatorRef symbolicator = CSSymbolicatorCreateWithPid(windowServerPID);

    __block bool symbolFound = false;
    CSSymbolicatorForeachSymbolWithMangledNameAtTime(symbolicator, "_gum_process_get_id", kCSNow, ^ int(CSSymbolRef symbol) {
        symbolFound = true;
        return 0;
    });

    if (!symbolFound) {
        if (kill(windowServerPID, SIGKILL) == 0) {
            WindowServerKilled = true;
        } else {
            perror("Failed to kill WindowServer");
        }
    } else {
        LogToFile("Symbol '_gum_process_get_id' found. WindowServer not killed.\n");
    }
    
    //CFRelease(symbolicator);
}

int (*SpawnOld)(pid_t * pid, const char * path, const posix_spawn_file_actions_t * ac, const posix_spawnattr_t * ab, char *const __argv[], char *const __envp[]);
int SpawnNew(pid_t * pid, const char * path, const posix_spawn_file_actions_t * ac, const posix_spawnattr_t * ab, char *const __argv[], char *const __envp[])
{
    char *fakeEnvVar;
    
    if (strcmp(path, "/usr/libexec/xpcproxy") == 0)
    {
        fakeEnvVar = "DYLD_INSERT_LIBRARIES="SupportFolderP"liblibinfect.dylib";
    } else if (strcmp(path, "/System/Library/Frameworks/CryptoTokenKit.framework/ctkahp.bundle/Contents/MacOS/ctkahp") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strcmp(path, "/System/Library/PrivateFrameworks/Heimdal.framework/Helpers/kcm") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strcmp(path, "/System/Library/Frameworks/CryptoTokenKit.framework/UserSelector") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strcmp(path, "/System/Library/Frameworks/CryptoTokenKit.framework/ctkd") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strcmp(path, "/System/Library/Frameworks/GSS.framework/Helpers/GSSCred") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strcmp(path, "/System/Library/CoreServices/iconservicesagent") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strcmp(path, "/System/Library/CoreServices/iconservicesd") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strcmp(path, "/usr/libexec/UserEventAgent") == 0)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strstr(path, "Wallpaper") != NULL)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strstr(path, "sandboxd") != NULL)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strstr(path, "Driver") != NULL)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else if (strstr(path, "fileproviderd") != NULL)
    {
        return SpawnOld(pid, path, ac, ab, __argv, __envp);
    } else
    {
        fakeEnvVar = "DYLD_INSERT_LIBRARIES="SupportFolderP"libopener.dylib";
    }
    
    int envCount = 0;
    while (__envp[envCount] != NULL) { envCount++; }

    char **newEnvp = (char **)malloc((envCount + 2) * sizeof(char *));
    if (newEnvp == NULL) { return -1; }

    for (int i = 0; i < envCount; i++) 
    {
        newEnvp[i] = strdup(__envp[i]);
        if (newEnvp[i] == NULL)
        {
            for (int j = 0; j < i; j++) 
            {
                free(newEnvp[j]);
            }
            free(newEnvp);
            return -1;
        }
    }
    
    newEnvp[envCount] = strdup(fakeEnvVar);
    if (newEnvp[envCount] == NULL)
    {
        for (int i = 0; i <= envCount; i++) { free(newEnvp[i]); }
        free(newEnvp);
        return -1;
    }
    newEnvp[envCount + 1] = NULL;
    
    LogToFile("---- %s\n", path);
    for (int i = 0; i < envCount + 1; i++) { LogToFile("-- %s\n", newEnvp[i]); }
    LogToFile("\n");
    
    int k = SpawnOld(pid, path, ac, ab, __argv, (char *const *)newEnvp);
    for (int i = 0; i <= envCount; i++) { free(newEnvp[i]); }
    free(newEnvp);
    
    FixupWindowServerOnce();

    return k;
}

int SpawnPNew(pid_t *restrict pid, const char *restrict path, const posix_spawn_file_actions_t * ac, const posix_spawnattr_t *restrict ab, char *const *restrict argv, char *const *restrict envp)
{
    return SpawnNew(pid, path, ac, ab, argv, envp);
}

int (*_CSOps)(pid_t pid, unsigned int  ops, void * useraddr, size_t usersize);
int HookedCSOps(pid_t pid, unsigned int ops, void *useraddr, size_t usersize) {
    int result = _CSOps(pid, ops, useraddr, usersize);
    if (result != 0) return result;
    if (ops == 0) { // CS_OPS_STATUS
       *((uint32_t *)useraddr) |= 0x4000001; // CS_PLATFORM_BINARY
    }
    return result;
}

int (*_CSOpsAuditToken)(pid_t pid, unsigned int  ops, void * useraddr, size_t usersize, audit_token_t * token);
int HookedCSOpsAuditToken(pid_t pid, unsigned int ops, void * useraddr, size_t usersize, audit_token_t * token) {
    int result = _CSOpsAuditToken(pid, ops, useraddr, usersize, token);
    if (result != 0) return result;
    if (ops == 0) { // CS_OPS_STATUS
       *((uint32_t *)useraddr) |= 0x4000001; // CS_PLATFORM_BINARY
    }
    return result;
}

void __attribute__((constructor)) Infect(void)
{
    gum_init_embedded();
    GumInterceptor *interceptor = gum_interceptor_obtain();
    gum_interceptor_begin_transaction (interceptor);
    gum_interceptor_replace (interceptor, (gpointer)gum_module_find_export_by_name(NULL, "posix_spawn"), (gpointer)SpawnNew, NULL, (gpointer *)&SpawnOld);
    gum_interceptor_replace (interceptor, (gpointer)gum_module_find_export_by_name(NULL, "posix_spawnp"), (gpointer)SpawnPNew, NULL, (gpointer *)(NULL));
    gum_interceptor_replace (interceptor, (gpointer)gum_module_find_export_by_name(NULL, "sandbox_check_by_audit_token"), (gpointer)SandboxCheck, NULL, (gpointer *)&SandboxCheckOld);
    gum_interceptor_replace (interceptor, (gpointer)gum_module_find_export_by_name(NULL, "csops"), (gpointer)HookedCSOps, NULL, (gpointer *)&_CSOps);
    gum_interceptor_replace (interceptor, (gpointer)gum_module_find_export_by_name(NULL, "csops_audittoken"), (gpointer)HookedCSOpsAuditToken, NULL, (gpointer *)&_CSOpsAuditToken);
    gum_interceptor_end_transaction (interceptor);
}
