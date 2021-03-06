/*
  This file is part of UFFS, the Ultra-low-cost Flash File System.
  
  Copyright (C) 2005-2009 Ricky Zheng <ricky_gz_zheng@yahoo.co.nz>

  UFFS is free software; you can redistribute it and/or modify it under
  the GNU Library General Public License as published by the Free Software 
  Foundation; either version 2 of the License, or (at your option) any
  later version.

  UFFS is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  or GNU Library General Public License, as applicable, for more details.
 
  You should have received a copy of the GNU General Public License
  and GNU Library General Public License along with UFFS; if not, write
  to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA  02110-1301, USA.

  As a special exception, if other files instantiate templates or use
  macros or inline functions from this file, or you compile this file
  and link it with other works to produce a work based on this file,
  this file does not by itself cause the resulting work to be covered
  by the GNU General Public License. However the source code for this
  file must still be made available in accordance with section (3) of
  the GNU General Public License v2.
 
  This exception does not invalidate any other reasons why a work based
  on this file might be covered by the GNU General Public License.
*/

/**
 * \file static-mem-allocate.c
 * \brief demostrate how to use static memory allocation. This example use 
 *        file emulated NAND flash.
 * \author Ricky Zheng
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uffs/uffs_config.h"
#include "uffs/uffs_public.h"
#include "uffs/uffs_fs.h"
#include "uffs/uffs_utils.h"
#include "uffs/uffs_core.h"
#include "uffs/uffs_mtb.h"
#include "cmdline.h"
#include "uffs_fileem.h"

#define PFX "static-example: "

#if CONFIG_USE_STATIC_MEMORY_ALLOCATOR == 0
int main()
{
	uffs_Perror(UFFS_ERR_NORMAL, "This example need CONFIG_USE_STATIC_MEMORY_ALLOCATOR = 1");
	return 0;
}
#else

extern struct cli_commandset * get_helper_cmds(void);

#define DEFAULT_EMU_FILENAME "uffsemfile.bin"

#define PAGE_DATA_SIZE    512
#define PAGE_SPARE_SIZE   16
#define PAGES_PER_BLOCK   32
#define TOTAL_BLOCKS      128

#define PAGE_SIZE					(PAGE_DATA_SIZE + PAGE_SPARE_SIZE)
#define BLOCK_DATA_SIZE				(PAGES_PER_BLOCK * PAGE_DATA_SIZE)
#define TOTAL_DATA_SIZE				(TOTAL_BLOCKS * BLOCK_DATA_SIZE)
#define BLOCK_SIZE					(PAGES_PER_BLOCK * PAGE_SIZE)
#define TOTAL_SIZE					(BLOCK_SIZE * TOTAL_BLOCKS)

#define MAX_MOUNT_TABLES		10
#define MAX_MOUNT_POINT_NAME	32

static uffs_Device demo_device = {0};
static struct uffs_MountTableEntrySt demo_mount = {
	&demo_device,
	0,    /* start from block 0 */
	-1,   /* use whole chip */
	"/",  /* mount point */
	NULL
};

static struct uffs_StorageAttrSt emu_storage = {0};
static struct uffs_FileEmuSt emu_private = {0};

/* static alloc the memory */
static int static_buffer_pool[UFFS_STATIC_BUFF_SIZE(PAGES_PER_BLOCK, PAGE_SIZE, TOTAL_BLOCKS) / sizeof(int)];


static void setup_emu_storage(struct uffs_StorageAttrSt *attr)
{
	attr->total_blocks = TOTAL_BLOCKS;			/* total blocks */
	attr->page_data_size = PAGE_DATA_SIZE;		/* page data size */
	attr->spare_size = PAGE_SPARE_SIZE;		  	/* page spare size */
	attr->pages_per_block = PAGES_PER_BLOCK;	/* pages per block */
	attr->block_status_offs = 4;				/* block status offset is 5th byte in spare */
    attr->ecc_opt = UFFS_ECC_SOFT;              /* ecc option */
    attr->layout_opt = UFFS_LAYOUT_UFFS;        /* let UFFS do the spare layout */    

}

static void setup_emu_private(uffs_FileEmu *emu)
{
	memset(emu, 0, sizeof(uffs_FileEmu));
	emu->emu_filename = DEFAULT_EMU_FILENAME;
}

static int init_uffs_fs(void)
{
	struct uffs_MountTableEntrySt *mtbl = &demo_mount;

	/* setup emu storage */
	setup_emu_storage(&emu_storage);
	setup_emu_private(&emu_private);
	emu_storage._private = &emu_private;
	mtbl->dev->attr = &emu_storage;

	/* setup memory allocator */
	uffs_MemSetupStaticAllocator(&mtbl->dev->mem, static_buffer_pool, sizeof(static_buffer_pool));

	/* setup device */
	uffs_fileem_setup_device(mtbl->dev);

	/* register mount table */
	uffs_RegisterMountTable(mtbl);

	return uffs_InitMountTable() == U_SUCC ? 0 : -1;
}

static int release_uffs_fs(void)
{
	return uffs_ReleaseMountTable();
}

int main(int argc, char *argv[])
{
	int ret;

	ret = init_uffs_fs();

	if (ret != 0) {
		printf ("Init file system fail: %d\n", ret);
		return -1;
	}

	cli_add_commandset(get_helper_cmds());
	cliMain();

	release_uffs_fs();

	return 0;
}

#endif

