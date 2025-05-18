#ifndef PKG_H
#define PKG_H


#define MEMORY_SIZE 2048  // Define memory size (adjust as needed)
#define PROGRAM_START 0   // Starting point for program code
extern unsigned int PC;


typedef union {
    unsigned int instr;
    struct {
        unsigned int unused: 11;
        unsigned int rd : 5;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opcode : 6;
    } R_type;
    struct {
        signed int imm : 16;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opcode : 6;
    } I_type;
} Instruction;


extern Instruction inst;
extern unsigned int memory[MEMORY_SIZE];
#endif
