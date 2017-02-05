#ifndef APCTRL_H
#define APCTRL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/time.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>

typedef unsigned long  u64;
typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
#endif

#define APCTRL_IOCTL_MAGIC 0x3e

struct apctrl_caps {
	unsigned long bufs_nr;
};

#define APCTRL_IOCTL_GET_CAPS\
	_IOR(APCTRL_IOCTL_MAGIC, 0, struct apctrl_caps)

#define APCTRL_IOCTL_START\
	_IOW(APCTRL_IOCTL_MAGIC, 1, unsigned long)

#define APCTRL_IOCTL_STOP\
	_IO(APCTRL_IOCTL_MAGIC, 2)

struct apctrl_buf_desc {
	unsigned long nr;
	unsigned long offset;
	unsigned long size;
};

#define APCTL_IOCTL_GET_BUF_DESC\
	_IOWR(APCTRL_IOCTL_MAGIC, 6, struct apctrl_buf_desc)

#define APCTL_IOCTL_ENQUEUE\
	_IOW(APCTRL_IOCTL_MAGIC, 3, unsigned long *)

#define APCTL_IOCTL_QUEUE_POP\
	_IOR(APCTRL_IOCTL_MAGIC, 4, unsigned long)

#define APCTL_IOCTL_RELEASE\
	_IOW(APCTRL_IOCTL_MAGIC, 5, unsigned long)

#define APCTL_IOCTL_GET_CUR\
	_IOR(APCTRL_IOCTL_MAGIC, 7, unsigned long)

#define APCTL_IOCTL_WAIT\
	_IOR(APCTRL_IOCTL_MAGIC, 8, unsigned long)
   
struct apctrl_reg {
	u32 offset;
	u32 val;
	u32 mask;
};

#define APCTL_IOCTL_GETREG\
	_IOR(APCTRL_IOCTL_MAGIC,  9, struct apctrl_reg)

#define APCTL_IOCTL_SETREG\
	_IOW(APCTRL_IOCTL_MAGIC, 10, struct apctrl_reg)

#endif
