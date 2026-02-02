#!/usr/bin/env bash

# The "--needed" argument tells pacman to not reinstall packages that are already installed and up-to-date
xargs -a setup/packages_arch_linux.txt sudo pacman -S --needed --noconfirm