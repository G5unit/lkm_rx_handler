/*
  rxh.c

  Example Linux kernel module to register RX_HANDLER with netdevice and 
  parse  packet IP/TCP headers.


  Released under GPLv3.0 license

  Created by github/G5unit on 4/5/2018.

*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/getcpu.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RXH");
MODULE_AUTHOR("Faruk Grozdanic/G5unit");

/* rxhPacketIn function is called when a packet is received on a registered netdevice */
rx_handler_result_t rxhPacketIn(struct sk_buff **ppkt) {
    struct sk_buff* pkt;
    pkt = *ppkt;
    struct iphdr *ip_header;
    int cpuid;
    struct tcphdr *tcp_header;
    uint8_t *tcp_headerflags;
    
    ip_header = (struct iphdr *)skb_network_header(pkt);
    
    /* Check if IPv4 packet */
    if(ip_header->version != 4){
        //printk(KERN_INFO "[RXH] Got packet that is not IPv4\n");
        return RX_HANDLER_PASS;
    }
    
    /* Report CPU id, source IP, destination IP, and Protocol version */
    cpuid = raw_smp_processor_id();
    printk(KERN_INFO "[RXH] CPU id [%i], Source IP [%x], Destination IP [%x], Protocol [%i]\n",cpuid,ip_header->saddr,ip_header->daddr,ip_header->protocol);

    /* Check if TCP protocol */
    if(ip_header->protocol != 6) {
          return RX_HANDLER_PASS;
    }

    /* check if fragmented packet,
       frag_off is shifted left 2 places and checked if it is > 0; if so it is a fragmented packet
    */
    if(ip_header->frag_off << 2) {
           printk(KERN_INFO "[RXH] Got fragmented packet\n");
           return RX_HANDLER_PASS;
    }
                                    
    /* Parse TCP header */
    tcp_header = (struct tcphdr *)skb_transport_header(pkt) ;
    tcp_headerflags = ((uint8_t *)&tcp_header->ack_seq) + 5;
    printk(KERN_INFO "TCP sourcePort [%u], destinationPort[%u], TCP flags 8 bit [%u], CWR [%u], ECE [%u], URG [%u], ACK [%u], PSH [%u], RST [%u], SYN [%u], FIN [%u]\n",
          (unsigned int)ntohs(tcp_header->source) ,(unsigned int)ntohs(tcp_header->dest), *tcp_headerflags,
          (uint)tcp_header->cwr, (uint)tcp_header->ece,(uint)tcp_header->urg,(uint)tcp_header->ack,(uint)tcp_header->psh,(uint)tcp_header->rst,
          (uint)tcp_header->syn,(uint)tcp_header->fin);


    return RX_HANDLER_PASS;

     /* This was derived from linux source code net/core/net.c .
       Valid return values are RX_HANDLER_CONSUMED, RX_HANDLER_ANOTHER, RX_HANDER_EXACT, RX_HANDLER_PASS.
       If your intention is to handle the packet here in your module code then you should 
       return RX_HANDLER_CONSUMED, in which case you are responsible for release of skbuff
       and should be done via call to kfree_skb(pkt).
    */
}

int registerRxHandlers(void) {
    struct net_device *device;
    int regerr;
    read_lock(&dev_base_lock);
    device = first_net_device(&init_net);
    while (device) {
        printk(KERN_INFO "[RXH] Found [%s] netdevice\n", device->name);
        /* Register only net device with name lo (loopback) */
        if(!strcmp(device->name,"lo")) {
            rtnl_lock();
            regerr = netdev_rx_handler_register(device,rxhPacketIn,NULL);
            rtnl_unlock();
            if(regerr) {
                printk(KERN_INFO "[RXH] Could not register handler with device [%s], error %i\n", device->name, regerr);
            } else {
                printk(KERN_INFO "[RXH] Handler registered with device [%s]\n", device->name);
            }
        }

        device = next_net_device(device);
    }
    read_unlock(&dev_base_lock);

    return 0;
}

void unregisterRxHandlers(void) {
    struct net_device *device;
    read_lock(&dev_base_lock);
    device = first_net_device(&init_net);
    while (device) {
        /* Unregister only lo (loopback) */
        if(!strcmp(device->name,"lo")) {
            rtnl_lock();
            netdev_rx_handler_unregister(device);
            rtnl_unlock();
            printk(KERN_INFO "[RXH] Handler un-registered with device [%s]\n", device->name);
        }
        device = next_net_device(device);
    }
    read_unlock(&dev_base_lock);
}

int init_module() {
    int i = 0;
    printk(KERN_INFO "[RXH] Kernel module loaded!\n");
    i=registerRxHandlers();
    return 0;
}
               
void cleanup_module() {
    unregisterRxHandlers();
    printk(KERN_INFO "[RXH] Kernel module unloaded.\n");
}
