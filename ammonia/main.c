//
//  main.c
//  ammonia
//
//  Created by bedtime on 11/19/23.
//

#include "main.h"
#include "ammonia.h"
#include <xpc/xpc.h>

void DefiantInject(int pid)
{
#ifdef __arm64__
    AmmoniaInjectArm(pid, SupportFolderP"liblibinfect.dylib");
#else
    AmmoniaInjectX86(pid, SupportFolderP"liblibinfect.dylib");
#endif
}

// --
// Main
// --
int main(void)
{
    AmmoniaPrepareForUsage();
    DefiantInject(1);
    return 0;
}
