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

struct UserInfo {
  char userCount[BUF_SIZE];
};

void collectUserInfo(int writeEnd) {
  int num = 0;
  // check malloc
  struct utmp *x;
  utmpname(_PATH_UTMP);
  setutent();
  x = getutent();
  char addw[400];
  char ret_word[1024] = "";

  size_t bytes_written = 0; // Track the number of bytes written to ret_word

  while (x != NULL) {
    if (x->ut_type == USER_PROCESS) {

      int written =
          snprintf(ret_word + bytes_written, sizeof(ret_word) - bytes_written,
                   "%s\t%s (%s)\n", x->ut_user, x->ut_line, x->ut_host);
      if (written > 0) {
        bytes_written += written; // to protect overflow
      }
    }
    x = getutent();
  }

  if (write(writeEnd, ret_word, strlen(ret_word) + 1) == -1) {
    // fprintf(stderr, "error with code %d\n", errno);
    exit(1);
  }
  // Close the utent file
  endutent();
}

void print_user_section() {
  struct UserInfo userInfo;
  printf("_____________________________________________________\n");
  int fd[2];
  if (pipe(fd) == -1) {
    fprintf(stderr, "Error code: %d\n", errno);
    exit(1);
  }
  int error_check = fork();
  if (error_check == -1) {
    fprintf(stderr, "Forking failed with code: %d\n", errno);
    exit(1);
  }
  // Child process
  else if (error_check == 0) {
    close(fd[0]);
    collectUserInfo(fd[1]);
    printf("user is collected\n");
    close(fd[1]);
    exit(0);
  }
  close(fd[1]);
  // Parent process

  if (read(fd[0], &userInfo.userCount, sizeof(char) * 1024) == -1) {
    fprintf(stderr, "Reading from pipe failed with error code %d\n", errno);
    exit(1);
  }

  close(fd[0]);
  printf("%s\n", userInfo.userCount);
  printf("_____________________________________________________\n");
}
