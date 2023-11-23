//
//  rosetta.h
//  ammonia
//
//  Created by bedtime on 11/19/23.
//

#ifndef rosetta_h
#define rosetta_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <mach/host_info.h>
#include <Availability.h>

#define SupportFolderP "/usr/local/bin/ammonia/"

void AmmoniaPrepareForUsage(void);
void AmmoniaInjectArm(int process_id, const char * dylib_path);
void AmmoniaInjectX86(int process_id, const char * dylib_path);

typedef bool (*RosettaIsCurrentProcessTranslated)();
typedef bool (*RosettaIsProcessTranslated)(pid_t pid);
typedef bool (*RosettaIsTranslationAvailable)();
typedef kern_return_t (*RosettaThreadCreateRunning)(task_t parent_task, thread_state_flavor_t flavor, thread_state_t new_state, mach_msg_type_number_t new_stateCnt, thread_act_t *child_act);
typedef int (*RosettaThreadGetRip)(thread_act_t, uint64_t *);

#ifdef __AmmoniaImpl
RosettaIsCurrentProcessTranslated IsCurrentProcessTranslated;
RosettaIsProcessTranslated IsProcessTranslated;
RosettaIsTranslationAvailable IsTranslationAvailable;
RosettaThreadCreateRunning ThreadCreateRunning;
RosettaThreadGetRip ThreadGetRip;
#else
extern RosettaIsCurrentProcessTranslated IsCurrentProcessTranslated;
extern RosettaIsProcessTranslated IsProcessTranslated;
extern RosettaIsTranslationAvailable IsTranslationAvailable;
extern RosettaThreadCreateRunning ThreadCreateRunning;
extern RosettaThreadGetRip ThreadGetRip;
#endif
#endif /* rosetta_h */
