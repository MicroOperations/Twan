# Twanvisor demo

```
make defconfig
vim .config
```

change:

```# CONFIG_DEMO_PV_TWANVISOR is not set``` 

to:

```CONFIG_DEMO_PV_TWANVISOR=y```

```
make kvm
```

you should see:

```
...
guest created! vid: 1
root vcpus are now preemptive!
hello from guest
guest terminated!
...
```