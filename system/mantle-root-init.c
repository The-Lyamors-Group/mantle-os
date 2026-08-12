#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

static void ensure_mount(const char *src,const char *dst,const char *type,unsigned long flags,const char *data){mkdir(dst,0755);mount(src,dst,type,flags,data);}
static void serial_line(const char *line){int fd=open("/dev/ttyS0",O_WRONLY|O_NOCTTY|O_CLOEXEC);if(fd>=0){dprintf(fd,"%s\n",line);close(fd);}}
static void reap(int sig){(void)sig;while(waitpid(-1,NULL,WNOHANG)>0){}}
int main(void){
    if(getpid()!=1)return 1; signal(SIGCHLD,reap);
    dprintf(1,"[mantle-init] rootfs MantleOS monté, userspace actif\n");
    serial_line("MANTLE_ROOTFS_OK");
    ensure_mount("proc","/proc","proc",0,NULL); ensure_mount("sysfs","/sys","sysfs",0,NULL);
    ensure_mount("devtmpfs","/dev","devtmpfs",MS_NOSUID|MS_NOEXEC,NULL); ensure_mount("devpts","/dev/pts","devpts",MS_NOSUID|MS_NOEXEC,NULL);
    ensure_mount("tmpfs","/run","tmpfs",MS_NOSUID|MS_NODEV,"mode=755"); ensure_mount("tmpfs","/tmp","tmpfs",MS_NOSUID|MS_NODEV,"mode=1777");
    mkdir("/var",0755);mkdir("/var/log",0750);mkdir("/home",0755);
    int c=open("/dev/console",O_RDWR|O_NOCTTY);if(c>=0){dup2(c,0);dup2(c,1);dup2(c,2);if(c>2)close(c);}
    FILE *h=fopen("/etc/hostname","r");char hostname[128]={0};if(h){fgets(hostname,sizeof(hostname),h);fclose(h);for(char*p=hostname;*p;p++)if(*p=='\n')*p=0;if(hostname[0]){int fd=open("/proc/sys/kernel/hostname",O_WRONLY);if(fd>=0){write(fd,hostname,strlen(hostname));close(fd);}}}
    mkdir("/run/mantle",0770);chown("/run/mantle",1000,1000);mkfifo("/run/mantle/log",0620);
    execl("/sbin/mantle-supervise","mantle-supervise",NULL);return errno;
}
