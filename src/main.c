#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

#define DAEMON_LOG "daemon.log"
#define log_errno(msg) log_error(msg ": %s", strerror(errno))

void replace_fds(void) {
    int fd;

    if (isatty(STDIN_FILENO)) {
        log_trace("redirecting stdin to /dev/null");
        fd = open("/dev/null", O_WRONLY);
        if (fd == -1) {
            log_errno("cannot replace stderr");
            exit(1);
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            log_errno("cannot replace stderr");
            exit(1);
        }
    }

    if (isatty(STDOUT_FILENO)) {
        log_trace("redirecting stdout to " DAEMON_LOG);
        fd = open(DAEMON_LOG, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            log_errno("cannot replace stdout");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            log_errno("cannot replace stdout");
            exit(1);
        }
    }

    if (isatty(STDERR_FILENO)) {
        log_trace("redirecting stderr to stdout");
        if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
            log_errno("cannot replace stderr");
            exit(1);
        }
    }
}

void run(int argc, char **argv) {
    pid_t pid = getpid();
    log_trace("parent: pid=%d pgid=%d sid=%d", pid, getpgid(pid), getsid(pid));

    pid = fork();
    if (pid == -1) {
        log_errno("fork1");
        exit(1);
    }
    if (pid > 0) exit(0);

    pid = getpid();
    log_trace("fork1: pid=%d pgid=%d sid=%d", pid, getpgid(pid), getsid(pid));
    if (setsid() == -1) {
        log_errno("setsid");
        exit(1);
    }
    log_trace("setsid: pid=%d pgid=%d sid=%d", pid, getpgid(pid), getsid(pid));

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid == -1) {
        log_errno("fork2");
        exit(1);
    }
    if (pid > 0) exit(0);

    pid = getpid();
    log_trace("fork2: pid=%d pgid=%d sid=%d", pid, getpgid(pid), getsid(pid));
    log_info("daemon process: pid=%d", pid);

    replace_fds();

    char *l[argc];
    int   i = 1;
    for (; i < argc; i++) {
        l[i - 1] = (char *)malloc(strlen(argv[i]) + 1);
        l[i - 1] = strcpy(l[i - 1], argv[i]);
    }
    l[i - 1] = NULL;

    int ret = execvp(l[0], l);

    i = 0;
    for (; i < argc - 1; i++) {
        free(l[i]);
    }

    if (ret == -1) {
        perror("daemon: execvp");
        exit(1);
    }
}

void usage(void) {
    fprintf(stderr, "Usage: daemon COMMAND [ARG]..\n\n");
    fprintf(stderr, "Example:\n  daemon python3 -m http.server\n");
    fprintf(stderr, "  daemon python3 -m http.server > custom_log_file.log\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        exit(2);
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "help") == 0) {
        usage();
        exit(0);
    }
    run(argc, argv);
    return 0;
}
