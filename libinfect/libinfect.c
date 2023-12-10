//
//  libinfect.c
//  libinfect
//
//  Created by bedtime on 11/19/23.
//

#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <libproc.h>
#include <spawn.h>

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
    return k;
}

void __attribute__((constructor)) Infect(void)
{
    gum_init_embedded();
    GumInterceptor *interceptor = gum_interceptor_obtain();
    gum_interceptor_begin_transaction (interceptor);
    gum_interceptor_replace (interceptor, (gpointer)gum_module_find_export_by_name(NULL, "posix_spawn"), (gpointer)SpawnNew, NULL, (gpointer *)&SpawnOld);
    gum_interceptor_end_transaction (interceptor);
}
