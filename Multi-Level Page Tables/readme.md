

Operating Systems (0368-2162)

Multi-Level Page Tables Assignment

Due date (via moodle): April 7, 23:59

Individual work policy

The work you submit in this course is required to be the result of your individual eﬀort

only. You may discuss concepts and ideas with others, but you must program individually.

You should never observe another student’s code, from this or previous semesters.

Students violating this policy will receive a 250 grade in the course (“did not complete course

duties”).

1 Introduction

The goal in this assignment is to implement simulated OS code that handles a multi-level (trie-based)

page table. You will implement two functions. The ﬁrst function creates/destroys virtual memory

mappings in a page table. The second function checks if an address is mapped in a page table. (The

second function is needed if the OS wants to ﬁgure out which physical address a process virtual

address maps to.)

Your code will be a simulation because it will run in a normal process. We provide two ﬁles,

os.c and os.h, which contain helper functions that simulate some OS functionality that your code

will need to call. For your convenience, there’s also a main() function demonstrating usage of the

code. However, the provided main() only exercises your code in trivial ways. We recommend that

you change it to thoroughly test the functions that you implemented.

1.1 Target hardware

Our simulated OS targets an imaginary 32-bit x86-like CPU. When talking about addresses (virtual

or physical), we refer to the least signiﬁcant bit as bit 0 and to the most signiﬁcant bit as bit 31.

Virtual addresses The virtual address size of our hardware is 32 bits, all of which are used for

translation. The following depicts the virtual address layout:

Physical addresses The physical address size of our hardware is also 32 bits.

Page table structure The page/frame size is 4 KB (4096 bytes). Page table nodes occupy a

physical page frame, i.e., they are 4 KB in size. The size of a page table entry is 32 bits. Bit 0 is the

valid bit. Bits 1–11 are unused and must be set to zero. (This means that our target CPU does

not implement page access rights.) The top 20 bits contain the page frame number that this entry

points to. The following depicts the PTE format:

1





Number of page table levels To successfully complete the assignment, you must answer to

yourself: how many levels are there in our target machine’s multi-level page table?

1.2 OS physical memory manager

To write code that manipulates page tables, you need to be able to perform the following: (1) obtain

the page number of an unused physical page, which marks it as used; and (2) obtain the kernel

virtual address of a given physical address. The provided os.c contains functions simulating this

functionality:

\1. Use the following function to allocate a physical page (also called page frame):

uint32 t alloc page frame(void);

This function returns the physical page number of the allocated page. In this assignment, you

do not need to free physical pages. If alloc page frame() is unable to allocate a physical page,

it will exit the program. The content of the allocated page frame is all zeroes.

\2. Use the following function to obtain a pointer (i.e., virtual address) to a physical address:

void\* phys to virt(uint32 t phys addr);

The valid inputs to phys to virt() are addresses that reside in physical pages that were

previously returned by alloc page frame(). If it is called with an invalid input, it returns

NULL.

2 Assignment description

Implement the following two functions in a ﬁle named pt.c. This ﬁle should #include "os.h" to

obtain the function prototypes.

\1. A function to create/destroy virtual memory mappings in a page table:

void page table update(uint32 t pt, uint32 t vpn, uint32 t ppn);

This function takes the following arguments:

(a) pt: The physical page number of the page table root (this is the physical page that the

page table base register in the CPU state will point to). You can assume that pt has been

previously returned by alloc page frame().

(b) vpn: The virtual page number the caller wishes to map/unmap.

(c) ppn: Can be one of two cases. If ppn is equal to a special NO MAPPING value (deﬁned in

os.h), then vpn’s mapping (if it exists) should be destroyed. Otherwise, ppn speciﬁes the

physical page number that vpn should be mapped to.

2





\2. A function to query the mapping of a virtual page number in a page table:

uint32 t page table query(uint32 t pt, uint32 t vpn);

This function returns the physical page number that vpn is mapped to, or NO MAPPING if no

mapping exists. The meaning of the pt argument is the same as with page table update().

You can implement helper functions for your code, but make sake sure to implement them in

your pt.c ﬁle. You may not submit additional ﬁles (not even header ﬁles).

IMPORTANT: A page table node should be treated as an array of uint32 ts.

2.1 Something to think about (no need to submit)

How would your code change if you were required to free a page table node once all of the PTEs it

contains become invalid? How would you detect this condition? How eﬃcient would your approach

be (i.e., how much overhead would it add on every page table update)?

3 Submission instructions

\1. Submit just your pt.c ﬁle. (We will test it with our own main function.)

\2. The program must compile cleanly (no errors or warnings) when the following command is run

in a directory containing the source code ﬁles:

gcc -O3 -Wall -std=c11 os.c pt.c

3

