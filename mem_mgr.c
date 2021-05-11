//
//  memmgr.c
//  memmgr
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256

char main_mem[65536];
char main_mem_fifo[32768]; // 128 physical frames
int page_queue[128];
int qhead = 0, qtail = 0;
int tlb[16][2];
int current_tlb_entry = 0;
int page_table[256];
int current_frame = 0;
FILE* fstore;

// data for statistics
int pfc[5], pfc2[5]; // page fault count
int tlbh[5], tlbh2[5]; // tlb hit count
int count[5], count2[5]; // access count

#define PAGES 256
#define FRAMES_PART1 256
#define FRAMES_PART2 128

//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

int tlb_contains(unsigned x) {  // TODO:

  int tlbSize = 16; //size of tlb is 16
  int i = 0;

  while (i<tlbSize){
    if (tlb[i][0] == x)
    i++;
    return i; //return the location of x in tlb
  }

  return -1; //if not found in the while loop return -1

}

void update_tlb(unsigned page) {  // TODO:

  tlb[current_tlb_entry][0] = page;
  tlb[current_tlb_entry][1] = page_table[page];
  current_tlb_entry = ++current_tlb_entry % 16;

  
}

unsigned getframe(FILE* fstore, unsigned logic_add, unsigned page,
         int *page_fault_count, int *tlb_hit_count) {              // TODO
  
  // tlb hit
  int tlbIndex = tlb_contains(page);

  if (tlbIndex != 1){
    (*tlb_hit_count)++;
    return tlb[tlbIndex][1];
  }


  // tlb miss
  // if page table hit
  if (page_table[page] != -1){
    update_tlb(page);
    return page_table[page];
  }

  
  // page table miss -> page fault
  // find page location in backing_store
  fseek(fstore,logic_add,0);

  
  
  // bring data into memory, update tlb and page table
  ++(*page_fault_count);
  page_table[page] = current_frame;
  current_frame = ++current_frame % 256;
  fread(&main_mem[page_table[page]*256], sizeof(char), 256, fstore);
  update_tlb(page);
  return page_table[page];
  
}
  
  
  // queue full

if (page_queue[qtail]==-1 && qhead==qtail){
  page_queue[qtail]=page;
  int qtailSave2= qtail;
  qtail = ++qtail % 128;
  qhead = qtail;

  return qtailSave2;

 
}
  

  return -1;   

}

unsigned getframe_fifo(FILE* fstore, unsigned logic_add, unsigned page,
         int *page_fault_count, int *tlb_hit_count) {


  int tlbLocation = tlb_contains(page);
  if (tlbLocation > -1) {

    // tlb hit
    if (page_queue[tlb[tlbLocation][1]] == page) {
      ++(*tlb_hit_count);
      return tlb[tlbLocation][1];
    }

    // tlb miss, page table hit
    if (page_queue[page_table[page]] == page) {
      update_tlb(page);
      return page_table[page];
    }
  }

  
  // page table miss -> page fault
  // find location in backing_store
  int location = (logic_add / 256) * 256;
  fseek(fstore, location, 0);
  

   // bring data into memory, update tlb and page table
   ++(*page_fault_count);
  current_frame = get_available_frame(page);
  page_table[page] = current_frame;
  update_tlb(page);
  fread(&main_mem_fifo[page_table[page]*256], sizeof(char), 256, fstore);


  return page_table[page];



}

void open_files(FILE** fadd, FILE** fcorr, FILE** fstore) {
  *fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (*fadd ==  NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  *fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (*fcorr ==  NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  *fstore = fopen("BACKING_STORE.bin", "rb");
  if (*fstore ==  NULL) { fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");  exit(FILE_ERROR);  }
}


void close_files(FILE* fadd, FILE* fcorr, FILE* fstore) {
  fclose(fcorr);
  fclose(fadd);
  fclose(fstore);
}


void simulate_pages_frames_equal(void) {
  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt


  FILE *fadd, *fcorr, *fstore;
  open_files(&fadd, &fcorr, &fstore);
  
  // Initialize page table, tlb
  memset(page_table, -1, sizeof(page_table));
  for (int i = 0; i < 16;  ++i) { tlb[i][0] = -1; }
  
  int access_count = 0, page_fault_count = 0, tlb_hit_count = 0;
  current_frame = 0;
  current_tlb_entry = 0;
  
  printf("\n Starting nPages == nFrames memory simulation...\n");

  while (fscanf(fadd, "%d", &logic_add) != EOF) {
    ++access_count;

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    // fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(  logic_add);
    offset = getoffset(logic_add);
    frame = getframe(fstore, logic_add, page, &page_fault_count, &tlb_hit_count);

    physical_add = frame * FRAME_SIZE + offset;
    int val = (int)(main_mem[physical_add]);

    // update tlb hit count and page fault count every 200 accesses
    if (access_count > 0 && access_count % 200 == 0){
      tlbh[(access_count / 200) - 1] = tlb_hit_count;
      pfc[(access_count / 200) - 1] = page_fault_count;
      count[(access_count / 200) - 1] = access_count;
    }
    
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -> value: %4d  ok\n", logic_add, page, offset, physical_add, val);
    if (access_count % 5 ==  0) { printf("\n"); }

    assert(physical_add ==  phys_add);
    assert(value ==  val);
  }
  fclose(fcorr);
  fclose(fadd);
  fclose(fstore);
  
  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("ALL read memory value assertions PASSED!\n");

  printf("\n\t\t... nPages == nFrames memory simulation done.\n");
}


void simulate_pages_frames_not_equal(void) {
  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt

  printf("\n Starting nPages != nFrames memory simulation...\n");

  // Initialize page table, tlb, page queue
  memset(page_table, -1, sizeof(page_table));
  memset(page_queue, -1, sizeof(page_queue));
  for (int i = 0; i < 16;  ++i) { tlb[i][0] = -1; }
  
  int access_count = 0, page_fault_count = 0, tlb_hit_count = 0;
  qhead = 0; qtail = 0;

  FILE *fadd, *fcorr, *fstore;
  open_files(&fadd, &fcorr, &fstore);

  while (fscanf(fadd, "%d", &logic_add) != EOF) {
    ++access_count;

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    // fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(  logic_add);
    offset = getoffset(logic_add);
    frame = getframe_fifo(fstore, logic_add, page, &page_fault_count, &tlb_hit_count);

    physical_add = frame * FRAME_SIZE + offset;
    int val = (int)(main_mem_fifo[physical_add]);

    // update tlb hit count and page fault count every 200 accesses
    if (access_count > 0 && access_count%200 == 0){
      tlbh2[(access_count / 200) - 1] = tlb_hit_count;
      pfc2[(access_count / 200) - 1] = page_fault_count;
      count2[(access_count / 200) - 1] = access_count;
    }
    
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -> value: %4d  ok\n", logic_add, page, offset, physical_add, val);
    if (access_count % 5 ==  0) { printf("\n"); }

    assert(value ==  val);
  }
  close_files(fadd, fcorr, fstore);

  printf("ALL read memory value assertions PASSED!\n");
  printf("\n\t\t... nPages != nFrames memory simulation done.\n");
}


int main(int argc, const char* argv[]) {
  // initialize statistics data
  for (int i = 0; i < 5; ++i){
    pfc[i] = pfc2[i] = tlbh[i]  = tlbh2[i] = count[i] = count2[i] = 0;
  }

  simulate_pages_frames_equal(); // 256 physical frames
  simulate_pages_frames_not_equal(); // 128 physical frames

  // Statistics
  printf("\n\nnPages == nFrames Statistics (256 frames):\n");
  printf("Access count   Tlb hit count   Page fault count   Tlb hit rate   Page fault rate\n");
  for (int i = 0; i < 5; ++i) {
    printf("%9d %12d %18d %18.4f %14.4f\n",
           count[i], tlbh[i], pfc[i],
           1.0f * tlbh[i] / count[i], 1.0f * pfc[i] / count[i]);
  }

  printf("\nnPages != nFrames Statistics (128 frames):\n");
  printf("Access count   Tlb hit count   Page fault count   Tlb hit rate   Page fault rate\n");
  for (int i = 0; i < 5; ++i) {
    printf("%9d %12d %18d %18.4f %14.4f\n",
           count2[i], tlbh2[i], pfc2[i],
           1.0f * tlbh2[i] / count2[i], 1.0f * pfc2[i] / count2[i]);
  }
  printf("\n\t\t...memory management simulation completed!\n");

  return 0;
}


