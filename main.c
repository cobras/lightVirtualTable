#define _FILE_OFFSET_BITS  64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtable.h"

struct req_ctx {
  int nb_count;
  int dpy;
};
int test_cb(void *data, int argc, char **argv, char **azColName) {
	DBG("%s(argc =%d)\n", __func__, argc);
	struct req_ctx *req_ctx = (struct req_ctx *) data;
	int i;
	if (req_ctx->dpy)	printf("%d:", req_ctx->nb_count+1);
	for (i = 0; i < argc; i++) {
      if (req_ctx->dpy && i > 0) printf(", ");
      if (req_ctx->dpy) printf("%s=%s", azColName[i], argv[i]);
	}
	req_ctx->nb_count++;
	if (req_ctx->dpy) printf("\n");
	return SQLITE_OK;
}

int main(int argc, char **argv) {
    sqlite3 *db = NULL;
    struct s_data_vtab vtab = {
      .filename = argv[1]
    };	
    int rc;

    rc = sqlite3_open("test.db", &db);
    if( rc != SQLITE_OK){
      ERROR("Can't open database: %s\n", sqlite3_errmsg(db));
      goto end;
    }else{
      ERROR("Opened database successfully\n");
    }
    
    // register our module
    rc = register_vtable(db, &vtab);
    if( rc != SQLITE_OK){
      ERROR("Can't create module: %s\n", sqlite3_errmsg(db));
      goto end;
    }else{
      ERROR("create module ok \n");
    }
    char *msg = NULL;
    rc = sqlite3_exec(db, "create virtual table if not exists f using vtable", NULL, NULL, &msg);
    char req[1024] = "select * from f";
    struct req_ctx req_ctx = {
      .nb_count = 0,
      .dpy = argc > 3 && atoi(argv[3]) == 1,
    };
    if (argc > 2 )
      snprintf(req, sizeof req, "select * from f %s", argv[2]);
    
    rc = sqlite3_exec(db, req, test_cb, &req_ctx, &msg);
    printf("I have  read %d items\n", req_ctx.nb_count);
    
    if(rc != SQLITE_OK) { 
      ERROR("ERROR!: %s\n", msg);
      sqlite3_free(msg);
      goto end;
    }
    
	
    sqlite3_close(db);
 end:
    if (db)
      sqlite3_close(db);
    return 0;    
}
