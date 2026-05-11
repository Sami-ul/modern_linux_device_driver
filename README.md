# Modern Linux Device Driver

A modern Linux character device driver written in C, with modern features for ease of use. This project demonstrates how to build a loadable kernel module that registers a character device, creates a `/dev/` node automatically, and supports basic file operations such as `open`, `read`, `write`, `llseek`, and `release`.

## Features: Modern vs. Legacy Approach

- **Dynamic Device Management (`udev`):** Automatically creates and destroys the device node in `/dev/`, whereas legacy drivers required users to manually run `mknod`.
- **Dynamic Allocation:** Automatically requests a 12-bit major/minor device number from the kernel, completely bypassing the classic 8-bit manual allocation limits.
- **Sysfs Integration:** Registers device metadata under `/sys/class/` to allow the OS to properly track the device lifecycle.
- **Standard File Operations:** Fully supports `open`, `read`, `write`, `llseek`, and `release`.
- **Safe Memory Transfers:** Utilizes kernel-space buffer memory and safely handles transfers to user-space.
- **Plug & Play Testing:** Automatically sets device node permissions to `0666` for immediate user-space testing upon module load.

## Project Structure

```text
.
├── Makefile
├── modern_character_driver.c
└── README.md
