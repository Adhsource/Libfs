#include "struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void print_use(){
    fprintf(stderr,"Usage : mklibfs -s size -i inode \n");
}

int main(int argc, char ** argv){
    int opt;
    int nb_blocks;
    int nb_inodes;

    if(argc < 5){
        fprintf(stderr,"\nNot enough arguments\n");
        print_use();
        return 1;
    } else if(argc > 5){
        fprintf(stderr,"\nToo many arguments\n");
        print_use();
        return 1;
    }

    while((opt = getopt(argc, argv,"s:i:" )) !=-1) {
        switch(opt){
            case 's':
                nb_blocks = atoi(optarg);
                if(nb_blocks <= 0){
                    fprintf(stderr,"size cannot be null or negative\n");
                    return 1;
                }
                printf("\nNumbers of blocks = %d\n",nb_blocks);
                break;

            case 'i':
                nb_inodes = atoi(optarg);
                if(nb_inodes <= 0){
                    fprintf(stderr,"inodes cannot be null or negative\n");
                    return 1;
                }
                printf("\nNumbers of inodes = %d\n", nb_inodes);
                break;

            case '?':
                print_use();
                return 1;

            default:
                print_use();
                return 1;
        }
    }

    /* Creating the file */


    FILE * f_file;

    f_file = fopen("libfs.fs","wb");

    if(!f_file){
        fprintf(stderr,"\nError in file opening \n");
        return 1;
    }

    fseek(f_file,BSIZE,SEEK_SET); // Boot block

    /* Setting super-block */

    struct filsys super = {nb_blocks,nb_inodes,(nb_inodes-1)};

    /* Definition des inodes libres
     * L'inode 0 etant root, elle n'est pas libre
     */

    for(int i = 1; i<= NIFREE && i < nb_inodes; i++)
        super.s_inode[i-1] = i;

    fwrite(&super,BSIZE,1,f_file);

    /* Setting Bitmap blocks */

    // set some to 1

    /* one bit per block so divided by 8
        plus 7 for rounding up */

    int nb_bmap_bits = (nb_blocks+7)/8;

    fseek(f_file,(nb_bmap_bits),SEEK_CUR);

    /* Writing an entire block */

    fseek(f_file,(BSIZE-(nb_bmap_bits % BSIZE)),SEEK_CUR);


    /* Setting inodes */

    // writing root inode

    struct dinode root_inode = {2,0,0}; // voir position of direct struct !! A faire!!
    fwrite(&root_inode,32,1,f_file);

    // Default inode

    struct dinode def_inode = {0,0};

    for(int i = 1; i < nb_inodes; i++)
        fwrite(&def_inode,32,1,f_file);


    /* Writing an entire block */
    fseek(f_file,(BSIZE-((nb_inodes * 32) % BSIZE)),SEEK_CUR);

    fclose(f_file);
    return 0;

}

