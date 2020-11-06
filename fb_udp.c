#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <linux/fb.h>
#include <linux/fs.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define DEVICE_NAME "/dev/fb0"

int sd;
struct sockaddr_in addr;
socklen_t sin_size;
struct sockaddr_in from_addr;
char receiveBuff[2048]; // 受信バッファ
uint32_t *buf;

int OpenFrameBuffer(int);
void udpReceive(void);
int returnId(void);

void udpReceive(void){
    // IPv4 UDP のソケットを作成
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    addr.sin_addr.s_addr = INADDR_ANY; 

    if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind");
        return -1;
    }

    memset(receiveBuff, 0, sizeof(receiveBuff));

    while (returnId() != -1);

    int packet_end = true;
    int counter = 0;
    int ptrCounter = 0;

    while (packet_end) {
        int id = returnId();//update packetBuff
        buf = ptrCounter + receiveBuff
        if(counter>1500){
            packet_end = false;
        }
        counter++;
        ptrCounter += 1440-sizeof(int);　//1436
    }
    
    close(sd);
}

int returnId(void){
    if (recvfrom(sd, receiveBuff, sizeof(receiveBuff), 0, (struct sockaddr *)&from_addr, &sin_size) < 0){
        perror("recvfrom");
        return -1;
    }
    return receiveBuff[0] << 24 | receiveBuff[1] << 16 | receiveBuff[2] << 8 | receiveBuff[3]; //return ID
}

int OpenFrameBuffer(int fd){
    fd = open(DEVICE_NAME, O_RDWR);
    if (!fd){
        fprintf(stderr, "cannot open the FrameBuffer '%s'\n", DEVICE_NAME);
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv){
    int fd = 0;
    int screensize;
    int x, y, col;
    int r, g, b;
    fd = OpenFrameBuffer(fd);

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo))
    {
        fprintf(stderr, "cannot open fix info\n");
        exit(1);
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo))
    {
        fprintf(stderr, "cannot open variable info\n");
        exit(1);
    }

    int xres, yres, bpp, line_len;
    xres = vinfo.xres;
    yres = vinfo.yres;
    bpp = vinfo.bits_per_pixel;
    line_len = finfo.line_length;

    screensize = yres * line_len * bpp / 8;
    printf("RECVFRAM Atlys Ver0.1\n%d(pixel)x%d(line), %d(bit per pixel), %d(line length)\n", xres, yres, bpp, line_len);

    /* Handler if socket get a packet, it will be mapped on memory */

    if ((buf = (uint32_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0)
    {
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }

    b = 0;
    while (1)
    {
        for (y = 0; y < yres; ++y)
        {
            for (x = 0; x < xres; ++x)
            {
                r = (x * 256 / xres);
                g = (y * 256 / yres);
                *(buf + ((y * line_len / 4) + x)) = (r << 16) | (g << 8) | (b); // 00 RR GG BB
            }
        }
        if (++b > 255)
        {
            b = 0;
        }
    }

    munmap(buf, screensize);
    close(fd);
    return 0;
}