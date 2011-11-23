#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static ssize_t
fullwrite(int fd, const void *buf, size_t nbyte)
{
	ssize_t total = 0, ret;

	while (total < nbyte) {
		if ((ret = write(fd, buf + total, nbyte - total)) < 0)
			return (ret);
		total += ret;
	}

	return (total);
}

static ssize_t
fullread(int fd, void *buf, size_t nbyte)
{
	ssize_t total = 0, ret;

	while (total < nbyte) {
		if ((ret = read(fd, buf + total, nbyte - total)) < 0)
			return (ret);
		total += ret;
	}

	return (total);
}

static int
dowrite(int fd, long nblocks, long blocksize)
{
	int ii, jj;
	char *buf;

	if ((buf = malloc(blocksize)) == NULL) {
		perror("malloc");
		return (-1);
	}

	for (ii = 0; ii < nblocks; ii++) {
		for (jj = 0; jj < blocksize; jj += 2) {
			buf[jj] = ii;
			buf[jj + 1] = ii >> 8;
		}

		if (fullwrite(fd, buf, blocksize) < blocksize) {
			fprintf(stderr, "Failed to write %d bytes: %s\n",
			    blocksize, strerror(errno));
			free(buf);
			return (-1);
		}
	}

	free(buf);
	return (0);
}

static int
doread(int fd, long nblocks, long blocksize)
{
	int ii, jj;
	uint16_t s;
	uint8_t *buf;

	if ((buf = malloc(blocksize)) == NULL) {
		perror("malloc");
		return (-1);
	}

	for (ii = 0; ii < nblocks; ii++) {
		if (fullread(fd, buf, blocksize) < blocksize) {
			fprintf(stderr, "Failed to read %d bytes: %s\n",
			    blocksize, strerror(errno));
			free(buf);
			return (-1);
		}

		for (jj = 0; jj < blocksize; jj += 2) {
			printf("%02X%02X ", buf[jj + 1], buf[jj]);

			if (jj % 32 == 30)
				printf("\n");
		}
	}

	free(buf);
	return (0);
}

int
main(int argc, char *argv[])
{
	char *device, c;
	uint64_t nblocks = 8192i;
	uint64_t blocksize = 131072;
	uint64_t offset = 0;
	boolean_t isread = B_FALSE;
	int dev, rv;

	extern int optind, optopt;
	extern char *optarg;

	errno = 0;
	device = "/dev/zvol/dsk/zones/dump";

	while ((c = getopt(argc, argv, "d:n:o:rs:")) != EOF) {
		switch (c) {
		case 'd':
			device = optarg;
			break;
		case 'n':
			nblocks = atoi(optarg);
			break;
		case 'o':
			offset = atoi(optarg);
			break;
		case 'r':
			isread = B_TRUE;
			break;
		case 's':
			blocksize = atoi(optarg);
			break;
		default:
			fprintf(stderr,
			    "Unrecognized option: -%c\n", optopt);
			rv = -1;
			goto out;
		}
	}

	printf("Device: %s\nBlock Size: %d\nBlock Count: %d\n"
	    "Device Offset: %d\n\n", device, blocksize, nblocks, offset);

	if ((dev = open(device, O_RDWR | O_LARGEFILE)) < 0) {
		fprintf(stderr, "Failed to open device '%s': %s\n",
		    device, strerror(errno));
		rv = -1;
		goto out;
	}

	if (lseek(dev, offset * blocksize, SEEK_SET) < 0) {
		fprintf(stderr, "Failed to seek to offset %d: %s\n",
		    offset * blocksize, strerror(errno));
		rv = -1;
		goto out;
	}

	if (isread)
		rv = doread(dev, nblocks, blocksize);
	else
		rv = dowrite(dev, nblocks, blocksize);

	printf("%d %d", nblocks, blocksize);

	uint64_t nbytes = nblocks * blocksize;
	uint64_t nbytes_in_MiB = nbytes / 1024 / 1024;

	printf("\nProcessed %d blocks (%d MiB).\n",
	    nblocks, nbytes_in_MiB);

out:
	(void) close(dev);
	return (rv);
}
