#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <utmp.h>

#define BUF_SIZE 1024

#include "stats_functions.h"
#include <errno.h>
#include <limits.h>

void printMemoryChange(double prev, double current, double total,
                       struct MemoryInfo *memInfo) {
  const double epsilon = 0.0;
  char tempBuffer[1024]; // Temporary buffer for snprintf operations
  tempBuffer[0] =
      '\0'; // Ensure tempBuffer is initially empty and null-terminated

  if (prev < 0) {
    // If it's the first call (prev uninitialized), no change to display
    snprintf(tempBuffer, sizeof(tempBuffer), "|o 0.00 (%.2f)\n", current);
  } else {
    double change = current - prev;
    int bars =
        (fabs(change) / total) * 1000; // Calculate the number of bars/colons

    if (fabs(change) < epsilon) {
      // If the change is too small to consider, show as no change
      snprintf(tempBuffer, sizeof(tempBuffer), "|o 0.00 (%.2f)\n", current);
    } else {
      // Add the initial bar or colon
      char pattern = change > 0 ? '*' : '@';
      int len = snprintf(tempBuffer, sizeof(tempBuffer), "|");

      // Append bars or colons according to the magnitude of change
      for (int i = 0; i < bars && len < sizeof(tempBuffer) - 2; ++i) {
        tempBuffer[len++] = change > 0 ? '#' : ':';
      }
      tempBuffer[len] = '\0'; // Ensure the string is null-terminated

      // Append the change and current value information
      snprintf(tempBuffer + len, sizeof(tempBuffer) - len, " %c %.2f (%.2f)\n",
               pattern, change, current);
    }
  }

  // Append the content of tempBuffer to memInfo->final_string
  // Ensure we don't exceed memInfo->final_string's size
  strncat(memInfo->final_string, tempBuffer,
          BUF_SIZE - strlen(memInfo->final_string) - 1);
}

void collectMemoryInfo(int writeEnd) {
  struct sysinfo *x = malloc(sizeof(struct sysinfo));
  sysinfo(x);

  double total_physical = (double)x->totalram / (1024 * 1024 * 1024);
  double physical_used =
      total_physical - (double)x->freeram / (1024 * 1024 * 1024);
  double total_virtual =
      total_physical + (double)x->totalswap / (1024 * 1024 * 1024);
  double virtual_used = total_virtual -
                        (double)x->freeram / (1024 * 1024 * 1024) -
                        (double)x->freeswap / (1024 * 1024 * 1024);

  // struct MemoryInfo memInfo;

  // printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB ", physical_used,
  // total_physical, virtual_used, total_virtual);

  int written = write(writeEnd, &total_physical, sizeof(double));
  write(writeEnd, &physical_used, sizeof(double));
  write(writeEnd, &total_virtual, sizeof(double));
  write(writeEnd, &virtual_used, sizeof(double));
  // Write the memory information struct to the pipe
  // ssize_t written = write(writeEnd, &memInfo, sizeof(memInfo));
  if (written == -1) {
    perror("Failed to write memory info to pipe");
    exit(EXIT_FAILURE);
  }

  close(writeEnd); // Close the write end of the pipe
}

void printMemoryInfo(struct MemoryInfo *memInfo, double *prev, int graphics) {
  int pipeMemory[2];
  pid_t pidMemory;

  if (pipe(pipeMemory) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  // Fork and collect memory info
  pidMemory = fork();
  if (pidMemory == 0) {
    close(pipeMemory[0]); // Close read end
    collectMemoryInfo(pipeMemory[1]);
    exit(EXIT_SUCCESS);
  } else {
    close(pipeMemory[1]); // Close write end in parent
  }

  // wait(NULL); // Wait for all child processes to complete
  read(pipeMemory[0], &memInfo->total_physical, sizeof(double));
  read(pipeMemory[0], &memInfo->physical_used, sizeof(double));
  read(pipeMemory[0], &memInfo->total_virtual, sizeof(double));
  read(pipeMemory[0], &memInfo->virtual_used, sizeof(double));

  snprintf(
      memInfo->final_string, BUF_SIZE,
      "%.2f GB / %.2f GB  -- %.2f GB / %.2f GB", memInfo->physical_used,
      memInfo->total_physical, memInfo->virtual_used,
      memInfo->total_virtual); // printf("CPU Usage: %.2f%%\n", cpuInfo.usage);
  if (graphics > 0) {
    printMemoryChange(*prev, memInfo->physical_used, memInfo->total_physical,
                      memInfo);
  } else {
    strncat(memInfo->final_string, "\n",
            BUF_SIZE - strlen(memInfo->final_string) - 1);
    // snprintf(memInfo->final_string, 1024, "\n");
  }
  *prev = memInfo->physical_used;
  close(pipeMemory[0]);
}
