#include "gfx_userfs.h"

bool_t userfs_del(const char * fname){
    if(!unlink(fname)){
        return 1;
    }
    return 0;
}

bool_t userfs_exists(const char * fname){
    struct stat statbuf;
    if(stat(fname, &statbuf) == 0){
        return 1;
    }
    return 0;
}

long int userfs_filesize(const char * fname){
    struct stat statbuf;
    if(stat(fname, &statbuf) == 0){
        return statbuf.st_size;
    }
    return 0;
}

bool_t userfs_ren(const char * oldname, const char * newname){
    return rename(oldname, newname) != -1;
}

bool_t userfs_open(GFILE *f, const char * fname){
    switch(f->flags){
        case GFILEFLG_READ:
            f->obj = fopen(fname, "r");
            break;
        case GFILEFLG_WRITE:
            f->obj = fopen(fname, "w");
            break;
        case GFILEFLG_READ|GFILEFLG_WRITE:
            f->obj = fopen(fname, "rw");
            break;
        case GFILEFLG_WRITE|GFILEFLG_APPEND:
            f->obj = fopen(fname, "wa");
            break;
        default:
            f->obj = fopen(fname, "rw");
            break;
            /*return 0;*/
    }
    if(f->obj != NULL){
        f->flags |= GFILEFLG_OPEN;
        return 1;
    }
    return 0;
}

void userfs_close(GFILE *f){
    if(f->obj){
        fclose(f->obj);
        f->flags &= ~GFILEFLG_OPEN;
    }
}

int userfs_read(GFILE *f, void * buf, int size){
    if(f->obj){
        return fread(buf, 1, size, f->obj);
    }
    return 0;
}

int userfs_write(GFILE *f, const void *buf, int size){
    if(f->obj){
        return fwrite(buf, 1, size, f->obj);
    }
    return 0;
}

bool_t userfs_setpos(GFILE *f, long int pos){
    if(f->obj){
        return fseek(f->obj, pos, SEEK_SET) != -1;
    }
    return 0;
}

long int userfs_getsize(GFILE * f){
    if(f->obj){
        long int oldpos = ftell(f->obj);
        fseek(f->obj, 0, SEEK_END);
        long int size = ftell(f->obj);
        fseek(f->obj, oldpos, SEEK_SET);
        return size;
    }
    return 0;
}

bool_t userfs_eof(GFILE *f){
    if(f->obj){
        return feof(f->obj);
    }
    return 0;
}

bool_t userfs_mount(const char* drive){
    return 1;
}

bool_t userfs_unmount(const char* drive){
    return 1;
}

bool_t userfs_sync(GFILE * f){
    return 1;
}
