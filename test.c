#include "struct.h"
#include <stdio.h>

int main(){
    init_libfs();

    int ret;

    fprintf(stderr,"\n--- current dir : %d\n\n\n",current.u_cdir);

    fprintf(stderr,"\n--- inode iget()\n");
    struct inode * rt = iget(2);
    fprintf(stderr,"\n--- inode: mode: %d , num: %d \n\n\n",rt->i_mode,rt->i_numb);

    fprintf(stderr,"\n--- mkdir()\n");
    ret = lfs_mkdir("/bin",IFDIR);
    fprintf(stderr,"\n--- mkdir() : %d\n\n\n",ret);


    // iput(rt);
    fprintf(stderr,"\n--- iput: mode: %d , num: %d \n",rt->i_mode,rt->i_numb);
    fprintf(stderr,"\n--- creat()\n");
    ret = lfs_creat("test",IFNORM|IREAD|IWRITE);  //debug
    fprintf(stderr,"\n--- creat() : %d\n\n\n",ret);

    fprintf(stderr,"\n--- open()\n");
    //int fd = lfs_creat("test",IREAD|IWRITE);
    //fprintf(stderr,"\n--- open() fd: %d\n\n\n",fd);



    fprintf(stderr,"\n--- write()\n");

    fprintf(stderr,"\n--- read()\n");

    lfs_chdir("test");
    fprintf(stderr,"\n--- unlink()\n");
    lfs_unlink("test");

    //struct inode * in = namei(".",0);
    //printf("namei : %d\n",in->i_numb);

    //lfs_chdir("/bin");
    fprintf(stderr,"--- current dir : %d\n\n\n",current.u_cdir);

    close_libfs();

    return 0;
}
