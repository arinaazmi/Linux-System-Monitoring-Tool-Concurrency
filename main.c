// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

// #include <fcntl.h>
// #include <ctype.h>
// #include <getopt.h>
// #include <math.h>

// #include <sys/resource.h>
// #include <sys/sysinfo.h>
// #include <sys/utsname.h>
// #include <utmp.h>

#define BUF_SIZE 1024

// #include <errno.h>
// #include <limits.h>
// #include "stats_functions.h"
// #include "stats_functions.c"
#include "cpu.c"
#include "memory.c"
#include "user.c"

#include "stats_functions.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void printDivider() {
  printf("--------------------------------------------------\n");
}

void numOfCore() {
  FILE *fp;
  char buffer[BUF_SIZE];
  char *keyword = "cpu cores";
  int cpu_cores = 0;

  fp = fopen("/proc/cpuinfo", "r");
  if (fp == NULL) {
    perror("Error opening file");
  }

  while (fgets(buffer, sizeof(buffer), fp)) {

    if (strncmp(buffer, keyword, strlen(keyword)) == 0) {
      cpu_cores++;
    }
  }

  fclose(fp);
  printf("Number of cores: %d\n", cpu_cores);
}
// error checking for passing invalid arguments
int validate_and_convert_integer(char *str, int *val) {
  char *endptr;
  errno = 0; // To distinguish success/failure after the call to strtol
  long int number = strtol(str, &endptr, 10);

  // Check for conversion errors
  if ((errno == ERANGE && (number == LONG_MAX || number == LONG_MIN)) ||
      (errno != 0 && number == 0)) {
    perror("strtol");
    return 0; // Indicate failure
  }

  if (endptr == str) {
    fprintf(stderr, "No digits were found\n");
    return 0; // Indicate failure
  }

  if (*endptr != '\0') { // Not the entire string was consumed, additional
                         // characters present after the number
    fprintf(stderr, "Further characters after number: %s\n", endptr);
    return 0; // Indicate failure
  }

  // Check for integer overflow/underflow
  if (number > INT_MAX || number < INT_MIN) {
    fprintf(stderr, "Integer overflow or underflow\n");
    return 0; // Indicate failure
  }

  *val = number; // Assign the converted number
  return 1;      // Indicate success
}

void sigint_handler(int code) {
  char ans;
  char answer[4];
  while (1) {
    printf("\nDo you really want to quit? [Y/N]: ");

    if (fgets(answer, sizeof(answer), stdin)) {
      // Remove newline, if present
      answer[strcspn(answer, "\n")] = 0;

      if (tolower(answer[0]) == 'y') {
        printf("Quitting...\n");
        exit(0);
      } else if (tolower(answer[0]) == 'n') {
        printf("\nContinuing execution...\n");
        break;
      } else {
        printf("Invalid input, try again.\n");
      }
    } else {
      // If fgets fails,
      int c;
      while ((c = getchar()) != '\n' && c != EOF) {
      } // Clear the input buffer
      printf("Error reading input, try again.\n");
    }
  }
}

void sigtstpHandler(int sig_num) {
  // Simply ignore SIGTSTP and return
  printf("\nIgnoring Ctrl-Z.\n");
}

void convertSeconds(int seconds) {
  int days = seconds / (24 * 3600);
  seconds = seconds % (24 * 3600);
  int hours = seconds / 3600;
  seconds %= 3600;
  int minutes = seconds / 60;
  seconds %= 60;

  printf("%d days %02d:%02d:%02d (%d:%02d:%02d)\n", days, hours, minutes,
         seconds, days * 24 + hours, minutes, seconds);
}

void getRunTime() {
  FILE *fp;
  char buffer[BUF_SIZE];
  fp = fopen("/proc/uptime", "r");
  if (fp == NULL) {
    perror("Error opening file");
  }

  fgets(buffer, BUF_SIZE, fp);
  char *element = strtok(buffer, " ");
  int a = atoi(element);
  printf("System running since last reboot: ");
  convertSeconds(a);
  fclose(fp);
}

void printSystemInfo() {
  struct utsname *x = malloc(sizeof(struct utsname));
  uname(x);
  printf("System Name = %s \n", x->sysname);
  printf("Machine Name = %s \n", x->nodename);
  printf("Version = %s \n", x->version);
  printf("Release = %s \n", x->release);
  printf("Architecture = %s \n", x->machine);
  getRunTime();
  free(x);
}

void handle_signal() {
  struct sigaction sigact;
  sigact.sa_handler = sigint_handler;
  sigact.sa_flags = 0;
  sigemptyset(&sigact.sa_mask);
  sigaction(SIGINT, &sigact, NULL);
  
  struct sigaction sigact2;
  sigact2.sa_handler = sigtstpHandler;
  sigact.sa_flags = 0;
  sigemptyset(&sigact2.sa_mask);
  sigaction(SIGTSTP, &sigact2, NULL);
}

void printNewLines(int i, int sample) {
  int numNeeded = sample - i - 1;
  // printf("number of new lines %d", numNeeded);
  for (i = 0; i < numNeeded; i++) {
    printf("\n");
  }
}
long returnMemoryUsage() {
  struct rusage *x = malloc(sizeof(struct rusage));
  if (getrusage(RUSAGE_SELF, x) != 0) {
    printf("error getting usage");
    return -1;
  }
  int result = x->ru_maxrss;
  free(x);
  return result;
}

void printParameters(int num_sample, int tdelay) {

  long memory_usage = returnMemoryUsage();

  // samples
  printf("Nbr of samples: %d -- every %d secs\n", num_sample, tdelay);

  // memory usage
  printf("Memory usage: %ld kilobytes\n", memory_usage);
  printDivider();
}

void printMachine(int sample, int tdelay, int system, int userUsage,
                  int sequential, int graphics) {

  struct MemoryInfo memInfo[sample];
  struct CPUInfo cpuInfo[sample];
  double prevPhysical = -1;
  // int graphic = 0;
  long prevCPU[2] = {-1, -1};

  if (sequential > 0) {
    for (int i = 0; i < sample; i++) {
      printf("iteration: %d\n", i);

      printParameters(sample, tdelay);

      if ((system == -1 && userUsage == -1) || system > 0) {

        printMemoryInfo(&memInfo[i], &prevPhysical, graphics);
        printf("%s", memInfo[i].final_string);

        printNewLines(i, sample);
      }

      // handle spaces
      if ((system == -1 && userUsage == -1) || userUsage > 0)
        print_user_section();
      if ((system == -1 && userUsage == -1) || system > 0) {
        numOfCore();
        printf(" total cpu use = "); // store the first number somewhere
        printCPUUtalization(tdelay, &prevCPU[0], &prevCPU[1], &cpuInfo[i],
                            graphics);

        if (graphics > 0) {
          for (int j = 0; j <= i; j++) {
            printf("%s", cpuInfo[j].final_string);
          }
        } else {
          printf("%s", cpuInfo[i].final_string);
        }
      }

      sleep(tdelay);
    }
    printDivider();
    printSystemInfo();
    return;
  }

  for (int i = 0; i < sample; i++) {

    printf("\x1B[2J\x1B[H");

    printParameters(sample, tdelay);

    if (system > 0) {
      for (int j = 0; j <= i; j++) {
        printMemoryInfo(&memInfo[i], &prevPhysical, graphics);
        printf("%s", memInfo[j].final_string);
      }
      printNewLines(i, sample);
    }

    // handle spaces
    if (userUsage > 0)
      print_user_section();
    if (system > 0) {
      numOfCore();
      printf(" total cpu use = "); // store the first number somewhere
      printCPUUtalization(tdelay, &prevCPU[0], &prevCPU[1], &cpuInfo[i],
                          graphics);

      if (graphics > 0) {
        for (int j = 0; j <= i; j++) {
          printf("%s", cpuInfo[j].final_string);
        }
      } else {
        printf("%s", cpuInfo[i].final_string);
      }
    }

    sleep(tdelay);
  }
  printDivider();
  printSystemInfo();
}

int main(int argc, char *argv[]) {
  handle_signal();

  int c;

  int system_flag = 0;
  int user_flag = 0;
  int graphics_flag = 0;
  int sequential_flag = 0;
  int samples_flag = 0;
  int tdelay_flag = 0;

  int samples_value = 10;
  int tdelay_value = 1;

  if (argc == 1) {
    // No arguments were provided other than the program name
    printMachine(samples_value, tdelay_value, 1, 1, 0, 0);
    // printf("no arg\n");
    return 0;

  } else {

    while (1) {
      int option_index = 0;
      static struct option long_options[] = {
          {"system", no_argument, 0, 's'}, // Using short option 's' as a marker
          {"user", no_argument, 0, 'u'},   // Using short option 'u' as a marker
          {"graphics", no_argument, 0,
           'g'}, // Using short option 'g' as a marker
          {"sequential", no_argument, 0,
           'S'}, // Using short option 'S' as a marker
          {"samples", required_argument, 0, 'c'},
          {"tdelay", required_argument, 0, 't'},
          {0, 0, 0, 0}};

      c = getopt_long(argc, argv, "usgS-:c:t:", long_options, &option_index);
      if (c == -1) {
        break;
      }
      switch (c) {
      case 0:
        // If this case is hit, it means a flag was automatically set by
        // getopt_long
        printf("option %s", long_options[option_index].name);
        if (optarg)
          printf(" with arg %s", optarg);
        printf("\n");
        break;

      case 'u':
        user_flag = 1;
        // printf("User is selected\n");
        break;
      case 's':
        system_flag = 1;
        // printf("system argument is written\n");
        break;
      case 'g':
        graphics_flag = 1;
        user_flag = 1;
        system_flag = 1;
        // printf("graphics argument is written\n");
        break;
      case 'S':
        sequential_flag = 1;
        user_flag = -1;
        system_flag = -1;
        // printf("sequential argument is written\n");
        break;
      case 'c':
        samples_flag = 1;
        user_flag = 1;
        system_flag = 1;

        samples_value = atoi(optarg);
        // printf("Samples is selected, value = %d\n", samples_value);
        break;

      case 't':
        tdelay_flag = 1;
        user_flag = 1;
        system_flag = 1;
        tdelay_value = atoi(optarg);
        printf("Tdelay is selected, value = %d\n", tdelay_value);
        break;
      }
    }
    if (argc - optind >= 1) { // If there's at least one additional argument
      if (!validate_and_convert_integer(argv[optind], &samples_value)) {
        fprintf(stderr, "Invalid integer for samples: %s\n", argv[optind]);
        exit(EXIT_FAILURE);
      }
      if (argc - optind >= 2) { // If there's a second additional argument
        if (!validate_and_convert_integer(argv[optind + 1], &tdelay_value)) {
          fprintf(stderr, "Invalid integer for tdelay: %s\n", argv[optind + 1]);
          exit(EXIT_FAILURE);
        }
      }
    }
    // Check for non-option arguments
    if (argc - optind >= 1) { // If there's at least one additional argument
      samples_value = atoi(argv[optind]); // Use the first for samples
      if (user_flag == 0 && system_flag == 0) {
        user_flag = 1;
        system_flag = 1;
      }
      if (argc - optind == 2) { // If there's a second, use it for tdelay
        tdelay_value = atoi(argv[optind + 1]);
      }
    }
    printf("Samples: %d, Tdelay: %d\n", samples_value, tdelay_value);
    printMachine(samples_value, tdelay_value, system_flag, user_flag,
                 sequential_flag, graphics_flag);

    return 0;
  }
}
