#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <grp.h>
#define LOG_FIFO "/run/mantle/log"
#define STOP_FILE "/run/mantle/session.stop"

static volatile sig_atomic_t stopping;
static void stop(int sig) { (void)sig; stopping = 1; }
static void serial_line(const char *line){int fd=open("/dev/ttyS0",O_WRONLY|O_NOCTTY|O_CLOEXEC);if(fd>=0){dprintf(fd,"%s\n",line);close(fd);}}
static int runv(char *const argv[]){pid_t p=fork();if(p==0){execv(argv[0],argv);_exit(127);}if(p<0)return 127;int s=0;while(waitpid(p,&s,0)<0&&errno==EINTR){}return WIFEXITED(s)?WEXITSTATUS(s):128;}
static void log_message(const char *msg) { int f=open(LOG_FIFO,O_WRONLY|O_NONBLOCK); if(f>=0){dprintf(f,"mantle-supervise: %s\n",msg);close(f);} }
static long pid_file(const char *name){char path[128];snprintf(path,sizeof(path),"/run/mantle/%s.pid",name);FILE*f=fopen(path,"r");long p=0;if(f){fscanf(f,"%ld",&p);fclose(f);}return p;}
static void set_pid_file(const char *name,pid_t pid){char path[128];snprintf(path,sizeof(path),"/run/mantle/%s.pid",name);int f=open(path,O_WRONLY|O_CREAT|O_TRUNC,0644);if(f>=0){dprintf(f,"%ld\n",(long)pid);close(f);}}
static void ensure_service(const char *name,const char *path){long old=pid_file(name);if(old>0&&kill((pid_t)old,0)==0)return;pid_t p=fork();if(p==0){execl(path,path,NULL);_exit(127);}if(p>0){set_pid_file(name,p);char msg[128];snprintf(msg,sizeof(msg),"service %s démarré",name);log_message(msg);}}
static int service_ready(const char *name){long p=pid_file(name);return p>0&&kill((pid_t)p,0)==0;}
static void run_boot_checks(void){
    char *version[]={(char*)"/bin/mantle-shell",(char*)"--version",NULL};
    char *command[]={(char*)"/bin/mantle-shell",(char*)"-c",(char*)"true",NULL};
    if(runv(version)==0&&runv(command)==0)serial_line("MANTLE_SHELL_OK");
    char *script[]={(char*)"/usr/bin/mantle-script",(char*)"/usr/share/mantleos/tests/boot.mt",NULL};
    if(runv(script)==0)serial_line("MANTLE_MT_OK");
    char *commands[]={(char*)"/usr/bin/mantle-command",(char*)"/usr/share/mantleos/tests/boot.mtc",NULL};
    if(runv(commands)==0)serial_line("MANTLE_MTC_OK");
}

int main(void) {
    if (getpid() != 1) return 1;
    signal(SIGTERM, stop); signal(SIGINT, stop);
    for (int i=0; i<60 && !stopping; i++) { if (access("/run/mantle-splash.done", F_OK)==0) break; usleep(100000); }
    int pidfd=open("/run/mantle-supervise.pid",O_WRONLY|O_CREAT|O_TRUNC,0644); if(pidfd>=0){dprintf(pidfd,"%ld\n",(long)getpid());close(pidfd);}
    ensure_service("log","/sbin/mantle-logd"); ensure_service("network","/sbin/mantle-network");
    int services_ready=0;for(int i=0;i<100&&!stopping;i++){if(service_ready("log")&&service_ready("network")){services_ready=1;break;}usleep(100000);}
    if(services_ready)serial_line("MANTLE_SERVICES_OK");
    fprintf(stderr, "[mantle-supervise] supervision console active\n");
    log_message("supervision console active");
    if(services_ready)run_boot_checks();
    while (!stopping) {
        ensure_service("log","/sbin/mantle-logd"); ensure_service("network","/sbin/mantle-network");
        if (access(STOP_FILE, F_OK) == 0) { usleep(100000); continue; }
        pid_t child = fork();
        if (child == 0) {
            if (getuid() == 0) { gid_t groups[]={1000}; setgroups(1,groups); setgid(1000); setuid(1000); setenv("HOME", "/home/mantle", 1); }
            char *const argv[] = {(char*)"/bin/sh", (char*)"-l", NULL};
            execv(argv[0], argv); _exit(127);
        }
        if (child < 0) { sleep(1); continue; }
        int pidfd=open("/run/mantle-session.pid",O_WRONLY|O_CREAT|O_TRUNC,0644); if(pidfd>=0){dprintf(pidfd,"%ld\n",(long)child);close(pidfd);}
        for (;;) { int status=0; pid_t done=waitpid(-1,&status,WNOHANG); if(done==child)break; if(done<0&&errno!=EINTR)break; ensure_service("log","/sbin/mantle-logd"); ensure_service("network","/sbin/mantle-network"); usleep(100000); }
        unlink("/run/mantle-session.pid");
        if (!stopping) sleep(1);
    }
    unlink("/run/mantle-supervise.pid"); unlink("/run/mantle-session.pid");
    sync(); return 0;
}
