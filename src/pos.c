/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pos - implementation of pos.h                                    */
/*                                                                   */
/*********************************************************************/

#include <stddef.h>

#include "pos.h"
#include "support.h"

static void int86n(unsigned int intno);
static void int86i(unsigned int intno, union REGS *regsin);


void PosTermNoRC(void)
{
    int86n(0x20);
    return;
}

void PosDisplayOutput(int ch)
{
    union REGS regsin;
    
    regsin.h.ah = 0x02;
    regsin.h.dl = ch;
    int86i(0x21, &regsin);
    return;
}

/* Written By NECDET COKYAZICI, Public Domain */

int PosGetCharInputNoEcho(void)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x08;
    int86(0x21, &regsin, &regsout);

    return (regsout.h.al);
}

void PosDisplayString(const char *buf)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.h.ah = 0x09;
#ifdef __32BIT__
    regsin.d.edx = (int)buf;
#else        
    regsin.x.dx = FP_OFF(buf);
    sregs.ds = FP_SEG(buf);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    return;
}


int PosSelectDisk(int drive)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x0e;
    regsin.h.dl = drive;
    int86(0x21, &regsin, &regsout);
    return (regsout.h.al);
}

int PosGetDefaultDrive(void)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x19;
    int86(0x21, &regsin, &regsout);
    return (regsout.h.al);
}

void PosSetDTA(void *dta)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;

    regsin.h.ah = 0x1a;
#ifdef __32BIT__
    regsin.d.edx = (int)dta;
#else        
    sregs.ds = FP_SEG(dta);
    regsin.x.dx = FP_OFF(dta);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    return;
}

void PosSetInterruptVector(int intnum, void *handler)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x25;
    regsin.h.al = (char)intnum;
#ifdef __32BIT__
    regsin.d.edx = (int)handler;
#else
    sregs.ds = FP_SEG(handler);
    regsin.x.dx = FP_OFF(handler);
#endif
    int86x(0x21, &regsin, &regsout, &sregs);
    return;
}

void PosGetSystemDate(int *year, int *month, int *day, int *dow)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x2a;
    int86(0x21, &regsin, &regsout);
    *year = regsout.x.cx;
    *month = regsout.h.dh;
    *day = regsout.h.dl;
    *dow = regsout.h.al;
    return;
}

void PosGetSystemTime(int *hour, int *min, int *sec, int *hundredths)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x2c;
    int86(0x21, &regsin, &regsout);
    *hour = regsout.h.ch;
    *min = regsout.h.cl;
    *sec = regsout.h.dh;
    *hundredths = regsout.h.dl;
    return;
}

void *PosGetDTA(void)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    void *dta;

    regsin.h.ah = 0x2f;
    int86x(0x21, &regsin, &regsout, &sregs);
#ifdef __32BIT__
    dta = (void *)regsout.d.ebx;
#else        
    dta = MK_FP(sregs.es, regsout.x.bx);
#endif    
    return (dta);
}

unsigned int PosGetDosVersion(void)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x30;
    int86(0x21, &regsin, &regsout);
    return ((regsout.h.al << 8) | regsout.h.ah);
}

void *PosGetInterruptVector(int intnum)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x35;
    regsin.h.al = (char)intnum;
    int86x(0x21, &regsin, &regsout, &sregs);
#ifdef __32BIT__
    return ((void *)regsout.d.ebx);
#else        
    return (MK_FP(sregs.es, regsout.x.bx));
#endif
}

int PosChangeDir(char *to)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x3b;
#ifdef __32BIT__
    regsin.d.edx = (int)to;
#else        
    regsin.x.dx = FP_OFF(to);
    sregs.ds = FP_SEG(to);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    if (regsout.x.cflag)
    {
        return (regsout.x.ax);
    }
    else
    {
        return (0);
    }    
}

int PosOpenFile(const char *buffer,
                int mode,
                int *handle)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x3d;
    regsin.h.al = (unsigned char)mode;
#ifdef __32BIT__    
    regsin.d.edx = (unsigned int)buffer;
#else
    sregs.ds = FP_SEG(buffer);
    regsin.x.dx = FP_OFF(buffer);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
#ifdef __32BIT__
    *handle = regsout.d.eax;
#else
    *handle = regsout.x.ax;
#endif
    return (regsout.x.cflag);
}

int PosCloseFile(int handle)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x3e;
#ifdef __32BIT__
    regsin.d.ebx = handle;
#else        
    regsin.x.bx = handle;
#endif
    int86(0x21, &regsin, &regsout);
#ifdef __32BIT__
    if (!regsout.x.cflag)
    {
        regsout.d.eax = 0;
    }
    return (regsout.d.eax);
#else
    if (!regsout.x.cflag)
    {
        regsout.x.ax = 0;
    }
    return (regsout.x.ax);
#endif            
}

void PosReadFile(int fh, void *data, size_t bytes, size_t *readbytes)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
     
    regsin.h.ah = 0x3f;
#ifdef __32BIT__
    regsin.d.ebx = (unsigned int)fh;
    regsin.d.ecx = (unsigned int)bytes;    
    regsin.d.edx = (unsigned int)data;
#else        
    regsin.x.bx = (unsigned int)fh;
    regsin.x.cx = (unsigned int)bytes;
    sregs.ds = FP_SEG(data);
    regsin.x.dx = FP_OFF(data);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    if (regsout.x.cflag)
    {
        *readbytes = 0;
    }
    else
    {
#ifdef __32BIT__
        *readbytes = regsout.d.eax;
#else            
        *readbytes = regsout.x.ax;
#endif
    }
    return;
}

int PosWriteFile(int handle,
                 const void *data,
                 size_t len)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x40;
#ifdef __32BIT__
    regsin.d.ebx = (unsigned int)handle;
    regsin.d.ecx = (unsigned int)len;    
    regsin.d.edx = (unsigned int)data;
#else        
    regsin.x.bx = (unsigned int)handle;
    regsin.x.cx = (unsigned int)len;
    sregs.ds = FP_SEG(data);
    regsin.x.dx = FP_OFF(data);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    if (regsout.x.cflag)
    {
        return (-1);
    }
    else
    {
#ifdef __32BIT__
        return (regsout.d.eax);
#else            
        return (regsout.x.ax);
#endif
    }
}

int PosDeleteFile(const char *fname)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x41;
#ifdef __32BIT__
    regsin.d.edx = (int)fname;
#else        
    sregs.ds = FP_SEG(fname);
    regsin.x.dx = FP_OFF(fname);
#endif
    int86x(0x21, &regsin, &regsout, &sregs);
    if (!regsout.x.cflag)
    {
        regsout.x.ax = 0;
    }
    return (regsout.x.ax);
}

long PosMoveFilePointer(int handle, long offset, int whence)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x42;
#ifdef __32BIT__
    regsin.d.ebx = handle;
    regsin.d.edx = offset;
#else        
    regsin.x.bx = handle;
    regsin.x.dx = FP_OFF(offset);
    regsin.x.cx = FP_SEG(offset);
#endif
    regsin.h.al = (char)whence;
    int86(0x21, &regsin, &regsout);
#ifdef __32BIT__
    if (!regsout.x.cflag)
    {
        regsout.d.eax = 0xffffffff;
    }
    return (regsout.d.eax);
#else
    if (!regsout.x.cflag)
    {
        regsout.x.ax = 0xffff;
        regsout.x.dx = 0xffff;
    }
    return (((unsigned long)regsout.x.dx << 16) | regsout.x.ax);
#endif            
}

int PosGetDeviceInformation(int handle, unsigned int *devinfo)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x44;
    regsin.h.al = 0x00;
#ifdef __32BIT__
    regsin.d.ebx = handle;
#else
    regsin.x.bx = handle;
#endif
    int86(0x21, &regsin, &regsout);
    if (!regsout.x.cflag)
    {
        regsout.x.ax = 0;
#ifdef __32BIT__
        *devinfo = regsout.d.edx;
#else
        *devinfo = regsout.x.dx;
#endif
    }
    return (regsout.x.ax);
}

int PosBlockDeviceRemovable(int drive)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x44;
    regsin.h.al = 0x08;
    regsin.h.bl = (char)drive;
    int86(0x21, &regsin, &regsout);
#ifdef __32BIT__
    if (regsout.x.cflag)
    {
        regsout.d.eax = -regsout.d.eax;
    }
    return (regsout.d.eax);
#else
    if (regsout.x.cflag)
    {
        regsout.x.ax = -regsout.x.ax;
    }
    return (regsout.x.ax);
#endif            
}

int PosGetCurDir(int drive, char *dir)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x47;
    regsin.h.dl = drive;
#ifdef __32BIT__    
    regsin.d.esi = (unsigned int)dir;
#else
    regsin.x.si = FP_OFF(dir);
    sregs.ds = FP_SEG(dir);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    if (regsout.x.cflag)
    {
#ifdef __32BIT__    
        return (regsout.d.eax);
#else
        return (regsout.x.ax);
#endif        
    }
    else
    {
        return (0);
    }    
}

void *PosAllocMem(unsigned int size)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x48;
#ifdef __32BIT__
    regsin.d.ebx = size;
#else        
    regsin.x.bx = size;
    regsin.x.bx >>= 4;
    if ((size % 16) != 0)
    {
        regsin.x.bx++;
    }
#endif
    int86(0x21, &regsin, &regsout);
#ifdef __32BIT__
    if (regsout.x.cflag)
    {
        regsout.d.eax = 0;
    }
    return ((void *)regsout.d.eax);
#else
    if (regsout.x.cflag)
    {
        regsout.x.ax = 0;
    }
    return (MK_FP(regsout.x.ax, 0));
#endif            
}

void *PosAllocMemPages(unsigned int pages, unsigned int *maxpages)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x48;
#ifdef __32BIT__
    regsin.d.ebx = pages * 16;
#else        
    regsin.x.bx = pages;
#endif
    int86(0x21, &regsin, &regsout);
#ifdef __32BIT__
    if (regsout.x.cflag)
    {
        regsout.d.eax = 0;
    }
    if (maxpages != NULL)
    {
        *maxpages = regsout.d.ebx;
    }
    return ((void *)regsout.d.eax);
#else
    if (regsout.x.cflag)
    {
        regsout.x.ax = 0;
    }
    if (maxpages != NULL)
    {
        *maxpages = regsout.x.bx;
    }
    return (MK_FP(regsout.x.ax, 0));
#endif            
}

int PosFreeMem(void *ptr)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x49;
#ifdef __32BIT__
    regsin.d.ebx = (int)ptr;
#else        
    sregs.es = FP_SEG(ptr);
#endif
    int86x(0x21, &regsin, &regsout, &sregs);
    if (!regsout.x.cflag)
    {
        regsout.x.ax = 0;
    }
    return (regsout.x.ax);
}

int PosReallocMem(void *ptr, unsigned int newpages, unsigned int *maxp)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x4a;
#ifdef __32BIT__
    regsin.d.ecx = (int)ptr;
    regsin.d.ebx = newpages;
#else        
    sregs.es = FP_SEG(ptr);
    regsin.x.bx = newpages;
#endif
    int86x(0x21, &regsin, &regsout, &sregs);
    if (!regsout.x.cflag)
    {
        regsout.x.ax = 0;
#ifdef __32BIT__
        *maxp = regsout.d.ebx;
#else
        *maxp = regsout.x.bx;
#endif
    }
    return (regsout.x.ax);
}

void PosExec(char *prog, void *parmblock)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
     
    regsin.h.ah = 0x4b;
    regsin.h.al = 0;
#ifdef __32BIT__
    regsin.d.edx = (int)prog;
    regsin.d.ebx = (int)parmblock;
#else
    sregs.ds = FP_SEG(prog);
    regsin.x.dx = FP_OFF(prog);
    sregs.es = FP_SEG(parmblock);
    regsin.x.bx = FP_OFF(parmblock);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    return;
}

void PosTerminate(int rc)
{
    union REGS regsin;
    
    regsin.h.ah = 0x4c;
    regsin.h.al = rc;
    int86i(0x21, &regsin);
    return;
}

int PosGetReturnCode(void)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x4d;
    int86(0x21, &regsin, &regsout);
    return (regsout.x.ax);
}

int PosFindFirst(char *pat, int attrib)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x4e;
    regsin.x.cx = attrib;
#ifdef __32BIT__
    regsin.d.edx = (int)pat;
#else        
    regsin.x.dx = FP_OFF(pat);
    sregs.ds = FP_SEG(pat);
#endif    
    int86x(0x21, &regsin, &regsout, &sregs);
    if (regsout.x.cflag)
    {
        return (regsout.x.ax);
    }
    else
    {
        return (0);
    }
}

int PosFindNext(void)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x4f;
    int86(0x21, &regsin, &regsout);    
    if (regsout.x.cflag)
    {
        return (regsout.x.ax);
    }
    else
    {
        return (0);
    }
}

int PosRenameFile(const char *old, const char *new)
{
    union REGS regsin;
    union REGS regsout;
    struct SREGS sregs;
    
    regsin.h.ah = 0x56;
#ifdef __32BIT__
    regsin.d.edx = (int)old;
    regsin.d.edi = (int)new;
#else        
    sregs.ds = FP_SEG(old);
    regsin.x.dx = FP_OFF(old);
    sregs.es = FP_SEG(new);
    regsin.x.di = FP_OFF(new);
#endif
    int86x(0x21, &regsin, &regsout, &sregs);
    if (!regsout.x.cflag)
    {
        regsout.x.ax = 0;
    }
    return (regsout.x.ax);
}


/* extension to display an integer for debugging purposes */

void PosDisplayInteger(int x)
{
    union REGS regsin;
    
    regsin.h.ah = 0xf3;
    regsin.h.al = 0;
#ifdef __32BIT__    
    regsin.d.ecx = x;
#else
    regsin.x.cx = x;
#endif        
    int86i(0x21, &regsin);
    return;
}

/* extension to reboot computer */

void PosReboot(void)
{
    union REGS regsin;
    
    regsin.h.ah = 0xf3;
    regsin.h.al = 1;
    int86i(0x21, &regsin);
    return;
}

/* pos extension to set runtime library */

void PosSetRunTime(void *pstart, void *capi)
{
    union REGS regsin;
    
    regsin.h.ah = 0xf3;
    regsin.h.al = 2;
#ifdef __32BIT__    
    regsin.d.ebx = (int)pstart;
    regsin.d.ecx = (int)capi;
#endif
    int86i(0x21, &regsin);
}



/* int86n - do an interrupt with no registers */

static void int86n(unsigned int intno)
{
    union REGS regsin;
    union REGS regsout;

    int86(intno, &regsin, &regsout);
    return;
}    

/* int86i - do an interrupt with input registers only */

static void int86i(unsigned int intno, union REGS *regsin)
{
    union REGS regsout;
    
    int86(intno, regsin, &regsout);
    return;
}    
