#include <kern/e1000.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000;
uint32_t mac_addr[2] = {0x12005452,0x5634};

struct trans_desc trans_d[TRANSMIT_DESC_SIZE];
struct recv_desc recv_d[RECV_DESC_SIZE];
struct packet_buffer trans_b[TRANSMIT_DESC_SIZE];
struct packet_buffer recv_b[RECV_DESC_SIZE];

static void register_init(){
	e1000[TDBAL/4]=PADDR(trans_d);
	e1000[TDBAH/4]=0;
	e1000[TDLEN/4]=TRANSMIT_DESC_SIZE*sizeof(struct trans_desc);
	e1000[TDH/4]=0;
	e1000[TDT/4]=0;
	e1000[TCTL/4]=TCTL_EN|TCTL_PSP|(TCTL_CT&(0x10<<4))|(TCTL_COLD&(0x40<<12));
	e1000[TIPG/4]=10|(4<<10)|(6<<20);

	e1000[RA/4]=mac_addr[0];
	e1000[RA/4+1]=mac_addr[1];
	e1000[RA/4+1]|=RAV;
	cprintf("our mac addr %x:%x\n",mac_addr[1],mac_addr[0]);
	memset((void *)&e1000[MTA/4],0,128*4);
	e1000[IMS/4]=0;
	//e1000[ICS/4]=0;
	e1000[RDBAL/4]=PADDR(recv_d);
	e1000[RDBAH/4]=0;
	e1000[RDLEN/4]=RECV_DESC_SIZE*sizeof(struct recv_desc);
	e1000[RDH/4]=0;
	e1000[RDT/4]=RECV_DESC_SIZE-1;
	e1000[RCTL/4]=RCTL_EN|RCTL_BSIZE|RCTL_SECRC;
}

static void trans_desc_init(){
	for(int i=0;i<TRANSMIT_DESC_SIZE;i++){
		trans_d[i].addr=PADDR(trans_b+i);
		trans_d[i].status=TXD_STAT_DD;
		trans_d[i].cmd=TXD_CMD_EOP|TXD_CMD_RS;
	}
}

static void recv_desc_init(){
	for(int i=0;i<RECV_DESC_SIZE;i++){
		recv_d[i].addr=PADDR(recv_b+i);
	}
}

int pci_e1000_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
	e1000=mmio_map_region(pcif->reg_base[0],pcif->reg_size[0]);
	cprintf("e1000 at phys base %x size %x\ne1000 status %x (should be 0x80080783)\n",
		pcif->reg_base[0],pcif->reg_size[0],e1000[8/4]);

	trans_desc_init();
	recv_desc_init();
	register_init();	
	return 1;
}

int e1000_transmit(char *addr,uint32_t size){
	int tail=e1000[TDT/4];
	struct trans_desc *tail_desc=trans_d+tail;
	struct trans_desc *next_desc=trans_d+((tail+1)%TRANSMIT_DESC_SIZE);
	if(!(next_desc->status&TXD_STAT_DD))
		return -1;
	if(size>BUFSIZE)
		panic("e1000_transmit: size>BUFSIZE\n");
	memmove(trans_b+tail,addr,size);
	tail_desc->length = (uint16_t)size;
	tail_desc->status &= ~TXD_STAT_DD;
	e1000[TDT/4]=(tail+1)%TRANSMIT_DESC_SIZE;
	return 0;
}

int e1000_receive(char *addr,uint32_t size){
	int tail=(e1000[RDT/4]+1)%RECV_DESC_SIZE;
	struct recv_desc *next_desc=recv_d+tail;
	if(!(next_desc->status&RXD_STAT_DD))
		return -1;
	if((next_desc->length)>BUFSIZE)
		panic("e1000_receive: (next_desc->length)>BUFSIZE\n");
	if((next_desc->length)<size) size=next_desc->length;
	memmove(addr,recv_b+tail,size);
	next_desc->status &= ~RXD_STAT_DD;
	e1000[RDT/4]=tail;
	return size;
}
