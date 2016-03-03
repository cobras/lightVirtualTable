#include <stdio.h>
#include <stdint.h>
#include <sqlite3.h>

struct s_sensor {
	int64_t date;
	uint64_t val_row;
	float    val_comp;
	unsigned short trigger;
	u_char   index;
	u_char   type;
}__attribute__((packed));

static int vt_create( sqlite3 *db, void *pAux, int argc, const char *const*argv,
											sqlite3_vtab **pp_vt, char **pzErr )
{
	int rc = SQLITE_OK;
	vtab* p_vt;
	/* Allocate the sqlite3_vtab/vtab structure itself */
	p_vt = (vtab*)sqlite3_malloc(sizeof(*p_vt));

	if(p_vt == NULL) {
		return SQLITE_NOMEM;
	}
	p_vt->db = db;
	apr_pool_create(&p_vt->pool, NULL);
	/* Declare the vtable's structure */
	rc = sqlite3_declare_vtab(db, ddl);
	/* Success. Set *pp_vt and return */
	*pp_vt = &p_vt->base;
	return SQLITE_OK;
}


static sqlite3_module example_module =
{
	0,              /* iVersion */
	vt_create,      /* xCreate       - create a vtable */
	vt_connect,     /* xConnect      - associate a vtable with a connection */
	vt_best_index,  /* xBestIndex    - best index */
	vt_disconnect,  /* xDisconnect   - disassociate a vtable with a connection */
	vt_destroy,     /* xDestroy      - destroy a vtable */
	vt_open,        /* xOpen         - open a cursor */
	vt_close,       /* xClose        - close a cursor */
	vt_filter,      /* xFilter       - configure scan constraints */
	vt_next,        /* xNext         - advance a cursor */
	vt_eof,         /* xEof          - indicate end of result set*/
	vt_column,      /* xColumn       - read data */
	vt_rowid,       /* xRowid        - read data */
	NULL,           /* xUpdate       - write data */
	NULL,           /* xBegin        - begin transaction */
	NULL,           /* xSync         - sync transaction */
	NULL,           /* xCommit       - commit transaction */
	NULL,           /* xRollback     - rollback transaction */
	NULL,           /* xFindFunction - function overloading */
};
/* vtab: represents a virtual table. */
struct vtab
{
	sqlite3_vtab base;
	sqlite3 *db;
};
/* vtab: represents a singe cursor with which it iterate over the virtual table. */
struct vtab_cursor
{
	sqlite3_vtab_cursor base;
};

int main(int argc, char **argv) {
	
	return 0;
	
}
