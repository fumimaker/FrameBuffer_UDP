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

#define WIDTH   1280
#define HEIGHT  720
#define COLOR_DEPTH   3

#define false   0
#define true    1

#define SIZE_OF_DATA        1441
#define SIZE_OF_PAYLOAD     SIZE_OF_DATA - sizeof(int) // 1437
#define PACKET_TIMES        WIDTH*HEIGHT*COLOR_DEPTH / SIZE_OF_PAYLOAD //1925回
#define SIZE_OF_ID          sizeof(int)

int sd;
struct sockaddr_in addr;
socklen_t sin_size;
struct sockaddr_in from_addr;
char receiveBuff[2048]; // 受信バッファ
uint32_t *buf;

int OpenFrameBuffer(int);
void waitForNewframe(void);
int returnId(void);

void waitForNewframe(void){
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
    if ((buf = (uint32_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0){
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }

    waitForNewframe();

    int frame_end = false;
    int packetCounter = 0;
    char tempbuff[SIZE_OF_PAYLOAD * PACKET_TIMES];

    //メモリ確保 1437byte * 1925回
    //tempbuff = (char *)malloc(sizeof(char) * (SIZE_OF_PAYLOAD * 1925)); 

    while (!frame_end){
        int id = returnId(); //update packetBuff
        printf("id: %d\n\r", id);
        while (id > packetCounter){
            printf("id: %d, packetCounter: %d\n\r", id, packetCounter);
            packetCounter++;
        }

        int line_end = false;
        int counter = 0;
        while (!line_end){  //1パケット(1441byte)を処理するループ
            *(tempbuff + counter / 3) =
                (*(receiveBuff + SIZE_OF_ID + counter    ) << 16) |
                (*(receiveBuff + SIZE_OF_ID + counter + 1) << 8) |
                (*(receiveBuff + SIZE_OF_ID + counter + 2));
            printf("counter: %d\n\r", counter);

            // ID + counterが1441を超えたらライン終了
            if(counter+SIZE_OF_ID >= SIZE_OF_DATA){ 
                line_end = true;
                printf("line end\n\r");
            }
            else {
                counter += 3;
            }
        }

        //lineCounter >= 1925になったら終了
        if (packetCounter >= PACKET_TIMES){
            frame_end = true;
            printf("finish frame\n\r");
        }
        packetCounter++;
    }
    close(sd);

    int x = 0, y = 0;
    int r = 0, g = 0, b = 0;
    int pixelCounter = 0;
    while (1)
    {
        for (y = 0; y < yres; y++)
        {
            for (x = 0; x < xres; x++)
            {
                if ((x < WIDTH) || (y < HEIGHT)) {
                    r = *(tempbuff + pixelCounter);
                    g = *(tempbuff + pixelCounter + 1);
                    b = *(tempbuff + pixelCounter + 2);
                    pixelCounter += 3;
                }
                else {
                    r = 0;
                    g = 0;
                    b = 0;
                }
                // 7680/4 = 1920
                *(buf + ((y * line_len / 4) + x)) = (r << 16) | (g << 8) | (b); // 00 RR GG BB
            }
        }
    }

    munmap(buf, screensize);
    close(fd);
    free(tempbuff); //memory解放
    return 0;
}