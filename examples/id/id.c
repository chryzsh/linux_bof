#include <sys/types.h>
#include "../../beacon.h"

#define SYS_GETUID 102
#define SYS_GETEUID 107

uid_t syscall_getuid() {
    uid_t uid;
    asm("mov $102, %%eax\n"  // SYS_GETUID
        "syscall\n"
        "mov %%eax, %0"
        : "=r"(uid)
        : 
        : "rax");
    return uid;
}

uid_t syscall_geteuid() {
    uid_t euid;
    asm("mov $107, %%eax\n"  // SYS_GETEUID
        "syscall\n"
        "mov %%eax, %0"
        : "=r"(euid)
        : 
        : "rax");
    return euid;
}

void go(char* args, int alen) {
    uid_t uid = syscall_getuid();
    uid_t euid = syscall_geteuid();

    BeaconPrintf(CALLBACK_OUTPUT, "UID: %d EUID: %d\n", uid, euid);
}

