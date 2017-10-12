#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/rtc.h>
#include <linux/ktime.h>


#define DEVICE_NAME "jprobe"    
#define CLASS_NAME  "proc" 
#define LICENSE "GPL"


MODULE_LICENSE(LICENSE);
struct proc_dir_entry* proc_file;
static char* buff = NULL;
static int bytes = 0;
static int pid;
static int majorNumber;                  
static struct class*  processClass  = NULL; 
static struct device* processDevice = NULL; 

module_param(pid, int, 0000);
static int rcount = 0;

ssize_t _read(struct file* fileptr, char* user_buffer, size_t length, loff_t* offset){
	
	if(!rcount){
		offset = 0;
		++rcount;
	}
	if(offset > 0) 
	return 0;	
	memcpy(user_buffer, buff, bytes);
	
	return bytes;
}


int page_fault(struct mm_struct* mm, struct vm_area_struct* vma, unsigned long addr, unsigned int flags){
	
	struct timespec time;
	getnstimeofday(&time);
	if(current->pid == pid){
		printk(KERN_INFO "pid: %d, virtual_address: %lu\n", current->pid, addr);
		char temp[50] = {0};
		int size = snprintf(temp, 50, "%ld,%lu\n", time.tv_nsec,addr);
		printk(KERN_INFO " virtual_address: %lu,time:\n%ld", addr,time.tv_nsec);		
		bytes = bytes+(size+1);
		if(buff == NULL){
			buff = (char*)kzalloc(sizeof(char)*bytes, GFP_KERNEL);
		}
		else{
			buff = (char*)krealloc(buff, bytes, GFP_KERNEL);
		}
		strcat(buff, temp);
	}
	jprobe_return();
	return 0;
}


static struct jprobe jprobe = {	
	.entry = page_fault,
	.kp = {
		.symbol_name = "handle_mm_fault",
	},
};


static struct file_operations fops = {
	.read = &_read,
	
};


static int __init jp_init(void){
	
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "Failed to register a major number\n");
	        return majorNumber;
   	}
	printk(KERN_INFO "Registered correctly with major number %d\n", majorNumber);
	processClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(processClass)){                
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(processClass);          
	}
	processDevice = device_create(processClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   	if (IS_ERR(processDevice)){               
		class_destroy(processClass);           
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(processDevice);
   	}

	int ret;
	proc_file = proc_create("data_file", 0000, NULL, &fops);
	ret = register_jprobe(&jprobe);
	return 0;
}

static void __exit jp_exit(void)
{
	kfree(buff);
	remove_proc_entry("data_file", NULL);
	unregister_jprobe(&jprobe);
	device_destroy(processClass, MKDEV(majorNumber, 0));     
   	class_unregister(processClass);                          
   	class_destroy(processClass);                             
   	unregister_chrdev(majorNumber, DEVICE_NAME);            
}

module_init(jp_init)
module_exit(jp_exit)
