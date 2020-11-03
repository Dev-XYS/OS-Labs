/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

TRAPHANDLER_NOEC(handler0, 0)
TRAPHANDLER(handler1, 1)
TRAPHANDLER(handler2, 2)
TRAPHANDLER_NOEC(handler3, 3)
TRAPHANDLER(handler4, 4)
TRAPHANDLER(handler5, 5)
TRAPHANDLER(handler6, 6)
TRAPHANDLER(handler7, 7)
TRAPHANDLER(handler8, 8)
TRAPHANDLER(handler9, 9)
TRAPHANDLER(handler10, 10)
TRAPHANDLER(handler11, 11)
TRAPHANDLER(handler12, 12)
TRAPHANDLER(handler13, 13)
TRAPHANDLER(handler14, 14)
TRAPHANDLER(handler15, 15)
TRAPHANDLER(handler16, 16)
TRAPHANDLER(handler17, 17)
TRAPHANDLER(handler18, 18)
TRAPHANDLER(handler19, 19)
TRAPHANDLER(handler20, 20)
TRAPHANDLER(handler21, 21)
TRAPHANDLER(handler22, 22)
TRAPHANDLER(handler23, 23)
TRAPHANDLER(handler24, 24)
TRAPHANDLER(handler25, 25)
TRAPHANDLER(handler26, 26)
TRAPHANDLER(handler27, 27)
TRAPHANDLER(handler28, 28)
TRAPHANDLER(handler29, 29)
TRAPHANDLER(handler30, 30)
TRAPHANDLER(handler31, 31)

/*
 * Lab 3: Your code here for _alltraps
 */

	.globl _alltraps		/* define global symbol for 'name' */
	.type _alltraps, @function;	/* symbol type is function */
	.align 2;			/* align function definition */
_alltraps:				/* function starts here */
	pushw $0
	pushw %ds
	pushw $0
	pushw %es
	pusha
	mov $GD_KD, %ax
	mov %ax, %ds
	mov $GD_KD, %ax
	mov %ax, %ds
	pushl %esp
	call trap
