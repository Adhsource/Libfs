#include "struct.h"
#include <stdlib.h>
#include <stdio.h>

void init_libfs(){
    FILE * f_file;
    f_file = fopen("libfs.fs","rb+");

    int size_super = sizeof(struct filsys);

    /* Loading super block */

    struct filsys * super = malloc(size_super);

    // skiping boot block
    fseek(f_file,BSIZE,SEEK_SET);
    // reading struct and checking
    fread(super,size_super,1,f_file);

    // debug
    fprintf(stderr,"FS size : %d, Nb inodes : %d, Free inodes : %d \n",super->s_fsize,super->s_isize,super->s_ninode);


    // Set Current Dir
}

int main(){
    init_libfs();
    return 0;
}
