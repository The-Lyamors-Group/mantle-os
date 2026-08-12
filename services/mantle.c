#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PATH 4096

static int runv(char *const argv[]) {
    pid_t child=fork();
    if(child==0){execvp(argv[0],argv);_exit(127);}
    if(child<0)return 127;
    int status=0;while(waitpid(child,&status,0)<0&&errno==EINTR){}
    return WIFEXITED(status)?WEXITSTATUS(status):128;
}
static int runv_dir(const char *dir,char *const argv[]) {
    pid_t child=fork();
    if(child==0){if(chdir(dir)<0)_exit(126);execvp(argv[0],argv);_exit(127);}
    if(child<0)return 127;
    int status=0;while(waitpid(child,&status,0)<0&&errno==EINTR){}
    return WIFEXITED(status)?WEXITSTATUS(status):128;
}
static int run_script(const char *interpreter,int argc,char **argv){
    char *args[64];int n=0;args[n++]=(char*)interpreter;
    for(int i=0;i<argc&&n<63;i++)args[n++]=argv[i];args[n]=NULL;return runv(args);
}
static int extension(const char *path,const char *suffix){const char *p=strrchr(path,'.');return p&&strcmp(p,suffix)==0;}
static int safe_rel(const char *path,int component_only){
    if(!path||!*path||path[0]=='/'||strstr(path,"\\")||strstr(path,".."))return 0;
    if(component_only&&strchr(path,'/'))return 0;
    for(const unsigned char *p=(const unsigned char*)path;*p;p++)
        if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||strchr("/._-+@",*p)))return 0;
    return 1;
}
static int critical_path(const char *path){
    return !strcmp(path,"etc/passwd")||!strcmp(path,"etc/shadow")||!strcmp(path,"etc/group")||
           !strcmp(path,"sbin/mantle-init")||!strcmp(path,"sbin/mantle-supervise")||
           !strcmp(path,"bin/mantle-shell")||!strcmp(path,"usr/bin/mantle");
}
static int mkdir_p(const char *path){char copy[MAX_PATH];snprintf(copy,sizeof(copy),"%s",path);for(char *p=copy+1;*p;p++)if(*p=='/'){*p=0;mkdir(copy,0755);*p='/';}return mkdir(copy,0755)==0||errno==EEXIST?0:-1;}
static int copy_path(const char *src,const char *dst){char *const a[]={(char*)"cp",(char*)"-a",(char*)src,(char*)dst,NULL};return runv(a);}
static int read_line(const char *path,char *out,size_t size){FILE*f=fopen(path,"r");if(!f)return 0;int ok=fgets(out,size,f)!=NULL;fclose(f);if(ok)out[strcspn(out,"\r\n")]=0;return ok;}
static int source_repo(const char *url,const char *dest){
    if(!safe_rel(dest,1)){fprintf(stderr,"mantle: destination de dépôt invalide\n");return 2;}
    char *const clone[]={(char*)"git",(char*)"clone",(char*)url,(char*)dest,NULL};int r=runv(clone);if(r)return r;
    char *const chmod_args[]={(char*)"find",(char*)dest,(char*)"-type",(char*)"f",(char*)"-name",(char*)"*.mt",(char*)"-exec",(char*)"chmod",(char*)"a-x",(char*)"{}",(char*)"+",NULL};
    r=runv(chmod_args);if(r)return r;
    char *const chmod_mtc[]={(char*)"find",(char*)dest,(char*)"-type",(char*)"f",(char*)"-name",(char*)"*.mtc",(char*)"-exec",(char*)"chmod",(char*)"a-x",(char*)"{}",(char*)"+",NULL};return runv(chmod_mtc);
}
static int stream_validate_tar(const char *pkg,int verbose){
    int pipefd[2];if(pipe(pipefd)<0)return 1;pid_t child=fork();
    if(child==0){dup2(pipefd[1],STDOUT_FILENO);close(pipefd[0]);close(pipefd[1]);char *const a[]={(char*)"tar",verbose?(char*)"-tvzf":(char*)"-tzf",(char*)pkg,NULL};execvp(a[0],a);_exit(127);}
    close(pipefd[1]);FILE*f=fdopen(pipefd[0],"r");if(!f){close(pipefd[0]);return 1;}char line[MAX_PATH+256];int bad=0;
    while(fgets(line,sizeof(line),f)){
        if(verbose){if(strlen(line)>=10&&(line[0]=='l'||line[0]=='h'||line[3]=='s'||line[3]=='S'||line[6]=='s'||line[6]=='S'||line[9]=='t'||line[9]=='T')){bad=1;break;}}
        else {line[strcspn(line,"\r\n")]=0;const char*p=line;if(!strncmp(p,"./",2))p+=2;if(strncmp(p,"META/",5)&&strncmp(p,"payload/",8)){bad=1;break;}if(!safe_rel(p,0)){bad=1;break;}}
    }
    fclose(f);int status=0;waitpid(child,&status,0);if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)bad=1;return bad?1:0;
}
static int manifest_path(const char *line,char *out,size_t size){
    char hash[80],path[MAX_PATH];if(sscanf(line,"%79s %4095s",hash,path)!=2)return 0;
    if(strlen(hash)!=64||strncmp(path,"payload/",8)||!safe_rel(path+8,0))return 0;for(size_t i=0;i<64;i++)if(!isxdigit((unsigned char)hash[i]))return 0;snprintf(out,size,"%s",path+8);return 1;
}
static int verify_manifest(const char *stage){
    char path[MAX_PATH],line[MAX_PATH+128];snprintf(path,sizeof(path),"%s/META/MANIFEST",stage);FILE*f=fopen(path,"r");if(!f)return 3;int count=0,ok=1;
    while(fgets(line,sizeof(line),f)){line[strcspn(line,"\r\n")]=0;if(!manifest_path(line,path,sizeof(path))){ok=0;break;}count++;}fclose(f);if(!ok||count==0)return 3;
    char *const a[]={(char*)"sha256sum",(char*)"-c",(char*)"META/MANIFEST",NULL};return runv_dir(stage,a);
}
static int verify_package_prepare(const char *pkg,char *stage,size_t stage_size,char *name,size_t name_size){
    if(!safe_rel(pkg,0)&&pkg[0]!='/'){fprintf(stderr,"mantle: chemin de paquet invalide\n");return 2;}
    snprintf(stage,stage_size,"/tmp/mantle-verify-%ld",(long)getpid());mkdir(stage,0700);
    char *const extract[]={(char*)"tar",(char*)"-xzf",(char*)pkg,(char*)"-C",stage,(char*)"--no-same-owner",(char*)"--no-same-permissions",NULL};
    if(stream_validate_tar(pkg,0)||stream_validate_tar(pkg,1)||runv(extract)){fprintf(stderr,"mantle: archive .mtpkg refusée\n");return 3;}
    char metadata[MAX_PATH];snprintf(metadata,sizeof(metadata),"%s/META/metadata.json",stage);FILE*mf=fopen(metadata,"r");if(!mf){fprintf(stderr,"mantle: metadata.json manquant\n");return 3;}
    char json[8192]={0};size_t used=fread(json,1,sizeof(json)-1,mf);fclose(mf);json[used]=0;
    const char *required[]={"name","version","architecture","dependencies","description","license","maintainer","build",NULL};for(int i=0;required[i];i++){char needle[96];snprintf(needle,sizeof(needle),"\"%s\"",required[i]);if(!strstr(json,needle)){fprintf(stderr,"mantle: champ metadata manquant: %s\n",required[i]);return 3;}}
    char p[MAX_PATH];snprintf(p,sizeof(p),"%s/META/NAME",stage);if(!read_line(p,name,name_size)||!safe_rel(name,1)){fprintf(stderr,"mantle: nom de paquet invalide\n");return 3;}
    char keyid[128],keypath[MAX_PATH],revoked[MAX_PATH];snprintf(p,sizeof(p),"%s/META/KEY-ID",stage);if(!read_line(p,keyid,sizeof(keyid))||!safe_rel(keyid,1)){fprintf(stderr,"mantle: identifiant de clé invalide\n");return 3;}
    snprintf(revoked,sizeof(revoked),"/etc/mantleos/trust/revoked");FILE*rf=fopen(revoked,"r");char row[128];if(rf){while(fgets(row,sizeof(row),rf)){row[strcspn(row,"\r\n")]=0;if(!strcmp(row,keyid)){fclose(rf);fprintf(stderr,"mantle: clé révoquée\n");return 3;}}fclose(rf);}
    snprintf(keypath,sizeof(keypath),"/etc/mantleos/trust/keys/%s.pub",keyid);if(access(keypath,R_OK)!=0){fprintf(stderr,"mantle: clé de dépôt non approuvée\n");return 3;}
    snprintf(p,sizeof(p),"%s/META/SIGNED",stage);if(access(p,R_OK)!=0){fprintf(stderr,"mantle: métadonnées signées absentes\n");return 3;}snprintf(p,sizeof(p),"%s/META/SIGNATURE",stage);if(access(p,R_OK)!=0){fprintf(stderr,"mantle: signature absente\n");return 3;}
    char *const verify[]={(char*)"openssl",(char*)"dgst",(char*)"-sha256",(char*)"-verify",keypath,(char*)"-signature",(char*)"META/SIGNATURE",(char*)"META/SIGNED",NULL};
    if(verify_manifest(stage)||runv_dir(stage,verify)){fprintf(stderr,"mantle: intégrité ou signature invalide\n");return 3;}return 0;
}
static int rollback_tx(const char *tx){
    char list[MAX_PATH];snprintf(list,sizeof(list),"%s/files",tx);FILE*f=fopen(list,"r");if(!f)return 2;char rel[MAX_PATH];int result=0;
    while(fgets(rel,sizeof(rel),f)){rel[strcspn(rel,"\r\n")]=0;if(!safe_rel(rel,0))continue;char target[MAX_PATH],backup[MAX_PATH];snprintf(target,sizeof(target),"/%s",rel);snprintf(backup,sizeof(backup),"%s/backup/%s",tx,rel);if(access(backup,F_OK)==0){unlink(target);if(copy_path(backup,target))result=1;}else if(unlink(target)<0&&errno!=ENOENT)result=1;}
    fclose(f);return result;
}
static int install_package(const char *pkg){
    if(geteuid()!=0){fprintf(stderr,"mantle: installation réservée à root (utiliser sudo)\n");return 3;}
    char stage[MAX_PATH],name[128];int r=verify_package_prepare(pkg,stage,sizeof(stage),name,sizeof(name));if(r)return r;
    char tx[MAX_PATH];snprintf(tx,sizeof(tx),"/var/lib/mantleos/transactions/%ld",(long)getpid());char backup[MAX_PATH];snprintf(backup,sizeof(backup),"%s/backup",tx);mkdir_p(backup);
    char manifest[MAX_PATH],line[MAX_PATH+128],rel[MAX_PATH],target[MAX_PATH],source[MAX_PATH],destdir[MAX_PATH];snprintf(manifest,sizeof(manifest),"%s/META/MANIFEST",stage);FILE*mf=fopen(manifest,"r");FILE*lf=NULL;if(mf){char files[MAX_PATH];snprintf(files,sizeof(files),"%s/files",tx);lf=fopen(files,"w");}
    int failed=!mf||!lf;
    while(!failed&&fgets(line,sizeof(line),mf)){line[strcspn(line,"\r\n")]=0;if(!manifest_path(line,rel,sizeof(rel))||critical_path(rel)){failed=1;break;}fprintf(lf,"%s\n",rel);snprintf(target,sizeof(target),"/%s",rel);snprintf(source,sizeof(source),"%s/payload/%s",stage,rel);snprintf(destdir,sizeof(destdir),"%s/backup/%s",tx,rel);char *slash=strrchr(destdir,'/');if(slash){*slash=0;mkdir_p(destdir);}if(access(target,F_OK)==0&&copy_path(target,destdir)){failed=1;break;}}
    if(mf)fclose(mf);if(lf)fclose(lf);
    if(!failed){char files[MAX_PATH];snprintf(files,sizeof(files),"%s/files",tx);FILE*in=fopen(files,"r");while(in&&fgets(rel,sizeof(rel),in)){rel[strcspn(rel,"\r\n")]=0;snprintf(target,sizeof(target),"/%s",rel);snprintf(source,sizeof(source),"%s/payload/%s",stage,rel);snprintf(destdir,sizeof(destdir),"%s",target);char *slash=strrchr(destdir,'/');if(slash){*slash=0;mkdir_p(destdir);}if(copy_path(source,target)){failed=1;break;}}if(in)fclose(in);}
    if(failed){rollback_tx(tx);fprintf(stderr,"mantle: transaction échouée, rollback appliqué\n");}else{char db[MAX_PATH],pkgdir[MAX_PATH];snprintf(pkgdir,sizeof(pkgdir),"/var/lib/mantleos/packages/%s",name);mkdir_p(pkgdir);snprintf(db,sizeof(db),"%s/files",pkgdir);char files[MAX_PATH];snprintf(files,sizeof(files),"%s/files",tx);copy_path(files,db);char status[MAX_PATH];snprintf(status,sizeof(status),"%s/COMMITTED",tx);int fd=open(status,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd>=0){dprintf(fd,"%s\n",name);close(fd);}}
    char cleanup[MAX_PATH];snprintf(cleanup,sizeof(cleanup),"/tmp/mantle-verify-%ld",(long)getpid());char *const rm[]={(char*)"rm",(char*)"-rf",cleanup,NULL};runv(rm);return failed?1:0;
}
static int verify_package(const char *pkg){char stage[MAX_PATH],name[128];int r=verify_package_prepare(pkg,stage,sizeof(stage),name,sizeof(name));char cleanup[MAX_PATH];snprintf(cleanup,sizeof(cleanup),"/tmp/mantle-verify-%ld",(long)getpid());char *const rm[]={(char*)"rm",(char*)"-rf",cleanup,NULL};runv(rm);return r;}
static int remove_package(const char *name){
    if(geteuid()!=0)return 3;if(!safe_rel(name,1))return 2;char path[MAX_PATH];snprintf(path,sizeof(path),"/var/lib/mantleos/packages/%s/files",name);FILE*f=fopen(path,"r");if(!f)return 2;char rel[MAX_PATH];int r=0;while(fgets(rel,sizeof(rel),f)){rel[strcspn(rel,"\r\n")]=0;if(safe_rel(rel,0)){char target[MAX_PATH];snprintf(target,sizeof(target),"/%s",rel);if(unlink(target)<0&&errno!=ENOENT)r=1;}}fclose(f);unlink(path);return r;
}
static void help(void){puts("mantle install|remove|search|update|upgrade|info|source|build|verify|rollback|run|exec|shell|docs");}
int main(int argc,char **argv){
    if(argc<2){help();return 0;}
    if(!strcmp(argv[1],"shell")){char *const a[]={(char*)"/bin/mantle-shell",NULL};return runv(a);}
    if(!strcmp(argv[1],"docs")){char *const a[]={(char*)"/bin/httpd",(char*)"-f",(char*)"-p",(char*)"127.0.0.1:8080",(char*)"-h",(char*)"/usr/share/mantleos/site",NULL};return runv(a);}
    if(!strcmp(argv[1],"run")&&argc>2)return run_script("/usr/bin/mantle-script",argc-2,argv+2);
    if(!strcmp(argv[1],"exec")&&argc>2)return run_script("/usr/bin/mantle-command",argc-2,argv+2);
    if(extension(argv[1],".mt"))return run_script("/usr/bin/mantle-script",argc-1,argv+1);
    if(extension(argv[1],".mtc"))return run_script("/usr/bin/mantle-command",argc-1,argv+1);
    if(!strcmp(argv[1],"source")&&argc>2)return source_repo(argv[2],argc>3?argv[3]:"source");
    if(!strcmp(argv[1],"verify")&&argc>2)return verify_package(argv[2]);
    if(!strcmp(argv[1],"install")&&argc>2){if(strstr(argv[2],".mtpkg"))return install_package(argv[2]);fprintf(stderr,"mantle: aucun dépôt configuré pour %s\n",argv[2]);return 2;}
    if(!strcmp(argv[1],"rollback")&&argc>2){if(geteuid()!=0||!safe_rel(argv[2],1))return 3;char tx[MAX_PATH];snprintf(tx,sizeof(tx),"/var/lib/mantleos/transactions/%s",argv[2]);return rollback_tx(tx);}
    if(!strcmp(argv[1],"build")){char *const a[]={(char*)"make",NULL};return runv(a);}
    if(!strcmp(argv[1],"search")||!strcmp(argv[1],"info")){fprintf(stderr,"mantle: dépôt local non configuré\n");return 2;}
    if(!strcmp(argv[1],"update")||!strcmp(argv[1],"upgrade")){fprintf(stderr,"mantle: aucun canal de mise à jour configuré\n");return 2;}
    if(!strcmp(argv[1],"remove")&&argc>2)return remove_package(argv[2]);help();return 2;
}
