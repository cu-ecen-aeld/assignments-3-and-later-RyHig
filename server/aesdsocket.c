#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT "9000"
#define BACKLOG 10
#define FILE_NAME "/var/tmp/aesdsocketdata"

bool caught_sigint = false;
bool caught_sigterm = false;

static void signal_handler(int signal_number) {
    if (signal_number == SIGINT) {
        caught_sigint = true;
    } 
    else if (signal_number == SIGTERM) {
        caught_sigterm = true;
    }
}

int send_and_receive(int sockfd) {
    int buf_size = 1024;
    int new_fd, byte_count, byte_total = 0;
    struct sockaddr_in their_addr;
    socklen_t sin = sizeof their_addr;
    char *buf = malloc(buf_size * sizeof(char));
    char ip_str[INET_ADDRSTRLEN];
    FILE *file_to_write = fopen(FILE_NAME, "a+");
    if (!file_to_write) {
        int err_val = errno;
        if (errno != EINTR) {
            syslog(LOG_ERR, "Error opening /var/tmp/aesdsocketdata: %s", strerror(err_val));
            free(buf);
            closelog();
            return -1;
        }
    }
    do {
        if (listen(sockfd, BACKLOG) == -1) {
            int err_val = errno;
            if (errno != EINTR) {
                syslog(LOG_ERR, "Error when starting to listen on socket file descriptor: %s", strerror(err_val));
                free(buf);
                closelog();
                return -1;
            }
        }
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin);
        if (new_fd == -1) { 
            int err_val = errno;
            if (errno != EINTR) { 
                syslog(LOG_ERR, "Error when starting accept on socket file descriptor: %s", strerror(err_val));
                free(buf);
                closelog();
                return -1;
            }
        }
        inet_ntop(AF_INET, &their_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        syslog(LOG_DEBUG, "Accepted connection to %s\n", ip_str);

        byte_count = recv(new_fd, buf, buf_size, 0);
        while(byte_count == buf_size) {
            fwrite(buf, sizeof buf[0], byte_count, file_to_write);
            byte_total += byte_count;
            byte_count = recv(new_fd, buf, buf_size, 0);
        }
        byte_total += byte_count;
        fwrite(buf, sizeof buf[0], byte_count, file_to_write);
        rewind(file_to_write);
        if(byte_total > buf_size) {
            if ((buf = realloc(buf, sizeof(char) * byte_total)) == NULL) {
                int err_val = errno;
                if (errno != EINTR) {
                    syslog(LOG_ERR, "Error reallocating memory for buf: %s", strerror(err_val));
                    free(buf);
                    closelog();
                    return -1;
                }
            }
        }
        fread(buf, sizeof buf[0], byte_total, file_to_write);
        byte_count = send(new_fd, buf, byte_total, 0);
        shutdown(new_fd, 2);
        syslog(LOG_DEBUG, "Closed connection to %s\n", ip_str);
    }
    while(!caught_sigint && !caught_sigterm);
    syslog(LOG_DEBUG, "Caught signal, exiting");
    fclose(file_to_write);
    if (remove(FILE_NAME) == -1) {
        int err_val = errno;
        if (errno != EINTR) {
            syslog(LOG_ERR, "Error while removing file: %s", strerror(err_val));
            free(buf);
            closelog();
        }
    }
    free(buf);
    shutdown(new_fd, 2);
    return 0;
}

int main(int argc, char* argv[]) {
    int sockfd; 
    struct addrinfo hints, *res;
    struct sigaction new_action;
    pid_t childpid;
    bool daemon = false;
    int yes=1;

    openlog("aesdsocket", 0, LOG_USER);
    
    if (argc >= 2) {
        if (strcmp(argv[1],"-d") == 0) {
            daemon = true;
            syslog(LOG_DEBUG, "Starting in daemon mode.");
        }
    } else {
        syslog(LOG_DEBUG, "Starting in user mode.");
    }
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    if (sigaction(SIGTERM, &new_action, NULL) != 0) {
        int err_val = errno;
        syslog(LOG_ERR, "Error registering handler for SIGTERM: %s", strerror(err_val));
        closelog();
        return -1;
    }
    if (sigaction(SIGINT, &new_action, NULL) != 0) {
        int err_val = errno;
        syslog(LOG_ERR, "Error registering handler for SIGINT: %s", strerror(err_val));
        closelog();
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &res);
    syslog(LOG_DEBUG, "Attempting to create a socket file descriptor");
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        int err_val = errno;
        if (errno != EINTR) {
            syslog(LOG_ERR, "Error creating a socket file descriptor: %s", strerror(err_val));
            freeaddrinfo(res);
            closelog();
            return -1;
        }
    }
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) { 
        int err_val = errno;
        if (errno != EINTR) {
            syslog(LOG_ERR, "Error adjusting socket file descriptor options: %s", strerror(err_val));
            freeaddrinfo(res);
            closelog();
            return -1;
        }
        exit(1);
    } 
    syslog(LOG_DEBUG, "Attempting to bind to socket file descriptor");
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        int err_val = errno;
        if (errno != EINTR) {
            syslog(LOG_ERR, "Error creating binding to socket file descriptor: %s", strerror(err_val));
            freeaddrinfo(res);
            closelog();
            return -1;
        }
    }
    freeaddrinfo(res);
    if(daemon) {
        switch(childpid = fork()) {
            case -1:
                shutdown(sockfd, 2);
                closelog();
                return -1;
            case 0:
                if (send_and_receive(sockfd) == -1) {
                    shutdown(sockfd, 2);
                    closelog();
                    return -1;
                }
            default:
                return 0;
        }
    }
    else {
        if (send_and_receive(sockfd) == -1) {
            shutdown(sockfd, 2);
            closelog();
            return -1;
        }
    }
    shutdown(sockfd, 2);
    closelog();
    return 0;
}
