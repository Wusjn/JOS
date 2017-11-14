// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/env.h>

#include <kern/pmap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{"backtrace","Display information about the calling trace",mon_backtrace},
	{"showmappings","Display mapping information",mon_showmappings},
	{"setperm","Change map perms",mon_setperm},
	{"memdump","Display memory",mon_memdump},
	{"si","step in a env(process)",mon_si},
	{"c","continue until next breakpoint",mon_c},
	{"transmit","transmit a packet",mon_transmit}
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

uint32_t
xtou(char *buf){
	uint32_t num=0;
	buf+=2;
	while(*buf){
		if(*buf>='a') num=num*16+(*buf-'a'+10);
		else num=num*16+(*buf-'0');
		buf++;
	}
	return num;
}

void pteprint(pte_t *cpte){
	cprintf("%x with perm PTE_P %d PTE_W %d PTE_U %d\n",PTE_ADDR(*cpte),
			((*cpte)&PTE_P)==PTE_P,((*cpte)&PTE_W)==PTE_W,((*cpte)&PTE_U)==PTE_U);
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf){
	if(argc!=3){
		cprintf("usage: showmappngs [vaddr begin] [vaddr end]\n");
		return 0;
	}
	uint32_t begin=ROUNDDOWN(xtou(argv[1]),PGSIZE);
	uint32_t end=ROUNDUP(xtou(argv[2]),PGSIZE);
	pte_t *cpte;
	uint32_t caddr;
	cprintf("begin : %x	end : %x\n",begin,end);
	for(caddr=begin;caddr<=end;caddr+=PGSIZE){
		cpte=pgdir_walk(kern_pgdir,(void *)caddr,0);
		if(cpte==NULL){
			cprintf("page table of vpage %x not mapped\n",caddr);
			continue;
		}
		cprintf("vpage %x at ppage ",caddr);
		pteprint(cpte);
	}
	return 0;
}

int mon_setperm(int argc, char **argv, struct Trapframe *tf){
	if(argc!=5){
		cprintf("usage: setperm [vaddr] [PTE_P] [PTE_W] [PTE_U]\n");
		return 0;
	}
	uint32_t addr=ROUNDDOWN(xtou(argv[1]),PGSIZE);
	pte_t *cpte;
	cpte=pgdir_walk(kern_pgdir,(void *)addr,0);
	if(cpte==NULL){
		cprintf("page tabel of vpage %x not mapped\n",addr);	
		return 0;
	}
	cprintf("previous:\n");
	cprintf("vpage %x at ppage ",addr);
	pteprint(cpte);
	uint32_t perm=0;
	if(*argv[2]!='0') perm|=PTE_P;
	if(*argv[3]!='0') perm|=PTE_W;
	if(*argv[4]!='0') perm|=PTE_U;
	*cpte=((*cpte)&(~0x7))|perm;
	cprintf("now\n");
	cprintf("vpage %x at ppage ",addr);
	pteprint(cpte);

	return 0;
}

void printmem(uint32_t begin,uint32_t end,int paddr){
	if(paddr){
		uint32_t pbegin=(uint32_t)KADDR(begin);
		uint32_t pend=(uint32_t)KADDR(end);
		for(;pbegin<pend;begin+=4,pbegin+=4){
			cprintf("PM at %x is %x\n",begin,*(uint32_t *)pbegin);
		}
		return;
	} 
	for(;begin<end;begin+=4){
		cprintf("VM at %x is %x\n",begin,*(uint32_t *)begin);
	}
	return;
}

int mon_memdump(int argc, char **argv, struct Trapframe *tf){
	if(argc!=4){
		cprintf("usage: memdump [paddr?] [begin] [end]\n");
		return 0;
	}
	uint32_t begin=xtou(argv[2]);
	uint32_t end=xtou(argv[3]);
	uint32_t paddr=(*argv[1]=='1');
	pte_t *cpte;
	cprintf("begin : %x	end : %x\n",begin,end);
	printmem(begin,end,paddr);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	unsigned int *p=(unsigned int *)read_ebp();
	unsigned int eip;
	struct Eipdebuginfo info;
	while(p){
		eip=*(p+1);
		cprintf("ebp %08x eip %08x args ",p,eip);
		for(int i=0;i<5;i++)
			cprintf("%08x ",*(p+i+2));
		cprintf("\n");
		if(debuginfo_eip(eip,&info)==0){
			cprintf("%s:%u: %.*s+%u\n",info.eip_file,info.eip_line,info.eip_fn_namelen,info.eip_fn_name,eip-info.eip_fn_addr);
		}
		p=(unsigned int*)*p;
	}
	return 0;
}

int
mon_si(int argc, char **argv, struct Trapframe *tf)
{
	if(curenv&&curenv->env_tf.tf_trapno&(T_BRKPT|T_DEBUG)){
		curenv->env_tf.tf_eflags |=1<<8;
		return -1;
	}
	else return 0;
}

int
mon_c(int argc, char **argv, struct Trapframe *tf)
{
	if(curenv&&curenv->env_tf.tf_trapno&(T_BRKPT|T_DEBUG)){
		curenv->env_tf.tf_eflags &=~(1<<8);
		return -1;
	}
	else return 0;
}

int
mon_transmit(int argc, char **argv, struct Trapframe *tf)
{
	char *addr=(char *)tf;
	return e1000_transmit(addr,sizeof(struct Trapframe));
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome %C4to the %C2JOS kernel %Cemonitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
