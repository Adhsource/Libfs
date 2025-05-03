#include "Libfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

// Mise en place de la recuperation du nom est necessaire

int main(int argc, char ** argv){
    int opt;
    int nb_blocks;
    int nb_inodes;

    if(argc < 5){
        fprintf(stderr,"\nNot enough arguments\n");
        fprintf(stderr,"Usage : mklibfs -s size -i inode \n");
        return 1;
    } else if(argc > 5){
        fprintf(stderr,"\nToo many arguments\n");
        fprintf(stderr,"Usage : mklibfs -s size -i inode \n");
        return 1;
    }

    while((opt = getopt(argc, argv,"s:i:" )) !=-1) {
        switch(opt){
            case 's':
                nb_blocks = atoi(optarg);
                printf("\nNumbers of blocks = %d\n",nb_blocks);
                break;
            case 'i':
                nb_inodes = atoi(optarg);
                printf("\nNumbers of inodes = %d\n", nb_inodes);
                break;
            case '?':
                fprintf(stderr,"\nUsage : mklibfs -s size -i inode \n");
                return 1;
            default:
                fprintf(stderr,"\nUsage : mklibfs -s size -i inode \n");
                return 1;
        }
    }

    // Creating the file


    int fs_file;

    fs_file = creat("libfs.fs",0600);

    if(!fs_file){
        fprintf(stderr,"\nError in file opening \n");
        return 1;
    }

    lseek(fs_file,BSIZE,SEEK_CUR); // Boot block

    /* Setting super-block */

    struct filsys super = {nb_blocks,nb_inodes,nb_inodes};


    write(fs_file,&super,BSIZE);

    /* Setting Bitmap blocks */

    /* one bit per block so divided by 8
        plus 7 for rounding up */

    int nb_bmap_bits = (nb_blocks+7)/8;

    lseek(fs_file,(nb_bmap_bits),SEEK_CUR);

    /* Writing an entire block */

    lseek(fs_file,(BSIZE-(nb_bmap_bits % BSIZE)),SEEK_CUR);


    /* Setting inodes */
    // Default inode

    struct dinode def_inode = {0,0};

    for(int i = 0; i < nb_inodes; i++)
        write(fs_file,&def_inode,32);


    close(fs_file);
    return 0;

}

