# HW-INTRO

## The A-Z’s of GDB
a. gdb ./map

b. (gdb) break main

c. (gdb) run

d. (gdb) print argv

What memory address does argv store?

0x7ffff7fba2e8

e. (gdb) x/10s *argv

Describe what’s located at that memory address. (What does argv point to?)

Starts with the program name and ends with the command line arguments.

f. (gdb) next

g. (gdb) print recur

What is the memory address of the recur function?

0x5555555551dc

h. (gdb) step

i. (gdb) next

j. (gdb) layout asm

k. (gdb) nexti

l. (gdb) info registers

What values are in all the registers?
```
rax            0x2                 2
rbx            0x555555555230      93824992236080
rcx            0x0                 0
rdx            0x0                 0
rsi            0x5555555592a0      93824992252576
```

m. (gdb) stepi

n. (gdb) layout src

o. (gdb) backtrace
```
#0  recur (i=0) at recurse.c:3
#1  0x0000555555555219 in recur (i=3) at recurse.c:7
#2  0x00005555555551d5 in main (argc=1, argv=0x7fffffffded8) at map.c:25
```
p. (gdb) break recur if i == 0

q. (gdb) continue

r. (gdb) backtrace
```
#0  recur (i=0) at recurse.c:3
#1  0x0000555555555219 in recur (i=2) at recurse.c:7
#2  0x0000555555555219 in recur (i=3) at recurse.c:7
#3  0x00005555555551d5 in main (argc=1, argv=0x7fffffffded8) at map.c:
```
s. (gdb) up

(gdb) print argc

1

t. (gdb) next

u. (gdb) layout asm

v.Which instructions correspond to the return 0 in C?

`mov    $0x0,%eax`

w. (gdb) layout src

x. (gdb) finish

y. (gdb) continue

z. (gdb) quit

## From Source Code to Executable
1. find which instruction(s) corresponds to the recursive call of recur(i - 1).

ANS: `call	recur`

2. What do the .text and .data sections contain?

ANS:
* .text segment: contains the executable code of the program (machine instructions)
* .data segment: contains initialized global variables and static variables.
3. What command do we use to view the symbols in an ELF file

ANS: nm map

4. What do the g, O, F, and *UND* flags mean?

ANS: 
* g: Global symbols (global)
* O: Symbols of object files (in.o files)
* F： function name
* UND: Undefined symbol (defined in another file)

5. Where else can we find a symbol for recur? Which file is this in?

ANS: map.o

6. Examine the symbol table of the entire map program now. What has changed?

ANS: 
Compared with the target file, the linked executable file:
* Contains the library function symbol of all links
* All undefined symbols are resolved
* Add the startup code symbol (such as _start)
* The symbol address is relocated to the final memory location

7. What segment(s)/section(s) contains recur (the function)? (The address of recur in objdump will
not be exactly the same as what you saw in gdb. An optional stretch exercise is to think about
why. See the Wikipedia article on relocation23 for a hint.)

ANS: 
* .text: The objdump command displays the relative addresses in a file, while GDB shows the actual virtual addresses loaded into memory.
* This discrepancy arises from the relocation process: when creating an executable,
the linker assigns symbolic addresses,
which the loader then maps to the physical virtual address space during runtime.

8. What segment(s)/section(s) contains global variables? Hint: look for the variables foo and stuff.

ANS: 
* foo (uninitialized global variables): in the.bss segment
* stuff (already initialized global variables): in the.data segment

9. Do you see the stack segment anywhere? What about the heap? Explain.

ANS: 
* Stack: Dynamically allocated at runtime, is part of the process memory layout (high addresses grow downward)
* Heap: Also dynamically allocated at runtime through malloc/brk
10. Based on the output of map, in which direction does the stack grow? Explain.

ANS: Based on the output of the map program, the stack grows downward (from high address to low address)