/** \file
 * Drive a Mac 128/Plus/SE video output from the xvfb
 *
 * apt-get install x11proto-core-dev
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <byteswap.h>
#include <X11/XWDFile.h>
#include "pru.h"

#define VRAM_WIDTH 512
#define VRAM_HEIGHT 384

static uint8_t threshold = 0x80;

void
fb_copy(
	void * const fb,
	const XWDFileHeader * const xfb
)
{
	const XWDColor * const xfb_colors
		= (const void*)((uintptr_t) xfb) + be32toh(xfb->header_size);
	const uint8_t * const xfb_data
		= (const uint8_t*)(xfb_colors + be32toh(xfb->ncolors));
/*
	// \todo: What is the correct offset?
	const uint8_t * const xfb_data
		= ((const uint8_t*)xfb_colors) + be32toh(xfb->ncolors)+0x20;
*/
	const size_t line_size = be32toh(xfb->bytes_per_line);

	static int first;
	if (first++ == 0)
		printf("offset %zx %zx\n",
			xfb_data - (const uint8_t*) xfb,
			((const uint8_t*)xfb_colors) - (const uint8_t*) xfb
		);

	//memset(fb, 0, VRAM_WIDTH*VRAM_HEIGHT/8);

	for (int y = 0 ; y < VRAM_HEIGHT ; y++)
	{
		const uint8_t * const xfb_row = xfb_data + y * line_size;
		uint8_t * const fb_row = ((uint8_t*)fb) + y * VRAM_WIDTH/8;

		for (int x = 0 ; x < VRAM_WIDTH ; x += 8)
		{
			uint8_t pix = 0;

			for (int x2 = 0 ; x2 < 8 ; x2++)
			{
				uint8_t p = xfb_row[(x + x2)];
#if 0
				const XWDColor * const c = &xfb_colors[p];
				const uint16_t r = c->red;
				const uint16_t g = c->green;
				const uint16_t b = c->blue;
				const uint8_t n1 = be32toh(c->pixel);
				const uint8_t n2 = (r+g+b) / (3*256);
				if (x == 128 && x2 == 0 && y == 100)
					printf("%08x %08x %04x %04x %04x => %02x\n",
						p,
						n1,
						r,
						g,
						b,
						n2
					);
#else
				// fixed scale?
				if (p == 1)
					p = 0xFF;
				else
				if (p != 0)
					p = p - 1;
#endif
			
				pix >>= 1;
				if (p > threshold)
					pix |= 0x80;
			}

			fb_row[x / 8] = pix;
		}
	}
}



int
main(
	int argc,
	char ** argv
)
{
	pru_t * const pru = pru_init(0);

	uint32_t * const pru_cmd = pru->data_ram;
	uint8_t * const vram = pru->ddr;

	pru_gpio(0, 22, 1, 1);
	pru_gpio(0, 23, 1, 1);
	pru_gpio(0, 27, 1, 1);

	const size_t fb_size = VRAM_WIDTH*VRAM_HEIGHT/8;
	memset(vram, 0x00, 2 * fb_size);
	void * const fb1 = vram + 0 * fb_size;
	void * const fb2 = vram + 1 * fb_size;
	const uintptr_t fb1_ddr = pru->ddr_addr + 0 * fb_size;
	const uintptr_t fb2_ddr = pru->ddr_addr + 1 * fb_size;

	pru_exec(pru, "./macvideo.bin");
	printf("pru %p\n", pru);
	printf("cmd %p\n", pru_cmd);
	printf("ddr %p (%08x)\n", vram, pru->ddr_addr);

	if (argc <= 1)
		die("usage: %s /path/to/Xfb\n", argv[0]);
	const char * const xfb_name = argv[1];
	int fd = open(xfb_name, O_RDONLY, 0666);
	if (fd < 0)
		die("%s: Unable to open\n", xfb_name);
	
	struct stat sb;
	if (fstat(fd, &sb) < 0)
		die("%s: Unable to stat\n", xfb_name);
	const size_t xfb_len = sb.st_size;

	//if (sb.st_size != VRAM_WIDTH*VRAM_HEIGHT)

	const XWDFileHeader * const xfb = mmap(
		NULL,
		xfb_len,
		PROT_READ,
		MAP_SHARED,
		fd,
		0
	);
	if (xfb == MAP_FAILED)
		die("%s: unable to map\n", xfb_name);
	close(fd);

	const char * const win_name = (const char *)(xfb + 1);
	const size_t line_size = be32toh(xfb->bytes_per_line);

	printf("XWD '%s' fmt=%d.%d.%d %dx%dx%d, %d colors @ %d, %zu bytes per line\n",
		win_name,
		be32toh(xfb->file_version),
		be32toh(xfb->pixmap_format),
		be32toh(xfb->visual_class),
		be32toh(xfb->pixmap_width),
		be32toh(xfb->pixmap_height),
		be32toh(xfb->pixmap_depth),
		be32toh(xfb->ncolors),
		be32toh(xfb->byte_order),
		line_size
	);

	while (1)
	{
		fb_copy(fb1, xfb);
		*pru_cmd = fb1_ddr;
		usleep(30000);

		fb_copy(fb2, xfb);
		*pru_cmd = fb2_ddr;
		usleep(30000);
	}

	return 0;
}
