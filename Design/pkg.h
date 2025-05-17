#ifndef PKG_H
#define PKG_H


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

#endif
