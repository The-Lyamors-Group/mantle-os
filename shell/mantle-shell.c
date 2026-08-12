#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_TOKENS 128
#define MAX_WORD 4096
enum token_kind { T_WORD, T_PIPE, T_IN, T_OUT, T_APPEND, T_SEMI, T_END };
struct token { enum token_kind kind; char text[MAX_WORD]; };
struct command { char *argv[MAX_TOKENS]; int argc; char *input; char *output; int append; };
struct parser { const char *line; size_t pos; struct token token; int error; };
static int last_status;

static int expand(const char *in, char *out, size_t cap) {
    size_t n=0; for(size_t i=0;in[i]&&n+1<cap;i++) {
        if(in[i]!='$'){out[n++]=in[i];continue;} size_t j=i+1; char name[256]={0};
        if(in[j]=='{'){j++;size_t s=j;while(in[j]&&in[j]!='}'&&j-s<sizeof(name)-1)j++;memcpy(name,in+s,j-s);name[j-s]=0;if(in[j]=='}')i=j;else i=j-1;}
        else {size_t s=j;if(in[j]=='?'){snprintf(name,sizeof(name),"?");i=j;}else{while(isalnum((unsigned char)in[j])||in[j]=='_')j++;if(j==s){out[n++]='$';continue;}memcpy(name,in+s,j-s);name[j-s]=0;i=j-1;}}
        const char *v=!strcmp(name,"?")?(last_status==0?"0":"1"):getenv(name);if(!v)v="";size_t l=strlen(v);if(n+l>=cap)l=cap-n-1;memcpy(out+n,v,l);n+=l;
    } out[n]=0; return 0;
}
static void next(struct parser *p) {
    while(isspace((unsigned char)p->line[p->pos])&&p->line[p->pos]!='\n')p->pos++;
    char c=p->line[p->pos]; if(!c||c=='\n'){p->token.kind=T_END;p->token.text[0]=0;return;}
    if(c=='|'){p->pos++;p->token.kind=T_PIPE;return;}if(c==';'){p->pos++;p->token.kind=T_SEMI;return;}if(c=='<'){p->pos++;p->token.kind=T_IN;return;}
    if(c=='>'){p->pos++;p->token.kind=p->line[p->pos]=='>'?(p->pos++,T_APPEND):T_OUT;return;}
    char raw[MAX_WORD]={0};size_t n=0;while(p->line[p->pos]&&!isspace((unsigned char)p->line[p->pos])&&!strchr("|;<>",p->line[p->pos])){
        c=p->line[p->pos++];if(c=='"'||c=='\''){char q=c;while(p->line[p->pos]&&p->line[p->pos]!=q&&n<sizeof(raw)-2){if(p->line[p->pos]=='\\'&&p->line[p->pos+1])p->pos++;raw[n++]=p->line[p->pos++];}if(p->line[p->pos]==q)p->pos++;}
        else if(c=='\\'&&p->line[p->pos])raw[n++]=p->line[p->pos++];else if(n<sizeof(raw)-1)raw[n++]=c;
    }raw[n]=0;p->token.kind=T_WORD;expand(raw,p->token.text,sizeof(p->token.text));
}
static int builtin(struct command *c) {
    if(!c->argc)return 0;
    if(!strcmp(c->argv[0],"cd")){const char *d=c->argc>1?c->argv[1]:getenv("HOME");return chdir(d?d:"/");}
    if(!strcmp(c->argv[0],"export")){for(int i=1;i<c->argc;i++){char *eq=strchr(c->argv[i],'=');if(eq){*eq=0;setenv(c->argv[i],eq+1,1);}}return 0;}
    if(!strcmp(c->argv[0],"unset")){for(int i=1;i<c->argc;i++)unsetenv(c->argv[i]);return 0;}
    if(!strcmp(c->argv[0],"pwd")){char p[4096];return getcwd(p,sizeof(p))?(puts(p),0):1;}
    if(!strcmp(c->argv[0],"exit")){exit(c->argc>1?atoi(c->argv[1]):0);}
    if(!strcmp(c->argv[0],"true"))return 0;if(!strcmp(c->argv[0],"false"))return 1;
    return -1;
}
static int execute(struct command *c, int in_fd, int out_fd, int background) {
    int b=builtin(c);if(b>=0&&in_fd==STDIN_FILENO&&out_fd==STDOUT_FILENO&&!background)return b;
    pid_t p=fork();if(p==0){if(in_fd!=STDIN_FILENO){dup2(in_fd,STDIN_FILENO);close(in_fd);}if(out_fd!=STDOUT_FILENO){dup2(out_fd,STDOUT_FILENO);close(out_fd);}
        if(c->input){int f=open(c->input,O_RDONLY);if(f<0)_exit(126);dup2(f,0);close(f);}if(c->output){int f=open(c->output,O_WRONLY|O_CREAT|(c->append?O_APPEND:O_TRUNC),0666);if(f<0)_exit(126);dup2(f,1);close(f);}execvp(c->argv[0],c->argv);dprintf(2,"mantle-shell: %s: %s\n",c->argv[0],strerror(errno));_exit(127);}
    if(p<0)return 127;int s=0;if(background)return (int)p;while(waitpid(p,&s,0)<0&&errno==EINTR){}return WIFEXITED(s)?WEXITSTATUS(s):128;
}
static int run_line(const char *line) {
    struct parser p={.line=line};struct command cmds[16]={0};int count=0;next(&p);
    while(p.token.kind!=T_END&&count<16){struct command*c=&cmds[count++];while(p.token.kind==T_WORD){if(c->argc<MAX_TOKENS-1)c->argv[c->argc++]=strdup(p.token.text);next(&p);}c->argv[c->argc]=NULL;
        if(p.token.kind==T_IN||p.token.kind==T_OUT||p.token.kind==T_APPEND){enum token_kind k=p.token.kind;next(&p);if(p.token.kind!=T_WORD){p.error=1;break;}if(k==T_IN)c->input=strdup(p.token.text);else{c->output=strdup(p.token.text);c->append=k==T_APPEND;}next(&p);}
        if(p.token.kind==T_PIPE){next(&p);if(p.token.kind==T_END){p.error=1;break;}continue;}break;
    }
    if(p.error||p.token.kind==T_WORD||p.token.kind==T_IN||p.token.kind==T_OUT||p.token.kind==T_APPEND||p.token.kind==T_SEMI){fprintf(stderr,"mantle-shell: syntax error (séparateur non pris en charge dans cette version)\n");return 2;}
    int last=0,in=STDIN_FILENO;pid_t pids[16]={0};for(int i=0;i<count;i++){int pipefd[2]={-1,-1};if(i<count-1&&pipe(pipefd)<0)return 1;int bg=count>1;int child=execute(&cmds[i],in,i<count-1?pipefd[1]:STDOUT_FILENO,bg);if(bg)pids[i]=(pid_t)child;else last=child;if(in!=STDIN_FILENO)close(in);if(i<count-1){close(pipefd[1]);in=pipefd[0];}}if(count>1){for(int i=0;i<count;i++){int s=0;waitpid(pids[i],&s,0);if(i==count-1)last=WIFEXITED(s)?WEXITSTATUS(s):128;}}
    for(int i=0;i<count;i++){for(int j=0;j<cmds[i].argc;j++)free(cmds[i].argv[j]);free(cmds[i].input);free(cmds[i].output);}last_status=last;return last;
}
int main(int argc,char **argv){
    if(argc>1&&(!strcmp(argv[1],"--version")||!strcmp(argv[1],"-V"))){puts("mantle-shell 0.1 (MantleOS)");return 0;}
    if(argc>1&&!strcmp(argv[1],"-c")){if(argc<3)return 2;return run_line(argv[2]);}
    char *line=NULL;size_t cap=0;int status=0;while(1){if(isatty(0)){fputs("mantle> ",stdout);fflush(stdout);}ssize_t n=getline(&line,&cap,stdin);if(n<0)break;status=run_line(line);}free(line);return status;
}
