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

//#define SIZE_OF_DATA 1441
#define SIZE_OF_DATA        1450
#define SIZE_OF_ID          2
#define SIZE_OF_datasize    4
#define SIZE_OF_HEADER      SIZE_OF_ID + SIZE_OF_datasize
#define SIZE_OF_FRAME       (WIDTH * HEIGHT * COLOR_DEPTH)

//1915になるはず
#define END_OF_ID (SIZE_OF_FRAME/(SIZE_OF_DATA-(SIZE_OF_HEADER)))+1

int sd;
struct sockaddr_in addr;
socklen_t sin_size;
struct sockaddr_in from_addr;
char receiveBuff[2048]; // 受信バッファ
uint32_t *buf;

int OpenFrameBuffer(int);
uint16_t returnId(void);

//パケットIDを返す
uint16_t returnId(void)
{
    recv(sd, receiveBuff, sizeof(receiveBuff), 0);
    return receiveBuff[0] << 8 | receiveBuff[1] << 0;
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

    int frame_end = false;
    char *p = (char *)buf; //PをTEMPにした。
    int finish = false;

    uint32_t frame = 0;
    uint32_t id = 0;
    uint32_t datasize = 0;
    uint32_t remain = 0;
    uint32_t pixel = 0;
    char *blank = (char *)calloc(1, sizeof(char)*SIZE_OF_DATA-SIZE_OF_HEADER);
    int init_done = 0;
    printf("while start\n\r");
    uint32_t id_init;
    do { //最初のIDじゃなかったら読み飛ばす(待つ)
        frame = recv(sd, receiveBuff, sizeof(receiveBuff), 0);
        id_init = receiveBuff[0] << 8 | receiveBuff[1] << 0;
        printf(".");
    } while(id_init != 0);
    printf("\n\r");
    id = id_init;
    init_done = 0;

    while (1){
        if(init_done==0){ //最初だけ読み込みが終わっているので最初だけは読まない
            init_done = 1;
        } else {
            frame = recv(sd, receiveBuff, sizeof(receiveBuff), 0); 
            id = receiveBuff[0] << 8 | receiveBuff[1] << 0;
        }

        if(frame!=SIZE_OF_DATA){ //1450byte
            printf("Error on udp. frame: %d\n\r",frame);
        }

        datasize = 
            receiveBuff[2] << 24 |
            receiveBuff[3] << 16 |
            receiveBuff[4] << 8  |
            receiveBuff[5] << 0;

        // while (id > pixel) { //フレームロスしたら黒埋めしてるけど意味ある？
        //     remain = (pixel != 1924) ? SIZE_OF_DATA-SIZE_OF_ID : (1280 * 720 * 3) - (pixel * SIZE_OF_DATA-SIZE_OF_ID);
        //     memcpy(p + (id * SIZE_OF_DATA-SIZE_OF_ID), blank, remain);
        //     printf("blank: id=%d, pixel=%d\n\r", id, pixel);
        //     pixel++;
        // }
        remain = (id != END_OF_ID) ? SIZE_OF_DATA-SIZE_OF_HEADER : datasize - (id * (SIZE_OF_DATA-SIZE_OF_HEADER));
        memcpy(p + (id * (SIZE_OF_DATA-SIZE_OF_HEADER)), receiveBuff + SIZE_OF_HEADER, remain);
        pixel += 1;
    }
    close(sd);

    printf("drawing finished.\n\r");

    munmap(buf, screensize);
    close(fd);
    return 0;
}