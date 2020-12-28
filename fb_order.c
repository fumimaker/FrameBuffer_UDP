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

#include <time.h>

#define DEVICE_NAME "/dev/fb0"

#define UDP_PORT 5001

#define WIDTH 1280
#define HEIGHT 720
#define COLOR_DEPTH 3

#define false 0
#define true 1

#define SIZE_OF_DATA 1441
#define SIZE_OF_ID sizeof(int)
#define SIZE_OF_FRAME (WIDTH * HEIGHT * COLOR_DEPTH)

int sd;
struct sockaddr_in addr;
socklen_t sin_size;
struct sockaddr_in from_addr;
char receiveBuff[2048]; // 受信バッファ
uint32_t *buf;

int OpenFrameBuffer(int);
int returnId(void);

int returnId(void)
{
    recv(sd, receiveBuff, sizeof(receiveBuff), 0);
    return receiveBuff[0] << 24 | receiveBuff[1] << 16 | receiveBuff[2] << 8 | receiveBuff[3];
    //return ID
}

int OpenFrameBuffer(int fd)
{
    fd = open(DEVICE_NAME, O_RDWR);
    if (!fd)
    {
        fprintf(stderr, "cannot open the FrameBuffer '%s'\n", DEVICE_NAME);
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv)
{

    int fd = 0;
    int screensize;
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

    int xres = vinfo.xres;
    int yres = vinfo.yres;
    int bpp = vinfo.bits_per_pixel;
    int line_len = finfo.line_length;

    screensize = yres * xres * bpp / 8;
    printf("RECVFRAM Atlys Ver0.1\n%d(pixel)x%d(line), %d(bit per pixel), %d(line length)\n", xres, yres, bpp, line_len);

    /* Handler if socket get a packet, it will be mapped on memory */
    if ((buf = (uint32_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0)
    {
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }

    // IPv4 UDP のソケットを作成
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return -1;
    }

    memset(receiveBuff, 0, sizeof(receiveBuff));

    printf("waiting for new frame\n\r");

    while (returnId() != 1924)
    {
        printf(".");
    }
    printf("\n\r");

    int frame_end = false;
    char *p = (char *)buf; //PをTEMPにした。
    int finish = false;

    int frame = 0;
    int id = 0;
    int remain = 0;
    int local_id = 0;
    char *blank = (char *)calloc(1, sizeof(char) * 1437);

    printf("while start\n\r");

    while (1)
    {
        frame = recv(sd, receiveBuff, sizeof(receiveBuff), 0);
        if (frame != 1441) {
            printf("Error on udp. frame: %d\n\r", frame);
        }

        id = receiveBuff[2] << 8 | receiveBuff[3];

        // while (id > local_id) { //フレームロスしたら黒埋めしてるけど意味ある？
        //     remain = (local_id != 1924) ? 1437 : (1280 * 720 * 3) - (local_id * 1437);
        //     memcpy(p + (id * 1437), blank, remain);
        //     printf("blank: id=%d, local_id=%d\n\r", id, local_id);
        //     local_id++;
        // }

        //remain = (id != 1924) ? 1437 : (1280 * 720 * 3) - (id * 1437);
        if(id < 1924) {
            remain = 1437;
            local_id += 1;
        } else {
            remain = 12;//(1280 * 720 * 3) - (id * 1437); //500
            if(local_id != 1924) {
                printf("%d\n\r", local_id);
            }
            local_id = 0;
        }
        memcpy(p + (id * 1437), receiveBuff + SIZE_OF_ID, remain);
    }
    close(sd);

    printf("drawing finished.\n\r");

    munmap(buf, screensize);
    close(fd);
    return 0;
}