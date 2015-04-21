/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int page_rep_choice;

void page_fault_handler( struct page_table *pt, int page )
{
    printf("page_rep_choice=%i\n",page_rep_choice);
    int frame,nframes,npages,retframe,bits;
    // get max number of pages and frames
    nframes = page_table_get_nframes(pt);
    npages = page_table_get_npages(pt);

    // check if frame number should be page number
    if (nframes>=npages){
	    printf("page fault on page #%d\n",page);
        frame = page;
    }
    // FIFO
    else if (nframes<npages){
	    printf("page fault on page #%d\n",page);
	    if (page_rep_choice==0){
	        // RAND
	        frame = page;
        }
        else if (page_rep_choice==1){
	        // FIFO
	        frame = page % nframes;
        }
        else if (page_rep_choice==2){
            // CUSTOM
            frame = page;
        }
    }

    // check permissions
    //  - no permissions -> read
    //  - read permissions -> write
    //  - write permissions -> all
    if (bits==0){
        page_table_set_entry(pt,page,frame,PROT_READ);
    }
    else if(bits==1){
        page_table_set_entry(pt,page,frame,PROT_WRITE);
    }
    else{
        page_table_set_entry(pt,page,frame,PROT_READ|PROT_WRITE|PROT_EXEC);
    }

    page_table_get_entry(pt,page,&retframe,&bits);
    printf("frame=%i\tbits=%i\n",retframe,bits);
    
//	exit(1);
}

int main( int argc, char *argv[] )
{
    // handle input
    if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	char *page_replacement = argv[3];
    const char *program = argv[4];
    if (!strcmp(page_replacement,"rand")){
        page_rep_choice = 0;
    }
    else if(!strcmp(page_replacement,"fifo")){
        page_rep_choice = 1;
    }
    else if(!strcmp(page_replacement,"custom")){
        page_rep_choice = 2;
    }
    else{
        printf("Choose rand, fifo, or custum\n");
        return 1;
    }

    // create new virtual disk with pointer to the disk object
	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

    // create page table with pointer to page table
    // includes virtual memory and physical memory
	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

    // return pointer to start of virtual memory
	char *virtmem = page_table_get_virtmem(pt);
    // return pointer to start of physical memory
	char *physmem = page_table_get_physmem(pt);

	// calls page_fault_handler
	// call one of the algorithms
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}

    page_table_print(pt);
    // clean up
	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
