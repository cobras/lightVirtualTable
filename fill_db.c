#define _FILE_OFFSET_BITS  64
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "vtable.h"
#define COUNT 10000000

int add_tomap(struct s_data_head *data, time_t time, off_t off) {
	struct tm *tm = gmtime(&time);
	if (!data->off[tm->tm_year][tm->tm_mon][tm->tm_mday]) {
		data->off[tm->tm_year][tm->tm_mon][tm->tm_mday] = off;
//		printf("Writing at offset %zu\n", off);
	}
	return 0;
}

int add_datas(const char *filename, uint64_t count) {
	uint64_t i;
	float val_comp = 0.2f;
	uint64_t _date = time(NULL);
	int fd = open(filename, O_CREAT|O_TRUNC|O_RDWR, 0666);
	struct s_data_head data_head;
	
	if (fd < 0) return -1;
	memset(&data_head, 0, sizeof data_head);
	lseek(fd, sizeof data_head, SEEK_SET);
	for ( i = 0; i < count; i++) {
		
		struct s_data elm = {
			.date = _date,
			.type = i % 3,
			.index = i % 6,
			.val_comp = val_comp,
			.val_raw = (uint64_t) i,
			.trigger = i % 2
		};
		off_t off = lseek(fd, 0, SEEK_CUR);
		add_tomap(&data_head, elm.date, off);
		write(fd, &elm, sizeof(elm));
		val_comp++;
		_date++;
	}
	lseek(fd, 0, SEEK_SET);
	write(fd, &data_head, sizeof data_head);
	return 0;
}

int display_data(const char *filename, uint64_t count) {
	int fd = open(filename, O_RDONLY);
	int ret = -1;
	ssize_t nb_bytes = 0;
	struct s_data elm;
	uint64_t i;

	if (fd >= 0) {
		if (lseek(fd, sizeof(struct s_data_head), SEEK_SET) >= 0) {
			if (count > 0) {
				for (i = 0; i < count; i++) {
					nb_bytes = read(fd, &elm, sizeof(struct s_data));
					if (nb_bytes < 0) {
						ret = 0;
						ERROR("Error: %s is truncated", filename);
						goto error;
					}
					if (!nb_bytes) break;
					dump_data(&elm);
				}
			} else {
				i = 0;
				while(read(fd, &elm, sizeof(struct s_data)) > 0) {
					printf("i = %zu", i);
					dump_data(&elm);
				}
			}
		}
		ret = 1;
	}
error:
	return ret;
}

int main(int argc, char **argv) {
	int fd;
	int i;
	ssize_t nb_bytes;
	
	if (argc > 2) {
		if (argv[2][0] == 'c') {
			nb_bytes = add_datas(argv[1], strtoul(argv[3], 0, 10));
			if (nb_bytes < 0) goto file_error;
		} else if (argv[2][0] == 'd') {
			if (display_data(argv[1], argc > 3 ? strtoul(argv[3], NULL, 10): 0) < 0)
				goto file_error;
		} else if (argv[2][0] == 'a') {
			printf("sizeof row = %zu, \n", sizeof(struct s_data));
			struct s_data elm;
			fd = open(argv[1], O_RDONLY);
			if (fd >= 0) {
				lseek(fd, sizeof(struct s_data_head), SEEK_SET);
				off_t off = (off_t)atoi(argv[3]);
				printf("off = %jd\n", off);
				off = lseek(fd, -off * sizeof(struct s_data), SEEK_END);
				printf("Nb items = %zu\n", (off - sizeof(struct s_data_head))/ sizeof(struct s_data));
				while(read(fd, &elm, sizeof(struct s_data)) > 0) {
					printf("%d:date = %zu, type = %d, index=%d, val_comp=%f val_raw=%zu, trigger=%d\n",
								 i++,elm.date, elm.type, elm.index, elm.val_comp, elm.val_raw, elm.trigger);
					
				}
			} else goto file_error;
		}else goto usage;
	} else goto usage;
    
	close(fd);
	return 0;
file_error:
	printf("Error: file %s\n", strerror(errno));
	return 1;
usage:
	printf("Usage: %s <datfile> c count\n", argv[0]);
	printf("Usage: %s <datfile> d count\n", argv[0]);
	printf("Usage: %s <datfile> a count\n", argv[0]);
	return 2;
}
