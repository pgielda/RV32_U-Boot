// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016-17 Microsemi Corporation.
 * Padmarao Begari, Microsemi Corporation <padmarao.begari@microsemi.com>
 *
 * Copyright (C) 2017 Andes Technology Corporation
 * Rick Chen, Andes Technology Corporation <rick@andestech.com>
 */

#include <common.h>
#include <asm/ptrace.h>
#include <asm/system.h>
#include <asm/encoding.h>

static void _exit_trap(int code, uint epc, struct pt_regs *regs);
static void handle_m_soft_interrupt(void);

extern void map(unsigned int, unsigned int, unsigned int);

static unsigned int location = 0;

void map_page(unsigned int phys, unsigned int virt, unsigned int flags)
{
	unsigned int entry = flags | phys;
	map(virt, entry, location++);
}

int interrupt_init(void)
{
	return 0;
}

/*
 * enable interrupts
 */
void enable_interrupts(void)
{
}

/*
 * disable interrupts
 */
int disable_interrupts(void)
{
	return 0;
}
uint handle_trap(uint mcause, uint epc, struct pt_regs *regs)
{
	uint is_int;

	is_int = (mcause & MCAUSE_INT);
	if ((is_int) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_EXT))
	{
		external_interrupt(0);	/* handle_m_ext_interrupt */
	}
	else if ((is_int) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_TIMER))
	{
		timer_interrupt(0);	/* handle_m_timer_interrupt */
	}
	else if ((is_int) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_SOFT))
	{
	      handle_m_soft_interrupt();
	}
	else if (!(is_int) && (((mcause & MCAUSE_CAUSE)  == 15) || (mcause & MCAUSE_CAUSE)  == 12))
	{
		unsigned long miss_addr = 0;
		unsigned int flags = 0xF8000000;
		unsigned int map_phys, map_virt;
		asm volatile("csrr %0, 0x343" : "=r"(miss_addr));
		printf("MMU Fault: Fault addr=0x%08lx, mcause:%02X, epc=0x%02X.\n", miss_addr, mcause, (uint32_t)epc);
		map_phys = ((miss_addr - 0x80000000) >> 12) & 0xfffff;
		map_virt = (miss_addr >> 12) & 0xfffff;
		map_page(map_phys, map_virt, flags);
	}
	else
		_exit_trap(mcause, epc, regs);

	return epc;
}

/*
 *Entry Point for PLIC Interrupt Handler
 */
__attribute__((weak)) void external_interrupt(struct pt_regs *regs)
{
}

__attribute__((weak)) void timer_interrupt(struct pt_regs *regs)
{
}
/**
 *
 */
static void handle_m_soft_interrupt(void)
{
	asm volatile ("li s1, 0x2000000\n\t"
	  "csrr s2, mhartid\n\t"
	  "slli s2, s2, 2\n\t"
	  "add s2, s2, s1\n\t"
	  "sw zero, 0(s2)\n\t");
}

static void _exit_trap(int code, uint epc, struct pt_regs *regs)
{
	static const char * const exception_code[] = {
		"Instruction address misaligned",
		"Instruction access fault",
		"Illegal instruction",
		"Breakpoint",
		"Load address misaligned"
	};

	printf("exception code: %d , %s , epc %08x , ra %08lx\n",
		code, exception_code[code], epc, regs->ra);
}
