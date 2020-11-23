#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#define PERIPHERAL_ADDRESS 0x3F000000
#define GPIO_ADDRESS (PERIPHERAL_ADDRESS + 0x00200000)
#define PAGE_SIZE 4096
#define GPFSEL0_OFFSET 0x0000
#define GPSET0_OFFSET 0x001C
#define GPCLR0_OFFSET 0x0028
#define GPLEV0_OFFSET 0x0034

#define DEVICE_FILE "/dev/mem"
#define MEMORY(addr) (*((volatile unsigned int*)(addr)))
#define GPFSEL0 MEMORY(address + GPFSEL0_OFFSET)
#define GPSET0 MEMORY(address + GPSET0_OFFSET)
#define GPCLR0 MEMORY(address + GPCLR0_OFFSET)

#define SEL_IN(n) (0b000 << (n * 3))
#define SEL_OUT(n) (0b001 << (n * 3))
#define SET(n) (1 << n)
#define CLR(n) (1 << n)

int main(int argc, char* argv[]) {
        unsigned int address;
        int fd;

  if ((fd = open(DEVICE_FILE, O_RDWR | O_SYNC)) < 0) {
    perror("failed to open()\n");
    return -1;
  }

  address = (unsigned int)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_ADDRESS);
  if (address == MAP_FAILED) {
    perror("failed with mmap()\n");
    close(fd);
    return -1;
  }

  GPFSEL0 = SEL_OUT(4);
  GPSET0 = SET(4);
  sleep(1);
  GPCLR0 = CLR(4);

  munmap((void*)address, PAGE_SIZE);
  close(fd);

  return 0;
}