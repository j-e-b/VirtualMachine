# Overview

This is an implementation of a virtual machine known as the P-machine. The P-machine is a stack machine with two memory stores: the "stack", which is organized as a stack and contains the data to be used by the PM/0 CPU, and the "code", which is organized as a list and contains the instructions for the virtual machine. The PM/0 CPU has four registers to handle the stack and code segments: The registers are named base pointer (BP), stack pointer (SP), program counter (PC), and instruction register (IR). The machine also has a register file (RF) with sixteen registers (0-15).

# Components

* [Makefile](Makefile): The makefile that helps to build, test and clean.

* [main.c](main.c): The C file that contains the main function and the code required to interpret command line arguments.

* [data.h](data.h): Defines the `Instruction` and `VirtualMachine` data types, and constants.

* [vm.h](vm.h): Includes the declaration of the `simulateVM()` function.

* [vm.c](vm.c): The driver of the virtual machine.

# Command Line Arguments
Usage: `./vm.out (ins_inp_file) (simul_outp_file) [vm_inp_file=stdin] [vm_outp_file=stdout]`

* ins_inp_file: The path to the file containing the list of instructions to be loaded to code memory of the virtual machine.

* simul_outp_file: The path to the file to write the simulation output, which contains both code memory and execution history.

* vm_inp_file: The path to the file that is going to be attached as the input stream to the virtual machine. Useful to feed input for `SIO` instructions. Use dash `-` to assign stdin.

* vm_outp_file: The path to the file that is going to be attached as the output stream to the virtual machine. Useful to save the output printed by `SIO` instructions. Use dash `-` to assign stdout.

# Build & Run
The build is done with the help of the Makefile included in the repository. Following command builds solution and obtains the executable file `vm.out`:
```
$ make
```
Then, to test the virtual machine, use the following command by feeding necessary command line arguments:

```
$ ./vm.out (ins_inp_file) (simul_outp_file) [vm_inp_file=stdin] [vm_outp_file=stdout]
```
