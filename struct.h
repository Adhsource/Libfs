/* libfs.h */

/* Taille des blocs */
#define BSIZE   512

/* Nombre d’adresses directes + indirecte */
#define NADDR   6

/* Modes et droits */
#define IFNORM  1   /* Fichier normal */
#define IFDIR   2   /* Répertoire */
#define IREAD   4   /* Lecture */
#define IWRITE  8   /* Écriture */

/* Taille d’entrée de répertoire */
#define DIRSIZ  12

/* Taille table d’inodes libres dans le superbloc */
#define NIFREE  100

/* Nombre max de fichiers ouverts par processus */
#define NFILE   16

/* Nombre max d’inodes en mémoire */
#define NINODE  200

/* --- Structures sur disque --- */

struct dinode {
    unsigned int di_mode;            /* Mode et droit du fichier */
    unsigned int di_size;            /* Nombre d’octets du fichier */
    unsigned int di_addr[NADDR];     /* Blocs composant le fichier */
};

struct direct {
    unsigned int d_ino;              /* Numéro d’inode */
    char         d_name[DIRSIZ];     /* Nom du fichier */
};

struct filsys {
    unsigned int s_fsize;            /* Taille de la partition (blocs) */
    unsigned int s_isize;            /* Nombre d’inodes */
    unsigned int s_ninode;           /* Nombre d’inodes libres */
    unsigned int s_inode[NIFREE];    /* Ensemble des inodes libres */
};

/* --- Structures et variables en mémoire --- */

struct inode {
    unsigned int i_mode;             /* Mode et droits du fichier */
    unsigned int i_size;             /* Nombre d’octets du fichier */
    unsigned int i_addr[NADDR];      /* Blocs composant le fichier */
};

struct file {
    char           f_flag;           /* Mode d’ouverture */
    struct inode  *f_inode;          /* Pointeur vers l’inode en mémoire */
};

struct user {
    int           u_cdir;                     /* Inode du répertoire courant */
    struct file  *u_ofile[NFILE];            /* Fichiers ouverts */
};

extern struct user    current;
extern struct inode   inode[NINODE];

/* --- API publique de la librairie --- */

void init_libfs(void);
void close_libfs(void);

int lfs_mkdir(const char *pathname, int mode);
int lfs_chdir(const char *path);
int lfs_unlink(const char *pathname);
int lfs_creat(const char *pathname, int mode);
int lfs_open(const char *pathname, int flags);
int lfs_read(int fd, void *buf, int count);
int lfs_write(int fd, void *buf, int count);
int lfs_close(int fd);

/* --- Fonctions internes à implémenter --- */

struct inode *namei(char *name, int flag);
struct inode *iget(int ino);
void         iput(struct inode *ip);
int          bmap(struct inode *ip, int bn, int flag);
