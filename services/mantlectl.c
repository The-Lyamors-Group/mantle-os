#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
static long read_pid(const char *file){FILE*f=fopen(file,"r");long p=0;if(f){fscanf(f,"%ld",&p);fclose(f);}return p;}
static long service_pid(const char *name){char p[128];snprintf(p,sizeof(p),"/run/mantle/%s.pid",name);return read_pid(p);}
static void touch(const char *file){int f=open(file,O_WRONLY|O_CREAT|O_TRUNC,0644);if(f>=0)close(f);}
int main(int argc,char **argv){
    const char *action=argc>1?argv[1]:"list"; long supervisor=read_pid("/run/mantle-supervise.pid"),session=read_pid("/run/mantle-session.pid");
    if(!strcmp(action,"list")){DIR*d=opendir("/etc/mantleos/services");struct dirent*e;if(d){while((e=readdir(d)))if(e->d_name[0]!='.')puts(e->d_name);closedir(d);}return 0;}
    if(!strcmp(action,"status")){printf("mantle-supervise: %s\n",supervisor>0&&kill(supervisor,0)==0?"running":"stopped");printf("session: %s\n",session>0&&kill(session,0)==0?"running":"stopped");long log=service_pid("log"),net=service_pid("network");printf("log: %s\n",log>0&&kill(log,0)==0?"running":"stopped");printf("network: %s\n",net>0&&kill(net,0)==0?"running":"stopped");return 0;}
    if(!strcmp(action,"stop")){touch("/run/mantle/session.stop");if(session>0)kill(session,SIGHUP);return 0;}
    if(!strcmp(action,"start")){unlink("/run/mantle/session.stop");return 0;}
    if(!strcmp(action,"restart")){unlink("/run/mantle/session.stop");if(session>0)kill(session,SIGHUP);return 0;}
    fprintf(stderr,"usage: mantlectl list|status|start|stop|restart\n");return 2;
}
