# Project 0: Pintos Introduction

## Find the Faulting Instruction

1. What virtual address did the program try to access from userspace that caused it to crash?

ANS: 0xc0000008

2. What is the virtual address of the instruction that resulted in the crash?

ANS：0x8048915

3. To investigate, disassemble the do-nothing binary using objdump (you used this tool in Homework
0). What is the name of the function the program was in when it crashed? Copy the disassembled
code for that function onto Gradescope, and identify the instruction at which the program crashed.

ANS: 
run the code `objdump -d do-nothing`
and find the following result:
```
0804890f <_start>:
 804890f:	55                   	push   %ebp
 8048910:	89 e5                	mov    %esp,%ebp
 8048912:	83 ec 18             	sub    $0x18,%esp
 8048915:	8b 45 0c             	mov    0xc(%ebp),%eax
 8048918:	89 44 24 04          	mov    %eax,0x4(%esp)
 804891c:	8b 45 08             	mov    0x8(%ebp),%eax
 804891f:	89 04 24             	mov    %eax,(%esp)
 8048922:	e8 6d f7 ff ff       	call   8048094 <main>
 8048927:	89 04 24             	mov    %eax,(%esp)
 804892a:	e8 2d 22 00 00       	call   804ab5c <exit>
 ```
 The instruction is `mov 0xc(%ebp),%eax`.
 The function name is `_start`

4. Find the C code for the function you identified above (hint: it was executed in userspace, so it’s
either in do-nothing.c or one of the files in proj0/src/lib or proj0/src/lib/user), and copy it
onto Gradescope. For each instruction in the disassembled function in #3, explain in a few words
why it’s necessary and/or what it’s trying to do. Hint: see 80x86 Calling Convention.

ANS: `void _start(int argc, char* argv[]) { exit(main(argc, argv)); }`

It's in `proj0/src/lib/user/entry.c`

Instruction explanation (based on x86 calling conventions):
* `push %ebp`：Save the caller's base pointer (though _start is the entry point with no caller, this is to set up the stack frame).
* `mov %esp,%ebp`：Set the base pointer of the current stack frame so that the ebp points to the top of the stack.
* `sub $0x18,%esp`：Allocate 24 bytes on the stack for local variables or parameter passing.
* `mov 0xc(%ebp),%eax`：Load the value from ebp+0xc into eax. This attempt to access the second parameter (argv) caused a crash. ebp+0xc should point to the address of the argv array.
* `mov %eax,0x4(%esp)`：Store the eax (i.e.,argv) at stack offset 4 to prepare the second parameter for the main function call.
* `mov 0x8(%ebp),%eax`：Load the value from ebp+0x8 into eax to retrieve the first parameter (argc).
* `mov %eax,(%esp)`：Store the argument count (argc) at the top of the stack to prepare the first parameter for the main function call.
* `call main`：call `main` function。
* `mov %eax,(%esp)`：Store the return value of main at the top of the stack as an argument for exit.
* `call exit`：Call the exit function to terminate the program.


5. Why did the instruction you identified in #3 try to access memory at the virtual address you
identified in #1? Don’t explain this in terms of the values of registers; we’re looking for a higher
level explanation.

ANS: 
* When the program starts, the kernel is responsible for setting up the user stack and passing command-line arguments (argc and argv).  The program attempts to access the argv argument on the stack via the ebp+0xc address in the `_start` function, but the stack pointer ebp is set to 0xbffffffc. 
* Consequently, the calculation of ebp+0xc results in 0xc0000008.  **This address exceeds the top of the user stack, entering kernel space.**  Since the user program lacks permission to access kernel memory, it triggers a page fault exception (#PF), causing the program to crash.  
* The root cause lies in the kernel potentially failing to properly initialize the user stack or pass valid arguments, leading the program to attempt access to an invalid memory region.

## Step Through the Crash
6. Step into the process_execute function. What is the name and address of the thread running this function? What other threads are present in Pintos at this time? Copy their struct threads. (Hint: for the last part dumplist &all_list thread allelem may be useful.)

ANS:
0xc000e000 "main"
```
(gdb) dumplist &all_list thread allelem
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, ame = "main", '\000' <repeats 11 times>, stack = 0xc000ed9c "=S\002\300\001", priority = 31, allelem = {prev = 0xc003b17c <all_list>, next = 0xc0104020}, elem = {prev = 0xc003b16c <fifo_ready_list>, next = 0xc003b174 <fifo_ready_list+8>}, pcb = 0xc010500c, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f14 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc003b184 <all_list+8>}, elem = {prev = 0xc003b16c <fifo_ready_list>, next = 0xc003b174 <fifo_ready_list+8>},
  pcb = 0x0, magic = 3446325067}
```
7. What is the backtrace for the current thread? Copy the backtrace from GDB as your answer and also copy down the line of C code corresponding to each function call.

ANS:
```
(gdb) backtrace
#0  process_execute (file_name=0xc0007d50 "do-nothing") at ../../userprog/process.c:57
#1  0xc0020a62 in run_task (argv=0xc003b06c <argv+12>) at ../../threads/init.c:315
#2  0xc0020ba4 in run_actions (argv=0xc003b06c <argv+12>) at ../../threads/init.c:388
#3  0xc0020421 in main () at ../../threads/init.c:136
```

corresponding code
```
  sema_init(&temporary, 0);
  process_wait(process_execute(task));
  a->function(argv);
  run_actions(argv);
```

8. Set a breakpoint at start_process and continue to that point. What is the name and address of the thread running this function? What other threads are present in Pintos at this time? Copy their struct threads.

ANS: start_process （file_name_=0xc010a000）
```
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000ee7c "", priority = 31, allelem = {prev = 0xc003b17c <all_list>, next = 0xc0104020}, elem = {prev = 0xc003cb98 <temporary+4>, next = 0xc003cba0 <temporary+12>}, pcb = 0xc010500c, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f14 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010b020}, elem = {prev = 0xc003b16c <fifo_ready_list>, next = 0xc003b174 <fifo_ready_list+8>},
  pcb = 0x0, magic = 3446325067}
pintos-debug: dumplist #2: 0xc010b000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010bfd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc003b184 <all_list+8>}, elem = {prev = 0xc003b16c <fifo_ready_list>, next = 0xc003b174 <fifo_ready_list+8>},
  pcb = 0x0, magic = 3446325067}
```

9. Where is the thread running start_process created? Copy down this line of code.

ANS: `thread_create("idle", PRI_MIN, idle, &idle_started);`

10. Step through the start_process() function until you have stepped over the call to load(). Note that load() sets the eip and esp fields in the if_ structure. Print out the value of the if_ structure, displaying the values in hex (hint: print/x if_).

ANS: 
```
$1 = {edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23,
  fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code = 0x0, frame_pointer = 0x0, eip = 0x804890f, cs = 0x1b,
  eflags = 0x202, esp = 0xc0000000, ss = 0x23}
```

11. The first instruction in the asm volatile statement sets the stack pointer to the bottom of the if_ structure. The second one jumps to intr_exit. The comments in the code explain what’s happening here. Step into the asm volatile statement, and then step through the instructions. As you step through the iret instruction, observe that the function “returns” into userspace. Why does the processor switch modes when executing this function? Feel free to explain this in terms of the values in memory and/or registers at the time iret is executed, and the functionality of the iret instruction.

ANS: When executing the iret instruction, the processor switches from kernel mode to user mode because:
* Iret restores the previously saved state from the stack: EIP, CS, EFLAGS, ESP, SS
* The restored CS segment selector value is 0x1b (binary 00011011), where the last two bits 11 indicate user mode (privilege level 3).
* Similarly, the SS segment selector value of 0x23 (binary 00100011) is also in user mode
* When the processor loads these segment registers, it detects a privilege level change from 0 (kernel) to 3 (user) and automatically switches modes.
* Iret also restores the user stack pointer (ESP) and instruction pointer (EIP) from the stack.

12. Once you’ve executed iret, type info registers to print out the contents of registers. Include the output of this command on Gradescope. How do these values compare to those when you
printed out if_?

ANS: 
```
(gdb) info registers
eax            0x0                 0
ecx            0x0                 0
edx            0x0                 0
ebx            0x0                 0
esp            0xc0000000          0xc0000000
ebp            0x0                 0x0
esi            0x0                 0
edi            0x0                 0
eip            0x804890f           0x804890f
eflags         0x202               [ IOPL=0 IF ]
cs             0x1b                27
ss             0x23                35
ds             0x23                35
es             0x23                35
fs             0x23                35
gs             0x23                35
```


These values perfectly match the previously printed if_ structure, confirming that **iret successfully restored all user mode register states**.

13. Notice that if you try to get your current location with backtrace you’ll only get a hex address. This is because because pintos-gdb ./kernel.o only loads in the symbols from the kernel. Now that we are in userspace, we have to load in the symbols from the Pintos executable we are running, namely do-nothing. To do this, use loadusersymbols tests/userprog/do-nothing. Now, using backtrace, you’ll see that you’re currently in the _start function. Using the disassemble and stepi commands, step through userspace instruction by instruction until the page fault occurs. At this point, the processor has immediately entered kernel mode to handle the page fault, so backtrace will show the current stack in kernel mode, not the user stack at the time of the page fault. However, you can use btpagefault to find the user stack at the time of the page fault. Copy down the output of btpagefault.

ANS: 
```
#0  0x08048915 in ?? ()
#1  0xf000ff53 in ?? ()
```

## Debug
14. Modify the Pintos kernel so that do-nothing no longer crashes. Your change should be in the Pintos kernel, not the userspace program (do-nothing.c) or libraries in proj0/src/lib. This should not involve extensive changes to the Pintos source code. Our staff solution solves this with a single-line change to process.c. Explain the change you made to Pintos and why it was necessary. After making this change, the do-nothing test should pass but all others will still fail.

ANS: I change `*esp = PHYS_BASE;` to `*esp = PHYS_BASE - 12;` 

Explain
* The crash occurs because `_start` tries to access argv at `ebp+0xc`. With the initial stack pointer at `PHYS_BASE` (0xc0000000), ebp gets set to 0xbffffffc after the initial push `%ebp`, making `ebp+0xc` equal to 0xc0000008, which is in kernel space.
* The x86 calling convention expects the stack to contain:
  * argc (argument count)
  * argv[0] (pointer to program name)
  * argv[1] (NULL terminator)
  * ... and potentially environment variables
* The Fix: By setting `*esp = PHYS_BASE - 12`, we create space on the stack for:
  * 4 bytes for argc 
  * 4 bytes for argv[0] (pointer to program name)
  * 4 bytes for argv[1] (NULL terminator)
* How It Works: This creates a valid stack layout that satisfies the x86 calling convention without accessing kernel space. The `_start` function can now properly access its arguments from user-accessible memory addresses.

15. It is possible that your fix also works for the stack-align-0 test, but there are solutions for do-nothing that do not. Take a look at the stack-align-0 test. It behaves similarly to do-nothing, but it returns the value of esp % 16. Write down what this program should return (hint: this can be found in stack-align-0.ck) as well as why this is the case. You may wish to review stack alignment from Section 02. Then make sure that your previous fix for do-nothing also passes stack-align-0.

ANS：I change `*esp = PHYS_BASE - 12;` to `*esp = PHYS_BASE - 20;`. 
The program should return 12.

Explain (look into the assembly code): 
* Initial：esp = PHYS_BASE - 20 = 0xbfffffec. 0xbfffffec % 16 = 12
* Go into `_start`：
```
push %ebp                ; esp = 0xbfffffe8 (now %16 = 8)
mov %esp, %ebp
sub $0x18, %esp          ; esp = 0xbfffffd0 (now %16 = 0)
```
* `_start` prepare arguments for main：
```
mov 0xc(%ebp), %eax         ; get argv
mov %eax, 0x4(%esp)         ; store argv into esp+4
mov 0x8(%ebp), %eax         ; get argc
mov %eax, (%esp)            ; store argc into esp
```
 still esp is 0xbfffffd0 (%16 = 0). Notice the stak is 16 bytes aligned before call main which meets the requirement for stck align.
* call main：
```
call main                   ; push return address，esp = 0xbfffffcc (now %16 = 12)
```
* now go into main
```
mov %esp, %eax ; store esp(0xbfffffcc) into eax(return value)
and    $0xf,%eax ; do esp % 16
ret 
```
0xbfffffcc % 16 = 12, and that's what we expect.

16. Re-run GDB as before. Execute the loadusersymbols command, set a breakpoint at _start, and continue, to skip directly to the beginning of userspace execution. Using the disassemble and stepi commands, execute the do-nothing program instruction by instruction until you reach the int $0x30 instruction in proj0/src/lib/user/syscall.c. At this point, print the top two
words at the top of the stack by examining memory (hint: x/2xw $esp) and copy the output.

ANS: 0xbfffff98:     0x00000001      0x000000a2

17. The int $0x30 instruction switches to kernel mode and pushes an interrupt stack frame onto the kernel stack for this process. Continue stepping through instruction-by-instruction until you reach syscall_handler. What are the values of args[0] and args[1], and how do they relate to your answer to the previous question?

ANS: args[0] = 1, args[1] = 162. It matches the results in question 16.

Explain:

* System call number and arguments:
  * args[0] = 1: This is the **system call number** corresponding to SYS_EXIT (system exit call).
  * args[1] = 162: This is the **exit status code**.
* Relationship with stack contents during int $0x30 execution:
  * When executing the int $0x30 instruction, the top two words on the stack should be:
    * First word: System call number (1 = SYS_EXIT)
    * Second word: Exit status code (162)
  * This matches exactly with the stack contents we previously observed.
* Behavior of do-nothing program:
  * The logic of the do-nothing program is:main function returns 162,`_start` function passes main's return value to exit system call. Exit system call uses system call number 1 and argument 162.
* Parameter passing mechanism for system calls: In Pintos, system call parameters are passed through the following methods:
  * System call number: Placed in the **eax** register 
  * Parameters: Sequentially placed in the ebx,ecx,edx,esi,edi registers
  * When executing int $0x30: The processor automatically saves register states to the kernel stack. Syscall_handler extracts parameters from saved register states. The args array contains these register values.
