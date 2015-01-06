#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char*argv[])
{
	int fd;
	int ret = 0;
	
	fd = open("/dev/watchdog", O_WRONLY);
	printf("watchdog fd = %d\n", fd);
	if (fd == -1) {
		printf("watchdog fail\n");
		exit(EXIT_FAILURE);
	}
	while (1) {
		ret = write(fd, "\0", 1);
		printf("watchdog write %d\n", ret);
		if (ret != 1) {
			ret = -1;
			printf("watchdog write fail\n");
			break;
		}
		sleep(10);
	}
	close(fd);
	return ret;
}
