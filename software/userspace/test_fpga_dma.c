#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATTERN_LEN 16

int main(int argc, char *argv[])
{
	FILE *f;
	int ret;
	long dest_addr = 0x400;
	unsigned char bytepattern[PATTERN_LEN] = {
		0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
		0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e
	};
	unsigned char readback[PATTERN_LEN];

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [chrdev]\n", argv[0]);
		return -1;
	}

	f = fopen(argv[0], "w+");
	if (f == NULL) {
		perror("fopen");
		return -1;
	}

	ret = fseek(f, dest_addr, SEEK_SET);
	if (ret < 0) {
		perror("first fseek");
		goto cleanup;
	}

	ret = fwrite(bytepattern, sizeof(unsigned char), PATTERN_LEN, f);
	if (ret < 0) {
		perror("fwrite");
		goto cleanup;
	}

	ret = fseek(f, dest_addr, SEEK_SET);
	if (ret < 0) {
		perror("second fseek");
		goto cleanup;
	}

	ret = fread(readback, sizeof(unsigned char), PATTERN_LEN, f);
	if (ret < 0) {
		perror("fread");
		goto cleanup;
	}

	if (memcmp(bytepattern, readback, PATTERN_LEN) != 0) {
		fprintf(stderr, "patterns do not match\n");
		ret = -1;
		goto cleanup;
	}

	printf("patterns match!\n");

	ret = 0;
cleanup:
	fclose(f);
	return ret;
}
