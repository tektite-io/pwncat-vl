/*
 * Minimal PTY helper for busybox/Alpine environments.
 * Allocates a PTY via forkpty() and execs a shell, relaying I/O
 * between stdin/stdout and the PTY master. This gives pwncat a
 * proper PTY on systems that lack script, python, etc.
 *
 * Compile statically with musl:
 *   musl-gcc -static -Os -s -o pty_helper pty_helper.c -lutil
 */

#include <errno.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t child_dead = 0;

static void sigchld(int sig) {
    (void)sig;
    child_dead = 1;
}

int main(int argc, char *argv[]) {
    int master;
    pid_t pid;
    const char *shell = "/bin/sh";

    if (argc > 1)
        shell = argv[1];

    signal(SIGCHLD, sigchld);

    pid = forkpty(&master, NULL, NULL, NULL);
    if (pid < 0)
        return 1;

    if (pid == 0) {
        /* Child: exec the shell */
        execlp(shell, shell, "-i", (char *)NULL);
        _exit(127);
    }

    /* Parent: relay I/O between stdin/stdout and PTY master */
    struct pollfd fds[2];
    char buf[4096];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = master;
    fds[1].events = POLLIN;

    while (!child_dead) {
        int ret = poll(fds, 2, 200);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        if (fds[0].revents & POLLIN) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n <= 0) break;
            write(master, buf, n);
        }
        if (fds[0].revents & (POLLHUP | POLLERR))
            break;

        if (fds[1].revents & POLLIN) {
            ssize_t n = read(master, buf, sizeof(buf));
            if (n <= 0) break;
            write(STDOUT_FILENO, buf, n);
        }
        if (fds[1].revents & (POLLHUP | POLLERR))
            break;
    }

    close(master);
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
