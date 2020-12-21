#include <inc/lib.h>

void *mmap(envid_t envid, void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
	struct Stat stat;
	int r;

	r = fstat(fd, &stat);
	if (r < 0) return NULL;

	size_t size = ROUNDUP(len, PGSIZE);

	for (uint32_t i = 0; i < len; i += PGSIZE) {
		r = sys_page_alloc(0, (void *)UTEMP, PTE_W | PTE_U | PTE_P);
		if (r < 0) return NULL;
		seek(fd, offset + i);
		readn(fd, UTEMP, MIN(PGSIZE, stat.st_size - i));
		r = sys_page_map(0, UTEMP, envid, (void *)(addr + i), prot | PTE_U | PTE_P);
		if (r < 0) return NULL;
		sys_page_unmap(0, UTEMP);
	}

	return addr;
}
