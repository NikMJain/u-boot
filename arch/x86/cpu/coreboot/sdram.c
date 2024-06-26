// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2010,2011
 * Graeme Russ, <graeme.russ@gmail.com>
 */

#include <init.h>
#include <asm/e820.h>
#include <asm/cb_sysinfo.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

unsigned int install_e820_map(unsigned int max_entries,
			      struct e820_entry *entries)
{
	return cb_install_e820_map(max_entries, entries);
}

/*
 * This function looks for the highest region of memory lower than 4GB which
 * has enough space for U-Boot where U-Boot is aligned on a page boundary. It
 * overrides the default implementation found elsewhere which simply picks the
 * end of ram, wherever that may be. The location of the stack, the relocation
 * address, and how far U-Boot is moved by relocation are set in the global
 * data structure.
 */
phys_addr_t board_get_usable_ram_top(phys_size_t total_size)
{
	uintptr_t dest_addr = 0;
	int i;

	for (i = 0; i < lib_sysinfo.n_memranges; i++) {
		struct memrange *memrange = &lib_sysinfo.memrange[i];
		/* Force U-Boot to relocate to a page aligned address. */
		uint64_t start = roundup(memrange->base, 1 << 12);
		uint64_t end = memrange->base + memrange->size;

		/* Ignore non-memory regions. */
		if (memrange->type != CB_MEM_RAM)
			continue;

		/* Filter memory over 4GB. */
		if (end > 0xffffffffULL)
			end = 0x100000000ULL;
		/* Skip this region if it's too small. */
		if (end - start < total_size)
			continue;

		/* Use this address if it's the largest so far. */
		if (end > dest_addr)
			dest_addr = end;
	}

	/* If no suitable area was found, return an error. */
	if (!dest_addr)
		panic("No available memory found for relocation");

	return (ulong)dest_addr;
}

int dram_init(void)
{
	int i;
	phys_size_t ram_size = 0;

	for (i = 0; i < lib_sysinfo.n_memranges; i++) {
		struct memrange *memrange = &lib_sysinfo.memrange[i];
		unsigned long long end = memrange->base + memrange->size;

		if (memrange->type == CB_MEM_RAM && end > ram_size)
			ram_size += memrange->size;
	}

	gd->ram_size = ram_size;
	if (ram_size == 0)
		return -1;

	return 0;
}

int dram_init_banksize(void)
{
	int i, j;

	if (CONFIG_NR_DRAM_BANKS) {
		for (i = 0, j = 0; i < lib_sysinfo.n_memranges; i++) {
			struct memrange *memrange = &lib_sysinfo.memrange[i];

			if (memrange->type == CB_MEM_RAM) {
				gd->bd->bi_dram[j].start = memrange->base;
				gd->bd->bi_dram[j].size = memrange->size;
				j++;
				if (j >= CONFIG_NR_DRAM_BANKS)
					break;
			}
		}
	}

	return 0;
}
