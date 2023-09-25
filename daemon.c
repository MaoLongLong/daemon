#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_LOG_FILE "/dev/null"

enum {
    EXIT_CANNOT_INVOKE = 126, /* Program located, but not usable.  */
    EXIT_ENOENT        = 127  /* Could not find program to exec.  */
};

int replace_fds(void) {
    int fd;
    int saved_stderr_fd = STDERR_FILENO;

    if (isatty(STDIN_FILENO)) {
        fd = open("/dev/null", O_WRONLY);
        if (fd == -1) {
            perror("cannot replace stderr");
            exit(1);
        }
        if (dup2(fd, STDIN_FILENO) != STDIN_FILENO) {
            perror("cannot replace stderr");
            close(fd);
            exit(1);
        }
    }

    if (isatty(STDOUT_FILENO)) {
        fd = open("/dev/null", O_WRONLY);
        if (fd == -1) {
            perror("cannot replace stdout");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO) {
            perror("cannot replace stdout");
            close(fd);
            exit(1);
        }
    }

    if (isatty(STDERR_FILENO)) {
        saved_stderr_fd = dup(STDERR_FILENO);
        if (dup2(STDOUT_FILENO, STDERR_FILENO) != STDERR_FILENO) {
            perror("cannot replace stderr");
            exit(1);
        }
    }

    return saved_stderr_fd;
}

void run(char **cmd) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("failed to fork_1");
        exit(1);
    }
    if (pid > 0) {
        waitpid(pid, NULL, 0);
        exit(0);
    }

    if (setsid() == -1) {
        perror("failed to setsid");
        exit(1);
    }

    signal(SIGCHLD, SIG_IGN);

    pid = fork();
    if (pid == -1) {
        perror("failed to fork_2");
        exit(1);
    }
    if (pid > 0) {
        fprintf(stderr, "%d\n", pid);
        exit(0);
    };

    int saved_stderr_fd = replace_fds();
    execvp(*cmd, cmd);

    int status      = errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE;
    int saved_errno = errno;

    if (dup2(saved_stderr_fd, STDERR_FILENO) == STDERR_FILENO)
        fprintf(stderr, "failed to execvp: %s\n", strerror(saved_errno));

    exit(status);
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
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage();
        exit(0);
    }
    run(argv + 1);
    return 0;
}
