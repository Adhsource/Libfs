#include "struct.h"
#include <stdio.h>

int main(){
    init_libfs();

    int ret;

    printf("\n--- current dir : %d\n\n\n",current.u_cdir);

    printf("\n--- inode iget()\n");
    struct inode * rt = iget(0);
    printf("\n--- inode: mode: %d , num: %d \n\n\n",rt->i_mode,rt->i_numb);

    printf("\n--- mkdir()\n");
    //ret = lfs_mkdir("/bin",IFDIR);
    printf("\n--- mkdir() : %d\n\n\n",ret);


    // iput(rt);
    // printf("\n--- iput: mode: %d , num: %d \n",rt->i_mode,rt->i_numb);
    printf("\n--- creat()\n");
    ret = lfs_creat("bob",IFNORM|IREAD|IWRITE);  //debug
    printf("\n--- creat() : %d\n\n\n",ret);

    //struct inode * in = namei(".",0);
    //printf("namei : %d\n",in->i_numb);

    //lfs_chdir("/bin");
    printf("--- current dir : %d\n\n\n",current.u_cdir);

    close_libfs();

    return 0;
}
