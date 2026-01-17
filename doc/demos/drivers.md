# Drivers demo

```
make defconfig
vim .config
```

change:

```# CONFIG_DEMO_DRIVERS is not set``` 

to:

```CONFIG_DEMO_DRIVERS=y```

```
make kvm
```

you should see:

```
...
Hello World!
...
```