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
#define NUMMAX 10
#define DUMMY_DEVICE_NAME "DUMMY_DEVICE"

//node of a linked list
typedef struct _linkedlist_t {
    int value; //value
    struct _linkedlist_t* next; //next node inside a linked-list
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
struct semaphore sem; //semaphore used in random_insert() and random_delete()
static linkedlist_t *linkedlist_head; //linked-list header
static int linkedlist_len = 0; //length of the linked-list which head is 'linkedlist_head'

/*************************************************************
 * FUNCTION NAME: random_insert                                         
 * PARAMETER: 1)value: a value which user want to write 
 * PURPOSE: insert a node into the linked list in random postion  
 ************************************************************/
static void random_insert(int value)
{
    linkedlist_t *iter_node, *new_node;
    int i;
    //get a random value
    int rand_pos = get_random_int() % (linkedlist_len + 1); 

    //get the lock (entering critical region)
    down(&sem);
    //printk("random insert: value=%d, rand_pos=%d, linkedlist_len=%d\n", value, rand_pos, linkedlist_len);
    iter_node = linkedlist_head;
    //move into the random value's index inside the linked-list
    for (i = 0; i < rand_pos; i++) 
        iter_node = iter_node->next;
    
    //case1: randomly selected position is tail of the linked-list
    if(rand_pos == linkedlist_len)
    {
        new_node = (linkedlist_t *)kmalloc(sizeof(linkedlist_t), __GFP_IO | __GFP_FS);
        new_node->value = value;
        new_node->next = NULL;
        iter_node->next = new_node;

        printk("random insert: write %d on %d position in the linked list\n", value, rand_pos); 
    }
    //case2: randomly selected position is not tail of the linked-list
    else
    {
        new_node = (linkedlist_t *)kmalloc(sizeof(linkedlist_t), __GFP_IO | __GFP_FS);
        new_node->value = value;
        new_node->next = iter_node->next;
        iter_node->next = new_node;

        printk("random insert: write %d on %d position in the linked list\n", value, rand_pos); 
    }
    linkedlist_len++;
    //release the lock
    up(&sem);
}

/*************************************************************
 * FUNCTION NAME: randomd_delete                                         
 * PARAMETER: NONE                                              
 * PURPOSE: delete a node randomly 
 ************************************************************/
static int random_delete(void)
{
    linkedlist_t *prev_node, *iter_node;
    int pos = 0;
    //get a random value
    int value = get_random_int() % NUMMAX; 
    
    //get the lock (entering the critical region)
    down(&sem);
    //printk("random delete: value=%d, linkedlist_len=%d\n", value, linkedlist_len);
    //if the linked-list has no elements, return -1
    if(linkedlist_len == 0)
    {
        printk("random delete(no node): failed to delete %d\n", value);
        up(&sem);
        return -1;
    }

    prev_node = linkedlist_head;
    iter_node = prev_node;

    //traverse through the linked-list, and find a node which has same value as the random value above
    while(pos < linkedlist_len)
    {
        iter_node = iter_node->next;

        //case 1: find a node
        if(iter_node->value == value)
        {
            if(pos == (linkedlist_len - 1))
                prev_node->next = NULL;
            else
                prev_node->next = iter_node->next;

            kfree(iter_node);
            linkedlist_len--;
            printk("random delete: delete %d on %d position in the linked list\n", value, pos);
            up(&sem);
            return value;
        }

        prev_node = prev_node->next;
        pos++;
    }

    //case 2: fail to find a node
    printk("random delete: failed to delete %d\n", value);
//    for debugging / iterating a linkend list
//    iter_node = linkedlist_head;
//    for(pos = 0; pos < linkedlist_len; pos++)
//    {
//        iter_node = iter_node->next;
//        printk("pos=%d, value=%d\n", pos, iter_node->value);
//    }
    //release the lock
    up(&sem); 
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

//read a random value from the linked-list and copy it to a user buffer
ssize_t dummy_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
    int value;

    value = random_delete();
    if(copy_to_user(buffer, &value, sizeof(int)))
        return -EFAULT;

    return 0;
}

//get a random value from a user buffer and write it into the linked-list
ssize_t dummy_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
    int value;

    if(copy_from_user(&value, buffer, sizeof(int)))
        return -EFAULT;

    random_insert(value);

    return 0;
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

