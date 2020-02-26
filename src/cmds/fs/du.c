/**
 * @file
 * @brief
 *
 * @date 26.02.20
 * @author Bartlomiej Kocot
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BLK_SIZE 512

static void print_usage(void) {
	printf("Usage: du [-b BYTES] [-B BLOCK_SIZE] [FILE_OR_DIR] ...\n");
}

long long divide_with_round_up(long long size, size_t block_size) {
    long long  result = size / block_size;
    return result * block_size == size ? result : result + 1;
}

static void print_file_mem_usage(const char *path, struct stat *sb, size_t block_size) {
    printf("%-21llu %s\n", divide_with_round_up((long long)sb->st_blocks, block_size), path);
}

static void print(char *path, DIR *dir, size_t block_size) {
	struct dirent *dent;

	while (NULL != (dent = readdir(dir))) {
		int pathlen = strlen(path);
		int dent_namel = strlen(dent->d_name);
		char line[pathlen + dent_namel + 3];
		struct stat sb;

		if (pathlen > 0) {
			sprintf(line, "%s/%s", path, dent->d_name);
		} else {
			strcpy(line, dent->d_name);
		}

		if (-1 == stat(line, &sb)) {
			printf("Cannot stat %s\n", line);
			continue;
		}

        if (S_ISREG(sb.st_mode)) {
		    print_file_mem_usage(line, &sb, block_size);
		} else if(S_ISDIR(sb.st_mode)) {
			DIR *d;

			if (NULL == (d = opendir(line))) {
				printf("Cannot recurse to %s\n", line);
			}

			print(line, d, block_size);

			closedir(d);
		}
	}
}

int main(int argc, char *argv[]) {
	int opt;
    DIR *dir;
    char dir_name[NAME_MAX];
    size_t block_size = BLK_SIZE;

	while (-1 != (opt = getopt(argc, argv, "B:bh"))) {
		switch(opt) {
		case 'h':
			print_usage();
			return 0;
        case 'b':
            block_size = 1;
            break;
        case 'B':
            if ((optarg == NULL) || (!sscanf(optarg, "%d", &block_size)) || block_size <= 0) {
				printf("du -B: positive number expected\n");
				print_usage();
				return -EINVAL;
			}
            break;
        case '?':
			break;
		default:
			printf("du: invalid option -- '%c'\n", optopt);
            return -EINVAL;
		}
	}

	if (optind < argc) {
		struct stat sb;

		if (-1 == stat(argv[optind], &sb)) {
			return 0;
		}

		if (!S_ISDIR(sb.st_mode)) {
			print_file_mem_usage(argv[optind], &sb, block_size);
			return 0;
		}

		sprintf(dir_name, "%s", argv[optind]);
	} else {
		strcpy(dir_name, ".");
	}

	if (NULL == (dir = opendir(dir_name))) {
		return -errno;
	}

	print(dir_name, dir, block_size);

	closedir(dir);

	return 0;
}