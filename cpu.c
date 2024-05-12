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

struct CPUInfo {
  long total1;
  long idle1;
  char final_string[BUF_SIZE];
};

long getIdle() {
  FILE *fp;
  char buffer[BUF_SIZE];

  fp = fopen("/proc/stat", "r");
  if (fp == NULL) {
    perror("Error opening /proc/stat");
    exit(EXIT_FAILURE);
  }

  fgets(buffer, BUF_SIZE, fp);
  char *element = strtok(buffer, " ");
  long values[10];
  int i = 0;

  // Skip the first token ("cpu")
  element = strtok(NULL, " ");

  // Convert remaining elements to integers
  while (element != NULL && i < 10) {
    values[i] = strtol(element, NULL, 10);
    element = strtok(NULL, " ");
    i++;
  }
  fclose(fp);
  // printf("idle got successfully %ld\n", values[3]);
  return values[3];
}

long getTotalCPU() {
  FILE *fp;
  char buffer[BUF_SIZE];

  fp = fopen("/proc/stat", "r");
  if (fp == NULL) {
    perror("Error opening /proc/stat");
    exit(EXIT_FAILURE);
  }

  fgets(buffer, BUF_SIZE, fp);
  char *element = strtok(buffer, " ");
  long values[10]; // Assuming at most 10 elements
  int i = 0;

  // Skip the first token ("cpu")
  element = strtok(NULL, " ");

  // Convert remaining elements to integers
  while (element != NULL && i < 10) {
    values[i] = strtol(element, NULL, 10);
    element = strtok(NULL, " ");
    i++;
  }

  long total = 0;

  for (int j = 0; j < i; j++) {
    total = total + values[j];
  }
  fclose(fp);
  // printf("total got successfully %ld\n", total);

  return total;
}
// void printGraphicalCPUUtilization(double utilization) {
//     int bars = utilization * 10; // scaling factor
//     for (int i = 0; i < bars; i++) {
//         printf("|");
//     }
//     printf(" %.2f %%\n", utilization);
// }

void appendGraphicalCPUUtilization(double utilization, char *final_string,
                                   size_t bufsize) {
  int bars = utilization * 10; // scaling factor
  size_t len = strlen(final_string);
  for (int i = 0; i < bars && len < bufsize - 1; i++, len++) {
    final_string[len] = '|'; // Append bars directly to final_string
  }
  final_string[len] = '\0'; // Null-terminate the string
}

void printCPUUtalization(int tdelay, long *prevIdle, long *prevTotal,
                         struct CPUInfo *x, int graphics) {
  // first iteration
  int f = 0;
  if (*prevIdle == -1 || *prevTotal == -1) {
    // printf("first iteration\n");
    *prevIdle = getIdle();
    *prevTotal = getTotalCPU();
    f = 1;
    sleep(tdelay);
  }

  long first_idle = getIdle();

  long first_total = getTotalCPU();
  // sleep(tdelay);
  // printf("1st idle %ld, first total %ld\n", first_idle, first_total);
  // printf("prev idle %ld, prev total %ld\n", *prevIdle, *prevIdle);

  long u2 = first_total - first_idle;
  long u1 = *prevTotal - *prevIdle;
  long bigU = u2 - u1;
  long bigT = first_total - *prevTotal;
  // printf("u1 = %ld - u2= %ld = %ld\n", u1, u2, bigU);
  // printf("t2 = %ld - t1 = %ld = %ld\n", *prevTotal, first_total, bigT);

  double cpu = (double)(bigU) / (double)(bigT) * 100;
  if (f == 1)
    snprintf(x->final_string, BUF_SIZE, "%.2f %%\n", cpu);

  // double idle_diff = second_idle - first_idle;
  double total_diff = first_total - *prevTotal;
  double utilization = 0;

  if (graphics > 0) {
    appendGraphicalCPUUtilization(cpu, x->final_string, BUF_SIZE);
    int len = strlen(x->final_string);
    snprintf(x->final_string + len, BUF_SIZE - len, " %.2f %%\n",
             cpu); // Append utilization text
  } else {
    snprintf(x->final_string, BUF_SIZE, "%.2f %%\n",
             cpu); // Overwrite if no graphics
  }

  // if (graphics > 0){
  //     double utilization = cpu;
  //     //printf("printing graphics\n");
  //     printGraphicalCPUUtilization(utilization);

  // }
  x->idle1 = first_idle;
  x->total1 = first_total;
  *prevIdle = x->idle1;
  *prevTotal = x->total1;
  // make sure its not division by 0
}

void collectCPUInfo(int writeEnd) {
  int graphics = 0; // edit this later

  long first_idle = getIdle();
  long first_total = getTotalCPU();

  int written = write(writeEnd, &first_idle, sizeof(long));
  write(writeEnd, &first_total, sizeof(long));
  if (written == -1) {
    perror("Failed to write memory info to pipe");
    exit(EXIT_FAILURE);
  }
  // printf("preidle %ld pretotal %ld\n", first_idle, first_total);
  close(writeEnd); // Close the write end of the pipe
}