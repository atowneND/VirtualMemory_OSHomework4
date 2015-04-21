/*
Do not modify this file.
Make all of your changes to main.c instead.
*/

#include "disk.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

extern ssize_t pread (int __fd, void *__buf, size_t __nbytes, __off_t __offset);
extern ssize_t pwrite (int __fd, const void *__buf, size_t __nbytes, __off_t __offset);

// disk structure - disk physical memory information
struct disk {
	int fd;
	int block_size;
	int nblocks;
};

// open the disk (create, if necessary)
struct disk * disk_open( const char *diskname, int nblocks )
{
	struct disk *d;

	d = malloc(sizeof(*d));
	if(!d) return 0;

	d->fd = open(diskname,O_CREAT|O_RDWR,0777);
	if(d->fd<0) {
		free(d);
		return 0;
	}

	d->block_size = BLOCK_SIZE; // 4KB
	d->nblocks = nblocks;

	if(ftruncate(d->fd,d->nblocks*d->block_size)<0) {
		close(d->fd);
		free(d);
		return 0;
	}

	return d;
}

// write to disk
void disk_write( struct disk *d, int block, const char *data )
{
    // check if block is within scope
	if(block<0 || block>=d->nblocks) {
		fprintf(stderr,"disk_write: invalid block #%d\n",block);
		abort();
	}

    // write a whole block at the proper offset block*d->block_size
	int actual = pwrite(d->fd,data,d->block_size,block*d->block_size);
	if(actual!=d->block_size) {
		fprintf(stderr,"disk_write: failed to write block #%d: %s\n",block,strerror(errno));
		abort();
	}
}

// read from disk
void disk_read( struct disk *d, int block, char *data )
{
    // check if block is within scope
	if(block<0 || block>=d->nblocks) {
		fprintf(stderr,"disk_read: invalid block #%d\n",block);
		abort();
	}

    // read a whole block from the proper offset
	int actual = pread(d->fd,data,d->block_size,block*d->block_size);
	if(actual!=d->block_size) {
		fprintf(stderr,"disk_read: failed to read block #%d: %s\n",block,strerror(errno));
		abort();
	}
}

// set number of blocks and return this value
int disk_nblocks( struct disk *d )
{
	return d->nblocks;
}

// close the disk fd
void disk_close( struct disk *d )
{
	close(d->fd);
	free(d);
}
