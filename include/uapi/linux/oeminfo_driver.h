/*
 * OEMINFO PARTITION
 */

#ifndef _OEMINFO_DRIVER_H_
#define _OEMINFO_DRIVER_H_

enum oeminfo__index {
    OEMINFO_PRODUCTLINE = 0,
    OEMINFO_CUSTOMIZATION,
    OEMINFO_LCD,
    OEMINFO_CALIBRATION,
    OEMINFO_MISC,
    OEMINFO_FCTINFO,
    OEMINFO_COUNT,//MUST be the last
};

#define TAG_LENGTH    16
#define ONE_BLOCK_SIZE    512

#define OEMINFO_IOC_MAGIC  'k'

#define OEMINFO_IOCGETDATA  _IOR(OEMINFO_IOC_MAGIC, 1, char[ONE_BLOCK_SIZE])
#define OEMINFO_IOCSETDATA  _IOW(OEMINFO_IOC_MAGIC, 2, char[ONE_BLOCK_SIZE])

#define OEMINFO_IOC_MAXNR 3

#endif

