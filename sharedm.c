#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct
{
	int id;
	int pid_creator;
	uint flags;
	int refcount;
	int size;
	char *physical_share_address[10];
} shared_mem[20];

struct spinlock spin;

int shm_open(int id, int page_count, int flag)
{
	int index = 0;

	for (int i = 0; i < 20; ++i)
	{
		if (shared_mem[i].id == id)
			return -1;
		if (!shared_mem[i].id)
		{
			index = i + 1;
			break;
		}
	}
	if (index)
	{
		cprintf("in shm_open %d\n", index);
		index--;
		struct proc *p = myproc();
		pte_t *pte;
		uint pa;
		for (int i = 0; i < page_count; i++)
		{
			if ((pte = walkpgdir1(p->pgdir, (void *)(p->sz), 0)) == 0)
				panic("copyuvm: pte should exist");

			pa = PTE_ADDR(*pte);

			if ((shared_mem[index].physical_share_address[i] = kalloc()) == 0)
			{
				panic("open fail: pte should exist");
				return -1;
			}
			acquire(&spin);

			memmove(shared_mem[index].physical_share_address[i], (char *)P2V(pa), PGSIZE);
			mappages1(pte, (void *)p->sz, PGSIZE, V2P(shared_mem[index].physical_share_address[i]), PTE_P || PTE_W);
			p->sz += PGSIZE;
			release(&spin);
		}
		shared_mem[index].flags = flag;
		shared_mem[index].id = id;
		shared_mem[index].pid_creator = p->pid;
		shared_mem[index].refcount = 1;
		shared_mem[index].size = page_count;
		cprintf("going out of shm_open\n");

		return 0;
	}
	return -1;
}

void *shm_attach(int id)
{
	int index = 0;
	acquire(&spin);
	for (int i = 0; i < 20; ++i)
		if (shared_mem[i].id == id)
		{
			index = i + 1;
			break;
		}
	if (index)
	{

		index--;
		struct proc *p = myproc();
		pte_t *pte;
		cprintf("index : %d flags : %d\n", index, shared_mem[index].flags);
		cprintf("this proc id : %d parrent : %d shm id : %d\n", p->pid, p->parent->pid, shared_mem[index].pid_creator);

		if (((shared_mem[index].flags % 2) == 1 && (p->parent->pid == shared_mem[index].pid_creator)) || (shared_mem[index].flags % 2) == 0)
		{
			cprintf("%d\n\n", shared_mem[index].refcount);
			for (int i = 0; i < shared_mem[index].size; i++)
			{
				if ((pte = walkpgdir1(p->pgdir, (void *)(p->sz), 0)) == 0)
					panic("copyuvm: pte should exist");

				if (shared_mem[index].flags >= 2)
					mappages1(pte, (void *)p->sz, PGSIZE, V2P(shared_mem[index].physical_share_address[i]), PTE_P || PTE_W);
				else
					mappages1(pte, (void *)p->sz, PGSIZE, V2P(shared_mem[index].physical_share_address[i]), PTE_P);

				p->sz += PGSIZE;
			}
			shared_mem[index].refcount++;
			cprintf("%d\n\n", shared_mem[index].refcount);
		}
		release(&spin);
		return (void *)(p->sz - (PGSIZE)*shared_mem[index].size);
	}
	release(&spin);
	return (void *)-1;
}

int shm_close(int id)
{
	int index = 0;
	acquire(&spin);

	for (int i = 0; i < 20; i++)
		if (shared_mem[i].id == id)
		{
			index = i + 1;
			break;
		}
	if (index)
	{
		index--;
		struct proc *p = myproc();
		if (p->pid == shared_mem[index].id && shared_mem[index].refcount != 1)
			return -1;
		else if (p->pid != shared_mem[index].id)
		{
			p->sz -= shared_mem[index].size * PGSIZE;
		}
		else
		{
			p->sz -= shared_mem[index].size * PGSIZE;

			for (int i = 0; i < shared_mem[index].refcount; i++)
			{
				kfree(shared_mem[index].physical_share_address[i]);
			}
			shared_mem[index].flags = 0;
			shared_mem[index].id = 0;
			shared_mem[index].pid_creator = 0;
			shared_mem[index].refcount = 0;
			shared_mem[index].size = 0;
		}
		release(&spin);
		return 0;
	}
	return -1;
}