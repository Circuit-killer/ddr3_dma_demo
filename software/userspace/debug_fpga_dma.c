#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define LWHPS2FPGA_BRIDGE_BASE 0xff200000
#define LWHPS2FPGA_BRIDGE_LEN 0x40

#define DMA_HPS2FPGA_BASE 0x20

#define DMA_STATUS 0
#define DMA_READADDR 1
#define DMA_WRITEADDR 2
#define DMA_LENGTH 3
#define DMA_CTRL 6

#define DMA_CTRL_WORD (1 << 2)
#define DMA_CTRL_GO (1 << 3)
#define DMA_CTRL_I_EN (1 << 4)
#define DMA_CTRL_REEN (1 << 5)
#define DMA_CTRL_WEEN (1 << 6)
#define DMA_CTRL_LEEN (1 << 7)
#define DMA_CTRL_RESET (1 << 12)

static volatile uint32_t *fpga_dma_iomem;

static void dump_hps2fpga_registers(void)
{
	printf("%x\n", fpga_dma_iomem[DMA_STATUS]);
	printf("%x\n", fpga_dma_iomem[DMA_READADDR]);
	printf("%x\n", fpga_dma_iomem[DMA_WRITEADDR]);
	printf("%x\n", fpga_dma_iomem[DMA_LENGTH]);
	printf("%x\n", fpga_dma_iomem[DMA_CTRL]);
}

static void run_hps2fpga_transfer(void)
{
	// write twice to reset
	fpga_dma_iomem[DMA_CTRL] = DMA_CTRL_RESET;
	fpga_dma_iomem[DMA_CTRL] = DMA_CTRL_RESET;

	printf("reset DMA controller\n");
	dump_hps2fpga_registers();

	// set up control register
	fpga_dma_iomem[DMA_CTRL] =
		DMA_CTRL_LEEN | DMA_CTRL_REEN | DMA_CTRL_WEEN |
		DMA_CTRL_I_EN | DMA_CTRL_WORD | DMA_CTRL_GO;
	// clear the status register
	fpga_dma_iomem[DMA_STATUS] = 0;
	printf("control setup, status cleared\n");

	// set the length
	fpga_dma_iomem[DMA_LENGTH] = 16;
	printf("length set\n");

	/*while (fpga_dma_iomem[DMA_HPS2FPGA_STATUS] & 0x2) {
		dump_hps2fpga_registers();
		sleep(1);
	}*/

	sleep(1);
	printf("done\n");
	dump_hps2fpga_registers();
	fpga_dma_iomem[DMA_STATUS] = 0;
}

int main(void)
{
	int memfd;
	int ret;
	volatile void *base_iomem;

	memfd = open("/dev/mem", O_RDWR);
	if (memfd < 0) {
		perror("open");
		return -1;
	}

	base_iomem = mmap(NULL, LWHPS2FPGA_BRIDGE_LEN,
				PROT_READ | PROT_WRITE,
				MAP_SHARED, memfd,
				LWHPS2FPGA_BRIDGE_BASE);
	if (base_iomem == MAP_FAILED) {
		perror("mmap");
		ret = -1;
		goto exit_close;
	}

	fpga_dma_iomem = (volatile uint32_t *) (base_iomem + DMA_HPS2FPGA_BASE);

	run_hps2fpga_transfer();

	ret = 0;
	if (munmap((void *) base_iomem, LWHPS2FPGA_BRIDGE_LEN) < 0) {
		ret = -1;
		perror("munmap");
	}
exit_close:
	if (close(memfd) < 0) {
		ret = -1;
		perror("close");
	}

	return ret;
}
