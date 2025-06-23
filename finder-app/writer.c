#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    // Open connection to syslog
    openlog("writer", LOG_PID, LOG_USER);

    // Validate arguments
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments: expected 2, got %d", argc - 1);
        fprintf(stderr, "Usage: %s <writefile> <writestr>\n", argv[0]);
        closelog();
        return 1;
    }

    const char *writefile = argv[1];
    const char *writestr  = argv[2];

    // Open the file for writing (create if not exists, truncate if exists)
    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open file %s: %s", writefile, strerror(errno));
        closelog();
        return 1;
    }

    // Write string to file
    ssize_t bytes_written = write(fd, writestr, strlen(writestr));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Failed to write to file %s: %s", writefile, strerror(errno));
        close(fd);
        closelog();
        return 1;
    }

    // Log the successful write
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    // Clean up
    close(fd);
    closelog();
    return 0;
}

