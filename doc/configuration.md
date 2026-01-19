# Configuration

Kernel configuration is handled via Kconfig and can be used to alter kernel behaviour before it is built

## Managing configurations

- ```make defconfig```  - Creates a .config file containing the default kernel configurations
- ```make menuconfig``` - Provides a GUI to configure the kernel, it eases kernel configuration by creating the configuration file and generating necessary files based on the set configurations.
- ```make genconfig``` - Generates necessary files based on set configurations in the .config file
- ```make distclean``` - Clears all generated files, aswell as configuration files, and the built kernel