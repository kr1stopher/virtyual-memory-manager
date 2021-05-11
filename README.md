# virtyual-memory-manager

Designing a virtual memory manager that ranslates logical to physical address space of size 2^16 = 65,536 bytes

Specifications:
•2^8 (256) entries in the page table•Page size of 2^8 bytes (256 bytes per page)
•16 entries in the TLB
•Frame size of 2^8 bytes (256 bytes), so one page == one frame.  This is not normally thecase, and it won’t be the case in the second part of this simulation.
•256 frames
•Physical memory of 65,536 bytes (256 frames x 256-byte frame size)


Running the program from terminal:
gcc -o mem mem_mgr.c
./mem address.txt 


Sample output file is contained in output.txt
