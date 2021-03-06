/**
 * virtmem.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 1024
#define PAGE_MASK /* TODO */ 0b1111111111

#define PAGE_SIZE 1024
#define FRAMES 256
#define OFFSET_BITS 10
#define OFFSET_MASK /* TODO */ 0b1111111111

#define MEMORY_SIZE FRAMES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
  int logical;
  int physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;


// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

int agingtable[PAGES];  // to calculate LRU algortihm 

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(int logical_page) {
    /* TODO */
    int result = -1;
    for(int i=0; i<TLB_SIZE; i++) {
      if(logical_page == tlb[i].logical) {
        result = tlb[i].physical;
        break;
      }
    }
    return result;
    
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(int logical, int physical) {
    /* TODO */
    tlb[tlbindex].logical = logical;
    tlb[tlbindex].physical = physical;
    if(tlbindex == 15) {
      tlbindex = 0;
    } else {
      tlbindex++;
    }
}

int main(int argc, const char *argv[])
{
  if (argc != 5) {
    fprintf(stderr, "Usage ./virtmem backingstore input -p mode\n");
    exit(1);
  }
  

  int select; // to select modes


  select = atoi(argv[4]);

  if(select == 0) {
    const char *backing_filename = argv[1]; 
    int backing_fd = open(backing_filename, O_RDONLY);            
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

    FILE *backing_f = fopen(argv[1], "rb");   // used f open to get the contents of BACKING_STORE.bin did not used mmap 


    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");
  
    // Fill page table entries with -1 for initially empty table.
    int i;
    for (i = 0; i < PAGES; i++) {
      pagetable[i] = -1;
    }
  
    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];
  
    // Data we need to keep track of to compute stats at end.
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;
  
    // Number of the next unallocated physical page in main memory
    int free_page = 0;

    int counter = 0;

  

  
    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
      total_addresses++;
      int logical_address = atoi(buffer);

      /* TODO 
      / Calculate the page offset and logical page number from logical_address */
      int offset = logical_address & OFFSET_MASK;
      int logical_page = logical_address >> OFFSET_BITS;
      logical_page = logical_page & PAGE_MASK;
      ///////
    
      int physical_page = search_tlb(logical_page);
      // TLB hit
      if (physical_page != -1) {
        tlb_hits++;
        // TLB miss
      } else {
        physical_page = pagetable[logical_page];
      
        // Page fault
        if (physical_page == -1) {
            /* TODO */
            page_faults++;


          
            physical_page = free_page;  // allocating the next frame
            free_page++;
        
            if(free_page == FRAMES) {   // getting index of the memory space to fifo
              free_page = 0;
            }


            for(int i=0; i<PAGES; i++) {
              if(pagetable[i] == physical_page) {  // deleting the old page
                pagetable[i] = -1;
              }
            }


            pagetable[logical_page] = physical_page;  // updating page table with the new frame
          

            fseek(backing_f, logical_page * PAGE_SIZE, SEEK_SET);     //  getting the contens from BACKING_STORE.bin
            fread(main_memory + physical_page * PAGE_SIZE, sizeof(signed char), PAGE_SIZE, backing_f);  // writing them on the new frame of the main memory
            
        
          
        }

        add_to_tlb(logical_page, physical_page);
      }
    
      int physical_address = (physical_page << OFFSET_BITS) | offset;
      signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  
  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));



  } else if(select == 1) {


  const char *backing_filename = argv[1]; 
  int backing_fd = open(backing_filename, O_RDONLY);
  backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

  FILE *backing_f = fopen(argv[1], "rb");   // used f open to get the contents of BACKING_STORE.bin did not used mmap 


  const char *input_filename = argv[2];
  FILE *input_fp = fopen(input_filename, "r");
  
  // Fill page table entries with -1 for initially empty table.
  int i;
  for (i = 0; i < PAGES; i++) {
    pagetable[i] = -1;
  }

  for (int j = 0; j < PAGES; j++) {   //filling aging entries with -1 for initially empty table.
    agingtable[j] = -1;
  }
  
  // Character buffer for reading lines of input file.
  char buffer[BUFFER_SIZE];
  
  // Data we need to keep track of to compute stats at end.
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;
  
  // Number of the next unallocated physical page in main memory
  int free_page = 0;

  int counter = 0;   // counting for memory table for LRU

  

  
  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
    total_addresses++;
    int logical_address = atoi(buffer);

    /* TODO 
    / Calculate the page offset and logical page number from logical_address */
    int offset = logical_address & OFFSET_MASK;
    int logical_page = logical_address >> OFFSET_BITS;
    logical_page = logical_page & PAGE_MASK;
    ///////
    
    int physical_page = search_tlb(logical_page);
    // TLB hit
    if (physical_page != -1) {
      tlb_hits++;
      agingtable[logical_page] = 0;  //updaeting new entries of agingtable


      // TLB miss
    } else {
      physical_page = pagetable[logical_page];

      agingtable[logical_page] = 0; //updaeting new entries of agingtable
      
      // Page fault
      if (physical_page == -1) {
          /* TODO */
          page_faults++;
          
          



          if(counter < FRAMES) {
            counter++;
            physical_page = free_page;  // allocating the next frame
            free_page++;
        


            for(int i=0; i<PAGES; i++) {
              if(pagetable[i] == physical_page) {  // updating page table and agingtable -1 entries
                pagetable[i] = -1;
                agingtable[i] = -1;
              }
            }


          pagetable[logical_page] = physical_page;  // updating page table with the new frame
          agingtable[logical_page] = 0;
          

          fseek(backing_f, logical_page * PAGE_SIZE, SEEK_SET);     //  getting the contens from BACKING_STORE.bin
          fread(main_memory + physical_page * PAGE_SIZE, sizeof(signed char), PAGE_SIZE, backing_f);  // writing them on the new frame of the main memory
          printf("free page %d\n", free_page);
          


          } else {
            counter++;

            int oldest;
            int max = 0;

            for(int i=0; i<PAGES; i++) {
              if(agingtable[i] > max) {   // getting largest age number entrie from the aging table
                max = agingtable[i];
                oldest = i;
              }
            }
            physical_page = pagetable[oldest];
            
            

            for(int i=0; i<PAGES; i++) {
              if(pagetable[i] == physical_page) {     // updating page table and agingtable -1 entries
                pagetable[i] = -1;
                agingtable[i] = -1;
              }
            }

            pagetable[logical_page] = physical_page;  // updating page table with the new frame
            agingtable[logical_page] = 0;

            fseek(backing_f, logical_page * PAGE_SIZE, SEEK_SET);     //  getting the contens from BACKING_STORE.bin
            fread(main_memory + physical_page * PAGE_SIZE, sizeof(signed char), PAGE_SIZE, backing_f);  // writing them on the new frame of the main memory



          }
          
        
          
      }
      

      add_to_tlb(logical_page, physical_page);
    }

    for(int i=0; i<PAGES; i++) {
              if(agingtable[i] != -1) {   // before going to next adress incresing every agingtable entries by 1 
                agingtable[i]++;
              }
            }
    
    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  
  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));


  } else {
    fprintf(stderr, "Usage ./virtmem backingstore input -p mode\n");
    exit(1);
  }




  
  
  
   
  
  return 0;
}
