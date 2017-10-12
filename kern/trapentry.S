/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


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


TRAPHANDLER_NOEC(divide_e,T_DIVIDE);
TRAPHANDLER_NOEC(debug_e,T_DEBUG);
TRAPHANDLER_NOEC(nmi_e,T_NMI);
TRAPHANDLER_NOEC(brkpt_e,T_BRKPT);
TRAPHANDLER_NOEC(oflow_e,T_OFLOW);
TRAPHANDLER_NOEC(bound_e,T_BOUND);
TRAPHANDLER_NOEC(illop_e,T_ILLOP);
TRAPHANDLER_NOEC(device_e,T_DEVICE);
TRAPHANDLER(dblflt_e,T_DBLFLT);
TRAPHANDLER(tss_e,T_TSS);
TRAPHANDLER(segnp_e,T_SEGNP);
TRAPHANDLER(stack_e,T_STACK);
TRAPHANDLER(gpflt_e,T_GPFLT);
TRAPHANDLER(pgflt_e,T_PGFLT);
TRAPHANDLER_NOEC(fperr_e,T_FPERR);
TRAPHANDLER(align_e,T_ALIGN);
TRAPHANDLER_NOEC(mchk_e,T_MCHK);
TRAPHANDLER_NOEC(simderr_e,T_SIMDERR);
TRAPHANDLER_NOEC(syscall_e,T_SYSCALL);


/*
 * Lab 3: Your code here for _alltraps
 */


_alltraps:
	pushl %ds
	pushl %es
	pushal
	movl $GD_KD,%eax
	movl %eax,%es
	movl %eax,%ds
	pushl %esp
	call trap
	addl $4,%esp
	popal
	popl %es
	popl %ds
	addl $8,%esp
	iret



