#include "struct.h"
#include <string.h>

/* Flags pour bmap : lecture ou écriture */
#define BREAD  0
#define BWRITE 1

/* Fonctions externes pour lecture/écriture de blocs */
extern void bread(int blkno, char *buf);
extern void bwrite(int blkno, char *buf);

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
    int ino_new = ip - inode;           /* numéro d'inode du nouveau répertoire */
    int ino_parent = current.u_cdir;    /* inode du répertoire courant */

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
