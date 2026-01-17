# Late Initcalls demo

```
make defconfig
vim .config
```

change:

```# CONFIG_DEMO_LATE_INITCALLS is not set``` 

to:

```CONFIG_DEMO_LATE_INITCALLS=y```

```
make kvm
```

you should see:

```
...
Hello World!
...
```