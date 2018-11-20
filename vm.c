#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "data.h"
#define INT_MIN -2147483647

/* ************************************************************************************ */
/* Declarations                                                                         */
/* ************************************************************************************ */

void initVM(VirtualMachine*);

int readInstructions(FILE*, Instruction*);

void dumpInstructions(FILE*, Instruction*, int numOfIns);

int getBasePointer(int *stack, int currentBP, int L);

void dumpStack(FILE*, int* stack, int sp, int bp);

int executeInstruction(VirtualMachine* vm, Instruction ins, FILE* vmIn, FILE* vmOut);

/* ************************************************************************************ */
/* Global Data and misc structs & enums                                                 */
/* ************************************************************************************ */

// Allows conversion from opcode to opcode string.
const char *opcodes[] =
{
    "illegal", // opcode 0 is illegal
    "lit", "rtn", "lod", "sto", "cal", // 1, 2, 3 ..
    "inc", "jmp", "jpc", "sio", "sio",
    "sio", "neg", "add", "sub", "mul",
    "div", "odd", "mod", "eql", "neq",
    "lss", "leq", "gtr", "geq"
};

enum { CONT, HALT };

/* ************************************************************************************ */
/* Definitions                                                                          */
/* ************************************************************************************ */

// Initialize Virtual Machine
void initVM(VirtualMachine* vm)
{
    if(vm)
    {
      // Registers
      vm->SP = 0;
      vm->BP = 1;
      vm->PC = 0;
      vm->IR = 0;
    }
}


// Fill the (ins)tructions array by reading instructions from (in)put file
// Return the number of instructions read
int readInstructions(FILE* in, Instruction* ins)
{
    // Instruction index
    int i = 0;

    while(fscanf(in, "%d %d %d %d", &ins[i].op, &ins[i].r, &ins[i].l, &ins[i].m) != EOF)
    {
        i++;
    }

    // Return the number of instructions read
    return i;
}

// Dump instructions to the output file
void dumpInstructions(FILE* out, Instruction* ins, int numOfIns)
{
    // Header
    fprintf(out,
        "***Code Memory***\n%3s %3s %3s %3s %3s \n",
        "#", "OP", "R", "L", "M"
        );

    // Instructions
    int i;
    for(i = 0; i < numOfIns; i++)
    {
        fprintf(
            out,
            "%3d %3s %3d %3d %3d \n", // formatting
            i, opcodes[ins[i].op], ins[i].r, ins[i].l, ins[i].m
        );
    }
}

// Returns the base pointer for the lexiographic level L
int getBasePointer(int *stack, int currentBP, int L)
{
  int base_ptr = currentBP;

  while (L > 0)
  {
    base_ptr = stack[base_ptr + 0]; // 1
    L--;
  }

  return base_ptr;
}

// Function that dumps the whole stack into output file
// Do not forget to use '|' character between stack frames
void dumpStack(FILE* out, int* stack, int sp, int bp)
{
    if(bp == 0)
        return;

    // bottom-most level, where a single zero value lies
    if(bp == 1)
    {
        fprintf(out, "%3d ", 0);
    }

    // former levels - if exists
    if(bp != 1)
    {
        dumpStack(out, stack, bp - 1, stack[bp + 2]);
    }

    // top level: current activation record
    if(bp <= sp)
    {
        // indicate a new activation record
        fprintf(out, "| ");

        // print the activation record
        int i;
        for(i = bp; i <= sp; i++)
        {
            fprintf(out, "%3d ", stack[i]);
        }
    }
}

// Executes the (ins)truction on the (v)irtual (m)achine.
// This changes the state of the virtual machine.
// Returns HALT if the executed instruction was meant to halt the VM.
// .. Otherwise, returns CONT
int executeInstruction(VirtualMachine* vm, Instruction ins, FILE* vmIn, FILE* vmOut)
{
    switch(ins.op)
    {
        // Opcode: "LIT".
        // Stores ins.m into one of the virtual machine's registers based on ins.r
        case 1:
          vm->RF[ins.r] = ins.m;
          break;

        // Opcode: "RTN".
        // Returns from a subroutine and restores caller environment.
        case 2:
          vm->SP = vm->BP - 1;
          vm->BP = vm->stack[vm->SP + 2]; //3
          vm->PC = vm->stack[vm->SP + 3]; //4
          break;

        // Opcode: "LOD".
        // Load a value into a register from the stack at offset ins.m at ins.l
        // levels down.
        case 3:
          vm->RF[ins.r] = vm->stack[getBasePointer(vm->stack, vm->BP, ins.l) + ins.m - 1];
          break;

        // Opcode: "STO".
        // Store a value from a register from the stack at offset ins.m at ins.l
        // levels down.
        case 4:
          vm->stack[getBasePointer(vm->stack, vm->BP, ins.l) + ins.m - 1] = vm->RF[ins.r];
          //printf("get BP: %d, ins.m: %d\n", getBasePointer(vm->stack, vm->BP, ins.l),  ins.m);
          break;

        // Opcode: "CAL".
        // Call procedure at ins.m (generates new AR).
        case 5:
          vm->stack[vm->SP + 0] = 0;
          vm->stack[vm->SP + 1] = getBasePointer(vm->stack, vm->BP, ins.l);
          vm->stack[vm->SP + 2] = vm->BP;
          vm->stack[vm->SP + 3] = vm->PC;
          vm->BP = vm->SP + 1;
          vm->PC = ins.m;
          //vm->SP += 4;
          break;

        // Opcode: "INC".
        // Allocate ins.m locals by incrementing the stack pointer.
        case 6:
          vm->SP += ins.m;
          break;

        // Opcode: "JMP".
        // Jump the PC to the specified line in code (ins.m).
        case 7:
          vm->PC = ins.m;
          break;

        // Opcode: "JPC".
        // If the given register equals zero, jump to a specified line of code.
        case 8:
          if (vm->RF[ins.r] == 0)
            vm->PC = ins.m;
          break;

        // Opcode: "SIO" (1).
        // Console output the register at ins.r.
        case 9:
          fprintf(vmOut, "%d ", vm->RF[ins.r]);
          break;

        // Opcode: "SIO" (2).
        // Store console input at ins.r.
        case 10:
          fscanf(vmIn, "%d", &vm->RF[ins.r]);
          break;

        // Opcode: "SIO" (3).
        // End of program, return HALT.
        case 11:
          return HALT;

        // Opcode: "NEG".
        // Stores the contents of register ins.l * -1 into register ins.r.
        case 12:
          vm->RF[ins.r] = -1 * vm->RF[ins.l];
          break;

        // Opcode: "ADD".
        // Add register ins.l and ins.m and store result into register ins.r.
        case 13:
          vm->RF[ins.r] = vm->RF[ins.l] + vm->RF[ins.m];
          break;

        // Opcode: "SUB".
        // Subtract register ins.l and ins.m and store result into register ins.r.
        case 14:
          vm->RF[ins.r] = vm->RF[ins.l] - vm->RF[ins.m];
          break;

        // Opcode: "MUL".
        // Multiply register ins.l and ins.m and store result into register ins.r.
        case 15:
          vm->RF[ins.r] = vm->RF[ins.l] * vm->RF[ins.m];
          break;

        // Opcode: "DIV".
        // Divide register ins.l into ins.m and store result into register ins.r.
        case 16:
          vm->RF[ins.r] = vm->RF[ins.l] / vm->RF[ins.m];
          break;

        // Opcode: "ODD".
        // Store register ins.r % 2 into register ins.r.
        case 17:
          vm->RF[ins.r] = vm->RF[ins.r] % 2;
          break;

        // Opcode: "MOD".
        // Store register ins.l % register ins.m into register ins.r.
        case 18:
          vm->RF[ins.r] = vm->RF[ins.l] % vm->RF[ins.m];
          break;

        // Opcode: "EQL".
        // If register ins.l == register ins.m, store 1 into register ins.r.
        // Otherwise store 0 into register ins.r.
        case 19:
          vm->RF[ins.r] = (vm->RF[ins.l] == vm->RF[ins.m]);
          break;

        // Opcode: "NEQ".
        // If register ins.l != register ins.m, store 1 into register ins.r.
        // Otherwise store 0 into register ins.r.
        case 20:
          vm->RF[ins.r] = (vm->RF[ins.l] != vm->RF[ins.m]);
          break;

        // Opcode: "LSS".
        // If register ins.l < register ins.m, store 1 into register ins.r.
        // Otherwise store 0 into register ins.r.
        case 21:
          vm->RF[ins.r] = (vm->RF[ins.l] < vm->RF[ins.m]);
          break;

        // Opcode: "LEQ".
        // If register ins.l <= register ins.m, store 1 into register ins.r.
        // Otherwise store 0 into register ins.r.
        case 22:
          vm->RF[ins.r] = (vm->RF[ins.l] <= vm->RF[ins.m]);
          break;

        // Opcode: "GTR".
        // If register ins.l > register ins.m, store 1 into register ins.r.
        // Otherwise store 0 into register ins.r.
        case 23:
          vm->RF[ins.r] = (vm->RF[ins.l] > vm->RF[ins.m]);
          break;

        // Opcode: "GEQ".
        // If register ins.l >= register ins.m, store 1 into register ins.r.
        // Otherwise store 0 into register ins.r.
        case 24:
          vm->RF[ins.r] = (vm->RF[ins.l] >= vm->RF[ins.m]);
          break;

        default:
          fprintf(stderr, "Illegal instruction?");
          return HALT;
    }

    return CONT;
}

/**
 * inp: The FILE pointer containing the list of instructions to
 *         be loaded to code memory of the virtual machine.
 *
 * outp: The FILE pointer to write the simulation output, which
 *       contains both code memory and execution history.
 *
 * vm_inp: The FILE pointer that is going to be attached as the input
 *         stream to the virtual machine. Useful to feed input for SIO
 *         instructions.
 *
 * vm_outp: The FILE pointer that is going to be attached as the output
 *          stream to the virtual machine. Useful to save the output printed
 *          by SIO instructions.
 * */
void simulateVM
    (
      FILE* inp,
      FILE* outp,
      FILE* vm_inp,
      FILE* vm_outp
    )
{
    // Read instructions from file
    int numOfIns;
    Instruction ins[MAX_CODE_LENGTH];
    numOfIns = readInstructions(inp, ins);

    // Dump instructions to the output file
    dumpInstructions(outp, ins, numOfIns);

    // Before starting the code execution on the virtual machine,
    // .. write the header for the simulation part (***Execution***)
    fprintf(outp, "\n***Execution***\n");
    fprintf(
        outp,
        "%3s %3s %3s %3s %3s %3s %3s %3s %3s \n",         // formatting
        "#", "OP", "R", "L", "M", "PC", "BP", "SP", "STK" // titles
    );

    // Create a virtual machine
    VirtualMachine *vm = calloc(1, sizeof(VirtualMachine));

    // Initialize the virtual machine
    initVM(vm);

    int i, j, k, currentAR = 0, halt = CONT, arIndex;

    // Will be storing the size of each activation record in this frequency array.
    // This will help us when printing the stack.
    int *AR = calloc(100, sizeof(int));

    int flag = 0, oldSP;

    // Fetch & Execute the instructions on the virtual machine until halt is reached.
    while(!halt)
    {
        // Fetch.
        // Store the current PC in i, that way we can print it later after
        // modifying the PC. If we return from a call, oldSP will be used to help
        // free the associated data, since SP gets modified when returning.
        i = vm->PC;
        oldSP = vm->SP;
        vm->IR = ins[vm->PC].op;

        // Advance PC - before execution!
        vm->PC++;

        // Execute the instruction.
        halt = executeInstruction(vm, ins[i], vm_inp, vm_outp);


        /* ************************************************* */
        /* Update AR frequency array.                        */
        /* ************************************************* */

        // Return to caller environment. Free the associated memory with the AR
        // by setting each index to 0 and decrement the currentAR.
        if (vm->IR == 2)
        {
          for (j = 0; j < AR[currentAR]; j++)
          {
            vm->stack[oldSP - j] = 0;
          }

          AR[currentAR] = 0;
          currentAR--;
        }
        // A new AR was added to the stack. If this is the first call we are
        // making and we haven't allocated any space on the stack beforehand,
        // there is no reason to increment the currentAR (indicated by flag).
        else if (vm->IR == 5 && flag != 0)
        {
          currentAR++;
        }
        // Space is allocated on to the stack.
        else if (vm->IR == 6)
        {
          AR[currentAR] += ins[i].m;
          flag = 1;
        }

        /* ************************************************* */
        /* Print current state.                              */
        /* ************************************************* */

        fprintf(
            outp,
            "%3d %3s %3d %3d %3d %3d %3d %3d ", // formatting
            i, // place of instruction at memory
            opcodes[vm->IR], ins[i].r,   // instruction info
            ins[i].l, ins[i].m, // instruction info
            vm->PC, vm->BP, vm->SP // vm info
        );

        /* ************************************************* */
        /* Print stack info.                                 */
        /* ************************************************* */

        // Initial zero.
        fprintf(outp, "%3d ", 0);

        arIndex = 0;

        for (j = 0, k = 0; AR[k] != 0; j++)
        {
          // Activation record divider.
          if (arIndex == 0)
          {
            fprintf(outp, "| ");
          }

          // Print the stack contents.
          fprintf(outp, "%3d ", vm->stack[j]);

          // We have printed this activation record's contents.
          if (arIndex == AR[k] - 1)
          {
            k++;
            arIndex = 0;
            continue;
          }

          arIndex++;
        }


        fprintf(outp, "\n");
    }

    // Above loop ends when machine halts. Therefore, dump halt message.
    fprintf(outp, "HLT\n");
    return;
}
