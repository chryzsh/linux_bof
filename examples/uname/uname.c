#include "../../beacon.h"

// Define NULL if not already defined
#ifndef NULL
#define NULL ((void*)0)
#endif

// Define syscall number for x86_64 Linux
#define SYS_UNAME 63

// Define the utsname structure
struct utsname {
    char sysname[65];      // Operating system name
    char nodename[65];     // Network node hostname
    char release[65];      // OS release
    char version[65];      // OS version
    char machine[65];      // Hardware identifier
    char domainname[65];   // NIS or YP domain name
};

// Implement syscall wrapper for uname
int sys_uname(struct utsname *buf) {
    int ret;
    asm volatile (
        "syscall"
        : "=a" (ret)
        : "0"(SYS_UNAME), "D"(buf)
        : "rcx", "r11", "memory"
    );
    return ret;
}

void go(char *args, int alen) {
    struct utsname uts;

    // Initialize the uts structure to zero
    for (int i = 0; i < sizeof(struct utsname); i++) {
        ((char *)&uts)[i] = 0;
    }

    // Call the uname syscall
    int res = sys_uname(&uts);
    if (res < 0) {
        BeaconPrintf(CALLBACK_ERROR, "uname syscall failed\n");
        return;
    }

    // Output the system information
    BeaconPrintf(CALLBACK_OUTPUT, "System Name    : %s\n", uts.sysname);
    BeaconPrintf(CALLBACK_OUTPUT, "Node Name      : %s\n", uts.nodename);
    BeaconPrintf(CALLBACK_OUTPUT, "Release        : %s\n", uts.release);
    BeaconPrintf(CALLBACK_OUTPUT, "Version        : %s\n", uts.version);
    BeaconPrintf(CALLBACK_OUTPUT, "Machine        : %s\n", uts.machine);
    BeaconPrintf(CALLBACK_OUTPUT, "Domain Name    : %s\n", uts.domainname);
}
