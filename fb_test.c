/*****************************************************
 *						     *
 *						     *
 *						     *
 *
 *  ### CHECK FIREWALLs on your system               *
 ****************************************************/		
​
​
/* incldue file for Standard Linbrary for C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
​
/* include file for socket communication */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
​
/* include file for FrameBuffer */
#include <linux/fb.h>
#include <linux/fs.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
​
/* DEFINE the Parameter */
#define DEVICE_NAME "/dev/fb0"
​
​
int OpenFrameBuffer(int fd){
     fd = open(DEVICE_NAME, O_RDWR);
     if(!fd){
	 fprintf(stderr,"cannot open the FrameBuffer '%s'\n",DEVICE_NAME);
	 exit(1);
     }
​
     return fd;
}
​
int main(int argc, char **argv)
{
     /* Open a DeviceFile of FrameBuffer */
     int fd = 0; 
     int screensize;
     int x,y,col;
     int r,g,b;
     fd = OpenFrameBuffer(fd);
     
     struct fb_var_screeninfo vinfo;
     struct fb_fix_screeninfo finfo;
​
     if(ioctl(fd,FBIOGET_FSCREENINFO, &finfo)){
	 fprintf(stderr, "cannot open fix info\n");
	 exit(1);
     }
     if(ioctl(fd,FBIOGET_VSCREENINFO, &vinfo)){
	 fprintf(stderr, "cannot open variable info\n");
	 exit(1);
     }
     
     int xres,yres,bpp,line_len;
     xres = vinfo.xres; yres = vinfo.yres; bpp = vinfo.bits_per_pixel;
     line_len = finfo.line_length;
​
     screensize = xres * line_len * bpp / 8;
     printf("RECVFRAM Atlys Ver0.1\n%d(pixel)x%d(line), %d(bit per pixel), %d(line length)\n",xres,yres,bpp,line_len);
     /* Handler if socket get a packet, it will be mapped on memory */ 
     
     uint32_t *buf;
​
     if( (buf = (uint32_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0){
	 fprintf(stderr, "cannot get framebuffer");
	 exit(1);
     }
​
     b=0;
     
     /* Loop for Recvfrom SOCKET UDP */
     while (1) {
	     for (y=0; y<yres; ++y) {
		for (x=0; x<xres; ++x) {
			r = (x * 256 / xres);
			g = (y * 256 / yres);
			*(buf + ((y * line_len/4) + x)) = (r << 16) | (g << 8) | (b);    // 00 RR GG BB
		}
	     }
	     if (++b > 255)
		     b = 0;
     }
     
     munmap(buf,screensize);
​
     close(fd);
​
     return 0;
}