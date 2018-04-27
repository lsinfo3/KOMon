/* Jprobe to monitor timestamps of incoming and outgoing packets */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <net/udp.h>
#include <uapi/linux/udp.h>
#include <linux/ip.h>
#include <uapi/linux/ip.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include "newhash.h"
#include "circBuf.h"
#include "packet.h"
#include <linux/uio.h>

// Define the name of the /proc/ filesystem name
#define procfs_name "vnfinfo_udp"

// Define and register command line parameters that are read during module load time
int port = 0;
int cbsize = 100;
int debug = 0;

module_param(port, int, 0);
MODULE_PARM_DESC(port, "The local port to monitor");

module_param(cbsize, int , 0);
MODULE_PARM_DESC(cbsize, "The size of the circular buffer to store processing times");

module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Debug level");

typedef enum {ACTIVE, WAITING} state;
static state current_state;
static int counter = 0;
static int sample_size = 0;

// The circular buffer to store processing times
static circBuf_u32_t cBuf_proc;
static circBuf_packet_t cBuf_packet;

// The struct to store /proc/ filesystem properties
struct proc_dir_entry *proc_file;

// Function pointers for the rcv functions
static int (*__udp4_lib_rcv_orig)(struct sk_buff *, struct udp_table *, int);
extern int (*__udp4_lib_rcv_ptr)(struct sk_buff *, struct udp_table *, int);

// Function pointers for the send functions
extern int udp_sendmsg(struct sock *, struct msghdr *, size_t len);
extern struct proto udp_prot;

// Called whenever the userspace reads from /proc/$PROCFS_NAME
static int proc_read(struct seq_file *f, void *v) {
  int i, cBuf_proc_size = 0, cBuf_packet_size = 0;
  bool first_iteration_done = false;

  if (debug>=1) {
    printk(KERN_INFO "Read from /proc/\n");
    printk(KERN_INFO "Counter=%d\n", counter);
    printk(KERN_INFO "sample_size=%d\n", sample_size);
  }

  if (current_state == WAITING && counter == sample_size) {

    cBuf_proc_size = cBuf_proc.head >= cBuf_proc.tail ?
      (cBuf_proc.head - cBuf_proc.tail) :
      ((cBuf_proc.head+cBuf_proc.maxLen - cBuf_proc.tail)+1);

    cBuf_packet_size = cBuf_packet.head >= cBuf_packet.tail ?
      (cBuf_packet.head - cBuf_packet.tail) :
      ((cBuf_packet.head+cBuf_packet.maxLen - cBuf_packet.tail)+1);

    /*
    seq_printf(f, "cBuf_proc_size:\t\t%d/%d\n", cBuf_proc_size,
	       cBuf_proc.maxLen);
    seq_printf(f, "cBuf_packet_size:\t%d/%d\n", cBuf_packet_size,
	       cBuf_packet.maxLen);
    */
    
    for (i = 0; i < sample_size; i++) {
      if (first_iteration_done) {
	seq_printf(f, ";%u", cBuf_proc.buffer[i]);
      } else {
	seq_printf(f, "%u", cBuf_proc.buffer[i]);
	first_iteration_done = true;
      }
    }
    seq_printf(f, "\n");
  } else if (current_state == ACTIVE && counter < sample_size) {
    seq_printf(f, "ACTIVE. %d Packets monitored.\n", counter);
  } else {
    seq_printf(f, "WAITING. Activate first!\n");
  }

  return 0;
}

// Called whenever the userspace opens /proc/$PROCFS_NAME
static int proc_open(struct inode *inode, struct file *file) {
  return single_open(file, proc_read, NULL);
}

static ssize_t proc_write(struct file *file, const char *buf, size_t count,
			  loff_t *pos) {
  char  * cmd = kmalloc(sizeof(char)*count, GFP_KERNEL);
  long samplesize;
  sprintf(cmd, "%.*s", (int)count, buf);
  if (strncmp(cmd, "reset_packets", strlen("reset_packets")) == 0) {
    if (debug>=1) printk(KERN_INFO "Command=%.*s\n", (int)(strlen("reset_packets"))
			 , cmd);
    circBuf_packet_Clear(&cBuf_packet);
  } else if (strncmp(cmd, "reset_proc", strlen("reset_proc")) == 0) {
    if (debug>=1) printk(KERN_INFO "Command=%.*s\n", (int)(strlen("reset_proc"))
			 , cmd);
    circBuf_u32_Clear(&cBuf_proc);
    counter = 0;
  } else if (strncmp(cmd, "start", strlen("start")) == 0) {
    if (debug>=1) printk(KERN_INFO "Command=%.*s\n", (int)(strlen("start"))
			 , cmd);

    circBuf_packet_Clear(&cBuf_packet);
    circBuf_u32_Clear(&cBuf_proc);
    counter = 0;

    if (debug>=1) printk(KERN_INFO "Monitoring activated\n");
    current_state = ACTIVE;
  } else if (strncmp(cmd, "stop", strlen("stop")) == 0) {
    if (debug>=1) printk(KERN_INFO "Command=%.*s\n", (int)(strlen("stop"))
			 , cmd);

    circBuf_packet_Clear(&cBuf_packet);
    circBuf_u32_Clear(&cBuf_proc);
    counter = 0;

    if (debug>=1) printk(KERN_INFO "Monitoring deactivated\n");
    current_state = WAITING;
  } else {
    if (!current_state == ACTIVE) {
      if (!kstrtol(cmd, 0, &samplesize)) {
	if (samplesize <= 0 || samplesize > cbsize) {
	  sample_size = cbsize;
	} else {
	  sample_size = (int)samplesize; // This is a bold move cotton, lets see if it plays out
	}
      } else {
	sample_size = cbsize;
      }
    } else {
      if (debug>=2) printk(KERN_INFO "Can't change samplesize during ACTIVE phase. Unchanged.");
    }
    if (debug>=1) printk(KERN_INFO "Samplesize=%d\n", sample_size);
  }
  kfree(cmd);
  return count;
}

// Stores the handles for the /proc/ file
static const struct file_operations proc_file_fops = {
						      .owner = THIS_MODULE,
						      .open = proc_open,
						      .read = seq_read,
						      .write = proc_write,
						      .llseek = seq_lseek,
						      .release = single_release,
};


// Called whenever a UDP packet is received via udp_recv
int __udp4_lib_rcv_wrapper(struct sk_buff * skb, struct udp_table * udptable,
			   int proto) {

  if (unlikely(current_state == ACTIVE && counter < sample_size)) {

    ktime_t recv_time;

    // unsigned char * data;
    unsigned char * data_ptr;

    packet_t packet;

    u16 ulen;

    struct udphdr * udp_header;
    // struct iphdr * ip_header;

    //u16 src_port;
    u16 dst_port;
    //u32 src_ip, dst_ip;

    if (unlikely(debug>=2)) printk(KERN_INFO "Packet received during ACTIVE phase\n");

    // ip_header = ip_hdr(skb);
    udp_header = udp_hdr(skb);

    ulen = udp_header->len;
    ulen = (ulen>>8 | ulen<<8);
    ulen = ulen - 8;

    //src_port = udp_header->source;
    dst_port = udp_header->dest;
    //src_ip = ip_header->saddr;
    //dst_ip = ip_header->daddr;

    dst_port = (dst_port>>8 | dst_port<<8);
    //src_port = (src_port>>8 | src_port<<8);

    if (dst_port == port) {

      //if (unlikely(debug>=2)) printk(KERN_INFO "Matching packet received during ACTIVE phase\n");

      data_ptr = (char *)((unsigned char *)udp_header + 8);
      //data = (char *)kmalloc(ulen+1, GFP_KERNEL);
      //memcpy(data, data_ptr, ulen);
      //data[ulen]='\0';

      //printk(KERN_INFO "HASH IN: %u\n", MurmurHash2(data_ptr, ulen, 10));

      recv_time = ktime_get();

      //packet.id = 0; // Currently unused so all packets get ID 0
      //packet.src_ip = src_ip;
      //packet.dst_ip = dst_ip;
      //packet.src_port = src_port;
      //packet.dst_port = dst_port;
      //packet.protocol = ip_header->protocol;
      packet.timestamp = recv_time;
      packet.digest = MurmurHash2(data_ptr, ulen, 10);


      circBuf_packet_Push(&cBuf_packet, packet);

      /*
      if (unlikely(debug>=3)) {
	printk(KERN_INFO "[Packet received (%u)] (%u;%s) (%lld) (%pI4;%u;%pI4;%u;%d) (%u)",
	       packet.id,
	       current->pid,
	       current->comm,
	       ktime_to_ns(recv_time),
	       &src_ip,
	       src_port,
	       &dst_ip,
	       dst_port,
	       ip_header->protocol,
	       ulen);
      
	       print_hex_dump(KERN_INFO, "payload: ", DUMP_PREFIX_ADDRESS, 16, 1, data, ulen, 1);
      }
      */
    //kfree(data);
    //kfree(data_ptr);
    }
  }
  return (*__udp4_lib_rcv_orig)(skb, udptable, proto);
}

// Called whenever a packet is send via udp_sendmsg
int udp_sendmsg_wrapper(struct sock *sk, struct msghdr *msg, size_t size) {

  if (unlikely(current_state == ACTIVE && counter < sample_size)) {
    ktime_t send_time;

    unsigned char * data_ptr;

    packet_t packet;
    s64 time_diff;

    u16 dst_port = sk->sk_dport;
    // u16 src_port = sk->sk_num;
    // u32 dst_ip = sk->sk_daddr;
    // u32 src_ip = sk->sk_rcv_saddr;

    dst_port = (dst_port>>8 | dst_port<<8);
    //src_port = (src_port>>8 | src_port<<8); // This is not needed this time - obviously!

    /*
    if (unlikely(debug>2)) printk(KERN_INFO "Packet sent during ACTIVE phase\n");
    if (unlikely(debug>=3)) printk(KERN_INFO "[Packet sent] (%u;%s) (%pI4;%u;%pI4;%u;%d)\n",
				   current->pid,
				   current->comm,
				   &src_ip,
				   src_port,
				   &dst_ip,
				   dst_port,
				   sk->sk_protocol);
    */

    if (sk->sk_protocol == 17 && dst_port == port) {

      send_time = ktime_get();

      
      data_ptr = (char *)kmalloc(size, GFP_KERNEL);
      if (!copy_from_iter(data_ptr, size, &msg->msg_iter)) {
	printk(KERN_ERR "Error copying user data");
	kfree(data_ptr);
	return -EIO;
      }
      // Rewind the iterator so that the network driver can use it
      iov_iter_revert(&msg->msg_iter, size);   // requires v4.11 (!)
      // data_ptr[size]='\0';
      
      // This only works for FIFO - we still need to figure out a matching procedure
      // Do only if we were able to Pop a packet; i.e. circBuf_packet is not empty
      if (!circBuf_packet_Get(&cBuf_packet, &packet)) {
	/*		  
	if (unlikely(debug>=3)) {
	  printk(KERN_INFO "[Packet sent (%u)] (%u;%s) (%lld) (%pI4;%u;%pI4;%u;%d) (%lu)",
		 packet.id,
		 current->pid,
		 current->comm,
		 ktime_to_ns(send_time),
		 &src_ip,
		 src_port,
		 &dst_ip,
		 dst_port,
		 sk->sk_protocol,
		 size);
	  
	  print_hex_dump(KERN_INFO, "payload: ", DUMP_PREFIX_ADDRESS, 16, 1, data_ptr, size, 1);
	}

	printk(KERN_INFO "HASH OUT: %u\n", MurmurHash2(data_ptr, size, 10));
	*/

	if (packet.digest == MurmurHash2(data_ptr, size, 10)) {
	  //printk(KERN_INFO "Hash match!");
	  time_diff = ktime_to_us(ktime_sub(send_time, packet.timestamp));
	  circBuf_packet_Pop(&cBuf_packet, &packet);
	
	/*		  
	if (unlikely(debug>=3)) printk(KERN_INFO "[Processing Time (%u)] %lld\n",
				       packet.id,
				       time_diff);
	*/		  
	  if (!circBuf_u32_Push(&cBuf_proc, time_diff))
	    counter++;
	}
      }
      //kfree(data_ptr);
    }
    if (counter == sample_size) {
      current_state = WAITING;
      // if (debug>=1) printk(KERN_INFO "Monitoring finished. %d Packets seen\n", counter);
    }
  }
  return udp_sendmsg(sk, msg, size);
}

// Called during module load to register the probes as well as the /proc/ file
static int __init mod_init(void) {
  // Allocate cbsize u32 slots on the circular buffer and set the max number of elements to be stored
  cBuf_proc = INIT_CIRCBUF_U32(cbsize);
  cBuf_packet = INIT_CIRCBUF_PACKET(cbsize);
  
  sample_size = cbsize;
  
  // Store the original rcv function and set new one
  __udp4_lib_rcv_orig = __udp4_lib_rcv_ptr;
  __udp4_lib_rcv_ptr = __udp4_lib_rcv_wrapper;

  // Store the original send function and set new one
  udp_prot.sendmsg = udp_sendmsg_wrapper;

  printk(KERN_INFO "Module loaded\n");

  // Creating /proc/ file
  proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);

  if (proc_file == NULL) {
    remove_proc_entry(procfs_name, NULL);
    printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
	   procfs_name);
    return -ENOMEM;
  }

  printk(KERN_INFO "/proc/%s created\n", procfs_name);

  current_state = WAITING;

  return 0;
}

// Called on module unload to unregister probes and remove /proc/ file
static void __exit mod_exit(void) {
  remove_proc_entry(procfs_name, NULL);

  // Restore the original rcv function
  __udp4_lib_rcv_ptr = __udp4_lib_rcv_orig;

  // Restore the original rcv function
  udp_prot.sendmsg = udp_sendmsg;

  printk(KERN_INFO "/proc/%s removed\n", procfs_name);
  printk(KERN_INFO "Module unloaded\n");

}


// Definition of callback methods for load and unload of the module
module_init(mod_init)
  module_exit(mod_exit)

// License, Author, ... declaration
  MODULE_LICENSE("GPL");
