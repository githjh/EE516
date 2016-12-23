/*
		Dummy Driver for LinkedList
*/

#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/random.h>

#define DUMMY_MAJOR_NUMBER 250
#define DUMMY_DEVICE_NAME "DUMMY_DEVICE"

typedef struct _linkedlist_t {
    int value;
    struct _linkedlist_t* next;
} linkedlist_t;

int dummy_open(struct inode *, struct file *);
int dummy_release(struct inode *, struct file *);
ssize_t dummy_read(struct file *, char *, size_t, loff_t *);
ssize_t dummy_write(struct file *, const char *, size_t, loff_t *);
long dummy_ioctl(struct file *, unsigned int, unsigned long);


/* file operation structure */
struct file_operations dummy_fops ={
	open: dummy_open,
	read: dummy_read,
	write: dummy_write,
	release: dummy_release, 
	unlocked_ioctl : dummy_ioctl,
};

char devicename[20];

static struct cdev my_cdev;
static dev_t device_num;   // For device minor number
static struct class *cl;
struct semaphore sem;
static linkedlist_t *linkedlist_head;
static int linkedlist_len = 0;

static void random_insert(int value)
{
    linkedlist_t *temp_node, *new_node;
    int i;
    int rand_pos = get_random_int(); 

    down(&sem);
    temp_node = linkedlist_head;
    for (i = 0; i <rand_pos; i++) 
        temp_node = temp_node->next;
    
    if(temp_node->next == NULL)
    {
        new_node = (linkedlist_t *)kmalloc(sizeof(linkedlist_t), __GFP_IO | __GFP_FS);
        new_node->value = value;
        new_node->next = NULL;
        temp_node->next = new_node;

        printk("random insert: write %d on %d position in the linked list\n", value, rand_pos); 
    }
    else
    {
        new_node = (linkedlist_t *)kmalloc(sizeof(linkedlist_t), __GFP_IO | __GFP_FS);
        new_node->value = value;
        new_node->next = temp_node->next;
        temp_node->next = new_node;

        printk("random insert: write %d on %d position in the linked list\n", value, rand_pos); 
    }
    linkedlist_len++;
    up(&sem);
}

static int random_delete(void)
{
    linkedlist_t *prev_node, *temp_node;
    int pos = 0;
    int value = get_random_int(); 
    prev_node = linkedlist_head;
    temp_node = prev_node->next;
    for(; temp_node != NULL; temp_node = temp_node->next)
    {
        if(temp_node->value == value)
        {
            prev_node->next= temp_node;
            kfree(temp_node);
            linkedlist_len--;
            printk("random delete: delete %d on %d position in the linked list\n", value, pos);
            return value;
        }
        pos++;
    }
    printk("random delete: failed to delete %d\n", value);

    return -1;
}


static int __init dummy_init(void)
{

    printk("Dummy Driver : Module Init\n");
    strcpy(devicename, DUMMY_DEVICE_NAME);

    // Allocating device region 
    if (alloc_chrdev_region(&device_num, 0, 1, devicename)){
        return -1;
    }
    if ((cl = class_create(THIS_MODULE, "chardrv" )) == NULL) {
        unregister_chrdev_region(device_num, 1);
        return -1;
    }

    // Device Create == mknod /dev/DUMMY_DEVICE 
    if (device_create(cl, NULL, device_num, NULL, devicename) == NULL)	{
        class_destroy(cl);
        unregister_chrdev_region(device_num, 1);
        return -1;
    }

    // Device Init 
    cdev_init(&my_cdev, &dummy_fops);
    if ( cdev_add(&my_cdev, device_num, 1) == -1 ){
        device_destroy(cl, device_num);
        class_destroy(cl);
        unregister_chrdev_region(device_num, 1);
    }

    //semaphore initialization 
    sema_init(&sem, 1);

    //linkedlist header initialization 
    linkedlist_head = (linkedlist_t *)kmalloc(sizeof(linkedlist_t), __GFP_IO | __GFP_FS);


    return 0;
}

static void __exit dummy_exit(void)
{
    linkedlist_t *prev_node, *temp_node; 

    prev_node = linkedlist_head;
    temp_node = prev_node->next;

    //deallocation linkedlist nodes
    while(1)
    {
        kfree(prev_node);
        prev_node = temp_node;

        if(temp_node)
            break;
        else
            temp_node = temp_node->next;
    }

    printk("Dummy Driver : Clean Up Module\n");
    cdev_del(&my_cdev);
    device_destroy(cl, device_num);
    class_destroy(cl);
    unregister_chrdev_region(MKDEV(DUMMY_MAJOR_NUMBER,0),128);
}

ssize_t dummy_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
    int value;

    value = random_delete();
    if(copy_to_user(buffer, &value, sizeof(int)))
        return -EFAULT;

    if(value == -1)
        return -1;

    return sizeof(int);
}

ssize_t dummy_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
    int value;

    if(copy_from_user(&value, buffer, sizeof(int)))
        return -EFAULT;
    random_insert(value);

    return sizeof(int);
}

int dummy_open(struct inode *inode, struct file *file)
{
    printk("Dummy Driver : Open Call\n");
    return 0;
}

int dummy_release(struct inode *inode, struct file *file)
{
    printk("Dummy Driver : Release Call\n");
    return 0;
}

long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long argument)
{
    printk("ioctl");
    return 0;
}


module_init(dummy_init);
module_exit(dummy_exit);

MODULE_DESCRIPTION("Dummy_LinkedList_Driver");
MODULE_LICENSE("GPL");

