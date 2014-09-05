#include "image3d/grayscale.h"
#include "image3d/binary.h"

#include "image3d/threshold.h"

void threshold(binary_t bin, grayscale_t gs, uint16_t tv) {
	for (int z = 0; z < gs->size_z; z++) {
		for (int y = 0; y < gs->size_y; y++) {
			for (int x = 0; x < gs->size_x; x++) {
				if (grayscale_at(gs, x, y, z) >= tv) {
					binary_at(bin, x, y, z) = 0xff;
				} else {
					binary_at(bin, x, y, z) = 0x00;
				}
			}
		}
	}
}
