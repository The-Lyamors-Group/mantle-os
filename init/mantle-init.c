#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void make_dir(const char *path) { mkdir(path, 0755); }
static void mount_fs(const char *source, const char *target, const char *type, unsigned long flags) {
    make_dir(target); mount(source, target, type, flags, "size=32m");
}
static void log_line(const char *line) { int fd=open("/dev/console", O_WRONLY|O_NOCTTY); if(fd>=0){ dprintf(fd,"[mantle-init] %s\n",line); close(fd); } }
static void serial_line(const char *line) { int fd=open("/dev/ttyS0", O_WRONLY|O_NOCTTY|O_CLOEXEC); if(fd>=0){ dprintf(fd,"%s\n",line); close(fd); } }
static void reap(int sig) { (void)sig; while(waitpid(-1, NULL, WNOHANG)>0) {} }
static int cmdline_value(const char *key, char *out, size_t size) {
    FILE *f=fopen("/proc/cmdline","r"); if(!f) return 0; char line[2048];
    if(!fgets(line,sizeof(line),f)){fclose(f);return 0;} fclose(f);
    char *p=strstr(line,key); if(!p || (p!=line && p[-1]!=' ')) return 0; p+=strlen(key);
    char *e=strchr(p,' '); size_t n=e?(size_t)(e-p):strlen(p); if(n>=size)n=size-1; memcpy(out,p,n); out[n]=0; return 1;
}
static int mount_real_root(void) {
    char device[256]={0};
    if (cmdline_value("mantle.root=",device,sizeof(device))) {
        if (mount(device,"/newroot","ext4",0,"rw") == 0) return 0;
    }
    make_dir("/run/mantle-media");
    for (int i=0;i<50;i++) {
        if (mount("/dev/sr0","/run/mantle-media","iso9660",MS_RDONLY,"") == 0) {
            pid_t child=fork();
            if(child==0){execl("/bin/mount","mount","-t","ext4","-o","loop,ro","/run/mantle-media/boot/rootfs.ext4","/newroot",NULL);_exit(127);}
            int status=0;if(child>0)waitpid(child,&status,0);
            if(child>0 && WIFEXITED(status) && WEXITSTATUS(status)==0) return 0;
            umount("/run/mantle-media");
        }
        usleep(100000);
    }
    return -1;
}
static int switch_root(void) {
    make_dir("/newroot");
    if (mount_real_root() < 0) return -1;
    make_dir("/newroot/proc"); make_dir("/newroot/sys"); make_dir("/newroot/dev"); make_dir("/newroot/run");
    mount("/proc","/newroot/proc",NULL,MS_MOVE,NULL); mount("/sys","/newroot/sys",NULL,MS_MOVE,NULL);
    mount("/dev","/newroot/dev",NULL,MS_MOVE,NULL); mount("/run","/newroot/run",NULL,MS_MOVE,NULL);
    if (chdir("/newroot") < 0 || chroot(".") < 0 || chdir("/") < 0) return -1;
    char *const argv[]={(char*)"/sbin/init",NULL}; execv(argv[0],argv); return -1;
}

int main(void) {
    if (getpid() != 1) return 1;
    signal(SIGCHLD, reap);
    mount_fs("proc", "/proc", "proc", 0);
    mount_fs("sysfs", "/sys", "sysfs", 0);
    mount_fs("devtmpfs", "/dev", "devtmpfs", MS_NOSUID|MS_NOEXEC);
    mount_fs("devpts", "/dev/pts", "devpts", MS_NOSUID|MS_NOEXEC);
    mount_fs("tmpfs", "/run", "tmpfs", MS_NOSUID|MS_NODEV);
    make_dir("/tmp"); mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID|MS_NODEV, "mode=1777");
    serial_line("MANTLE_KERNEL_OK");
    serial_line("MANTLE_INIT_OK");
    int console=open("/dev/console", O_RDWR|O_NOCTTY);
    if (console >= 0) { dup2(console,0); dup2(console,1); dup2(console,2); if(console>2)close(console); }
    log_line("MantleOS init 0.1 démarré");
    if (switch_root() == 0) return 0;
    log_line("rootfs MantleOS indisponible, mode récupération");
    setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);
    setenv("HOME", "/root", 1); setenv("TERM", "linux", 1);
    make_dir("/root");
    pid_t splash=fork();
    if (splash==0) { execl("/sbin/mantle-splash", "mantle-splash", NULL); _exit(0); }
    char *const argv[]={(char*)"/sbin/mantle-supervise",NULL};
    execv(argv[0], argv);
    log_line(strerror(errno));
    reboot(RB_POWER_OFF); return 1;
}
