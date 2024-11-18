#include <sys/types.h>
#include "../../beacon.h"

// Define NULL if not already defined
#ifndef NULL
#define NULL ((void*)0)
#endif

// Define syscall numbers for x86_64 architecture
#define SYS_OPEN   2
#define SYS_READ   0
#define SYS_CLOSE  3

// Implement syscall wrappers
ssize_t sys_read(int fd, void *buf, size_t count) {
    ssize_t ret;
    asm volatile (
        "syscall"
        : "=a" (ret)
        : "0"(SYS_READ), "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
    );
    return ret;
}

int sys_open(const char *pathname, int flags, int mode) {
    int ret;
    asm volatile (
        "syscall"
        : "=a" (ret)
        : "0"(SYS_OPEN), "D"(pathname), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory"
    );
    return ret;
}

int sys_close(int fd) {
    int ret;
    asm volatile (
        "syscall"
        : "=a" (ret)
        : "0"(SYS_CLOSE), "D"(fd)
        : "rcx", "r11", "memory"
    );
    return ret;
}

// Implement own strlen
size_t my_strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

// Implement own strchr
char *my_strchr(const char *s, int c) {
    while (*s) {
        if (*s == c)
            return (char *)s;
        s++;
    }
    return NULL;
}

// Implement own strtoul (hexadecimal only)
unsigned long my_strtoul(const char *nptr, char **endptr, int base) {
    unsigned long result = 0;
    const char *ptr = nptr;
    char c;

    if (base != 16) {
        // For simplicity, we only handle base 16 (hexadecimal)
        if (endptr) *endptr = (char *)nptr;
        return 0;
    }

    while ((c = *ptr)) {
        unsigned int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            break; // Invalid character
        }
        result = (result << 4) + digit;
        ptr++;
    }

    if (endptr) *endptr = (char *)ptr;
    return result;
}

void go(char *args, int alen) {
    int fd = sys_open("/proc/net/tcp", 0 /* O_RDONLY */, 0);
    if (fd < 0) {
        BeaconPrintf(CALLBACK_ERROR, "Failed to open /proc/net/tcp\n");
        return;
    }

    char buffer[16384];
    ssize_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
    sys_close(fd);

    if (bytes_read <= 0) {
        BeaconPrintf(CALLBACK_ERROR, "Failed to read /proc/net/tcp\n");
        return;
    }

    buffer[bytes_read] = '\0';

    char *ptr = buffer;
    char *end = buffer + bytes_read;
    int line_num = 0;

    while (ptr < end) {
        char *line_start = ptr;
        // Find end of line
        while (ptr < end && *ptr != '\n') ptr++;
        *ptr = '\0'; // Null-terminate the line
        ptr++; // Move to start of next line

        if (line_num > 0) { // Skip header line
            char *tokens[15];
            int token_index = 0;
            char *token_ptr = line_start;

            // Tokenize the line by spaces
            while (*token_ptr && token_index < 15) {
                // Skip leading spaces
                while (*token_ptr == ' ' || *token_ptr == '\t') token_ptr++;
                if (*token_ptr == '\0') break;
                tokens[token_index++] = token_ptr;
                // Find next space
                while (*token_ptr && *token_ptr != ' ' && *token_ptr != '\t') token_ptr++;
                if (*token_ptr) {
                    *token_ptr = '\0'; // Null-terminate the token
                    token_ptr++;
                }
            }

            if (token_index >= 4) {
                char *local_address = tokens[1];
                char *rem_address = tokens[2];
                char *state = tokens[3];

                char *local_port_hex = my_strchr(local_address, ':');
                if (local_port_hex != NULL) {
                    *local_port_hex = '\0';
                    local_port_hex++;
                }

                char *rem_port_hex = my_strchr(rem_address, ':');
                if (rem_port_hex != NULL) {
                    *rem_port_hex = '\0';
                    rem_port_hex++;
                }

                unsigned int local_port = (unsigned int)my_strtoul(local_port_hex, NULL, 16);
                unsigned int rem_port = (unsigned int)my_strtoul(rem_port_hex, NULL, 16);

                unsigned int local_ip_int = (unsigned int)my_strtoul(local_address, NULL, 16);
                unsigned int rem_ip_int = (unsigned int)my_strtoul(rem_address, NULL, 16);

                unsigned char local_bytes[4];
                local_bytes[0] = (local_ip_int >> 0) & 0xFF;
                local_bytes[1] = (local_ip_int >> 8) & 0xFF;
                local_bytes[2] = (local_ip_int >> 16) & 0xFF;
                local_bytes[3] = (local_ip_int >> 24) & 0xFF;

                unsigned char rem_bytes[4];
                rem_bytes[0] = (rem_ip_int >> 0) & 0xFF;
                rem_bytes[1] = (rem_ip_int >> 8) & 0xFF;
                rem_bytes[2] = (rem_ip_int >> 16) & 0xFF;
                rem_bytes[3] = (rem_ip_int >> 24) & 0xFF;

                unsigned int state_num = (unsigned int)my_strtoul(state, NULL, 16);

                char *tcp_states[] = {
                    "UNKNOWN", "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT1",
                    "FIN_WAIT2", "TIME_WAIT", "CLOSE", "CLOSE_WAIT", "LAST_ACK",
                    "LISTEN", "CLOSING", "NEW_SYN_RECV"
                };
                char *state_str = (state_num < (sizeof(tcp_states)/sizeof(tcp_states[0]))) ? tcp_states[state_num] : "UNKNOWN";

                // Output the connection info
                BeaconPrintf(CALLBACK_OUTPUT, "%u.%u.%u.%u:%u\t%u.%u.%u.%u:%u\t%s\n",
                    local_bytes[0], local_bytes[1], local_bytes[2], local_bytes[3], local_port,
                    rem_bytes[0], rem_bytes[1], rem_bytes[2], rem_bytes[3], rem_port,
                    state_str);
            }
        }
        line_num++;
    }
}

