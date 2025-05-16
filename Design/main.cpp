#include <iostream>
#include <cstdio>
#include "pkg.h"

int main() {

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
                printf("\n I Type: opcode = %x\n",inst.I_type.opcode);
                printf("\n R Type: opcode = %x\n",inst.R_type.opcode);

            }
            else {
                printf("\nParsing failed!");
            }
        }
    }

    fclose(fd);
    return 0;
}