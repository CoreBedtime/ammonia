//
//  opener.c
//  opener
//
//  Created by whisper on 9/2/23.
//

#include "opener.h"
#include <string.h>
#include <stdlib.h>
#include <mach/mach.h>

const char *GetExePath(void)
{
    uint32_t bufsize = 0;
    _NSGetExecutablePath(NULL, &bufsize);
    char *executablePath = malloc(bufsize);
    _NSGetExecutablePath(&executablePath[0], &bufsize);
    return executablePath;
}

void Open(void) {
    DIR *dr;
    struct dirent *en;
    dr = opendir(SupportFolderP"tweaks/"); // Open the directory
    if (dr) {
        while ((en = readdir(dr)) != NULL) {
            if (en->d_type == DT_REG) { // Check if it's a regular file
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%stweaks/%s", SupportFolderP, en->d_name);

                // Load the blacklist file for the current dylib
                char blacklist_file[PATH_MAX];
                snprintf(blacklist_file, sizeof(blacklist_file), "%stweaks/%s.blacklist", SupportFolderP, en->d_name);

                // Check if the blacklist file exists and open it
                FILE *blacklist_fp = fopen(blacklist_file, "r");
                if (blacklist_fp) {
                    char process_name[256]; // Adjust the buffer size as needed
                    while (fgets(process_name, sizeof(process_name), blacklist_fp) != NULL) {
                        // Remove newline characters from the process_name
                        size_t len = strlen(process_name);
                        if (len > 0 && process_name[len - 1] == '\n') {
                            process_name[len - 1] = '\0';
                        }

                        // Check if the current process name is blacklisted
                        if (strstr(GetExePath(), process_name) != NULL) 
                        {
                            // Process name is blacklisted, skip loading the dylib
                            syslog(LOG_INFO, "Process name %s is blacklisted for %s.", process_name, en->d_name);
                            fclose(blacklist_fp);
                            goto cleanup;
                        }
                    }
                    fclose(blacklist_fp); // Close the blacklist file

                    // If not blacklisted, attempt to dynamically load the shared library
                    void *handle = dlopen(full_path, RTLD_NOW | RTLD_GLOBAL);
                    if (handle == NULL) 
                    {
                        syslog(LOG_ERR, "Error loading %s: %s", full_path, dlerror());
                    }
                }

            cleanup:
                continue; // Continue with the next file
            }
        }
        closedir(dr); // Close the directory
    } else {
        syslog(LOG_ERR, "Error opening directory.");
    }
    closelog();
}

void __attribute__((constructor)) ctor_main(void) { Open(); }
