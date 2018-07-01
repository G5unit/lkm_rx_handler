# lkm_rx_handler
Example Linux Kernel Module to register rx_handler with netdevice (network interface).

To compile execute "make" in directory you download the code. "make clean" is also an option. 
You will need linux kernel headers installed to compile successfuly.
To load the module once compiled execute "insmod rxh.ko" with root priviledges.
To remove module execute "rmmod rxh".

Note, any module created that registers RX_HANDLER must be released under GPL terms as it is considered a derived work of Linux Kernel.
