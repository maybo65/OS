// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE



#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/ioctl.h>


MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations

#include "message_slot.h"

channels device_info[257];

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode, struct file*  file )
{
  file_data *data;
  data = kmalloc(sizeof(file_data), GFP_KERNEL);
  if (data==NULL){
    printk("malloc failed in device_open(%p)\n", file);
		return -ENOMEM;
  }
  data->minor_num = iminor(inode);
  data->curr_channel=NULL;
  file->private_data = (void*) (data);
  return 0;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode, struct file*  file)
{
  printk("Invoking device_release(%p,%p)\n", inode, file);
  kfree(file->private_data);
  return 0;
}


//==================== UPDATE MSG SLOTH =============================
static int update_channel(node* channel, int len){
if (channel->msg != NULL){
  kfree(channel->msg);
} 
channel->msg=kmalloc(len * sizeof(char), GFP_KERNEL);
if (channel->msg==NULL){
  printk("memory allocation failed.\n");
  return -1;
}
channel->msg_size=len;
return 0;
}
//----------------------------------------------------------------


//==================== UPDATE DATA FILE =============================
static void update_data_file(file_data *data, node* channel, unsigned long  new_channel_id){
	data->curr_channel_id=new_channel_id;
	data->curr_channel=channel;
}
//----------------------------------------------------------------

//==================== UPDATE NODE =============================
static void update_node(node* channel, unsigned long  new_channel_id){
	channel->channel_id= new_channel_id;
  channel->next= NULL;
  channel->msg_size= 0;
  channel->msg= NULL;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file, char __user* buffer, size_t length,  loff_t* offset )
{
  int i;
  int error=0;
  int BUF_LEN;
  char* msg;
  file_data *data = (file_data*)(file->private_data);
  if (data->curr_channel_id==0){ //no chanel has been set
    return -EINVAL;
  }
  printk("invoking read");
  BUF_LEN=data->curr_channel->msg_size;

  if (BUF_LEN==0){ //no message exists on the channel
    return -EWOULDBLOCK;
  }

  msg=data->curr_channel->msg;
  
  if (length<BUF_LEN){ //the buffer supplied by the user wasnt big enough for the msg
    return -ENOSPC;
  }

  for( i = 0; i < length && i < BUF_LEN; ++i )
  {
    if (put_user(msg[i], &buffer[i]) != 0)
      error=1;
  }
  if (error==1)
    return -EINVAL;
 
  // return the number of input characters used
  return i;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{ 
  int i;
  node* channel;
  file_data *data = (file_data*)(file->private_data);
  if (MAX_BUF_SIZE<length || length ==0 ){ //the msg supplied by the user was to big for our buffer or the passed message length is 0
    return -EMSGSIZE;
  }
  if (data->curr_channel_id==0){ //no channel has been set on the file descriptor
    return -EINVAL;
  }
  channel= data->curr_channel; 
  printk("Invoking device_write(%p,%ld)\n", file, length);
  if (update_channel(channel, length)<0){
    return -EINVAL;
  }
  for( i = 0; i < length ; ++i )
  {
    get_user(channel->msg[i], &buffer[i]);
  }
 
  // return the number of input characters used
  return i;
}
//----------------------------------------------------------------

static long device_ioctl( struct   file* file, unsigned int   ioctl_command_id, unsigned long  ioctl_param )
{
  
  file_data *data;
  node *head;
  printk("Invoking device_ioctl(%p)\n", file);
  if(ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0){ //The passed command is not MSG_SLOT_CHANNEL, or the passed channel id is 0
		return -EINVAL;
  }
  data = (file_data*)(file->private_data);
  head = device_info[data->minor_num].head;
  if (head==NULL){
    head = kmalloc(sizeof(node), GFP_KERNEL);
    if (head ==NULL){
      printk("memory allocation failed.\n");
      return -ENOMEM;
    }
    update_node(head, ioctl_param);
    update_data_file(data, head, ioctl_param);
    device_info[data->minor_num].head=head;
    return 0;

  }
  else{
    node * curr = head;
    while (curr->next !=NULL && curr->channel_id !=ioctl_param){
        curr=curr->next;
      }
    if (curr->channel_id ==ioctl_param){
      update_data_file(data, curr, ioctl_param);
      return 0;
    }
    else{ //the node of this channel does not exist
        curr->next=kmalloc(sizeof(node), GFP_KERNEL);
      if (curr->next==NULL){
        printk("memory allocation failed.\n");
        return -ENOMEM;
      }
      update_node(curr->next, ioctl_param);
      update_data_file(data, curr->next, ioctl_param);
      return 0;
    }
  }
}
//----------------------------------------------------------------

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;
  int i;
  // init dev struct
  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );
  // Negative values signify an error
  if( rc < 0 )
  {
    printk( KERN_ALERT "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
  }

  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  for (i=0; i<257; i++){
      device_info[i].head=NULL;
  }
  return 0;
}

//---------------------------------------------------------------


//==================== CLEAN LINKED LIST =============================
static void delete_linked_list(node* head){
  node* tmp;
	while (head!=NULL){
    tmp = head->next;
    kfree(head->msg);
    kfree(head);
    head=tmp;
  }
}

//----------------------------------------------------------------


static void __exit simple_cleanup(void)
{ 
  int i;
  for (i=0; i<257; i++){
    delete_linked_list(device_info[i].head);
  }

  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
