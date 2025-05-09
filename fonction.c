#include "struct.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Flags pour bmap : lecture ou écriture */
#define BREAD  0
#define BWRITE 1

/* Structure FILE du fichier libfs.fs */
FILE * f_file;

/* Variables globales du fs */
struct user current;
struct inode inode[NINODE];

struct filsys super; // Superblock

/* Fonctions auxiliaires appele par d'autres fonctions */
void goto_inodes(){
    /* Deplace la tete de lecture au debut des inodes */
    // size of bitmap(to skip)
    int size_bitmap = (super.s_fsize+7)/8;

    fseek(f_file,(2*BSIZE),SEEK_SET); // seek to start of bitmap

    fseek(f_file,size_bitmap,SEEK_CUR);   // seek to start of inodes
    fseek(f_file,(BSIZE-(size_bitmap % BSIZE)),SEEK_CUR);

}

void goto_bitmap_block(int blkno){
    fseek(f_file,(2*BSIZE),SEEK_SET); // seek to start of bitmap
    fseek(f_file,(blkno/8),SEEK_CUR); // goto the correct byte
}

/* Fonctions externes pour lecture/écriture de blocs */
void bread(int blkno, void *buf){
    fseek(f_file,blkno*BSIZE,SEEK_SET);
    fread(buf,BSIZE,1 ,f_file);
}

void bwrite(int blkno, void *buf){
    fseek(f_file,blkno*BSIZE,SEEK_SET);
    fwrite(buf,BSIZE,1 ,f_file);
}

/* Fonction retournant le 1er bloc libre */
int balloc(){
    char temp;
    goto_bitmap_block(0);

    // parcours de la bitmap
    for(int i = 0; i < ((super.s_fsize+7)/8); i++){
        fread(&temp,1,1,f_file);
        fseek(f_file,-1,SEEK_CUR);
        char val;
        for(int j = 0; i < 8; i++){
            val = (1<<j);
            if(!(temp&val)){
                temp |= val;
                fwrite(&temp,1,1,f_file);
                return ((i*8)+j);

            }
        }

    }
    fprintf(stderr,"No Free blocks");
    return -1;
}

/* Marque un bloc comme étant libre */
void bfree(int blkno){
    char bits;
    char mask = 1 << (blkno%8);

    goto_bitmap_block(blkno);


    fread(&bits,1,1,f_file);
    goto_bitmap_block(blkno);

    bits = bits & ~mask;

    fwrite(&bits,1,1,f_file);

}

/* Crée un répertoire à l'emplacement pathname */
int lfs_mkdir(const char *pathname, int mode) {
    struct inode *ip = namei((char *)pathname, 1);
    if (!ip) return -1;

    /* Définir le mode et la taille du répertoire (un bloc) */
    ip->i_mode = IFDIR | mode;
    ip->i_size = BSIZE;

    /* Allouer et initialiser le premier bloc */
    int blk = bmap(ip, 0, BWRITE);
    struct direct entries[BSIZE / sizeof(struct direct)];
    int ino_new = ip - inode;    /* numéro d'inode du nouveau répertoire */
    int ino_parent = current.u_cdir;  /* inode du répertoire courant */

    /* Entrée "." */
    entries[0].d_ino = ino_new;
    strncpy(entries[0].d_name, ".", DIRSIZ);
    /* Entrée ".." */
    entries[1].d_ino = ino_parent;
    strncpy(entries[1].d_name, "..", DIRSIZ);
    /* Nettoyer le reste */
    for (int i = 2; i < BSIZE / sizeof(struct direct); i++) {
        entries[i].d_ino = 0;
        entries[i].d_name[0] = '\0';
    }

    /* Écrire le bloc sur disque */
    bwrite(blk, (char *)entries);
    iput(ip);
    return 0;
}

/* Supprime un fichier ou répertoire vide */
int lfs_unlink(const char *pathname) {
    struct inode *ip = namei((char *)pathname, 2);
    if (!ip) return -1;
    /* namei(flag=2) supprime l'entrée dans le parent */
    iput(ip);
    return 0;
}

/* Lecture depuis un fichier ouvert */
int lfs_read(int fd, void *buf, int count) {
    if (fd < 0 || fd >= NFILE) return -1;
    struct file *fp = current.u_ofile[fd];
    if (!fp || !(fp->f_flag & IREAD)) return -1;

    struct inode *ip = fp->f_inode;
    int offset = 0, total = 0;
    char block[BSIZE];

    while (total < count && offset < (int)ip->i_size) {
        int bn = offset / BSIZE;
        int boff = offset % BSIZE;
        int blk = bmap(ip, bn, BREAD);
        bread(blk, block);

        int to_copy = BSIZE - boff;
        if (to_copy > count - total) to_copy = count - total;
        if (to_copy > (int)ip->i_size - offset) to_copy = ip->i_size - offset;

        memcpy((char *)buf + total, block + boff, to_copy);
        total += to_copy;
        offset += to_copy;
    }

    return total;
}

/* Écriture dans un fichier ouvert (append) */
int lfs_write(int fd, void *buf, int count) {
    if (fd < 0 || fd >= NFILE) return -1;
    struct file *fp = current.u_ofile[fd];
    if (!fp || !(fp->f_flag & IWRITE)) return -1;

    struct inode *ip = fp->f_inode;
    int total = 0;
    int offset = ip->i_size;
    char block[BSIZE];

    while (total < count) {
        int bn = offset / BSIZE;
        int boff = offset % BSIZE;
        int blk = bmap(ip, bn, BWRITE);

        /* Lire l'ancien contenu si partiellement remplissage */
        if (boff) bread(blk, block);
        else memset(block, 0, BSIZE);

        int to_copy = BSIZE - boff;
        if (to_copy > count - total) to_copy = count - total;

        memcpy(block + boff, (char *)buf + total, to_copy);
        bwrite(blk, block);

        total += to_copy;
        offset += to_copy;
    }

    ip->i_size = offset;
    return total;
}

/* Ferme un fichier ouvert */
int lfs_close(int fd) {
    if (fd < 0 || fd >= NFILE) return -1;
    struct file *fp = current.u_ofile[fd];
    if (!fp) return -1;

    current.u_ofile[fd] = NULL;
    iput(fp->f_inode);
    return 0;
}

/* Changement de dossier */
int lfs_chdir(const char * path){
    struct inode * node = namei(path,0);
    if(node){
        return -1; //fd
    }else if(node->i_mode & IFNORM){
        return -1;
    }else{
        current.u_cdir = node->i_numb;
        return 0;
    }
}
/* Parcours la table des fichiers pour un emplacement */
int get_file(struct inode * node, int flags){
    if(node->i_mode&flags){
        for(int i = 0; i < NFILE; i++){
            if(current.u_ofile[i] == NULL){
                current.u_ofile[i] = malloc(sizeof(struct file));
                current.u_ofile[i]->f_flag = flags;
                current.u_ofile[i]->f_inode = node;
                return i;
            }
        }
        fprintf(stderr,"\nNo free file slot\n");
        return -1;

    } else {
        fprintf(stderr,"\nInvalid opening mode\n");
        abort();
        return -1;
    }
}

/* Creation d'un fichier */  // check current
int lfs_creat(const char *pathname, int mode){

    struct inode * node = namei(pathname,0);

    struct inode * new = NULL;
    const char* name = strrchr(pathname, '/')+1; // Verifie que les slash
    // si le nom a pas de / alors strrchr retourne Ox1

    if(name == (char*)0x1){
        //printf("Failure of strrchr");
        name = pathname;
    }
    printf("name: %s",name);

    int new_i_no = 0;
    if(node->i_mode&IFDIR){
        //Recuperation d'une inode libre
            if(super.s_ninode){
                for(int i = 0; i < NIFREE ; i++){
                    if(super.s_inode[i] != 0){
                        new_i_no = super.s_inode[i];
                        new = iget(new_i_no);
                        new->i_mode = mode;

                        super.s_ninode--;
                        super.s_inode[i] = 0;

                    }
                }
            } // else seek all inodes
        /* Recherche  */
        struct direct * folder = malloc(BSIZE);
        memset(folder,0,BSIZE);

        int i = 0;
        for(; i <NADDR-2; i++){
                if(node->i_addr[i])
                    bread(node->i_addr[i],folder);
                else
                    node->i_addr[i] = balloc(); // voir folder et bwrite
        }

        /* Recherche d'une entree directory libre */
        for(int j = 0; j < BSIZE/sizeof(struct direct) ; j++){
            if(folder[j].d_name[0] == '\0'){ // On essaye avec le nom vu que l'inode 0 est valide :c
                strncpy(folder[j].d_name,name,12);
                folder[j].d_ino = new_i_no;

                /* Sync sur disque */
                bwrite(node->i_addr[i],folder);

                return get_file(new,mode);
            }
        }
        int * indir_dir = malloc(BSIZE);
        memset(indir_dir,0,BSIZE);

        /* Recherche dans les blocs indirect */
        if(node->i_addr[NADDR-1]){
            bread(node->i_addr[NADDR-1],indir_dir);
        } else {
            indir_dir[0] = balloc(); // voir folder et bwrite
        }

        /* lecture des directory des blocs indirects */
        for(int k = 0; k < BSIZE/sizeof(int); k++){
            if(indir_dir[k])
                bread(indir_dir[k],folder);
            else
                indir_dir[k] = balloc();

                /* Recherche d'une entree directory libre */
            for(int j = 0; j < BSIZE/sizeof(struct direct) ; j++){
                if(folder[j].d_name[0] == '\0'){ // On essaye avec le nom vu que l'inode 0 est valide :c
                    strncpy(folder[j].d_name,name,12);
                    folder[j].d_ino = new_i_no;

                    /* Sync sur disque */
                    bwrite(node->i_addr[NADDR-1],folder);
                    bwrite(indir_dir[k],folder);
                    free(indir_dir);

                    return get_file(new,mode);
                }
            }
        }


    }else{
        return -1;
    }
return 0;
}

/* Ouverture d'un fichier */
int lfs_open(const char *pathname,int flags ){
    struct inode * node = namei(pathname,0);
    // gestion des flags
    if(!node){
        return lfs_creat(pathname,flags);
    }else{
        return get_file(node,flags);
    }
}



/*  Initialisation en memoire du libfs */
void init_libfs(){
    f_file = fopen("libfs.fs","rb+");
    if(!f_file){
        fprintf(stderr,"Failure in opening libfs.fs, make sure the file exists\n");
        exit(1);
    }

    int size_super = sizeof(super);

    /* Loading super block */

    // skiping boot block
    fseek(f_file,BSIZE,SEEK_SET);
    // reading struct and checking
    fread(&super,size_super,1,f_file);

    // debug
    fprintf(stderr,"FS size : %d, Nb inodes : %d, Free inodes : %d \n",super.s_fsize,super.s_isize,super.s_ninode);

    /* Set current dir */
    current.u_cdir=0;
    iget(0);


}

/*  fermeture et synchro du fichier libfs */
void close_libfs(){

    for(int i = 0; i < NINODE; i++){
        if (inode[i].i_mode)
            iput(&inode[i]);
    }
    fseek(f_file,BSIZE,SEEK_SET);
    fwrite(&super,BSIZE,1 ,f_file);

    fclose(f_file);
}

/* Focntion de recherche d'inode par nom */
struct inode *namei(const char * name, int flag){
    struct inode * i_out = NULL;
    struct inode * i_out_n = NULL;


    /* copy of the name (because strtok) */
    char * new_name = strdup(name);

    /* Seraching from root ? */
    if(name[0]=='/'){
        i_out = &inode[0]; // rootdir
        //printf("\n- namei Debug root");
    }else{
        i_out = iget(current.u_cdir); //current dir
        //printf("\n- namei Debug curr");
    }

    char * path = strtok(new_name,"/");
    while(path){
        printf("\n- namei debug: keyword: %s\n",path);
        printf("\n- namei debug: mode parent: %d\n",i_out->i_mode);
        if((i_out->i_mode)&IFDIR){
            struct direct * dir_names = malloc(BSIZE);
            /* Parcours des blocks directs */
            for(int i = 0; i <NADDR-2; i++){
                i_out_n = NULL;

                /* Si adresse lecture, sinon passage suivant */
                if(i_out->i_addr[i])
                    bread(i_out->i_addr[i],dir_names);
                else
                    continue;


                /* Pour un bloc entier de struct direct , recherche du nom */
                for(int j = 0; j < BSIZE/sizeof(struct direct) ; j++){
                    if(!strcmp(path,dir_names[j].d_name)){
                        i_out_n = iget(dir_names[j].d_ino);
                        break;
                    }
                }
                /* Si un nouveau nom a bien ete trouve */
                if (i_out_n != i_out && i_out_n){ // ici
                    i_out = i_out_n;
                    break;
                }
            }
            /* Parcours des blocs indirects */  // cassé


            if(!i_out_n){
                int * indir_dir = malloc(BSIZE);

                //Verification de la presence du bloc indirect

                if(i_out->i_addr[NADDR-1]){
                    bread(i_out->i_addr[NADDR-1],dir_names);
                } else {
                    break;
                }

                // lecture des adresses des blocs indirects
                for(int k = 0; k < BSIZE/sizeof(int); k++){
                    if(indir_dir[k])
                        bread(indir_dir[k],dir_names);
                    else
                        continue;

                    for(int j = 0; j < BSIZE/sizeof(struct direct) ; j++){
                        if(!strcmp(path,dir_names[j].d_name)){
                            i_out_n = iget(dir_names[j].d_ino);
                            break;
                        }
                    }

                    if (i_out_n != i_out){
                        i_out = i_out_n;
                        break;
                    }
                }
                free(indir_dir);
            }

        free(dir_names);

        }else{ // if name not found
            fprintf(stderr,"\nName not found\n");
            break;
        }

}
    return i_out;
}

/* Recuperation de l'inode */
struct inode * iget(int ino){

    if(ino > (super.s_isize - 1)){
        fprintf(stderr,"\nInode out of range\n");
        return NULL;
    }

    // emplacement de l'entree dans la table des inodes
    int free_slot = -1;
    struct dinode tmp;


    /* parcours des inodes déja en mémoire */
    for(int i = 0 ;i<NINODE ;i++){
        if(inode[i].i_numb==ino && ino)
            return &inode[i];
        if(inode[i].i_mode != 0 && !ino) //gestion root
            return &inode[i];

        if(free_slot == -1 && inode[i].i_mode == 0)
            free_slot=i;
    }

    if(free_slot == -1){
        fprintf(stderr,"\nNo free inode slot\n");
        return NULL;
    } else {
        goto_inodes();
        fseek(f_file,(sizeof(struct dinode)*ino),SEEK_CUR);
        fread(&tmp,sizeof(struct dinode),1,f_file);


        inode[free_slot].i_mode = tmp.di_mode;
        inode[free_slot].i_size = tmp.di_size;
        inode[free_slot].i_numb = ino;
        memcpy(&inode[free_slot],tmp.di_addr,sizeof(int)*6 );

    }


    /* recuperation de l'inode disque */
    goto_inodes();
    fseek(f_file,32*ino,SEEK_CUR); //seek to correct inode

    struct dinode temp;
    fread(&temp,sizeof(temp),1 ,f_file);
    // printf("\niget debug: inode: %d mode: %d , size: %d, zone = %d \n",ino,temp.di_mode,temp.di_size,free_slot);

    // modification of the free_slot in memory
    memcpy(&inode[free_slot].i_addr,&temp.di_addr,sizeof(unsigned int) * 6 );

    inode[free_slot].i_mode = temp.di_mode;
    inode[free_slot].i_size= temp.di_size;
    inode[free_slot].i_numb = ino;

    return &inode[free_slot];
}

/* Suppression de l'inode en memoire et synchro */
void iput(struct inode *ip){

    goto_inodes();
    fseek(f_file,32*ip->i_numb,SEEK_CUR); //seek to correct inode

    // Création d'une inode disque et clonage de l'inode memoire
    struct dinode temp;

    memcpy(&temp.di_addr,&ip->i_addr,sizeof(unsigned int) * 6 );

    temp.di_mode = ip->i_mode;
    temp.di_size = ip->i_size;

    // Ecriture sur disque
    fwrite(&temp,sizeof(temp),1 ,f_file);

    // Libération de l'inode mémoire
    ip->i_mode = 0;
}


/* Fonction a analyser 100% */
int bmap(struct inode *ip, int bn, int flag){


    /* If no indirections */
    if(bn < NADDR-1){
        int realbn = ip->i_addr[bn];
        if(!realbn){
            int newblk = balloc();
            if(newblk > 0){
                ip->i_addr[bn] = newblk;
                return newblk;
            }
            return realbn;
        }

    /* If indirect block not set*/
    } else {
        int realbn = ip->i_addr[NADDR-1];

        if(!realbn){
            int newblk = balloc();
            if(newblk > 0){
                ip->i_addr[NADDR-1] = newblk;
            }
        }

        /* Reading indirect block */
        int * ind_blk = malloc(BSIZE);
        bread(ip->i_addr[NADDR-1],ind_blk);

        int indir_bn = bn - NADDR + 1;
        realbn = ind_blk[indir_bn];
        if(!realbn){
            realbn = balloc();
            if(realbn > 0){
                ind_blk[indir_bn] = realbn;
            }
        }
        free(ind_blk);
        return realbn;
        }
    return -1;
}
