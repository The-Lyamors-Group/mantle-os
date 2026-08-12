#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

enum kind { K_EOF,K_WORD,K_STRING,K_LBRACE,K_RBRACE,K_EQ,K_LP,K_RP,K_DOT,K_NEWLINE };
struct token { enum kind kind; char text[512]; int line; };
struct lexer { const char *src; size_t pos; int line; struct token token; };
struct value { char name[64]; char text[512]; };
static struct value vars[128];static int var_count;static char **script_args;static int script_argc;
static void lex(struct lexer *l){
    while(l->src[l->pos]&& (l->src[l->pos]==' '||l->src[l->pos]=='\t'||l->src[l->pos]=='\r'))l->pos++;
    l->token.line=l->line;l->token.text[0]=0;char c=l->src[l->pos];if(!c){l->token.kind=K_EOF;return;}if(c=='\n'){l->pos++;l->line++;l->token.kind=K_NEWLINE;return;}if(c=='#'){while(l->src[l->pos]&&l->src[l->pos]!='\n')l->pos++;lex(l);return;}
    if(c=='{'){l->pos++;l->token.kind=K_LBRACE;return;}if(c=='}'){l->pos++;l->token.kind=K_RBRACE;return;}if(c=='='){l->pos++;l->token.kind=K_EQ;return;}if(c=='('){l->pos++;l->token.kind=K_LP;return;}if(c==')'){l->pos++;l->token.kind=K_RP;return;}
    if(c=='"'||c=='\''){char q=c;l->pos++;size_t n=0;while(l->src[l->pos]&&l->src[l->pos]!=q&&n<sizeof(l->token.text)-1){c=l->src[l->pos++];if(c=='\\'&&l->src[l->pos])c=l->src[l->pos++];l->token.text[n++]=c;}if(l->src[l->pos]==q)l->pos++;l->token.text[n]=0;l->token.kind=K_STRING;return;}
    size_t n=0;while(l->src[l->pos]&&!isspace((unsigned char)l->src[l->pos])&&!strchr("{}=()",l->src[l->pos])&&n<sizeof(l->token.text)-1)l->token.text[n++]=l->src[l->pos++];l->token.text[n]=0;l->token.kind=K_WORD;
}
static const char *getvar(const char *name){for(int i=0;i<var_count;i++)if(!strcmp(vars[i].name,name))return vars[i].text;if(name[0]=='$'&&isdigit((unsigned char)name[1])){int n=atoi(name+1);return n<script_argc?script_args[n]:"";}return getenv(name);}
static void interpolate(const char *in,char *out,size_t cap){size_t n=0;for(size_t i=0;in[i]&&n+1<cap;i++){if(in[i]=='$'&&in[i+1]=='{'){size_t j=i+2;while(in[j]&&in[j]!='}')j++;char key[64];size_t k=j-(i+2);if(k>63)k=63;memcpy(key,in+i+2,k);key[k]=0;const char*v=getvar(key);if(!v)v="";size_t z=strlen(v);if(n+z>=cap)z=cap-n-1;memcpy(out+n,v,z);n+=z;if(in[j]=='}')i=j;}else out[n++]=in[i];}out[n]=0;}
static int run_argv(char **argv,int argc){if(!argc)return 0;argv[argc]=NULL;pid_t p=fork();if(p==0){execvp(argv[0],argv);dprintf(2,"mantle-script: commande inexistante: %s\n",argv[0]);_exit(127);}if(p<0)return 127;int s=0;while(waitpid(p,&s,0)<0&&errno==EINTR){}return WIFEXITED(s)?WEXITSTATUS(s):128;}
static int command_exists(const char *name){char *p=getenv("PATH");if(!p)p="/bin:/usr/bin";char copy[4096];snprintf(copy,sizeof(copy),"%s",p);for(char *part=strtok(copy,":");part;part=strtok(NULL,":")){char path[512];snprintf(path,sizeof(path),"%s/%s",part,name);if(access(path,X_OK)==0)return 1;}return 0;}
static void skip_lines(struct lexer *l){while(l->token.kind!=K_EOF&&l->token.kind!=K_RBRACE)lex(l);}
static int block(struct lexer *l,int execute);
static int statement(struct lexer *l,int execute){
    while(l->token.kind==K_NEWLINE)lex(l);if(l->token.kind==K_RBRACE||l->token.kind==K_EOF)return 0;if(l->token.kind!=K_WORD){fprintf(stderr,"mantle-script:%d: mot attendu\n",l->token.line);return 2;}
    char keyword[64];snprintf(keyword,sizeof(keyword),"%s",l->token.text);lex(l);
    if(!strcmp(keyword,"let")){if(l->token.kind!=K_WORD){fprintf(stderr,"mantle-script:%d: nom de variable attendu\n",l->token.line);return 2;}char name[64];snprintf(name,sizeof(name),"%s",l->token.text);lex(l);if(l->token.kind!=K_EQ){fprintf(stderr,"mantle-script:%d: '=' attendu\n",l->token.line);return 2;}lex(l);if(l->token.kind!=K_STRING&&l->token.kind!=K_WORD){fprintf(stderr,"mantle-script:%d: valeur attendue\n",l->token.line);return 2;}char value[512];interpolate(l->token.text,value,sizeof(value));if(execute&&var_count<128){snprintf(vars[var_count].name,sizeof(vars[var_count].name),"%s",name);snprintf(vars[var_count].text,sizeof(vars[var_count].text),"%s",value);var_count++;}lex(l);return 0;}
    if(!strcmp(keyword,"print")){if(l->token.kind!=K_STRING&&l->token.kind!=K_WORD){fprintf(stderr,"mantle-script:%d: texte attendu\n",l->token.line);return 2;}char value[512];interpolate(l->token.text,value,sizeof(value));if(execute)puts(value);lex(l);return 0;}
    if(!strcmp(keyword,"run")){char *args[64];int n=0;while(l->token.kind==K_WORD||l->token.kind==K_STRING){char *v=malloc(512);interpolate(l->token.text,v,512);if(n<63)args[n++]=v;else free(v);lex(l);}if(execute){int r=run_argv(args,n);for(int i=0;i<n;i++)free(args[i]);return r;}for(int i=0;i<n;i++)free(args[i]);return 0;}
    if(!strcmp(keyword,"if")||!strcmp(keyword,"require")){int condition=0;if(!strcmp(keyword,"require")){if(l->token.kind!=K_WORD||strcmp(l->token.text,"admin")){fprintf(stderr,"mantle-script:%d: condition require inconnue\n",l->token.line);return 2;}condition=geteuid()==0;lex(l);}else{if(l->token.kind!=K_WORD||strcmp(l->token.text,"command.exists")){fprintf(stderr,"mantle-script:%d: condition command.exists attendue\n",l->token.line);return 2;}lex(l);if(l->token.kind!=K_LP){fprintf(stderr,"mantle-script:%d: '(' attendu\n",l->token.line);return 2;}lex(l);if(l->token.kind!=K_STRING&&l->token.kind!=K_WORD){fprintf(stderr,"mantle-script:%d: commande attendue\n",l->token.line);return 2;}condition=command_exists(l->token.text);lex(l);if(l->token.kind!=K_RP){fprintf(stderr,"mantle-script:%d: ')' attendu\n",l->token.line);return 2;}lex(l);}while(l->token.kind==K_NEWLINE)lex(l);if(l->token.kind!=K_LBRACE){fprintf(stderr,"mantle-script:%d: '{' attendu\n",l->token.line);return 2;}lex(l);return block(l,execute&&condition);}
    if(!strcmp(keyword,"exit")){if(l->token.kind!=K_WORD){fprintf(stderr,"mantle-script:%d: code attendu\n",l->token.line);return 2;}return execute?atoi(l->token.text):0;}
    fprintf(stderr,"mantle-script:%d: instruction inconnue: %s\n",l->token.line,keyword);skip_lines(l);return 2;
}
static int block(struct lexer *l,int execute){int result=0;while(l->token.kind!=K_EOF&&l->token.kind!=K_RBRACE){int r=statement(l,execute);if(r&&execute)result=r;while(l->token.kind==K_NEWLINE)lex(l);}if(l->token.kind==K_RBRACE)lex(l);return result;}
int main(int argc,char **argv){if(argc<2){fprintf(stderr,"usage: mantle-script file.mt [args...]\n");return 2;}FILE*f=fopen(argv[1],"r");if(!f){perror(argv[1]);return 1;}fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);char *src=calloc(1,(size_t)size+1);if(!src){fclose(f);return 1;}fread(src,1,(size_t)size,f);fclose(f);script_args=argv+1;script_argc=argc-1;struct lexer l={.src=src,.line=1};lex(&l);int result=block(&l,1);free(src);return result;}
