#include <inc/types.h>
#include <kern/pci.h>
#include <kern/pmap.h>
#include <inc/string.h>
#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H


#define PCI_E1000_VENDOR 0x8086
#define PCI_E1000_DEVICE 0x100E
#define TRANSMIT_DESC_SIZE 64
#define RECV_DESC_SIZE 128
#define BUFSIZE 2048

#define TDBAL 0x03800
#define TDBAH 0x03804
#define TDLEN 0x03808
#define TDH 0x03810
#define TDT 0x03818
#define TCTL 0x00400
#define TCTL_EN 0x2
#define TCTL_PSP 0x8
#define TCTL_CT 0xff0
#define TCTL_COLD 0x3ff000
#define TIPG 0x00410
#define TXD_STAT_DD 0x1
#define TXD_CMD_RS 0x08
#define TXD_CMD_EOP 0x01

#define RA 0x5400
#define MTA 0x5200
#define IMS 0x00D0
#define RDBAL 0x2800
#define RDBAH 0x2804
#define RDLEN 0x2808
#define RDH 0x2810
#define RDT 0x2818
#define RCTL 0x0100
#define RCTL_EN 0x2
#define RCTL_BSIZE 0x0	//2048 byte
#define RCTL_SECRC 0x04000000
#define ICS 0x00C8
#define RAV 0x80000000
#define RXD_STAT_DD 0x01


struct trans_desc{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};

struct recv_desc{
	uint64_t addr;
	uint16_t length;
	uint16_t checksum;
	uint8_t status;
	uint8_t errors;
	uint16_t special;
};

struct packet_buffer{
	char buffer[BUFSIZE];
};

int pci_e1000_attach();
int e1000_transmit(char *addr,uint32_t size);
int e1000_receive(char *addr,uint32_t size);

#endif	// JOS_KERN_E1000_H
