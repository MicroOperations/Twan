# Getting started

## Install

Clone the repository and install dependancies

```
git clone --recursive https://github.com/MicroOperations/Twan
cd Twan
# Debian and Debian-based distributions (using apt):
sudo bash setup/debian.sh
# Arch Linux (using pacman):
sudo bash setup/arch_linux.sh
```

## Building Twan

Twan can be built using make

```
make
```

## Testing

Twan can be tested in QEMU/KVM using nested virtualisation

```
make kvm
```

## Demos

Twan provides demos to help newcomers get started with development. Documentation on how to setup demos can be found [here](demos/).