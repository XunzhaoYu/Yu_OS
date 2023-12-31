#ifndef __X86_H__
#define __X86_H__

// CPU protection level
#define DPL_KERN    0x0         // Kernel (ring 0)
#define DPL_USER    0x3         // User (ring 3)

// Application segment type bits
#define STA_X       0x8         // Executable segment
#define STA_W       0x2         // Writeable (non-executable segments)
#define STA_R       0x2         // Readable (executable segments)

// System segment type bits
#define STS_T32A    0x9         // Available 32-bit TSS
#define STS_IG32    0xE         // 32-bit Interrupt Gate
#define STS_TG32    0xF         // 32-bit Trap Gate

// Eflags register
#define FL_TF       0x00000100  // Trap Flag
#define FL_IF       0x00000200  // Interrupt Enable

// Control Register flags
#define CR0_PE      0x00000001  // Protection Enable
//0位是保护允许位PE，如果PE=1，保护模式启动，如果PE=0，实模式启动。
#define CR0_PG      0x80000000  // Paging——CR0的第31位是分页允许位

// Page directory and page table constants
#define PG_SIZE     4096        // Bytes mapped by a page
#define NR_PDE      1024        // # directory entries per page directory
#define NR_PTE      1024        // # PTEs per page table
#define PGSHFT      12          // log2(PGSIZE)
#define PTXSHFT     12          // Offset of PTX in a linear address
#define PDXSHFT     22          // Offset of PDX in a linear address

// Page table/directory entry flags
#define PTE_P       0x001       // Present
#define PTE_W       0x002       // Writeable
#define PTE_U       0x004       // User
#define PTE_PWT     0x008       // Write-Through
#define PTE_PCD     0x010       // Cache-Disable
#define PTE_A       0x020       // Accessed
#define PTE_D       0x040       // Dirty

// GDT entries
#define NR_SEG      6           // GDT size
#define SEG_KCODE   1           // Kernel code
#define SEG_KDATA   2           // Kernel data/stack
#define SEG_UCODE   3           // User code
#define SEG_UDATA   4           // User data/stack
#define SEG_TSS     5           // Global unique task state segement

// Selectors
#define KSEL(desc) (((desc) << 3) | DPL_KERN)
#define USEL(desc) (((desc) << 3) | DPL_USER)

// IDT size
#define NR_IRQ      256         // IDT size

// The following macros will not be seen by the assembler
#ifndef __ASSEMBLER__

#include "const.h"
#include "types.h"

// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/\------ OFF(va) ------/
typedef uint32_t PTE;
typedef uint32_t PDE;
#define PDX(va)         (((uint32_t)(va) >> PDXSHFT) & 0x3ff)
#define PTX(va)         (((uint32_t)(va) >> PTXSHFT) & 0x3ff)
#define OFF(va)         ((uint32_t)va & 0xfff)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uint32_t)((d) << PDXSHFT | (t) << PTXSHFT | (o)))

#define PGROUNDUP(sz)   (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a)  (((a)) & ~(PGSIZE-1))

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)

// Segment Descriptor
struct SegDesc {
	uint32_t lim_15_0 : 16;  // Low bits of segment limit
	uint32_t base_15_0 : 16; // Low bits of segment base address
	uint32_t base_23_16 : 8; // Middle bits of segment base address
	uint32_t type : 4;       // Segment type (see STS_ constants)
	uint32_t s : 1;          // 0 = system, 1 = application
	uint32_t dpl : 2;        // Descriptor Privilege Level
	uint32_t p : 1;          // Present
	uint32_t lim_19_16 : 4;  // High bits of segment limit
	uint32_t avl : 1;        // Unused (available for software use)
	uint32_t rsv1 : 1;       // Reserved
	uint32_t db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
	uint32_t g : 1;          // Granularity: limit scaled by 4K when set
	uint32_t base_31_24 : 8; // High bits of segment base address
};
typedef struct SegDesc SegDesc;

#define SEG(type, base, lim, dpl) (SegDesc)                   \
{	((lim) >> 12) & 0xffff, (uint32_t)(base) & 0xffff,        \
	((uint32_t)(base) >> 16) & 0xff, type, 1, dpl, 1,         \
	(uint32_t)(lim) >> 28, 0, 0, 1, 1, (uint32_t)(base) >> 24 }

#define SEG16(type, base, lim, dpl) (SegDesc)                 \
{	(lim) & 0xffff, (uint32_t)(base) & 0xffff,                \
	((uint32_t)(base) >> 16) & 0xff, type, 0, dpl, 1,         \
	(uint32_t)(lim) >> 16, 0, 0, 1, 0, (uint32_t)(base) >> 24 }

// Gate descriptors for interrupts and traps
struct GateDesc {
	uint32_t off_15_0 : 16;   // Low 16 bits of offset in segment
	uint32_t cs : 16;         // Code segment selector
	uint32_t args : 5;        // # args, 0 for interrupt/trap gates
	uint32_t rsv1 : 3;        // Reserved(should be zero I guess)
	uint32_t type : 4;        // Type(STS_{TG,IG32,TG32})
	uint32_t s : 1;           // Must be 0 (system)
	uint32_t dpl : 2;         // Descriptor(meaning new) privilege level
	uint32_t p : 1;           // Present
	uint32_t off_31_16 : 16;  // High bits of offset in segment
};
typedef struct GateDesc GateDesc;

#define GATE(type, cs, entry, dpl) (GateDesc)                 \
{	(uint32_t)(entry) & 0xffff, (cs), 0, 0, (type), 0, (dpl), \
	1, (uint32_t)(entry) >> 16 }


// Task state segment format
struct TSS {
	uint32_t link;         // Unused
	uint32_t esp0;         // Ring 0 Stack pointer and segment selector
	uint32_t ss0;
	char dontcare[88];
};
typedef struct TSS TSS;

// Interrupt and exception frame
struct TrapFrame {
	uint32_t edi, esi, ebp, esp_;
	uint32_t ebx, edx, ecx, eax;   // Register saved by pushal
	uint32_t gs, fs, es, ds;       // Segment register
	int      irq;                  // # of irq
	uint32_t err, eip, cs, eflags; // Execution state before trap 
	uint32_t esp, ss;              // Used only when returning to DPL=3
};
typedef struct TrapFrame TrapFrame;

#define MSG_SZ      300  
#define MSG_NUM     1000 
#define ANY         -1

#define STK_SZ      10000  
#define PCB_NUM     300 
 
// Enable external interrupt
static inline void
sti(void) {
	asm volatile("sti");
}

// Disable external interrupt
static inline void
cli(void) {
	asm volatile("cli");
}

// Put processor into idle until being interrupted
static inline void
wait_intr() {
	asm volatile("hlt");
}

// Fetch the value of %eflags
static inline uint32_t
readf(void) {
	volatile uint32_t eflags;
	asm volatile("pushf; pop %0": "=r"(eflags));
	return eflags;
}

// Fetch the value of %cr0
static inline uint32_t
get_cr0(void) {
	volatile uint32_t val;
	asm volatile("movl %%cr0, %0" : "=r"(val));
	return val;
}

// Write the value to %cr0
static inline void
set_cr0(uint32_t cr0) {
	asm volatile("movl %0, %%cr0" : : "r"(cr0));
}

// Write the value to %cr3 (PDBR)
static inline void
set_cr3(void *pdir) {
	asm volatile("movl %0, %%cr3" : : "r"(pdir));
}

// Create and load a GDT descriptor
static inline void
set_gdt(SegDesc *gdt, uint32_t size) {
	volatile static uint16_t data[3];
	data[0] = size - 1;
	data[1] = (uint32_t)gdt;
	data[2] = (uint32_t)gdt >> 16;
	asm volatile("lgdt (%0)" : : "r"(data));
}

// Create and load a IDT descriptor
static inline void
set_idt(GateDesc *idt, uint32_t size) {
	volatile static uint16_t data[3];
	data[0] = size - 1;
	data[1] = (uint32_t)idt;
	data[2] = (uint32_t)idt >> 16;
	asm volatile("lidt (%0)" : : "r"(data));
}

// Write the value of %tr (TSS selector)
//TSS任务状态段(Task State Segment)
static inline void
set_tr(uint16_t selector) {
	asm volatile("ltr %0" : : "r"(selector));
}

// Read from I/O port
static inline uint8_t
in_byte(uint16_t port) {
	uint8_t data;
	asm volatile("in %1, %0" : "=a"(data) : "d"(port));
	return data;
}

// Read from I/O port
static inline uint32_t
in_long(uint16_t port) {
	uint32_t data;
	asm volatile("in %1, %0" : "=a"(data) : "d"(port));
	return data;
}

// Write to I/O port
static inline void
out_byte(uint16_t port, uint8_t data) {
	asm volatile("out %%al, %%dx" : : "a"(data), "d"(port));
}

// Write to I/O port
static inline void
out_long(uint16_t port, uint32_t data) {
	asm volatile("out %%eax, %%dx" : : "a"(data), "d"(port));
}

#endif

#endif
