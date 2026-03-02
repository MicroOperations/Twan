# Twanvisor demo

To run the Twanvisor demo follow these steps

1. Initialize the configuration file

```
make defconfig
vim .config
```

2. change:

```# CONFIG_DEMO_PV_TWANVISOR is not set``` 

to:

```CONFIG_DEMO_PV_TWANVISOR=y```

3. Build and run

```
make kvm
```

## Expected output

you should see:

```
...
guest created! vid: 1
root vcpus are now preemptive!
hello from guest
guest terminated!
...
```