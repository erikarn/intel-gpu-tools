/*
 * Copyright © 2008 Intel Corporation
 * Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/sysctl.h>

#include "intel_io.h"
#include "drmtest.h"
#include "igt_aux.h"

/**
 * intel_get_total_ram_mb:
 *
 * Returns:
 * The total amount of system RAM available in MB.
 */
uint64_t
intel_get_total_ram_mb(void)
{
	uint64_t physmem;
	size_t len = sizeof(physmem);

	if (sysctlbyname("hw.physmem", &physmem, &len, NULL, 0) < 0) {
		err(1, "Can't fetch hw.physmem");
	}

	return physmem / (1024*1024);
}

/**
 * intel_get_avail_ram_mb:
 *
 * Returns:
 * The amount of unused system RAM available in MB.
 */
uint64_t
intel_get_avail_ram_mb(void)
{
	uint64_t avail_pages;
	int pagesize, pageshift;
	size_t len;

	/* get the page size and calculate pageshift from it */
	pagesize = getpagesize();
	pageshift = 0;
	while (pagesize > 1) {
		pageshift++;
		pagesize >>= 1;
	}

	/* For now, return free */
	/* Later we should add inactive, cache */
	len = sizeof(avail_pages);
	if (sysctlbyname("vm.stats.vm.v_free_count", &avail_pages,
	    &len, NULL, 0) < 0) {
		err(1, "Can't fetch vm.stats.vm.v_free_count");
	}

	return ((avail_pages << pageshift) / (1024 * 1024));
}

/**
 * intel_get_total_swap_mb:
 *
 * Returns:
 * The total amount of swap space available in MB.
 */
uint64_t
intel_get_total_swap_mb(void)
{

	fprintf(stderr, "%s: called; unimplemented\n", __func__);
	/* For now, return 0 - fix it later */
	return 0;
}

/**
 * intel_require_memory:
 * @count: number of surfaces that will be created
 * @size: the size in bytes of each surface
 * @mode: a bitfield declaring whether the test will be run in RAM or in SWAP
 *
 * Computes the total amount of memory required to allocate @count surfaces,
 * each of @size bytes, and includes an estimate for kernel overhead. It then
 * queries the kernel for the available amount of memory on the system (either
 * RAM and/or SWAP depending upon @mode) and determines whether there is
 * sufficient to run the test.
 *
 * Most tests should check that there is enough RAM to hold their working set.
 * The rare swap thrashing tests should check that there is enough RAM + SWAP
 * for their tests. oom-killer tests should only run if this reports that
 * there is not enough RAM + SWAP!
 *
 * If there is not enough RAM this function calls igt_skip with an appropriate
 * message. It only ever returns if the requirement is fullfilled. This function
 * also causes the test to be skipped automatically on simulation under the
 * assumption that any test that needs to check for memory requirements is a
 * thrashing test unsuitable for slow simulated systems.
 */
void intel_require_memory(uint32_t count, uint32_t size, unsigned mode)
{
/* rough estimate of how many bytes the kernel requires to track each object */
#define KERNEL_BO_OVERHEAD 512
	uint64_t required, total;

	required = count;
	required *= size + KERNEL_BO_OVERHEAD;
	required = XALIGN(required, 4096);

	igt_debug("Checking %u surfaces of size %u bytes (total %llu) against %s%s\n",
		  count, size, (long long)required,
		  mode & (CHECK_RAM | CHECK_SWAP) ? "RAM" : "",
		  mode & CHECK_SWAP ? " + swap": "");

	total = 0;
	if (mode & (CHECK_RAM | CHECK_SWAP))
		total += intel_get_avail_ram_mb();
	if (mode & CHECK_SWAP)
		total += intel_get_total_swap_mb();
	total *= 1024 * 1024;

	igt_skip_on_f(total <= required,
		      "Estimated that we need %llu bytes for the test, but only have %llu bytes available (%s%s)\n",
		      (long long)required, (long long)total,
		      mode & (CHECK_RAM | CHECK_SWAP) ? "RAM" : "",
		      mode & CHECK_SWAP ? " + swap": "");

	igt_skip_on_simulation();
}

void
intel_purge_vm_caches(void)
{

	/* Not yet implemented */
	fprintf(stderr, "%s: not implemented\n", __func__);
}


/*
 * When testing a port to a new platform, create a standalone test binary
 * by running:
 * cc -o porttest intel_drm.c -I.. -DSTANDALONE_TEST `pkg-config --cflags libdrm`
 * and then running the resulting porttest program.
 */
#ifdef STANDALONE_TEST
void *mmio;

int main(int argc, char **argv)
{
    igt_info("Total RAM:  %"PRIu64" Mb\n", intel_get_total_ram_mb());
    igt_info("Total Swap: %"PRIu64" Mb\n", intel_get_total_swap_mb());

    return 0;
}
#endif /* STANDALONE_TEST */
