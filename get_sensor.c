#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include "vtable.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump_data(struct s_data *data) {
//	printf("offset=%p,", data);
	struct tm *t= gmtime((time_t *)&data->date);
	char buffer[100];
	
	strftime(buffer, sizeof buffer,  "%d/%m/%Y %H:%M:%S", t);
//	printf("time = %zu,", data->date);
		printf("time = %s,", buffer);
	printf("type = %d,", data->type);
	printf("index = %d,", data->index);
	printf("val_comp = %f,", data->val_comp);
	printf("val_raw = %zu,", data->val_raw);
	printf("trigger = %d", data->trigger);
	printf("\n");
}

void dump_map_offset(struct s_data_head *map) {
	int year;
	int month;
	int day;
	
	for (year = 0; year < NB_YEAR; year++) {
		for (month = 0; month < NB_MONTH; month++) {
			for (day = 0; day < NB_DAY; day++) {
				if (map->off[year][month][day])
					DBG("map_offset:: year = %d, month=%d,day=%d, off=%zu\n",
							year, month, day, map->off[year][month][day]);
			}
		}
	}
}

off_t get_filter_offset(struct s_data_cursor *ctx, uint64_t start_date) {

	struct tm *tm_date = gmtime((time_t *)&start_date);
	int year;
	int month;
	int day;
	off_t ret;
	char found = 0;
	
	for (year = tm_date->tm_year; year >= 0; year--) {
		if (found) break;
		for (month = tm_date->tm_mon; month >= 0; month--) {
			if (found) break;
			for (day = 0; day >= tm_date->tm_mday; day--) {
				ret = ctx->map_offset->off[year][month][day];
				if (ret) found = 1;
				if (found) break;
			}
		}
	}
	
	DBG("map_search:: start_date = %zu year = %d, month=%d,day=%d, off=%zu\n",
			start_date,
			tm_date->tm_year, tm_date->tm_mon, tm_date->tm_mday, ret);
	if (ret) {
		DBG("map_found:: year = %d, month=%d,day=%d, off=%zu\n",
				tm_date->tm_year, tm_date->tm_mon, tm_date->tm_mday, ret);
	}
	return ret;
}


int init_read(struct s_data_cursor *ctx, const char *filename) {
	int ret = -1;
	
	if (ctx) {
		ctx->fd = open(filename, O_RDONLY);
		ret = ctx->fd;
		if (ret >= 0) {
			struct stat stat_f;
			if (!stat(filename, &stat_f)) {
				ctx->mmap_size = (size_t)stat_f.st_size;
				ctx->mmap_file = mmap(NULL, ctx->mmap_size, PROT_READ, MAP_SHARED, ctx->fd, 0);
				if (!ctx->mmap_file) {
					ERROR("mmap on '%s': %s \n", filename, strerror(errno));
					ret = -1;
				} else {
					ctx->map_offset = (struct s_data_head *) ctx->mmap_file;
					ctx->data = ctx->mmap_file + sizeof(struct s_data_head);
					ctx->data_len = ctx->mmap_size - sizeof(struct s_data_head);
					DBG("data ptr is at %p len %zu nb = %zu\n", ctx->data, ctx->data_len,
							ctx->data_len / sizeof (struct s_data));
					dump_map_offset(ctx->map_offset);
					if (ctx->data_len % sizeof(struct s_data)) {
						ERROR("Bad or corrupted file format");
						ret = -1;
					}
				}
			}
		}
	}

	return ret;
}

ssize_t read_data(struct s_data_cursor *ctx) {
	ssize_t nb_bytes = -1;
	DBG("%s(fd=%d, %zu)\n", __func__, ctx->fd, sizeof(ctx->data));
	
	nb_bytes = (ssize_t)sizeof(struct s_data);
	if (!ctx->cur) {
		if (!ctx->cri.desc)
			ctx->cur = (struct s_data *)ctx->data;
		else
			ctx->cur = (struct s_data *) ctx->data + ctx->data_len - sizeof (struct s_data);
	} else {
		if (!ctx->cri.desc) {
			if ((void *)ctx->cur + sizeof(struct s_data) < ctx->data + ctx->data_len) {
				ctx->cur++;
			} else {
				ctx->eof = 1;
				nb_bytes = 0;
			}
		} else {
			if ((void *)ctx->cur - sizeof(struct s_data) >= ctx->data) {
				ctx->cur--;
			} else {
				ctx->eof = 1;
				nb_bytes = 0;
			}
		}
	}
	
	return nb_bytes;
}


void close_data(struct s_data_cursor *ctx) {
	close(ctx->fd);
	ctx->fd = -1;
}
