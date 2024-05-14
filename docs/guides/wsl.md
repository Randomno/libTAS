---
layout: page
title: Running libTAS using WSL 2
permalink: /guides/wsl/
---

# Introduction
libTAS is designed for and only works on the Linux operating system. However, users of Windows can access Linux programs quite easily with the [Windows Subsystem for Linux](https://learn.microsoft.com/en-us/windows/wsl/about) (WSL).

This guide assumes **Windows 10 64-bit version 2004** or later, and WSL 2 requires Windows 10 64-bit version 1903 or later. If you have an older version of Windows, you will to need to try a [different method](https://github.com/clementgallet/libTAS?tab=readme-ov-file#non-linux-users) to use libTAS.

**Important note**: If you have a specific game in mind, it's not guaranteed to be compatible with libTAS. Ideally, the game should have a native Linux version, meaning it doesn't require [Wine](https://www.winehq.org/) or [Proton](https://github.com/ValveSoftware/Proton). Some games can be ported to Linux, such as certain Unity or GameMaker games. Old Windows games may be playable through [PCem](https://tasvideos.org/EmulatorResources/PCem). Most Adobe Flash games work well with [Ruffle](https://tasvideos.org/EmulatorResources/Ruffle). You may wish to ask in the [libTAS Discord server](https://discord.gg/3MBVAzU) about the compatibility of your game.

# Installing WSL 2
Microsoft has a detailed [guide for setting up WSL 2](https://learn.microsoft.com/en-us/windows/wsl/setup/environment), so you should refer to it if you encounter problems.

Open **Command Prompt** and enter the `wsl --install` command.

![image](https://i.imgur.com/byoDYFD.png) ![image](https://i.imgur.com/xDyQoN2.png)

This does the following:
* Enables the optional WSL and Virtual Machine Platform components
* Downloads and installs the latest Linux kernel
* Sets WSL 2 as the default
* Downloads and installs the **Ubuntu** distribution of Linux

Now, **restart** your computer.

After restarting, Ubuntu should open. If it doesn't, you can launch it from the Start Menu. You will be prompted to create a username and password. This is unrelated to your existing Windows username/password. Note that while entering the password, the characters will not appear on the screen.

![image](https://i.imgur.com/XJ3IZr3.png)

Once the installation is complete, you will see this terminal prompt:

![image](https://i.imgur.com/SeHEKI7.png)

You are now in WSL 2.

# Installing libTAS
For simplicity, I have combined all subsequent steps into a single command to enter into the terminal. You can paste it with right click or Ctrl+V:

`curl https://raw.githubusercontent.com/Randomno/libTASinstall/main/libTASinstall.sh | sh`

This downloads and runs the script which does the following:
* Updates and upgrades packages
* Downloads the latest version of libTAS from GitHub
* Installs libTAS
* Disables Wayland (this is a window manager incompatible with libTAS)
* Disables Wayland on future launches of Ubuntu

Make sure to enter your password when prompted.

Assuming all has gone well, simply enter the `libTAS` command to run the program:

![image](https://i.imgur.com/GVUkPSl.png)

![image](https://i.imgur.com/ZQ9pIa6.png)

# Using Linux and libTAS
Becoming proficient with Linux can take time and this tutorial could not cover every possibility. It has a large array of commands to perform tasks through the terminal, but this is not very beginner-friendly; most things can instead be done with a GUI (graphical user interface).

To access your files, you can use Windows Explorer.

![image](https://i.imgur.com/4HLEZsO.png)

Most of your files will be in `/home/<username>` (whatever your username is). This directory is often shortened to `~` (for example `~/libTAS/` is the same as `/home/<username>/libTAS/`).

Alternatively, you can install a file explorer inside WSL, such as Nautilus, with `sudo apt install nautilus`. Run it with `nautilus`. Your Windows files can be found in `/mnt/`. `c` is the `C:` drive.

![image](https://i.imgur.com/wzp50Cn.png)

Depending on your game, you can download the Linux version outside of WSL, and copy it over with either of these methods. If you want an example game to test, libTAS includes a `simplestgame` for this purpose. I have compiled the game for download [here](https://github.com/Randomno/simplestgame/releases/tag/1.0). You can download this and copy it to your `/home/<username>/` directory.

Note that executable files on Linux often have no file extension (i.e. there is no `.exe` like Windows). Alternatively they can have a `.x86` or `.x86_64` extension. The file still needs permission to run as an executable. This can be done with `chmod +x <filename>`, or through Nautilus by right clicking, going to Properties>Permissions, and checking Allow executing file as program. You will now be able to run the program by double clicking it.

![image](https://i.imgur.com/4fT1jD8.png)

# Shortcut

You may notice a new shortcut has been created after installing libTAS:

![image](https://i.imgur.com/OAjR5C5.png)

This shortcut works, but is not the preferred way of launching libTAS, as the program often logs information and errors to the terminal. You should instead launch Ubuntu and then enter `libTAS`. Alternatively, you can create a new shortcut with the location `C:\Windows\System32\wsl.exe -e bash -lic "libTAS"` (assuming WSL is on the C drive).

