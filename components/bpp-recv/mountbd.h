#ifndef MOUNTBD_H
#define MOUNTBD_H

int bd_mount(BlockdevIf *iface, BlockdevifHandle *h, const char *path, size_t max_files);


#endif