#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define DEVICE_NAME "/dev/video0"
#define HEIGHT  720
#define WIDTH   1280
#define DEPTH   24
#define SCREENSIZE  (HEIGHT * WIDTH * (DEPTH/8))

int OpenFrameBuffer(int);


int OpenFrameBuffer(int fd){
    fd = open(DEVICE_NAME, O_RDWR);
    if (!fd)
    {
        fprintf(stderr, "cannot open the FrameBuffer '%s'\n", DEVICE_NAME);
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv){
    int fd = 0;
    uint32_t *buf;
    uint32_t screensize = SCREENSIZE;

    OpenFrameBuffer(fd);
    buf = (uint32_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if ( buf < 0){
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }
    return 0;
}