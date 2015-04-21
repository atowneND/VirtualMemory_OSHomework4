
/*
Do not modify this file.
Make all of your changes to main.c instead.
*/

#define _GNU_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ucontext.h>

#include "page_table.h"

// page table structure
struct page_table {
	int fd;
	char *virtmem;
	int npages;
	char *physmem;
	int nframes;
	int *page_mapping;
	int *page_bits;
	page_fault_handler_t handler;
};

struct page_table *the_page_table = 0;

static void internal_fault_handler( int signum, siginfo_t *info, void *context )
{
// context switch kind of thing
// save off the context (data pointer)
#ifdef i386 // dependent on processor
	char *addr = (char*)(((struct ucontext *)context)->uc_mcontext.cr2);
#else
	char *addr = info->si_addr;
#endif

	struct page_table *pt = the_page_table;

	if(pt) { // if the page table is nonzero (has been intialized)
	    // get page number
		int page = (addr-pt->virtmem) / PAGE_SIZE;

		if(page>=0 && page<pt->npages) {
		    // if page is in range
			pt->handler(pt,page);
			return;
		}
	}

	// if page is out of range or if the page table doesn't exist, seg fault
	fprintf(stderr,"segmentation fault at address %p\n",addr);
	abort();
}

// create a page table
struct page_table * page_table_create( int npages, int nframes, page_fault_handler_t handler )
{
	int i;
	struct sigaction sa;
	struct page_table *pt;
	char filename[256];

	pt = malloc(sizeof(struct page_table));
	if(!pt) return 0;

    // set the page table to pt
	the_page_table = pt;

	sprintf(filename,"/tmp/pmem.%d.%d",getpid(),getuid());

	pt->fd = open(filename,O_CREAT|O_TRUNC|O_RDWR,0777);
	if(!pt->fd) return 0;

	ftruncate(pt->fd,PAGE_SIZE*npages);

    // basically delete it as soon as all processes are done with it
	unlink(filename);

    // CHECK PHYSICAL VS VIRTUAL MEMORY
    // new map pointer in virtual memory - frames
    // read or write, share this mapping
	pt->physmem = mmap(0,nframes*PAGE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,pt->fd,0);
	pt->nframes = nframes; // input

    // new map pointer in virtual memory - pages
    // pages may not be accessed, not swappable (if too little memory, too bad)
	pt->virtmem = mmap(0,npages*PAGE_SIZE,PROT_NONE,MAP_SHARED|MAP_NORESERVE,pt->fd,0);
	pt->npages = npages; // input

	pt->page_bits = malloc(sizeof(int)*npages);
	pt->page_mapping = malloc(sizeof(int)*npages);

	pt->handler = handler; // input

    // set page bits to zero
	for(i=0;i<pt->npages;i++) pt->page_bits[i] = 0;

    // set signal action - replace defaults with internal fault handler
	sa.sa_sigaction = internal_fault_handler;
	sa.sa_flags = SA_SIGINFO;

    // set all signals
	sigfillset( &sa.sa_mask );
	sigaction( SIGSEGV, &sa, 0 );

	return pt;
}

// delete page table
void page_table_delete( struct page_table *pt )
{
    // undo the mapping for virtual and physical memory
	munmap(pt->virtmem,pt->npages*PAGE_SIZE);
	munmap(pt->physmem,pt->nframes*PAGE_SIZE);
	free(pt->page_bits);
	free(pt->page_mapping);
	close(pt->fd);
	free(pt);
}

// set an entry in page table
void page_table_set_entry( struct page_table *pt, int page, int frame, int bits )
{
    // check page and frame ranges
	if( page<0 || page>=pt->npages ) {
		fprintf(stderr,"page_table_set_entry: illegal page #%d\n",page);
		abort();
	}

	if( frame<0 || frame>=pt->nframes ) {
		fprintf(stderr,"page_table_set_entry: illegal frame #%d\n",frame);
		abort();
	}

    // set page table
	pt->page_mapping[page] = frame;
	pt->page_bits[page] = bits;

    // reorder memory mapping; control allowable memory access
	remap_file_pages(pt->virtmem+page*PAGE_SIZE,PAGE_SIZE,0,frame,0);
	mprotect(pt->virtmem+page*PAGE_SIZE,PAGE_SIZE,bits);
}

// get page table entry
void page_table_get_entry( struct page_table *pt, int page, int *frame, int *bits )
{
    // check range
	if( page<0 || page>=pt->npages ) {
		fprintf(stderr,"page_table_get_entry: illegal page #%d\n",page);
		abort();
	}

    // get frame number and offset
	*frame = pt->page_mapping[page];
	*bits = pt->page_bits[page];
}

// print out page table entry
void page_table_print_entry( struct page_table *pt, int page )
{
    // check range
	if( page<0 || page>=pt->npages ) {
		fprintf(stderr,"page_table_print_entry: illegal page #%d\n",page);
		abort();
	}

	int b = pt->page_bits[page];

    // print page, frame, and permissions
	printf("page %06d: frame %06d bits %c%c%c\n",
		page,
		pt->page_mapping[page],
		b&PROT_READ  ? 'r' : '-',
		b&PROT_WRITE ? 'w' : '-',
		b&PROT_EXEC  ? 'x' : '-'
	);

}

// print the page table
void page_table_print( struct page_table *pt )
{
	int i;
	// print each page table entry
	for(i=0;i<pt->npages;i++) {
		page_table_print_entry(pt,i);
	}
}

// return values of page table structure
int page_table_get_nframes( struct page_table *pt )
{
	return pt->nframes;
}

int page_table_get_npages( struct page_table *pt )
{
	return pt->npages;
}

char * page_table_get_virtmem( struct page_table *pt )
{
	return pt->virtmem;
}

char * page_table_get_physmem( struct page_table *pt )
{
	return pt->physmem;
}
