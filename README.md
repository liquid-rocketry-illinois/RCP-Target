# RCP-Target

RCP-Target is a simple target side implementation of the RCP protocol. This is intended to be used on a 
microcontroller or other embedded system that responds to an RCP host. 

Several callback functions are available to implement, which allows the user program to interface with the packet 
system. `RCP::yield()` should be called periodically to ensure packets are being processed.

More docs to come.
