# Early Initcalls demo

```
make defconfig
vim .config
```

change:

```# CONFIG_DEMO_EARLY_INITCALLS is not set``` 

to:

```CONFIG_DEMO_EARLY_INITCALLS=y```

```
make kvm
```

you should see:

```
...
Hello World!
...
```