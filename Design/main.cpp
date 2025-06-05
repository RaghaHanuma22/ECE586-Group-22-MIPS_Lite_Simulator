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
        
        printf("\nLoaded %d words into memory\n", address);
        fclose(fd);
    }

int main(int argc, char* argv[]) {

    const char *filename =argv[1];
    int mode = atoi(argv[2]);  // 0 = none, 1 = no-forwarding only, 2 = with-forwarding only

loadMemory(filename);

unsigned int instruction_count = 1000;

for(int i = 0; i < instruction_count; i++) {
    unsigned int instr_address = memory[PC];

    inst.instr = instr_address;

   if(inst.I_type.opcode == 0b010001){
    break;
   }
   else if(inst.instr ==0x00000000){
    continue;
   }
   else{
    executeInstruction();
   PC++;
   }
    
    

}
//PIPELINE SIMULATOR FUNCTION
pipesim(filename);
 display();
 printstate();

 pipe_stats(mode);
    return 0;
}