# Laboration 2

## Compiling & Running

### make commands

`make` / `make all`

`make clean`

### Command-line args

**run**

*Default value (50,000) used if no args provided.*

`lab2`

`lab2 4000`

## Environment & Tools

A vagrant virtual-box running Ubuntu was used to compile and run the code using a Makefile while remaining on the Windows host machine.

**IDE:**
Intellij Clion 2020.2.4

**OS:**
Windows 10
Ubuntu 18.04

**Compiler:**
g++ clang

**Tools:**
Vagrant 2.2.10
Virtual Box 6.1

## Purpose

A program illustrating the use of inter-process communication using pipes and polling two separate forked child processes. Each child calculates the decimal places of a mathematical constant (pi or e). Each individual number is piped to the parent; the parent then writes the number to the appropriate files (pi.dat or e.dat). Progress is intermittently displayed by the parent process printing output to console.

## Procedures

* Two pipes are created (one for each child process) for inter-process communication using the `pipe()` function.
* A `struct pollfd` is initiated with the rules for `poll()` using the pipe file descriptors.
* Two files are opened and each is associated with a file descriptor.
* A fan of child processes is created using the system call `fork()`.

**Parent code**

* Two `std::map`s  are created to hold a count of how many times a certain number is received. After every 1000 digits received from a child, the distribution of the numbers is printed using the child's map.
* A `while()` loop is used to repeatedly poll, i.e. to check if input from either of the two child processes is available. `The poll()` timeout argument is used to break out of this loop. The return value of `poll()` is checked for errors.
* If `poll()` returns ready events from either of the children's file descriptors and no errors were set, the parent attempts to `read()` from the pipe and, if successful, performs a `write()` of the character to the appropriate file. Additionally, for the purposes of the distribution output, each child's count variable and `std::map` is updated.

 **Child code**
 
Each child simply calls their respective calculator, the results of which are piped to the parent.
