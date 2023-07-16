#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int fd, nr;

    openlog("writer", 0, LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "Number of arguments supplied not 2; got %d", argc-1);
        closelog();
        return 1;
    }
    const char *writefile = argv[1];
    const char *writestr = argv[2];

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    fd = open(writefile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to create and open file %s", writefile);
        closelog();
        return 1;
    }

    nr = write(fd, writestr, strlen(writestr));
    if (nr == -1) {
        syslog(LOG_ERR, "Failed to write %s to %s", writestr, writefile);
        closelog();
        return 1;
    }
    syslog(LOG_DEBUG, "Sucessfully wrote %s to %s", writestr, writefile);
    closelog();

    close(fd);
    return 0;
}
