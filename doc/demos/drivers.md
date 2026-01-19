# Drivers demo

To run the drivers demo follow these steps

1. Initialize the configuration file

```
make defconfig
vim .config
```

2. change:

```# CONFIG_DEMO_DRIVERS is not set``` 

to:

```CONFIG_DEMO_DRIVERS=y```

3. Build and run

```
make kvm
```

## Expected output

you should see:

```
...
Hello World!
...
```