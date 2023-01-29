#ifdef TARGET_PS2
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <malloc.h>
#include <libmc.h>
#include <sjis.h>

#include "ps2_memcard.h"

static int mc_Type, mc_Free, mc_Format;
extern unsigned int size_ps2_icon_data;
extern unsigned char ps2_icon_data;

#define ALIGN1K(x) (((x) + 1023) >> 10)
#define SAVE_SIZE (ALIGN1K(sizeof(mcIcon)) + ALIGN1K(size_ps2_icon_data) + 1 + 3)

static inline bool check_memory_card() {
	int ret;

	// Since this is the first call, -1 should be returned.
	mcGetInfo(0, 0, &mc_Type, &mc_Free, &mc_Format);
	mcSync(0, NULL, &ret);

	// Assuming that the same memory card is connected, this should return 0
	mcGetInfo(0, 0, &mc_Type, &mc_Free, &mc_Format);
	mcSync(0, NULL, &ret);

	return ret;
}

static inline bool check_save_file() {
	int fd;

	// Check if existing save is present
	fd = open("mc0:SM64/icon.sys", O_RDONLY);
	if(fd >= 0) {
		close(fd);
		return true;
	}

	return false;
}

static inline bool create_save_file(void) {
	int mc_fd;
	int icon_fd;
	mcIcon icon_sys;

	static iconIVECTOR bgcolor[4] = {
		{  68,  23, 116,  0 }, // top left
		{ 255, 255, 255,  0 }, // top right
		{ 255, 255, 255,  0 }, // bottom left
		{  68,  23, 116,  0 }, // bottom right
	};

	static iconFVECTOR lightdir[3] = {
		{ 0.5, 0.5, 0.5, 0.0 },
		{ 0.0,-0.4,-0.1, 0.0 },
		{-0.5,-0.5, 0.5, 0.0 },
	};

	static iconFVECTOR lightcol[3] = {
		{ 0.3, 0.3, 0.3, 0.00 },
		{ 0.4, 0.4, 0.4, 0.00 },
		{ 0.5, 0.5, 0.5, 0.00 },
	};

	static iconFVECTOR ambient = { 0.50, 0.50, 0.50, 0.00 };


	if(mkdir("mc0:SM64", 0777) < 0) return false;

	memset(&icon_sys, 0, sizeof(mcIcon));
	strcpy(icon_sys.head, "PS2D");
	strcpy_sjis((short *)&icon_sys.title, "Super\nMario 64");
	icon_sys.nlOffset = 16;
	icon_sys.trans = 0x60;
	memcpy(icon_sys.bgCol, bgcolor, sizeof(bgcolor));
	memcpy(icon_sys.lightDir, lightdir, sizeof(lightdir));
	memcpy(icon_sys.lightCol, lightcol, sizeof(lightcol));
	memcpy(icon_sys.lightAmbient, ambient, sizeof(ambient));
	strcpy(icon_sys.view, "sm64.icn"); // these filenames are relative to the directory
	strcpy(icon_sys.copy, "sm64.icn"); // in which icon.sys resides.
	strcpy(icon_sys.del, "sm64.icn");

	icon_fd = open("mc0:SM64/sm64.icn", O_WRONLY | O_CREAT);
	if(icon_fd < 0)
		return false;

	write(icon_fd, &ps2_icon_data, size_ps2_icon_data);
	close(icon_fd);
	printf("sm64.icn written sucessfully.\n");

	// Write icon.sys to the memory card (Note that this filename is fixed)
	mc_fd = open("mc0:SM64/icon.sys", O_WRONLY | O_CREAT);
	if(mc_fd < 0) return false;

	write(mc_fd, &icon_sys, sizeof(icon_sys));
	close(mc_fd);
	printf("icon.sys written sucessfully.\n");

	return true;
}

bool ps2_memcard_init(void) {
	// Initialize
	int ret;
	
	ret = init_memcard_driver(true);
	if(ret != 0) {
		printf("Failed to initialise memcard driver");
		return false;
	}

	ret = mcInit(MC_TYPE_MC);
	if (ret < 0) {
		printf("Failed to initialise memcard server");
		return false;
	}

	if(!check_memory_card())
		return false;

	return true;
}

bool ps2_memcard_load(void *data, const int ofs, const uint32_t size) {
	if(!check_memory_card())
		return false;

	if(!check_save_file())
		return false;

	int fd = open("mc0:SM64/save.bin", O_RDONLY);
    if (fd < 0)
		return false;
    
	if (lseek(fd, ofs, SEEK_SET) != (off_t)-1)
        read(fd, data, size);

    close(fd);

	return true;
}

bool ps2_memcard_save(const void *data, const int ofs, const uint32_t size) {
	if(!check_memory_card())
		return false;

	if(!check_save_file()) {
		if(!create_save_file())
			return false;
	}

	int fd = open("mc0:SM64/save.bin", O_WRONLY | O_CREAT);
    if (fd < 0)
		return false;
    
	if (lseek(fd, ofs, SEEK_SET) != (off_t)-1)
        write(fd, data, size);

    close(fd);

	return true;
}

#endif // TARGET_PS2
