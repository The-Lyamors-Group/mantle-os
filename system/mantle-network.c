#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

static char *value(const char *key, char *out, size_t n) {
    FILE *f=fopen("/etc/mantleos/network.conf","r"); if(!f)return NULL; char line[256];
    while(fgets(line,sizeof(line),f)){char *eq=strchr(line,'=');if(!eq)continue;*eq=0;char *v=eq+1;v[strcspn(v,"\r\n")]=0;if(!strcmp(line,key)){snprintf(out,n,"%s",v);fclose(f);return out;}}
    fclose(f);return NULL;
}
static int run(char *const argv[]){pid_t p=fork();if(p==0){execv(argv[0],argv);_exit(127);}int s=0;if(p>0)waitpid(p,&s,0);return p<0?-1:(WIFEXITED(s)?WEXITSTATUS(s):1);}
static int output_contains(char *const argv[],const char *needle){int fds[2];if(pipe(fds)<0)return 0;pid_t p=fork();if(p==0){dup2(fds[1],STDOUT_FILENO);close(fds[0]);close(fds[1]);execv(argv[0],argv);_exit(127);}if(p<0){close(fds[0]);close(fds[1]);return 0;}close(fds[1]);char buf[512];ssize_t n;int found=0;size_t carry=0;while((n=read(fds[0],buf+carry,sizeof(buf)-carry-1))>0){buf[carry+(size_t)n]=0;if(strstr(buf,needle))found=1;carry=0;}close(fds[0]);int status=0;waitpid(p,&status,0);return found&&WIFEXITED(status)&&WEXITSTATUS(status)==0;}
static void serial_line(const char *line){int fd=open("/dev/ttyS0",O_WRONLY|O_NOCTTY|O_CLOEXEC);if(fd>=0){dprintf(fd,"%s\n",line);close(fd);}}
int main(void){
    char mode[32]="dhcp", iface[32]="eth0", address[64]={0}, gateway[64]={0}, dns[128]={0};
    value("mode",mode,sizeof(mode));value("interface",iface,sizeof(iface));value("address",address,sizeof(address));value("gateway",gateway,sizeof(gateway));value("dns",dns,sizeof(dns));
    char *up[]={(char*)"/bin/ip",(char*)"link",(char*)"set",(char*)"dev",iface,(char*)"up",NULL};if(run(up)!=0)return 1;
    char *addresses[]={(char*)"/bin/ip",(char*)"addr",(char*)"show",(char*)"dev",iface,NULL};
    char *routes[]={(char*)"/bin/ip",(char*)"route",(char*)"show",(char*)"default",NULL};
    if(!strcmp(mode,"static") && address[0]){
        char *addr[]={(char*)"/bin/ip",(char*)"addr",(char*)"replace",address,(char*)"dev",iface,NULL};if(run(addr)!=0||!gateway[0])return 1;
        char *route[]={(char*)"/bin/ip",(char*)"route",(char*)"replace",(char*)"default",(char*)"via",gateway,(char*)"dev",iface,NULL};if(run(route)!=0)return 1;
        if(dns[0]){FILE*f=fopen("/etc/resolv.conf","w");if(f){fprintf(f,"nameserver %s\n",dns);fclose(f);}}
        if(!output_contains(addresses,"inet ")||!output_contains(routes,"default"))return 1;
        serial_line("MANTLE_NETWORK_OK"); for(;;) pause();
    }
    char *dhcp[]={(char*)"/bin/udhcpc",(char*)"-i",iface,(char*)"-s",(char*)"/etc/mantleos/dhcp.script",NULL};
    pid_t child=fork();if(child==0){execv(dhcp[0],dhcp);_exit(127);}if(child<0)return 1;
    int ready=0;for(int i=0;i<100;i++){if(access("/run/mantle/network.ready",F_OK)==0&&output_contains(addresses,"inet ")&&output_contains(routes,"default")){serial_line("MANTLE_NETWORK_OK");ready=1;break;}usleep(100000);}if(!ready){kill(child,SIGTERM);waitpid(child,NULL,0);return 1;}
    int status=0;while(waitpid(child,&status,0)<0&&errno==EINTR){}return WIFEXITED(status)?WEXITSTATUS(status):1;
}
