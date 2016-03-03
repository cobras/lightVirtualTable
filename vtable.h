#ifndef __VTABLE_H__
#define __VTABLE_H__
#include <stdint.h>
#include <sqlite3.h>
#include <stdlib.h>
enum {
	DATA_DATE=0,
	DATA_TYPE,
	DATA_INDEX,
	DATA_VAL_COMP,
	DATA_VAL_RAW,
	DATA_TRIGGER,
	DATA_NB_COLS
};

struct s_data {
	uint64_t date;
	char type;
	char index;
	float val_comp;
	uint64_t val_raw;
	unsigned short trigger;
} __attribute__((packed));

enum {
	NB_YEAR=255,
	NB_MONTH=12,
	NB_DAY=31
};
struct s_data_head {
	off_t off[NB_YEAR][NB_MONTH][NB_DAY];
}__attribute__((packed));

struct s_data_cursor
{
	sqlite3_vtab *pVtab;      /* Virtual table of this cursor */
	/* Virtual table implementations will typically add additional fields */
	int count;
	int eof;
	int fd;
	size_t sensor_size;
	struct {
		char  desc;
		int nb_constraint;
		struct {
			int col;
			unsigned char op;
			uint64_t val;
		} constraint[10];
	} cri;
	struct s_data_head *map_offset;
#ifdef USE_MMAP
	void   *mmap_file;
	size_t mmap_size;
	size_t data_len;
	void   *data;
	struct s_data *cur;
#else
	struct s_data data;
#endif
};

struct s_data_vtab {
  /* sqllite3 internal need values*/
  const sqlite3_module *pModule;  /* The module for this virtual table */
  int nRef;                       /* Number of open cursors */
  char *zErrMsg;                  /* Error message from sqlite3_mprintf() */
  char *filename;
  /*end*/
  struct s_data_cursor cursor;
};


#define ERROR(...) fprintf(stderr, __VA_ARGS__)
//#define DBG(...) fprintf(stderr, __VA_ARGS__)
#define DBG(...)

int init_read(struct s_data_cursor *p_cur, const char *filename);
ssize_t read_data(struct s_data_cursor *ctx);
void close_data(struct s_data_cursor *ctx);
int register_vtable(sqlite3 *db, void *cookie);
off_t get_filter_offset(struct s_data_cursor *ctx, uint64_t start_date);
void dump_data(struct s_data *data);
#endif // __VTABLE_H__

