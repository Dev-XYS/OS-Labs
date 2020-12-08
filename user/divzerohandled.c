// buggy program - causes a divide by zero exception

#include <inc/lib.h>

int zero;

extern void _pgfault_upcall(void);
extern void (*_pgfault_handler)(struct UTrapframe *utf);

void
handler(struct UTrapframe *utf)
{
	cprintf("Exception caught: divide zero\n");
	exit();
}

void
umain(int argc, char **argv)
{
	// We borrow the `_pgfault_upcall` and `_pgfault_handler` from the standard library
	// for concept verification. As a result, we are not able to handle page fault and
	// other exceptions together. To remedy this, we only need to copy `_pgfault_upcall`
	// a few times and invoke different handlers in them.
	_pgfault_handler = handler;
	sys_page_alloc(thisenv->env_id, (void *)UXSTACKTOP - PGSIZE, PTE_U | PTE_W | PTE_P);
	sys_env_set_exception_upcall(thisenv->env_id, T_DIVIDE, _pgfault_upcall);

	zero = 0;
	cprintf("1/0 is %08x!\n", 1/zero);
}

