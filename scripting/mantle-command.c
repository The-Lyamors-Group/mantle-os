#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
static int run(const char *line){char *a[64];int n=0;char buf[4096];snprintf(buf,sizeof(buf),"%s",line);for(char*p=strtok(buf," \t\r\n");p&&n<63;p=strtok(NULL," \t\r\n"))a[n++]=p;a[n]=NULL;if(!n)return 0;pid_t c=fork();if(c==0){execvp(a[0],a);_exit(127);}if(c<0)return 127;int s=0;while(waitpid(c,&s,0)<0&&errno==EINTR){}return WIFEXITED(s)?WEXITSTATUS(s):128;}
int main(int argc,char **argv){if(argc<2){fprintf(stderr,"usage: mantle-command file.mtc\n");return 2;}FILE*f=fopen(argv[1],"r");if(!f){perror(argv[1]);return 1;}char line[4096];int r=0;while(fgets(line,sizeof(line),f)){char*p=line;while(*p==' '||*p=='\t')p++;if(!*p||*p=='#'||*p=='\n')continue;r=run(p);if(r)break;}fclose(f);return r;}
