# Multitasking System (MTX)

## Overview
This project implements a simple multitasking system (MTX) in C, simulating basic process management and scheduling concepts found in operating systems. It is designed for educational purposes, demonstrating how processes can be created, scheduled, and managed in a cooperative multitasking environment.

## Features
- Process creation (fork)
- Cooperative context switching (switch)
- Process termination (exit)
- Process sleep and wakeup (sleep, wakeup)
- Waiting for child processes (wait)
- Process tree visualization
- Simple priority-based ready queue
- Interactive command-line interface for process management

## File Structure
- `t.c` - Main multitasking system logic and process management
- `Makefile` - Build instructions

## Build Instructions
This project targets 32-bit x86 Linux environments. To build the project, ensure you have `gcc` installed with 32-bit support:

```sh
sudo apt-get install gcc-multilib
make
```

This will produce the `mtx` executable.

## Usage
Run the multitasking system with:

```sh
./mtx
```

You will be greeted with an interactive shell. Available commands:
- `ps`      : Show process status
- `fork`    : Create a new process
- `switch`  : Switch to another process
- `exit`    : Terminate the current process
- `sleep`   : Put the process to sleep on an event
- `wakeup`  : Wake up processes sleeping on an event
- `wait`    : Wait for a child process to exit
- `shutdown`: (Only process 1) Shut down the system

The system displays the process tree and the state of all process queues after each command.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Author
- Copyright (c) 2025 marvelcn015 