#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void){
    const char *fifo="/run/mantle/log";const char *file="/var/log/mantleos.log";mkfifo(fifo,0620);
    int in=open(fifo,O_RDONLY);if(in<0)return 1;FILE*out=fopen(file,"a");if(!out)return 1;char line[1024];size_t bytes=0;
    while(1){ssize_t n=read(in,line,sizeof(line)-1);if(n<=0){close(in);in=open(fifo,O_RDONLY);continue;}line[n]=0;bytes+=n;if(bytes>1024*1024){fclose(out);rename(file,"/var/log/mantleos.log.1");out=fopen(file,"a");bytes=0;}fwrite(line,1,n,out);fflush(out);}
}
