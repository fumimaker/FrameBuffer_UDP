#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wiringPi.h>

#define PIN1    21
#define PIN2    20
#define PIN3    16

#define WIDTH 1280
#define HEIGHT 720
#define COLOR_DEPTH 3

#define false 0
#define true 1

#define SIZE_OF_DATA 1441
#define SIZE_OF_ID sizeof(int)
#define SIZE_OF_FRAME (WIDTH * HEIGHT * COLOR_DEPTH)

#define FB_NAME "/dev/fb0"
#define VIDEO_NAME "/dev/video0"

#define PORT 5001
#define ADDR "192.168.11.242"

void startCapture();
void copyBuffer(uint8_t *dstBuffer, uint32_t *size);
void stopCapture();
int saveFileBinary(const char *filename, uint8_t *data, int size);

int fd;
#define v4l2BufferNum 2
void *v4l2Buffer[v4l2BufferNum];
uint32_t v4l2BufferSize[v4l2BufferNum];

int main()
{
    wiringPiSetupGpio();
    pinMode(PIN1, OUTPUT);
    pinMode(PIN2, OUTPUT);
    pinMode(PIN3, OUTPUT);

    uint8_t buff[WIDTH * HEIGHT * COLOR_DEPTH];
    uint32_t size;
    int screensize;
    int fd_fb;

    int sd;
    struct sockaddr_in addr;

    /*     Socketの設定をする    */
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(ADDR);

    /*     Frame Bufferの設定をする    */
    fd_fb = open(FB_NAME, O_RDWR);
    if (!fd_fb)
    {
        fprintf(stderr, "cannot open the FrameBuffer '%s'\n", FB_NAME);
        exit(1);
    }
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &finfo))
    {
        fprintf(stderr, "cannot open fix info\n");
        exit(1);
    }
    if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &vinfo))
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

    uint32_t *framebuf;
    if ((framebuf = (uint32_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0)) < 0)
    {
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }

    /*     /dev/video0とV4L2を使ったカメラの設定をする    */
    startCapture();

    /*      カメラ受信と出力スタート        */
    printf("send start\n\r");
    int *linebuffer = (int *)malloc(sizeof(char) * 1441);
    while (1)
    {
        
        copyBuffer(buff, &size);
        //char *p = buff; //data
        
        for (int i = 0; i < 1925; i++)
        {
            if(i==0){
                digitalWrite(PIN2, HIGH);
            }
            
            linebuffer[0] = htonl(i);
            memcpy(linebuffer + 1, buff + (i * 1437), 1437);
            if (sendto(sd, linebuffer, 1441, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                perror("sendto");
                return -1;
            }

            if (i == 0) {
                digitalWrite(PIN2, LOW);
            }

            //printf("id: %d\n\r", linebuffer[0]);
        }
        //digitalWrite(PIN2, LOW);
        //char *pointer = (char *)framebuf;
        //memcpy((char *)framebuf, buff, WIDTH*HEIGHT*COLOR_DEPTH);
    }

    //stopCapture();
    //saveFileBinary("PiCamera.jpg", buff, size);
    free(linebuffer);
    close(fd_fb);
    close(sd);
    return 0;
}

void startCapture()
{
    fd = open(VIDEO_NAME, O_RDWR);

    /* 1. フォーマット指定。320 x 240のJPEG形式でキャプチャしてください */
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    ioctl(fd, VIDIOC_S_FMT, &fmt);

    /* 2. バッファリクエスト。バッファを2面確保してください */
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = v4l2BufferNum;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ioctl(fd, VIDIOC_REQBUFS, &req);

    /* 3. 確保されたバッファをユーザプログラムからアクセスできるようにmmapする */
    struct v4l2_buffer buf;
    for (uint32_t i = 0; i < v4l2BufferNum; i++)
    {
        /* 3.1 確保したバッファ情報を教えてください */
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ioctl(fd, VIDIOC_QUERYBUF, &buf);

        /* 3.2 取得したバッファ情報を基にmmapして、後でアクセスできるようにアドレスを保持っておく */
        v4l2Buffer[i] = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, fd, buf.m.offset);
        v4l2BufferSize[i] = buf.length;
    }

    /* 4. バッファのエンキュー。指定するバッファをキャプチャするときに使ってください */
    for (uint32_t i = 0; i < v4l2BufferNum; i++)
    {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ioctl(fd, VIDIOC_QBUF, &buf);
    }

    /* 5. ストリーミング開始。キャプチャを開始してください */
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    /* この例だと2面しかないので、2フレームのキャプチャ(1/30*2秒?)が終わった後、新たにバッファがエンキューされるまでバッファへの書き込みは行われない */
}

void copyBuffer(uint8_t *dstBuffer, uint32_t *size)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    digitalWrite(PIN1, HIGH);
    /* 6. バッファに画データが書き込まれるまで待つ */
    while (select(fd + 1, &fds, NULL, NULL, NULL) < 0);
    if (FD_ISSET(fd, &fds))
    {
        /* 7. バッファのデキュー。もっとも古くキャプチャされたバッファをデキューして、そのインデックス番号を教えてください */
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ioctl(fd, VIDIOC_DQBUF, &buf);

        /* 8. デキューされたバッファのインデックス(buf.index)と書き込まれたサイズ(buf.byteused)が返ってくる */
        *size = buf.bytesused;

        /* 9. ユーザプログラムで使いやすいように、別途バッファにコピーしておく */
        memcpy(dstBuffer, v4l2Buffer[buf.index], buf.bytesused);

        /* 10. 先ほどデキューしたバッファを、再度エンキューする。カメラデバイスはこのバッファに対して再びキャプチャした画を書き込む */
        ioctl(fd, VIDIOC_QBUF, &buf);
    }
    digitalWrite(PIN1, LOW);
}

void stopCapture()
{
    /* 11. ストリーミング終了。キャプチャを停止してください */
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);

    /* 12. リソース解放 */
    for (uint32_t i = 0; i < v4l2BufferNum; i++)
        munmap(v4l2Buffer[i], v4l2BufferSize[i]);

    /* 13. デバイスファイルを閉じる */
    close(fd);
}

int saveFileBinary(const char *filename, uint8_t *data, int size)
{
    FILE *fp;
    fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        return -1;
    }
    fwrite(data, sizeof(uint8_t), size, fp);
    fclose(fp);
    return 0;
}