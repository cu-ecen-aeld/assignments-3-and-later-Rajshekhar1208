#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define PORT "9000"
#define BACKLOG 10
#define BUFSIZE 256
#define TMPFILE "/var/tmp/aesdsocketdata"

bool sig_received = false;

void _graceful_exit_handler(int signo) {
    syslog(LOG_INFO, "Caught signal, exiting...");
    sig_received = true;
}

void usage(void) {
    printf("aesdsocket: Usage: aesdsocket [-d]\n");
}

int main(int argc, char** argv) {
    bool daemon_mode = false;
    int error;

    openlog("aesdsocket", LOG_PERROR | LOG_PID, LOG_USER);

    if (argc > 2) {
        usage();
        exit(EXIT_FAILURE);
    } else if (argc == 2) {
        if (strcmp(argv[1], "-d") != 0) {
            usage();
            exit(EXIT_FAILURE);
        }
        daemon_mode = true;
    }

    struct sigaction graceful_exit;
    memset(&graceful_exit, 0, sizeof(graceful_exit));
    graceful_exit.sa_handler = _graceful_exit_handler;
    sigfillset(&graceful_exit.sa_mask);
    graceful_exit.sa_flags = 0;

    if (sigaction(SIGINT, &graceful_exit, NULL) < 0 ||
        sigaction(SIGTERM, &graceful_exit, NULL) < 0) {
        syslog(LOG_ERR, "sigaction: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *server_info;
    error = getaddrinfo(NULL, PORT, &hints, &server_info);
    if (error != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    int socket_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_fd < 0) {
        syslog(LOG_ERR, "socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        syslog(LOG_ERR, "setsockopt: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (bind(socket_fd, server_info->ai_addr, server_info->ai_addrlen) < 0) {
        syslog(LOG_ERR, "bind: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(server_info);

    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "fork: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        if (setsid() < 0) {
            syslog(LOG_ERR, "setsid: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (chdir("/") < 0) {
            syslog(LOG_ERR, "chdir: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > 2) close(fd);
        }
    }

    if (listen(socket_fd, BACKLOG) < 0) {
        syslog(LOG_ERR, "listen: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    FILE* fp = fopen(TMPFILE, "a+");
    if (!fp) {
        syslog(LOG_ERR, "fopen: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while (!sig_received) {
        struct sockaddr_storage client_addr;
        socklen_t client_addrlen = sizeof(client_addr);

        int connection_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addrlen);
        if (connection_fd < 0) {
            if (sig_received) break;
            syslog(LOG_ERR, "accept: %s", strerror(errno));
            continue;
        }

        char client_ip[NI_MAXHOST];
        error = getnameinfo((struct sockaddr*)&client_addr, client_addrlen, client_ip, sizeof(client_ip), NULL, 0, NI_NUMERICHOST);
        if (error != 0) {
            syslog(LOG_ERR, "getnameinfo: %s", gai_strerror(error));
            close(connection_fd);
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        char buffer[BUFSIZE];

        while (!sig_received) {
            ssize_t count = recv(connection_fd, buffer, BUFSIZE, 0);
            if (count < 0) {
                syslog(LOG_ERR, "recv: %s", strerror(errno));
                break;
            } else if (count == 0) {
                break;
            }

            size_t write_count = fwrite(buffer, 1, count, fp);
            if (write_count < (size_t)count) {
                syslog(LOG_ERR, "fwrite: partial write");
                break;
            }

            if (memchr(buffer, '\n', count)) {
                fflush(fp);
                break;
            }
        }

        fflush(fp);
        fseek(fp, 0, SEEK_SET);

        while (!sig_received && !feof(fp)) {
            size_t read_count = fread(buffer, 1, BUFSIZE, fp);
            if (ferror(fp)) {
                syslog(LOG_ERR, "fread: %s", strerror(errno));
                break;
            }

            size_t sent = 0;
            while (sent < read_count) {
                ssize_t sent_now = send(connection_fd, buffer + sent, read_count - sent, 0);
                if (sent_now < 0) {
                    syslog(LOG_ERR, "send: %s", strerror(errno));
                    break;
                }
                sent += sent_now;
            }
        }

        close(connection_fd);
        rewind(fp);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    fclose(fp);
    remove(TMPFILE);
    close(socket_fd);
    closelog();
    return 0;
}

