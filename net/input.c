#include "ns.h"

#define BUFLEN 2
#define BUFHEAD 0x04000000

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	union Nsipc *buffer[BUFLEN];
	int pos=0;
	int length;
	for(int i=0;i<BUFLEN;i++){
		buffer[i]=(union Nsipc *)BUFHEAD+i*PGSIZE;
		if(sys_page_alloc(0,(void *)buffer[i],PTE_P|PTE_U|PTE_W)<0)
			panic("input: sys_page_alloc failed\n");
	}
	while(1){
		while((length=sys_packet_try_recv(buffer[pos]->pkt.jp_data,PGSIZE-sizeof(int)))<0){
			sys_yield();
		}
		buffer[pos]->pkt.jp_len=length;
		ipc_send(ns_envid,NSREQ_INPUT,buffer[pos],PTE_P|PTE_U|PTE_W);
		pos=(pos+1)%BUFLEN;
	}
}
