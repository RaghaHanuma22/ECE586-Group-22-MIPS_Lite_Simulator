#include <iostream>
#include <cstdio>
#include <vector>
#include "pkg.h"
#include "sim.h"
using namespace std;



// Define opcode values
enum Opcode {
    ADD = 000000,
    ADDI = 000001,
    SUB = 000010,
    SUBI = 000011,
    MUL = 000100,
    MULI = 000101,
    OR = 000110,
    ORI = 000111,
    AND = 001000,
    ANDI = 001001,
    XOR = 001010,
    XORI = 001011,
};

// Simulate 32 general purpose registers
int registers[32] = {0};
static unsigned int PC = 0;

// ALU logic
int startop(int opcode, int operand1, int operand2) {
    switch (opcode) {
        case ADD: {
            PC+=4;
            return operand1 + operand2;
            break;
        }
        case ADDI: {
            PC+=4;
            return operand1 + operand2; // Immediate addition
            break;
        }
        case SUB: {
            PC+=4;
            return operand1 - operand2;
            break;
        }
        case SUBI: return operand1 - operand2; // Immediate subtraction
        break;
        case MUL: return operand1 * operand2;
        break;
        case MULI: return operand1 * operand2; // Immediate multiplication
        break;
        case OR: return operand1 | operand2;
        break;
        case ORI: return operand1 | operand2; // Immediate OR
        break;
        case AND: return operand1 & operand2;
        break;
        case ANDI: return operand1 & operand2; // Immediate AND
        break;
        case XOR: return operand1 ^ operand2;
        break;
        case XORI: return operand1 ^ operand2; // Immediate XOR
        break;
        default:
            cerr << "Invalid opcode\n";
            //exit(1);
    }
}



void executeInstruction() {
    int opcode = inst.I_type.opcode;
    int rs = inst.I_type.rs;
    int rt = inst.I_type.rt;
    int rd = inst.R_type.rd;
    int immediate = inst.I_type.imm;
 
    // Check if the instruction is immediate or not
    bool isImmediate = (opcode == ADDI || opcode == SUBI || opcode == MULI ||
                        opcode == ANDI || opcode == ORI || opcode == XORI );
                        

    // Get the values from registers
    int operand1 = registers[rs];
    int operand2 = isImmediate ? immediate : registers[rt];
    int result = startop(opcode, operand1, operand2);

    // For all instructions, immediate or not, store result in rt (I-type) or rd (R-type)
    if (isImmediate)
        registers[rt] = result;
    else
        registers[rd] = result;
}


void printRegisters() {
    cout << "Registers:\n";
    for (int i = 0; i < 32; ++i) {
        cout << "R" << i << ": " << registers[i] << "\n";
    }
    printf("PC: %0d\n",PC);
}
