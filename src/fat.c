#include <string.h>

#include "fat.h"
#include "bos.h"
#include "unused.h"

static int fatEndCluster(FAT *fat, unsigned int cluster);
static void fatGetStartCluster(FAT *fat, const char *fnm, FATFILE *fatfile);
static void fatNextSearch(FAT *fat, char *search, const char **upto, int *last);
static void fatRootSearch(FAT *fat, char *search, FATFILE *fatfile);
static void fatDirSearch(FAT *fat, char *search, FATFILE *fatfile);
static void fatClusterAnalyse(FAT *fat,
                              unsigned int cluster,
                              unsigned long *startSector,
                              unsigned int *nextCluster);
static void fatDirSectorSearch(FAT *fat,
                               char *search,
                               FATFILE *fatfile,
                               unsigned long startSector,
                               int numsectors);
static void fatReadLogical(FAT *fat, long sector, void *buf);


/*
 * fatDefaults - initial call which cannot fail
 */
 
void fatDefaults(FAT *fat)
{
    fat->sectors_per_cylinder = 1;
    fat->num_heads = 1;
    fat->sectors_per_track = 1;
    fat->bootstart = 0;
    fat->fatstart = 1;
    fat->buf = fat->pbuf;
    return;
}


/*
 * fatInit - initialize the FAT handler.
 */
 
void fatInit(FAT *fat, 
             unsigned char *bpb,
             void (*readLogical)(void *diskptr, long sector, void *buf),
             void *parm)
{
    fat->readLogical = readLogical;
    fat->parm = parm;
    fat->drive = bpb[25];
    fat->sector_size = bpb[0] | ((unsigned int)bpb[1] << 8);
    fat->sectors_per_cluster = bpb[2];
    fat->numfats = bpb[5];
    fat->fatsize = bpb[11] | ((unsigned int)bpb[12] << 8);
    fat->sectors_per_disk = bpb[8] | ((unsigned int)bpb[9] << 8);
    fat->num_heads = bpb[15];
    if (fat->sectors_per_disk == 0)
    {
        fat->sectors_per_disk = 
            bpb[21]
            | ((unsigned int)bpb[22] << 8)
            | ((unsigned long)bpb[23] << 16)
            | ((unsigned long)bpb[24] << 24);
    }
    fat->hidden = bpb[17]
                  | ((unsigned long)bpb[18] << 8)
                  | ((unsigned long)bpb[19] << 16)
                  | ((unsigned long)bpb[20] << 24);
    fat->rootstart = fat->fatsize * fat->numfats + fat->fatstart;
    fat->rootentries = bpb[6] | ((unsigned int)bpb[7] << 8);
    fat->rootsize = fat->rootentries / (fat->sector_size / 32);
    fat->sectors_per_track = (bpb[13] | ((unsigned int)bpb[14] << 8));
    fat->filestart = fat->rootstart + fat->rootsize;
    fat->num_tracks = fat->sectors_per_disk / fat->sectors_per_track;
    fat->num_cylinders = (unsigned int)(fat->num_tracks / fat->num_heads);
    fat->sectors_per_cylinder = fat->sectors_per_track * fat->num_heads;
    if ((fat->sectors_per_disk / fat->sectors_per_cluster) < 4087)
    {
        fat->fat16 = 0;
    }
    else
    {
        fat->fat16 = 1;
    }
    return;
}


/*
 * fatTerm - terminate the FAT handler.
 */
 
void fatTerm(FAT *fat)
{
    unused(fat);
    return;
}


/*
 * fatEndCluster - determine whether the cluster number provided
 * is an indicator of "no more clusters, it's time to go to bed",
 * courtesy Neil, Young Ones.
 */

static int fatEndCluster(FAT *fat, unsigned int cluster)
{
    if (fat->fat16)
    {
        if (cluster >= 0xfff8U)
        {
            return (1);
        }
    }
    else
    {
        if (cluster >= 0xff8U)
        {
            return (1);
        }
    }
    return (0);
}


/* If opening root directory, abide by rootentries.  If opening
   subdirectory or file, follow clusters */

int fatOpenFile(FAT *fat, const char *fnm, FATFILE *fatfile)
{
    fat->notfound = 0;
    if ((fnm[0] == '\\') || (fnm[0] == '/'))
    {
        fnm++;
    }
    if (strcmp(fnm, "") == 0)
    {
        fatfile->root = 1;
        fatfile->nextCluster = 0xffff;
        fatfile->sectorCount = fat->rootsize;
        fatfile->sectorStart = fat->rootstart;
        fatfile->lastBytes = 0;
        fatfile->lastSectors = fat->rootsize;
        fatfile->cluster = 0;
        fatfile->dir = 1;
    }
    else
    {
        fatfile->root = 0;
        fatGetStartCluster(fat, fnm, fatfile);
        if (fat->notfound) return (-1);
        fatfile->lastBytes = (unsigned int)
                             (fatfile->fileSize 
                              % (fat->sectors_per_cluster
                                 * fat->sector_size));
        fatfile->lastSectors = fatfile->lastBytes / fat->sector_size;
        fatfile->lastBytes = fatfile->lastBytes % fat->sector_size;
        fatfile->dir = (fatfile->fileSize == 0);
        fatClusterAnalyse(fat, 
                          fatfile->cluster,
                          &fatfile->sectorStart,
                          &fatfile->nextCluster);
        fatfile->sectorCount = fat->sectors_per_cluster;
    }
    fatfile->currentCluster = fatfile->cluster;
    fatfile->sectorUpto = 0;
    fatfile->byteUpto = 0;
    if (fat->notfound)
    {
        return (-1);    
    }
    return (0);
}


/*
 * fatReadFile - read from an already-open file.
 */
 
size_t fatReadFile(FAT *fat, FATFILE *fatfile, void *buf, size_t szbuf)
{
    size_t bytesRead = 0;
    static unsigned char bbuf[MAXSECTSZ];
    int sectorsAvail;
    int bytesAvail;
    
    bytesAvail = fat->sector_size;
    sectorsAvail = fatfile->sectorCount;
    while (!fatEndCluster(fat, fatfile->currentCluster))
    {
        sectorsAvail = fatfile->sectorCount;
        if (fatEndCluster(fat, fatfile->nextCluster) && !fatfile->dir)
        {
            sectorsAvail = fatfile->lastSectors + 1;
        }
        while (fatfile->sectorUpto != sectorsAvail)
        {
            bytesAvail = fat->sector_size;
            if (fatEndCluster(fat, fatfile->nextCluster) && !fatfile->dir)
            {
                if (fatfile->sectorUpto == fatfile->lastSectors)
                {
                    bytesAvail = fatfile->lastBytes;
                }
            }
            while (fatfile->byteUpto != bytesAvail)
            {
                fatReadLogical(fat,
                               fatfile->sectorStart + fatfile->sectorUpto, 
                               bbuf);
                if ((bytesAvail - fatfile->byteUpto)
                    > (szbuf - bytesRead))
                {
                    memcpy((char *)buf + bytesRead, 
                           bbuf + fatfile->byteUpto,
                           szbuf - bytesRead);
                    fatfile->byteUpto += (szbuf - bytesRead);
                    bytesRead = szbuf;
                    return (bytesRead);
                }
                else
                {
                    memcpy((char *)buf + bytesRead,
                           bbuf + fatfile->byteUpto,
                           bytesAvail - fatfile->byteUpto);
                    bytesRead += (bytesAvail - fatfile->byteUpto);
                    fatfile->byteUpto += (bytesAvail - fatfile->byteUpto);
                }
            }
            fatfile->sectorUpto++;
            fatfile->byteUpto = 0;
        }
        fatfile->currentCluster = fatfile->nextCluster;
        fatClusterAnalyse(fat, 
                          fatfile->currentCluster, 
                          &fatfile->sectorStart,
                          &fatfile->nextCluster);
        fatfile->sectorUpto = 0;
    }
    return (bytesRead);
}


/*
 * fatGetStartCluster - given a filename, retrieve the starting
 * cluster, or set notfound flag if not found.
 */

static void fatGetStartCluster(FAT *fat, const char *fnm, FATFILE *fatfile)
{
    char search[11];
    const char *upto = fnm;
    int last;

    fatfile->cluster = 0;
    fatNextSearch(fat, search, &upto, &last);
    if (fat->notfound) return;
    fatRootSearch(fat, search, fatfile);
    while (!last)
    {
        fatNextSearch(fat, search, &upto, &last);
        if (fat->notfound) return;
        fatDirSearch(fat, search, fatfile);
    }
    return;
}


/*
 * fatNextSearch - get the next part of the filename in order
 * to do a search.
 *
 * Inputs:
 * fat - pointer to the current FAT object, unused.
 * *upto - the bit of the filename we are ready to search
 * 
 * Outputs:
 * *upto - the bit of the filename to search next time
 * search - set to the portion of the filename we should look
 *          for in the current directory
 * *last - set to 1 or 0 depending on whether this was the last
 *         portion of the filename.
 *
 * e.g. if the original filename was \FRED\MARY\JOHN.TXT and
 * *upto was pointing to MARY... on input, search will get
 * "MARY            " in it, and *upto will point to "JOHN...".
 */

static void fatNextSearch(FAT *fat, char *search, const char **upto, int *last)
{
    const char *p;
    const char *q;

    unused(fat);    
    p = strchr(*upto, '\\');
    q = strchr(*upto, '/');
    if (p == NULL)
    {
        p = q;
    }
    else if (q != NULL)
    {
        if (q < p)
        {
            p = q;
        }
    }
    if (p == NULL)
    {
        p = *upto + strlen(*upto);
        *last = 1;
    }
    else
    {
        *last = 0;
    }
    if ((p - *upto) > 12)
    {
        fat->notfound = 1;
        return;
    }
    q = memchr(*upto, '.', p - *upto);
    if (q != NULL)
    {
        if ((q - *upto) > 8)
        {
            fat->notfound = 1;
            return;
        }
        if ((p - q) > 4)
        {
            fat->notfound = 1;
            return;
        }
        memcpy(search, *upto, q - *upto);
        memset(search + (q - *upto), ' ', 8 - (q - *upto));
        memcpy(search + 8, q + 1, p - q - 1);
        memset(search + 8 + (p - q) - 1, ' ', 3 - ((p - q) - 1));
    }
    else
    {
        memcpy(search, *upto, p - *upto);
        memset(search + (p - *upto), ' ', 11 - (p - *upto));
    }
    if (*last)
    {
        *upto = p;
    }
    else
    {
        *upto = p + 1;
    }
    return;
}


/*
 * fatRootSearch - search the root directory for an entry
 * (given by "search").  The root directory is defined by a
 * starting sector number and the previously determined size
 * (number of sectors).
 */
 
static void fatRootSearch(FAT *fat, char *search, FATFILE *fatfile)
{
    fatDirSectorSearch(fat, search, fatfile, fat->rootstart, fat->rootsize);
    return;
}


/*
 * fatDirSearch - search a directory chain looking for the
 * search string.
 */
 
static void fatDirSearch(FAT *fat, char *search, FATFILE *fatfile)
{
    unsigned long startSector;
    unsigned int nextCluster;
    
    fatClusterAnalyse(fat, fatfile->cluster, &startSector, &nextCluster);
    fatDirSectorSearch(fat, 
                       search, 
                       fatfile, 
                       startSector, 
                       fat->sectors_per_cluster);
    while (fatfile->cluster == 0)    /* not found but not end */
    {
        if (fatEndCluster(fat, nextCluster))
        {
            fat->notfound = 1;
            return;
        }
        fatfile->cluster = nextCluster;
        fatClusterAnalyse(fat, fatfile->cluster, &startSector, &nextCluster);
        fatDirSectorSearch(fat, 
                           search, 
                           fatfile,
                           startSector,
                           fat->sectors_per_cluster);
    }
    return;
}


/*
 * fatClusterAnalyse - given a cluster number, this function
 * determines the sector that the cluster starts and also the
 * next cluster number in the chain.
 */
 
static void fatClusterAnalyse(FAT *fat,
                              unsigned int cluster,
                              unsigned long *startSector,
                              unsigned int *nextCluster)
{
    unsigned int fatSector;
    static unsigned char buf[MAXSECTSZ];
    int offset;
    
    *startSector = (cluster - 2) * (long)fat->sectors_per_cluster
                   + fat->filestart;
    if (fat->fat16)
    {
        fatSector = fat->fatstart + (cluster * 2) / fat->sector_size;
        fatReadLogical(fat, fatSector, buf);
        offset = (cluster * 2) % fat->sector_size;
        *nextCluster = buf[offset] | ((unsigned int)buf[offset + 1] << 8);
    }
    else
    {
        fatSector = fat->fatstart + (cluster * 3 / 2) / fat->sector_size;
        fatReadLogical(fat, fatSector, buf);
        offset = (cluster * 3 / 2) % fat->sector_size;
        *nextCluster = buf[offset];
        if (offset == (fat->sector_size - 1))
        {
            fatReadLogical(fat, fatSector + 1, buf);
            offset = -1;
        }
        *nextCluster = *nextCluster | ((unsigned int)buf[offset + 1] << 8);
        if ((cluster * 3 % 2) == 0)
        {
            *nextCluster = *nextCluster & 0xfffU;
        }
        else
        {
            *nextCluster = *nextCluster >> 4;
        }
    }
    return;
}                   


/*
 * fatDirSectorSearch - go through a block of sectors (as specified
 * by startSector and numsectors) which consist entirely of directory
 * entries, looking for the "search" string.  When found, retrieve some
 * information about the file, including the starting cluster and the
 * file size.  If we get to the end of the directory (NUL in first
 * character of directory entry), set the notfound flag.  Otherwise,
 * if we reach the end of this block of sectors without reaching the
 * end of directory marker, we set the cluster to 0.
 */

static void fatDirSectorSearch(FAT *fat,
                               char *search,
                               FATFILE *fatfile,
                               unsigned long startSector,
                               int numsectors)
{
    int x;
    unsigned char buf[MAXSECTSZ];
    unsigned char *p;
    
    for (x = 0; x < numsectors; x++)
    {
        fatReadLogical(fat, startSector + x, buf);
        for (p = buf; p < buf + fat->sector_size; p += 32)
        {            
            if (memcmp(p, search, 11) == 0)
            {
                fatfile->cluster = p[0x1a] | (p[0x1a + 1] << 8);
                fatfile->fileSize = p[0x1c]
                                    | ((unsigned int)p[0x1c + 1] << 8)
                                    | ((unsigned long)p[0x1c + 2] << 16)
                                    | ((unsigned long)p[0x1c + 3] << 24);
                fatfile->attr = p[0x0b];
                memcpy(fatfile->datetime, &p[0x16], 4);
                return;
            }
            else if (*p == '\0')
            {
                fat->notfound = 1;
                return;
            }
        }
    }
    fatfile->cluster = 0;
    return;
}


/*
 * fatReadLogical - read a logical disk sector by calling the
 * function provided at initialization.
 */
 
static void fatReadLogical(FAT *fat, long sector, void *buf)
{
    fat->readLogical(fat->parm, sector, buf);
    return;
}
