#include <stdio.h>
#include <string.h>
#include "vtable.h"
#include <errno.h>
typedef struct example_cursor example_cursor;

/* DDL defining the structure of the vtable */
static const char* ddl = "create table vtable ("
	"sensor_date  date,"
	"sensor_type  smallint, "
	"sensor_idx smallint, "
	"val_comp   double,"
	"val_raw   unsigned big int,"
	"trigger   smallint"
	")";



static int vt_destructor(sqlite3_vtab *pVtab)
{
	DBG("%s()\n", __func__);
	// no need to free vtable because it is static
	return 0;
}

static int vt_create( sqlite3 *db,
                      void *p_aux,
                      int argc, const char *const*argv,
                      sqlite3_vtab **pp_vt,
                      char **pzErr )
{
	int rc = SQLITE_OK;
	struct s_data_vtab *vtab = (struct s_data_vtab *) p_aux;
	DBG("%s(module=%s)\n", __func__, argv[0]);

	rc = sqlite3_declare_vtab(db, ddl);
	if(rc != SQLITE_OK)
	{
		ERROR("sqlite3_declare_vtab error\n");
		rc = SQLITE_ERROR;

	} else
		*pp_vt = (sqlite3_vtab *)vtab;
	return rc;
}

static int vt_connect( sqlite3 *db, void *p_aux,
                       int argc, const char *const*argv,
                       sqlite3_vtab **pp_vt, char **pzErr )
{
	DBG("%s()\n", __func__);
	return vt_create(db, p_aux, argc, argv, pp_vt, pzErr);
}

static int vt_disconnect(sqlite3_vtab *pVtab)
{
	DBG("%s()\n", __func__);
	return vt_destructor(pVtab);
}

static int vt_destroy(sqlite3_vtab *pVtab)
{
	DBG("%s()\n", __func__);
	int rc = SQLITE_OK;

	if(rc == SQLITE_OK)
	{
		rc = vt_destructor(pVtab);
	}

	return rc;
}

static int vt_open(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **pp_cursor)
{
	struct s_data_vtab *vtab = (struct s_data_vtab *)pVTab;
	struct s_data_cursor *cursor = &vtab->cursor;

	*pp_cursor = (sqlite3_vtab_cursor *) &vtab->cursor;

	int ret = SQLITE_OK;
	DBG("%s(%s)\n", __func__, vtab->filename);


	if (init_read(cursor, vtab->filename) < 0) {
		ERROR("FILE NOT FOUND\n");
		ret = SQLITE_ERROR;
		goto end;
	}
end:
	return ret;
}

static int vt_close(sqlite3_vtab_cursor *cur)
{
	struct s_data_cursor *p_cur = (struct s_data_cursor*)cur;
	DBG("%s()\n", __func__);
	if (p_cur && p_cur->eof)
		close_data(p_cur);

//	if (cur) sqlite3_free(cur);


	return SQLITE_OK;
}

static int vt_eof(sqlite3_vtab_cursor *vtab)
{
	struct s_data_cursor *vcurs = (struct s_data_cursor *) vtab;
	DBG("%s(cur=%d)\n", __func__, vcurs->eof);

	return vcurs->eof;
}


static int vt_column(sqlite3_vtab_cursor *vtab, sqlite3_context *ctx, int i)
{
	/* Just return the ordinal of the column requested. */
	DBG("%s(%d)\n", __func__, i);
	struct s_data_cursor *vcurs = (struct s_data_cursor *) vtab;
	struct s_data *data = vcurs->cur;
	
	switch (i) {
	case 	DATA_DATE:
	sqlite3_result_int64(ctx, data->date);
		break;
	case DATA_TYPE:
	sqlite3_result_int64(ctx, data->type);
		break;		
	case DATA_INDEX:
	sqlite3_result_int(ctx, data->index);
		break;		
	case DATA_TRIGGER:
	sqlite3_result_int(ctx, data->trigger);
		break;
	case DATA_VAL_COMP:
	sqlite3_result_double(ctx, data->val_comp);
		break;
	case DATA_VAL_RAW:
	sqlite3_result_int64(ctx, data->val_raw);
		break;

	}
	return SQLITE_OK;
}

static int vt_rowid(sqlite3_vtab_cursor *cur, sqlite_int64 *p_rowid)
{
	struct s_data_cursor *p_cur = (struct s_data_cursor *)cur;
	DBG("%s()\n", __func__);

	/* Just use the current row count as the rowid. */
	*p_rowid = p_cur->count;

	return SQLITE_OK;
}

static int vt_next(sqlite3_vtab_cursor *cur)
{
	struct s_data_cursor *p_cur = (struct s_data_cursor*)cur;
	DBG("%s(fd=%d)\n", __func__, p_cur->fd);
	ssize_t nb_bytes = read_data(p_cur);
	int ret= SQLITE_OK;

	if (!nb_bytes) {
		p_cur->eof = 1;
	} else if (nb_bytes != sizeof(struct s_data)) {
		ERROR("Can't read data : %s\n", strerror(errno));
		ret = SQLITE_ERROR;
	} else {
		p_cur->count++;
	}
	dump_data(p_cur->cur);
	return ret;
}

/* Pretty involved. We don't implement in this example. */
static int vt_best_index(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo)
{
	DBG("%s()\n", __func__);
	struct s_data_vtab *vtab = (struct s_data_vtab *) tab;
	memset(&vtab->cursor, 0, sizeof vtab->cursor);
	int i;
	int nb_date = 0;
	
	for (i = 0; i < pIdxInfo->nConstraint; i++) {
		if (!i) 	DBG("Constraint:\n");
		if (!pIdxInfo->aConstraint[i].usable) continue;

		/* printf("\tcol %d op=%d \n", */
		/* 			 pIdxInfo->aConstraint[i].iColumn, */
		/* 			 pIdxInfo->aConstraint[i].op */
			/* ); */
		if (pIdxInfo->aConstraint[i].iColumn == DATA_DATE) {
			switch (vtab->cursor.cri.constraint[nb_date].op) {
			case SQLITE_INDEX_CONSTRAINT_EQ:
			case SQLITE_INDEX_CONSTRAINT_GT:
			case SQLITE_INDEX_CONSTRAINT_GE:
				vtab->cursor.cri.constraint[nb_date].col = DATA_DATE;
				vtab->cursor.cri.constraint[nb_date].op = pIdxInfo->aConstraint[i].op;
				pIdxInfo->aConstraintUsage[i].argvIndex=++nb_date;
				break;
			}
		}
	}
	for (i = 0; i < pIdxInfo->nOrderBy; i++) {
		if (!i) 	DBG("Order:\n");
		if (!pIdxInfo->aConstraint[i].usable) continue;

		DBG("\tcol %d %d\n",
					 pIdxInfo->aOrderBy[i].iColumn,
					 pIdxInfo->aOrderBy[i].desc
			);
		if ( pIdxInfo->aOrderBy[i].iColumn == DATA_DATE) {
			pIdxInfo->orderByConsumed = 1;
			vtab->cursor.cri.desc = (char)pIdxInfo->aOrderBy[i].desc;
			DBG("Desc is set to %d\n", vtab->cursor.cri.desc);
		}
	}
	if (nb_date) vtab->cursor.cri.nb_constraint = nb_date;
	return SQLITE_OK;
}

#include <stdlib.h>
static int vt_filter( sqlite3_vtab_cursor *p_vtc, 
                      int idxNum, const char *idxStr,
                      int argc, sqlite3_value **argv )
{
	struct s_data_cursor *vcurs = (struct s_data_cursor *) p_vtc;
	/* int rc; */
	int i;
	DBG("%s(idxNum=%d, idxStr=%s, argc=%d)\n", __func__, idxNum, idxStr, argc);
	if (argc > 0) {
		for ( i = 0; i < argc; i++) {
			DBG("- %d -> %lld\n", i, sqlite3_value_int64(argv[i]));
			vcurs->cri.constraint[i].val = sqlite3_value_int64(argv[i]);
		}
		off_t off = get_filter_offset(vcurs, vcurs->cri.constraint[0].val);
		if (off > 0) {
			vcurs->cur = vcurs->mmap_file + off - sizeof(struct s_data);
			vcurs->count = 0;
			vcurs->eof = 0;
		}
		else
			vcurs->eof = 1;
	} else {
		/* Zero rows returned thus far. */
		vcurs->count = 0;
		/* Have not reached end of set. */
		vcurs->eof = 0;
	}
	/* Move cursor to first row. */
	return vt_next(p_vtc);
}

sqlite3_module vtable_module = 
{
	1,              /* iVersion */
	vt_create,      /* xCreate       - create a vtable */
	vt_connect,     /* xConnect      - associate a vtable with a connection */
	vt_best_index,  /* xBestIndex    - best index */
	vt_disconnect,  /* xDisconnect   - disassociate a vtable with a connection */
	vt_destroy,     /* xDestroy      - destroy a vtable */
	vt_open,        /* xOpen         - open a cursor */
	vt_close,       /* xClose        - close a cursor */
	vt_filter,      /* xFilter       - configure scan constraints */
	vt_next,        /* xNext         - advance a cursor */
	vt_eof,         /* xEof          - inidicate end of result set*/
	vt_column,      /* xColumn       - read data */
	vt_rowid,       /* xRowid        - read data */
	NULL,           /* xUpdate       - write data */
	NULL,           /* xBegin        - begin transaction */
	NULL,           /* xSync         - sync transaction */
	NULL,           /* xCommit       - commit transaction */
	NULL,           /* xRollback     - rollback transaction */
	NULL,           /* xFindFunction - function overloading */
};

int register_vtable(sqlite3 *db, void *cookie)
{
	DBG("%s()\n", __func__);
	return sqlite3_create_module(db, "vtable", &vtable_module, cookie);
}
