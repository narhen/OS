#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK_SIZ 512

int main(int argc, char *argv[])
{
    int fd;
    char buffer[BLOCK_SIZ];

    if (argc != 2)
        return 1;

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("open");
        return 1;
    }

    memset(buffer, 0, sizeof buffer);
    while (read(fd, buffer, sizeof buffer)) {
        write(1, buffer, sizeof buffer);
        memset(buffer, 0, sizeof buffer);
    }

    close(fd);

    return 0;
}
