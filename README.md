#Added trace and reverse engineering functionalities for the motiondev driver

trace size can be controlled from TRACE_SIZE (250000 at the moment)
number of allowed registers NUM_OF_REGS (167 at the moment)

# usage example

or wherever you have the usb stick mounted
* cd udisk-sda1 

kill the current executable
* killall motion.elf 

remove the curretn driver
* rmmod motiondev 

insert the new driver
* insmod motiondev.ko

#utility commands

basic help
* ./control h

reset trace buffer 
* ./control r

enable trace 
* ./control e

disable trace
* ./control d

set read bypass
means that everytime the app requests data from a driver address,
the code will read the fpga but will not return that value but instead
will return the provided bypass data
example at address 1(dec) with data a3d4(hex)
sc_y where x = address(dec), y = data(hex)
* ./control s1_a3d4

clear read bypass
restore normal operation at address
example at address 1(dec)
cx where x = address(dec)
* ./control c1

trace filter set
exclude events from trace
f(w/r)x_y where x = addr(dec), y = number_of_addresses(dec)
example fr0_1 will exclude any read at address 0,
fr0_2 will exclude any read at addresses 0 and 1. .... and so on
use w/r to control filter direction
* ./control fw1_2
* ./control fr1_2

trace filter pass
exclude events from trace
p(w/r)x_y where x = addr(dec), y = number_of_addresses(dec)
example pr0_1 will enable any read at address 0,
pr0_2 will exclude any read at addresses 0 and 1. .... and so on
use w/r to control filter direction
* ./control pw1_2
* ./control pr1_2

for example if you want just to see logs of writes at addresses 144, 145, 146
and reads at address 64 use:
block all first
*./control fw0_167
*./control fr0_167
allow
*./control pw144_3
*./control pr64_1

get unread log data
prints unread log events. also cleares the ones read
* ./control g
* ./control g > abc.xxx

read data
Rx where x = addr(dec)
reads and prints data from address 0 (also appears in trace). 
Use the original driver for untainted operations
* ./control R0

write data
Wx_y where x = addr(dec), y = data(hex)
writes and prints data at address 0 (also appears in trace). 
Use the original driver for untainted operations
* ./control W0_a1

The only thing needed now is to start the application and make traces.
For example:
* ./control r
* ./../mnd/nand1-1/motion.elf (exit with CTRL+C)
* ./control g >dump.txt


