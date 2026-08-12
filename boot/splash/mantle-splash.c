#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

struct image { unsigned w, h; unsigned char *p; };
static void done(void){int f=open("/run/mantle-splash.done",O_WRONLY|O_CREAT,0644);if(f>=0)close(f);}
static int load_ppm(const char *name, struct image *im) {
    FILE *f=fopen(name,"rb"); if(!f) return -1; char magic[3]; unsigned max;
    if(fscanf(f,"%2s %u %u %u",magic,&im->w,&im->h,&max)!=4 || magic[0]!='P' || magic[1]!='6'){fclose(f);return -1;}
    fgetc(f); im->p=malloc(im->w*im->h*3); if(!im->p){fclose(f);return -1;}
    if(fread(im->p,3,im->w*im->h,f)!=(size_t)(im->w*im->h)){free(im->p);fclose(f);return -1;} fclose(f); return 0;
}
static unsigned char blend(unsigned char a,unsigned char b,unsigned t){return (unsigned char)(a+((int)b-a)*t/1000);}
int main(void) {
    int fb=open("/dev/fb0",O_RDWR); if(fb<0){done();return 0;} struct fb_var_screeninfo v; struct fb_fix_screeninfo fix;
    if(ioctl(fb,FBIOGET_VSCREENINFO,&v)<0 || ioctl(fb,FBIOGET_FSCREENINFO,&fix)<0 || v.bits_per_pixel<24){close(fb);done();return 0;}
    size_t size=fix.smem_len; unsigned char *mem=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,fb,0); if(mem==MAP_FAILED){close(fb);done();return 0;}
    struct image dark={0},light={0}; if(load_ppm("/usr/share/mantleos/mantleos-dark.ppm",&dark)||load_ppm("/usr/share/mantleos/mantleos-light.ppm",&light)){munmap(mem,size);close(fb);done();return 0;}
    for(unsigned frame=0;frame<150;frame++){
        unsigned t=frame<90?frame*1000/90:1000; unsigned ox=v.xres>dark.w?(v.xres-dark.w)/2:0, oy=v.yres>dark.h?(v.yres-dark.h)/2:0;
        for(unsigned y=0;y<dark.h && oy+y<v.yres;y++) for(unsigned x=0;x<dark.w && ox+x<v.xres;x++){
            size_t i=(y*dark.w+x)*3; unsigned char r=blend(dark.p[i],light.p[i],t),g=blend(dark.p[i+1],light.p[i+1],t),b=blend(dark.p[i+2],light.p[i+2],t);
            size_t o=(oy+y)*fix.line_length+(ox+x)*(v.bits_per_pixel/8); if(v.bits_per_pixel==32){mem[o]=b;mem[o+1]=g;mem[o+2]=r;mem[o+3]=0xff;}else{mem[o]=b;mem[o+1]=g;mem[o+2]=r;}
        }
        usleep(33333);
    }
    done();
    free(dark.p);free(light.p);munmap(mem,size);close(fb);return 0;
}
