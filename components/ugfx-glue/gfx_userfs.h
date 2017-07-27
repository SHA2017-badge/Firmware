#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include "gfxconf.h"
#include "gfx.h"
#include <gfile/gfile.h>
#include <gfile/gfile_fs.h>

#ifndef __GFX_USERFS
#define __GFX_USERFS

bool_t userfs_del(const char * fname);
bool_t userfs_exists(const char * fname);
long int userfs_filesize(const char * fname);
bool_t userfs_ren(const char * oldname, const char * newname);
bool_t userfs_open(GFILE *f, const char * fname);
void userfs_close(GFILE *f);
int userfs_read(GFILE *f, void * buf, int size);
int userfs_write(GFILE *f, const void *buf, int size);
bool_t userfs_setpos(GFILE *f, long int pos);
long int userfs_getsize(GFILE * f);
bool_t userfs_eof(GFILE *f);
bool_t userfs_mount(const char* drive);
bool_t userfs_unmount(const char* drive);
bool_t userfs_sync(GFILE * f);

const GFILEVMT FsUSERVMT = {
    .flags = GFSFLG_WRITEABLE|GFSFLG_CASESENSITIVE|GFSFLG_SEEKABLE,
    .prefix = 0,
    .del = &userfs_del,
    .exists = &userfs_exists,
    .filesize = &userfs_filesize,
    .ren = &userfs_ren,
    .open = &userfs_open,
    .close = &userfs_close,
    .read = &userfs_read,
    .write = &userfs_write,
    .setpos = &userfs_setpos,
    .getsize = &userfs_getsize,
    .eof = &userfs_eof,
    .mount = &userfs_mount,
    .unmount = &userfs_unmount,
    .sync = &userfs_sync,
#if GFILE_NEED_FILELISTS
#error "GFILE_NEED_FILELISTS Not implemented"
    .flopen = 0,
    .flread = 0,
    .flclose = 0,
#endif
};

#endif
