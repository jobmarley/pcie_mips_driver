# pcie_mips_driver
This is the pci express driver for [Mips_core](https://github.com/jobmarley/MIPS_core)/[VSMipsProjectExtension](https://github.com/jobmarley/VSMipsProjectExtension)

It is pretty stable, I ran it for days or weeks without any issues.  
### Features
- DMA for ddr3 read/write
- Memory mapped registers operation to control and debug the processor
- Inverted call model to get events from the processor (only breakpoint notification right now)

## mipsdebug
This is a C user mode API to access the driver functionalities.

## test
Command line application to run some basic commands, makes use of mipsdebug
