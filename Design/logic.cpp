#include <iostream>
#include <cstdio>
#include <vector>
#include "pkg.h"
#include "sim.h"
#include <cstdlib>
using namespace std;




// Define opcode values
enum Opcode {
    ADD = 0b000000,
    ADDI = 0b000001,
    SUB = 0b000010,
    SUBI = 0b000011,
    MUL = 0b000100,
    MULI = 0b000101,
    OR = 0b000110,
    ORI = 0b000111,
    AND = 0b001000,
    ANDI = 0b001001,
    XOR = 0b001010,
    XORI = 0b001011,
    LDW =  0b001100,
    SDW = 0b001101,
    BZ =  0b001110,
    BEQ = 0b001111,
    JR =  0b010000,
    HALT = 0b010001
};

// Simulate 32 general purpose registers
int registers[31] = {0};
const int R0 = 0;


// ALU logic
int startop(int opcode, int operand1, int operand2, int location) {
    int temp = 0;
    printf("Opcode for now is = %d",opcode);
    switch (opcode) {
        
        case ADD: {
            
            return operand1 + operand2;
            break;
        }
        case ADDI: {
            
            return operand1 + operand2; // Immediate addition
            break;
        }
        case SUB: {
            
            return operand1 - operand2;
            break;
        }
        case SUBI: 
            
            return operand1 - operand2; // Immediate subtraction
            break;
        case MUL: 
                
            return operand1 * operand2;
            break;
        case MULI: 
        
        return operand1 * operand2; // Immediate multiplication
        break;
        case OR: 
        
        return operand1 | operand2;
        break;
        case ORI: 
        
        return operand1 | operand2; // Immediate OR
        break;
        case AND: 
        
        return operand1 & operand2;
        break;
        case ANDI: 
        
        return operand1 & operand2; // Immediate AND
        break;
        case XOR: 
        
        return operand1 ^ operand2;
        break;
        case XORI: 
        
        return operand1 ^ operand2; // Immediate XOR
        break;

        case LDW:
        {
        temp = operand1 + operand2;
        return memory[temp/4];        
        break;
        }
     
       
        case SDW:
        {
        break;
        } 

        case BZ:
        if(operand1 == 0) {
            return operand2;
        }
        else {
            return PC;
        }
        break;

        case BEQ:
        if(operand1 == operand2){
            return location;
        }
        else {
            return PC;
        }
        break;

        case JR:
        return operand1;
        break;

        case HALT:
        printf("Halt instruction executed! Terminating program!");
        exit(0);
        break;

        default:
            cerr << "Invalid opcode\n";
            exit(1);
            break;
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
                        opcode == ANDI || opcode == ORI || opcode == XORI || opcode == LDW || opcode == BZ || opcode == BEQ);
                        

    // Get the values from registers
    int operand1 = registers[rs];
    int operand2 = (opcode == BEQ)? registers[rt] : isImmediate ? immediate : registers[rt];
    int location = (opcode == BEQ)? immediate : 0;
    int result = startop(opcode, operand1, operand2,location);

    //For store word instructions
    if(opcode==SDW){
        int temp = rs + immediate;
        memory[temp/4] = registers[rt];
        printf("\nData written at %0d is %0d",temp/4,registers[rt]);
    }

    // For all instructions, immediate or not, store result in rt (I-type) or rd (R-type)
    if (isImmediate & ~(opcode == BZ))
        registers[rt] = result;
    else if (opcode == BZ | opcode == BEQ | opcode == JR)
    {
        PC = result;
    }
    
    else
        registers[rd] = result;
}


void printRegisters() {
    cout << "\nRegisters:\n";
    cout << "R0: " <<R0 << "\n";
    for (int i = 1; i < 32; ++i) {
        cout << "R" << i << ": " << registers[i] << "\n";
    }
    printf("PC: %0d\n",(PC+1)*4);
}
