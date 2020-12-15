// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	// Should `uvpd` be used?
	if (!(uvpd[PDX(addr)] & PTE_P) || !(uvpt[PGNUM(addr)] & PTE_P)) {
		panic("pgfault: The faulting address is not mapped. va = 0x%08x\n", addr);
	}

	if (!(uvpt[PGNUM(addr)] & PTE_COW)) {
		panic("pgfault: The faulting access is not to a copy-on-write page. va = 0x%08x\n", addr);
	}

	if (!(err & FEC_WR)) {
		panic("pgfault: The faulting access was not a write. va = 0x%08x\n", addr);
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	void *pg = ROUNDDOWN(addr, PGSIZE);
	void *tmppg = (void *)PFTEMP;

	r = sys_page_alloc(0, tmppg, PTE_U | PTE_W | PTE_P);
	if (r < 0) {
		panic("pgfault: sys_page_alloc() failed: %e\n", r);
	}

	memcpy(tmppg, pg, PGSIZE);

	r = sys_page_map(0, tmppg, 0, pg, PTE_U | PTE_W | PTE_P);
	if (r < 0) {
		panic("sys_page_map failed: %e\n", r);
	}

	r = sys_page_unmap(0, tmppg);
	if (r < 0) {
		panic("sys_page_unmap failed: %e\n", r);
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.

	void *addr = (void *)(pn * PGSIZE);

	// Lessons learned the hard way: cannot use `thisenv->env_id` to get the
	// child's envid. Because `thisenv` is not set yet, and the setting will
	// cause page faults.

	if (uvpt[pn] & PTE_SHARE) {
		// shared page
		r = sys_page_map(0, addr, envid, addr, uvpt[pn] & PTE_SYSCALL);
		if (r < 0) {
			return r;
		}
	}
	else if ((uvpt[pn] & PTE_COW) || (uvpt[pn] & PTE_W)) {
		// copy-on-write page
		r = sys_page_map(0, addr, envid, addr, PTE_COW | PTE_U | PTE_P);
		if (r < 0) {
			return r;
		}

		// The correctness needs to be rethought here.
		r = sys_page_map(0, addr, 0, addr, PTE_COW | PTE_U | PTE_P);
		if (r < 0) {
			return r;
		}
	}
	else {
		// other pages
		r = sys_page_map(0, addr, envid, addr, uvpt[pn] & PTE_SYSCALL);
		if (r < 0) {
			return r;
		}
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.

	int r;

	set_pgfault_handler(pgfault);

	envid_t eid = sys_exofork();

	if (eid < 0) {
		return eid;
	}

	if (eid) {
		// parent

		// duplicate the page table
		for (int pdn = 0; pdn < PDX(UTOP); pdn++) {
			if (uvpd[pdn] & (PTE_U | PTE_P)) {
				for (int ptn = 0; ptn < NPTENTRIES; ptn++) {
					int pn = pdn * NPTENTRIES + ptn;

					// skip the user exception stack page
					if (pn == PGNUM(UXSTACKTOP - PGSIZE)) continue;

					if ((uvpt[pn] & (PTE_U | PTE_P)) == (PTE_U | PTE_P)) {
						int r = duppage(eid, pn);
						if (r < 0) {
							cprintf("duppage error : %e\n", r);
						}
					}
				}
			}
		}

		// allocate the user exception stack
		r = sys_page_alloc(eid, (void *)UXSTACKTOP - PGSIZE, PTE_U | PTE_W | PTE_P);
		if (r < 0) {
			return r;
		}

		// set the page fault handler entrypoint for the child
		// (_pgfault_handler copied (or shared), but `env_pgfault_upcall` is in the kernel)
		extern void _pgfault_upcall(void);
		r = sys_env_set_pgfault_upcall(eid, _pgfault_upcall);
		if (r < 0) {
			return r;
		}

		// mark the child as runnable
		r = sys_env_set_status(eid, ENV_RUNNABLE);
		if (r < 0) {
			return r;
		}

		return eid;
	}
	else {
		// child

		envid_t envid = sys_getenvid();
		thisenv = &envs[ENVX(envid)];

		return 0;
	}
}

// Challenge!
int
sfork(void)
{
	int r;

	set_pgfault_handler(pgfault);

	envid_t eid = sys_exofork();

	if (eid < 0) {
		return eid;
	}

	if (eid) {
		// parent

		// duplicate the page table
		for (int pdn = 0; pdn < PDX(UTOP); pdn++) {
			if (uvpd[pdn] & (PTE_U | PTE_P)) {
				for (int ptn = 0; ptn < NPTENTRIES; ptn++) {
					int pn = pdn * NPTENTRIES + ptn;
					void *addr = (void *)(pn * PGSIZE);

					// skip the user exception stack page
					if (pn == PGNUM(UXSTACKTOP - PGSIZE)) continue;

					// duplicate the regular user stack
					else if (pn == PGNUM(USTACKTOP - PGSIZE)) {
						duppage(eid, pn);
					}

					// share the page (duplicate the mapping)
					else if (uvpt[pn] & (PTE_U | PTE_P)) {
						sys_page_map(0, addr, eid, addr, uvpt[pn] & PTE_SYSCALL);
					}
				}
			}
		}

		// allocate the user exception stack
		r = sys_page_alloc(eid, (void *)UXSTACKTOP - PGSIZE, PTE_U | PTE_W | PTE_P);
		if (r < 0) {
			return r;
		}

		// set the page fault handler entrypoint for the child
		// (_pgfault_handler copied (or shared), but `env_pgfault_upcall` is in the kernel)
		extern void _pgfault_upcall(void);
		r = sys_env_set_pgfault_upcall(eid, _pgfault_upcall);
		if (r < 0) {
			return r;
		}

		// mark the child as runnable
		r = sys_env_set_status(eid, ENV_RUNNABLE);
		if (r < 0) {
			return r;
		}

		return eid;
	}
	else {
		// child

		return 0;
	}
}
