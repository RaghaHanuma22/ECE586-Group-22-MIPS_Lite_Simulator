#include <iostream>
#include <cstdio>
#include "pkg.h"
#include "sim.h"
#include <cstdlib>



    // Function to load memory from a file
    void loadMemory(const char *filename) {
        FILE *fd = fopen(filename, "r");
        
        if(!fd) {
            printf("\nMemory image file could not be opened!");
            exit(1);
        }
        
        char line[256];  // Buffer to hold each line
        unsigned int value;
        int address = 0;
        
        // Read each line and store in memory array
        while(fgets(line, sizeof(line), fd) && address < MEMORY_SIZE) {
            if(line[0] != '\0' && line[1] != '\0') {
                if(sscanf(line, "%x", &value) == 1) {
                    memory[address] = value;
                    address++;
                }
            }
        }
        
        printf("\nLoaded %d words into memory", address);
        fclose(fd);
    }

int main() {


const char *filename = "ece586_sample_memory_image.txt";
loadMemory(filename);

// Option 1: Execute instructions from the same memory image
printf("\n\n--- Option 1: Executing instructions from memory ---");

unsigned int instruction_count = 25;

for(int i = 0; i < instruction_count; i++) {
    unsigned int instr_address = memory[PC];

    inst.instr = instr_address;
    executeInstruction();
    printRegisters();
    PC+=1;

}
    return 0;
}




/*

    const char *filename = "testing_img.txt";
    FILE *fd = fopen(filename, "r");

    if(fd) {
        printf("\nFile opened");
    }
    else {
        printf("\nFile not opened");
    }

    char line[256];  // Buffer to hold each line
    unsigned int address;

    while(fgets(line,sizeof(line),fd)) {
        if(line[0] != '\0' && line[1] != '\0'){
            if(sscanf(line,"%x",&address) == 1){
                printf("\nAddress in Hex: %x",address);
                inst.instr = address; //40 - 0000 0100
                printf("\n Instruction = %x",inst.instr);
                printf("\n I Type: opcode = %x",inst.I_type.opcode);
                printf("\n R Type: opcode = %x",inst.R_type.opcode);
                executeInstruction();
                printRegisters();
            }
            else {
                printf("\nParsing failed!");
            }
        }
    }

    fclose(fd);
    */