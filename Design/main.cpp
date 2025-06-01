#include <iostream>
#include <cstdio>
#include "pkg.h"
#include "sim.h"
#include "timing.h"
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

int main(int argc, char* argv[]) {

    const char *filename =argv[1];

loadMemory(filename);

// Option 1: Execute instructions from the same memory image
printf("\n\n--- Option 1: Executing instructions from memory ---\n");

unsigned int instruction_count = 5;

for(int i = 0; i < instruction_count; i++) {
    unsigned int instr_address = memory[PC];

    inst.instr = instr_address;

   if(inst.I_type.opcode == 0b010001){
    printf("Halt instruction executed! Terminating program!");
    break;
   }
   else if(inst.instr ==0x00000000){
    continue;
   }
   else{
    executeInstruction();
    PC+=1;
   }
    
    

}
//PIPELINE SIMULATOR FUNCTION
pipesim(filename);
 display();
 printstate();

 pipe_stats();
    return 0;
}