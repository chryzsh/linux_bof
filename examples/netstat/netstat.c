#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../beacon.h" // Include the Beacon API

// Helper function to parse IP and port from a hex string
void parse_ip_port(const char *hex_addr, char *ip, int *port) {
    unsigned int a, b, c, d, p;
    sscanf(hex_addr, "%02X%02X%02X%02X:%04X", &d, &c, &b, &a, &p);
    sprintf(ip, "%u.%u.%u.%u", a, b, c, d);
    *port = p;
}

// Function to display connections from /proc/net file
void display_connections(const char *protocol, const char *file) {
    FILE *fp = fopen(file, "r");
    if (!fp) {
        BeaconPrintf(CALLBACK_ERROR, "Failed to open %s\n", file);
        return;
    }

    char line[512];
    fgets(line, sizeof(line), fp); // Skip the header line

    while (fgets(line, sizeof(line), fp)) {
        char local_addr[128], rem_addr[128], state[8];
        int local_port, rem_port;
        int dummy;

        // Parse a line of /proc/net/tcp or /proc/net/udp
        sscanf(line, "%d: %64s %64s %s", &dummy, local_addr, rem_addr, state);

        // Extract and format local and remote IP/port
        char local_ip[32], rem_ip[32];
        parse_ip_port(local_addr, local_ip, &local_port);
        parse_ip_port(rem_addr, rem_ip, &rem_port);

        // Display connection info
        BeaconPrintf(CALLBACK_OUTPUT, "%s %s:%d -> %s:%d (State: %s)\n",
                     protocol, local_ip, local_port, rem_ip, rem_port, state);
    }

    fclose(fp);
}

// BOF entry point
void go(char *args, int alen) {
    BeaconPrintf(CALLBACK_OUTPUT, "Active TCP Connections:\n");
    display_connections("TCP", "/proc/net/tcp");

    BeaconPrintf(CALLBACK_OUTPUT, "\nActive UDP Connections:\n");
    display_connections("UDP", "/proc/net/udp");
}

