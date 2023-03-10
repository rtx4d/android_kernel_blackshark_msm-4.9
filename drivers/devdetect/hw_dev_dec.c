#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/crc-ccitt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/hw_dev_dec.h>
#include <linux/hw_dev_array.h>
#include <linux/of.h>

#define log_info(tmp, arg...)              printk(KERN_INFO tmp, ##arg)
#define log_err(tmp, arg...)               printk(KERN_ERR tmp, ##arg)
#define log_warn(tmp, arg...)              printk(KERN_WARNING tmp, ##arg)
//#define log_debug     printk

#ifndef TYPEDEF_UINT8
typedef unsigned char uint8;
#endif

#ifndef TYPEDEF_UINT16
typedef unsigned short uint16;
#endif

#ifndef TYPEDEF_UINT32
typedef unsigned int uint32;
#endif

#ifndef TYPEDEF_UINT64
typedef unsigned long long uint64;
#endif

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#define NO_CHECK 0 // don't need to check
#define BC_CHECK 1 // check at bc step and running test
#define RUNNING_TEST_CHECK 2 // check at the runnint test

#define PRINT_CHECK_MAX_LENGTH 22 // the device check result's decimalization max length

#define HWLOG_TAG devcheck
//HWLOG_REGIST();

static DEFINE_MUTEX(devcheck_lock);

#define DTS_COMP_DEVICE_CHECK_NAME "device_check" // the compatible name in the dts
struct dev_flag_device{
    const char  *name;
    struct device  *dev;
    int  index;
};

static struct dev_flag_device dev_dct = {
    .name = "dev_flag",
    .index = 0,
};

static struct dev_flag_device boardid_dev_dct = {
    .name = "boardid_devcheck_flag",
    .index = 0,
};
static struct dev_flag_device rt_dev_dct = {
    .name = "rt_devcheck_flag",
    .index = 0,
};


static uint64 dev_flag_long = 0; // save the check result
static uint64 boardid_devcheck_flag_long = 0;// save the config message of device, which need to check at board check and running test.
static uint64 rt_devcheck_flag_long = 0;// save the config message of device

int dev_dct_probe_flag = 0;
/* function: set the device check flag named dev_flag_long. It is used in the device's probe fundtion, which need to check.
    input:
        dev_id: choose the bit in device check table, which define in the hw_dev_check.h.
*/
int set_hw_dev_flag( int dev_id ){
    if( (dev_id >= 0) && ( dev_id < DEV_PERIPHIAL_MAX ) ){
        mutex_lock(&devcheck_lock);
        log_info("now set the dev_flag, and device id is  :%d !\n",dev_id);
        dev_flag_long = dev_flag_long | (1 << dev_id);
        log_info("set the dev_flag successfully in set dev_flag, and dev_flag  is  :%lld !\n",dev_flag_long);
        mutex_unlock(&devcheck_lock);
    }
    else{
        log_err("set the dev_flag fail in set dev_flag, and dev_id  is  :%d ,and it is not in periphial device array!\n",dev_id);
        return false;
    }
    return true;
}
/*
    function: get the device check's attribute value.
    input:
        np: the device node .
        dev_name: the attribute name.
    output:
        type: the dev_name's attribute value.
    return:
        return 0 if get the value successfully,or return the error code.
*/
static int get_hw_dts_devcheck_value(struct device_node *np, const char* dev_name, unsigned int* type){
    int result = of_property_read_u32(np, dev_name, type);
    if (!result){
        return 0;
    }
    else{
        log_warn("Get the node's %s value fail and the reason is %d\n", dev_name, result );
        return result;
    }
}

// show the board device check result
static ssize_t board_dev_flag_show(struct device *dev, struct device_attribute *attr, char *buf){
    int retval = 0;
    int i;
    int flag = 0;

    log_info("now checking the device, and dev_flag is  :%lld !\n",dev_flag_long);
    for (i = 0; i < (sizeof(hw_dec_device_array)/sizeof(hw_dec_struct)); i++ ){
        if ((1ULL << hw_dec_device_array[i].devices_id) & dev_flag_long) {
            continue;
        } else if ((1ULL << hw_dec_device_array[i].devices_id) & boardid_devcheck_flag_long) {
            flag = 1;
            retval += snprintf(buf+retval, PAGE_SIZE+retval, "%s:F\n",
                    hw_dec_device_array[i].devices_name);
        }
    }
    if (0 == flag) {
        retval = snprintf(buf, PAGE_SIZE, "P\n");
    }
    return retval;
}

// show the rt device check result
static ssize_t rt_dev_flag_show(struct device *dev, struct device_attribute *attr, char *buf){
    int retval = 0;
    int i;
    int flag = 0;

    log_info("now checking the device, and dev_flag is  :%lld !\n",dev_flag_long);
    for (i = 0; i < (sizeof(hw_dec_device_array)/sizeof(hw_dec_struct)); i++ ){
        if ((1ULL << hw_dec_device_array[i].devices_id) & dev_flag_long) {
            continue;
        }  else if ((1ULL << hw_dec_device_array[i].devices_id) & rt_devcheck_flag_long) {
            flag = 1;
            retval += snprintf(buf+retval, PAGE_SIZE+retval, "%s:F\n",
                    hw_dec_device_array[i].devices_name);
        }
    }
    if (0 == flag) {
        retval = snprintf(buf, PAGE_SIZE, "P\n");
    }
    return retval;
}

// show the config result which set in the dts, when we need to  check at the board check and running test.
static ssize_t boardid_devcheck_flag_show(struct device *dev, struct device_attribute *attr, char *buf){
    int retval = 0;
    int i;

    log_info("now checking the device, and boardid_devcheck_flag_long is  :%lld !\n",
            boardid_devcheck_flag_long);
    for (i = 0; i < (sizeof(hw_dec_device_array)/sizeof(hw_dec_struct)); i++ ){
        if ((1ULL << hw_dec_device_array[i].devices_id) & boardid_devcheck_flag_long) {
            retval += snprintf(buf+retval, PAGE_SIZE+retval, "%s\n",
                    hw_dec_device_array[i].devices_name);
        }
    }
    return retval;

}

// show the config result which set in the dts, when we need to check at the running test.
static ssize_t rt_devcheck_flag_show(struct device *dev, struct device_attribute *attr, char *buf){
    int retval = 0;
    int i;

    log_info("now checking the device, and rt_devcheck_flag_long is  :%lld !\n",rt_devcheck_flag_long);
    for (i = 0; i < (sizeof(hw_dec_device_array)/sizeof(hw_dec_struct)); i++ ){
        if ((1ULL << hw_dec_device_array[i].devices_id) & rt_devcheck_flag_long) {
            retval += snprintf(buf+retval, PAGE_SIZE+retval, "%s\n",
                    hw_dec_device_array[i].devices_name);
        }
    }
    return retval;
}

static DEVICE_ATTR(board_dev_flag, S_IRUGO, board_dev_flag_show, NULL);
static DEVICE_ATTR(rt_dev_flag, S_IRUGO, rt_dev_flag_show, NULL);

static DEVICE_ATTR(boardid_devcheck_flag, S_IRUGO, boardid_devcheck_flag_show, NULL);

static DEVICE_ATTR(rt_devcheck_flag, S_IRUGO, rt_devcheck_flag_show, NULL);


static int  dev_dct_probe(struct platform_device *pdev){
    int ret = 0;
    int i;
    int result;
    unsigned int type = 0;
    struct device *dev;
    struct device_node *np;
    struct class *myclass;

    if (dev_dct_probe_flag)
    {
        log_info("dev_dct_probe is done. Can't enter again!\n");
        return 0;
    }

    log_info("Enter device check function!\n");

    dev = &pdev->dev;
    np = dev->of_node;
    if (np == NULL){
        log_err("Unable to find %s device node\n", DTS_COMP_DEVICE_CHECK_NAME );
        return -ENOENT;
    }

    myclass = class_create (THIS_MODULE, "dev_dct");
    dev_dct.dev = device_create (myclass, NULL, MKDEV(0, dev_dct.index), NULL, dev_dct.name);
    ret = device_create_file(dev_dct.dev, &dev_attr_board_dev_flag);
    if (ret < 0){
        log_err("device_create_file fail at  board_dev_flag\n");
    }
    ret = device_create_file(dev_dct.dev, &dev_attr_rt_dev_flag);
    if (ret < 0){
        log_err("device_create_file fail at  rt_dev_flag\n");
    }

    boardid_dev_dct.dev = device_create(myclass, NULL, MKDEV(0, boardid_dev_dct.index), NULL, boardid_dev_dct.name);
    ret = device_create_file(boardid_dev_dct.dev, &dev_attr_boardid_devcheck_flag);
    if (ret < 0){
        log_err("device_create_file fail at  boardid_devcheck_flag\n");
    }

    rt_dev_dct.dev = device_create (myclass, NULL, MKDEV(0,rt_dev_dct.index), NULL, rt_dev_dct.name);
    ret = device_create_file (rt_dev_dct.dev, &dev_attr_rt_devcheck_flag);
    if (ret < 0){
        log_err("device_create_file fail at  rt_devcheck_flag\n");
    }

    for (i = 0; i<sizeof(hw_dec_device_array)/sizeof(hw_dec_struct); i++){
        if (!strncmp(hw_dec_device_array[i].devices_name, "NULL" , strlen("NULL")+1)){
            continue;
        }
        result = get_hw_dts_devcheck_value (np, hw_dec_device_array[i].devices_name, &type);
        if (!result){
            if (type == NO_CHECK){
                continue;
            }
            else if (type == BC_CHECK ){
                boardid_devcheck_flag_long = boardid_devcheck_flag_long | (1ULL << hw_dec_device_array[i].devices_id);

                if (hw_dec_device_array[i].devices_id >= DEV_PERIPHIAL_START && hw_dec_device_array[i].devices_id < DEV_PERIPHIAL_MAX){
                    rt_devcheck_flag_long = rt_devcheck_flag_long | (1ULL << hw_dec_device_array[i].devices_id);
                }
            }
            else if (type == RUNNING_TEST_CHECK ){
                if (hw_dec_device_array[i].devices_id >= DEV_PERIPHIAL_START && hw_dec_device_array[i].devices_id < DEV_PERIPHIAL_MAX){
                    rt_devcheck_flag_long = rt_devcheck_flag_long | (1ULL << hw_dec_device_array[i].devices_id);
                }
            }
            else{
                log_warn("device check type:%d is not support!", type);
            }
        }
    }

    dev_dct_probe_flag = 1;
    log_info("device check function end!\n");

    return 0;
}

static const struct of_device_id device_check_match_table[] = {
    {
        .compatible = DTS_COMP_DEVICE_CHECK_NAME,
        .data = NULL,
    },
};
MODULE_DEVICE_TABLE(of, device_check_match_table);


static struct platform_driver dev_dct_driver = {
    .driver = {
    .name   = DTS_COMP_DEVICE_CHECK_NAME,
    .of_match_table = of_match_ptr (device_check_match_table),
    },
    .probe  = dev_dct_probe,
    .remove = NULL,
};

static int __init hw_dev_dct_init(void){
    return platform_driver_register(&dev_dct_driver);
}

static void __exit hw_dev_dct_exit(void){
    platform_driver_unregister(&dev_dct_driver);
}

/* priority is 7s */
late_initcall_sync(hw_dev_dct_init);

module_exit(hw_dev_dct_exit);

MODULE_AUTHOR("sjm");
MODULE_DESCRIPTION("Device Detect Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dev_dct");