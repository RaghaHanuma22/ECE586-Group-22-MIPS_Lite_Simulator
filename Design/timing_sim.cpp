#include <iostream>
#include <cstdio>
#include "pkg.h"
#include "sim.h"
#include <cstdlib>
#include <cmath>
#include "timing.h"
#include <fstream>
#include <vector>
#include <iomanip>
#include <stdint.h>

using namespace std;

#define CAPACITY 100
#define MAX_INSTRUCTIONS 1000

// Global variables for timing simulation
static int PPC = 0;
int mem_size = 0;
int pregisters[32] = {0}; 
const int pr0 = 0;


int Arith_instns = 0;
int Logical_instns = 0;
int Memory_instns = 0;
int Control_instns = 0;
int Total_instns = 0;
int Stalls = 0;
int ldw_stalls = 0;
int Penalties = 0;
int Instn_idx = 0; // Current instruction index
bool written_regs[32] = {false}; 
vector<int> instn_lst(MAX_INSTRUCTIONS); // instruction list
vector<int> RAWhaz(MAX_INSTRUCTIONS, 0); // RAW hazard tracking
vector<int> branch_pen(MAX_INSTRUCTIONS, 0); // Branch penalty tracking
vector<bool> branch_instns(MAX_INSTRUCTIONS, false); // Branch instruction count

// NEW: Track branch penalties properly
int lastBranchPC = -1;
int branchTargetPC = -1;

// Memory tracking structure
struct MemEntry {
    int address;
    int value;
};
vector<MemEntry> memory_entries;
int memEntryCount = 0;

typedef enum {
    RTYPE, ITYPE, NULL_TYPE
} optype_t;

typedef enum {
    ADD = 0, ADDI = 1, SUB = 2, SUBI = 3, MUL = 4, MULI = 5,
    OR = 6, ORI = 7, AND = 8, ANDI = 9, XOR = 10, XORI = 11,
    LDW = 12, STW = 13, BZ = 14, BEQ = 15, JR = 16, HALT = 17,
    NOP = 63
} opcode_t;

typedef struct {
    optype_t optype;
    opcode_t opcode;
    int rd;
    int rs;
    int rt;
    int imm;
} element_t;

enum stage {IF=0, ID=1, EX=2, MEM=3, WB=4};
element_t stall = {NULL_TYPE, NOP, -1, -2, -3, -4};
element_t pipeline[5] = {stall, stall, stall, stall, stall};

// Hazard detection functions (adapted from mips_hazards.c)
void updateInstructionStats(opcode_t opcode) {
    switch(opcode) {
        case ADD: case ADDI: case SUB: case SUBI: case MUL: case MULI:
            Arith_instns++;
            break;
        case OR: case ORI: case AND: case ANDI: case XOR: case XORI:
            Logical_instns++;
            break;
        case LDW: case STW:
            Memory_instns++;
            break;
        case BZ: case BEQ: case JR: case HALT:
            Control_instns++;
            break;
        default:
            break;
    }
    Total_instns++;
}

bool isRAWhaz(element_t current, element_t previous) {
    // Check if previous instruction writes to a register that current instruction reads
    if (previous.optype == RTYPE && previous.rd != -1) {
        if (previous.rd == current.rs || previous.rd == current.rt) {
            return true;
        }
    } else if (previous.optype == ITYPE && previous.rt != -1) {
        if (previous.rt == current.rs || previous.rt == current.rt) {
            return true;
        }
    }
    return false;
}

void inject_stalls(element_t* memImage, int currentPC) {
    if (currentPC == 0) return;

    element_t curr = memImage[currentPC];
    
    // Check 1-instruction back for hazards
    if (currentPC >= 1) {
        element_t prev1 = memImage[currentPC - 1];
        element_t prev2 = memImage[currentPC - 2];
        if (isRAWhaz(curr, prev1)) {
            // WITHOUT forwarding: Always need 2 stalls for 1-instruction separation
            if (prev1.opcode == LDW) {
                // Load-use hazard: even with forwarding, still needs 1 stall
                ldw_stalls += 1; // Stall counter for forwarding case
                Stalls += 2;     // Stalls for no-forwarding case
                RAWhaz[Instn_idx] = -1; // Indicates 2-cycle stall
            } else {
                // Regular ALU operation hazard
                Stalls += 2;     // Without forwarding: 2 stalls needed
                RAWhaz[Instn_idx] = -1; // Indicates 2-cycle stall
                // With forwarding: no additional stalls needed (ldw_stalls += 0)
            }
            return;
        }
    }

    // Check 2-instructions back for hazards
    if (currentPC >= 2) {
        element_t prev2 = memImage[currentPC - 2];
        if (isRAWhaz(curr, prev2)) {
            // With 2-instruction separation, the hazard resolves naturally
            // No stalls needed even without forwarding
            return;
        }
    }
}

void check_hazards(element_t* memImage) {
    if (PPC == 0) return;
    
    // CORRECTED: Check if we just arrived at a branch target
    if (lastBranchPC != -1 && PPC == branchTargetPC) {
        Penalties += 2;            // Add branch misprediction penalty
        branch_pen[Instn_idx] = -1; // Mark this instruction as affected by branch penalty
        lastBranchPC = -1;         // Reset the branch tracking
        branchTargetPC = -1;
        return;                    // Skip further hazard checks this cycle
    }
    
    // Otherwise, continue with normal hazard checking for data hazards
    inject_stalls(memImage, PPC);
}

int executeInstruction(element_t instr) {
    int result = 0;
    
    switch(instr.opcode) {
        case ADD:
            result = pregisters[instr.rs] + pregisters[instr.rt];
            pregisters[instr.rd] = result;
            written_regs[instr.rd] = true;
            break;
        case ADDI:
            result = pregisters[instr.rs] + instr.imm;
            pregisters[instr.rt] = result;
            written_regs[instr.rt] = true;
            break;
        case SUB:
            result = pregisters[instr.rs] - pregisters[instr.rt];
            pregisters[instr.rd] = result;
            written_regs[instr.rd] = true;
            break;
        case SUBI:
            result = pregisters[instr.rs] - instr.imm;
            pregisters[instr.rt] = result;
            written_regs[instr.rt] = true;
            break;
        case MUL:
            result = pregisters[instr.rs] * pregisters[instr.rt];
            pregisters[instr.rd] = result;
            written_regs[instr.rd] = true;
            break;
        case MULI:
            result = pregisters[instr.rs] * instr.imm;
            pregisters[instr.rt] = result;
            written_regs[instr.rt] = true;
            break;
        case OR:
            result = pregisters[instr.rs] | pregisters[instr.rt];
            pregisters[instr.rd] = result;
            written_regs[instr.rd] = true;
            break;
        case ORI:
            result = pregisters[instr.rs] | instr.imm;
            pregisters[instr.rt] = result;
            written_regs[instr.rt] = true;
            break;
        case AND:
            result = pregisters[instr.rs] & pregisters[instr.rt];
            pregisters[instr.rd] = result;
            written_regs[instr.rd] = true;
            break;
        case ANDI:
            result = pregisters[instr.rs] & instr.imm;
            pregisters[instr.rt] = result;
            written_regs[instr.rt] = true;
            break;
        case XOR:
            result = pregisters[instr.rs] ^ pregisters[instr.rt];
            pregisters[instr.rd] = result;
            written_regs[instr.rd] = true;
            break;
        case XORI:
            result = pregisters[instr.rs] ^ instr.imm;
            pregisters[instr.rt] = result;
            written_regs[instr.rt] = true;
            break;
        case LDW:
            {
                int address = pregisters[instr.rs] + instr.imm;
                result = memory[address/4];
                pregisters[instr.rt] = result;
                written_regs[instr.rt] = true;
            }
            break;
        case STW:
            {
                int address = pregisters[instr.rs] + instr.imm;
                memory[address/4] = pregisters[instr.rt];
                // Track memory stores
                MemEntry entry = {address, pregisters[instr.rt]};
                memory_entries.push_back(entry);
                memEntryCount++;
            }
            break;
        case BZ:
            branch_instns[Instn_idx] = true;
            if (pregisters[instr.rs] == 0) {
                // Branch taken - record for penalty tracking
                lastBranchPC = PPC;
                branchTargetPC = PPC + instr.imm;
                return branchTargetPC;
            }
            return PPC + 1;
        case BEQ:
            branch_instns[Instn_idx] = true;
            if (pregisters[instr.rs] == pregisters[instr.rt]) {
                // Branch taken - record for penalty tracking
                lastBranchPC = PPC;
                branchTargetPC = PPC + instr.imm;
                return branchTargetPC;
            }
            return PPC + 1;
        case JR:
            branch_instns[Instn_idx] = true;
            // Branch taken - record for penalty tracking
            lastBranchPC = PPC;
            branchTargetPC = pregisters[instr.rs] / 4; // Convert to word address
            return branchTargetPC;
        case HALT:
            return PPC;
        default:
            return PPC + 1;
    }
    return PPC + 1;
}

// Display function (adapted from mips_hazards.c)
void display() {
    
    cout << "Instruction counts:\n "; 
    cout<<  " Total number of instructions: "<< Total_instns << endl;
    cout << "  Arithmetic Instructions             : " << Arith_instns << endl;
    cout << "  Logical Instructions                : " << Logical_instns << endl;
    cout << "  Memory access         : " << Memory_instns << endl;
    cout << "  Control transfer instructions  : " << Control_instns << endl;
    cout << "\n----------------------------------------------------------------------" << endl;
}

void pipe_stats(int mode) {
    // Calculate hazard statistics
    int hazardCount = 0, single = 0, dbl = 0, branchCount = 0;
    for (int i = 0; i < Instn_idx; i++) {
        if (RAWhaz[i] != 0) {
            hazardCount++;
            // Note: We removed 2-instruction separation stalls (-2), 
            // so single should be 0 now
            if (RAWhaz[i] == -2) single++;      // Should be 0 with corrected logic
            else if (RAWhaz[i] == -1) dbl++;    // 2-cycle stalls for 1-instruction separation
        }
        if (branch_pen[i] != 0) branchCount++;
    }
    
    // With corrected logic: hazardCount should equal dbl (all remaining hazards are 2-cycle)
    double avgStalls = hazardCount ? ((double)Stalls)/hazardCount : 0.0;
    double avgBranchPenalty = branchCount ? ((double)Penalties)/branchCount : 0.0;
    
    // Total cycles calculation:
    // Base execution: Instn_idx instructions + 5 pipeline latency
    int total_cycles_wo_fwd = Instn_idx + 5 + Stalls + Penalties;
    int total_cycles_w_fwd = Instn_idx + 5 + ldw_stalls + Penalties;
    #ifdef DEBUG
    double speedup = total_cycles_w_fwd > 0 ? (double)total_cycles_wo_fwd / total_cycles_w_fwd : 1.0;
   
    cout << "  avg  cycles/hazard        : " << fixed << setprecision(5) << avgStalls << endl;
    cout << "  single-cycle stalls       : " << single << endl;
    cout << "  double-cycle stalls       : " << dbl << endl;
    
    cout << "  avg cycles/branch         : " << fixed << setprecision(5) << avgBranchPenalty << endl;
    
    #endif
    cout<<"Timing Simulator"<<endl;
    if(mode==0){
        cout<<"Timing simulator only available in mode 1 and 2!"<<endl;
    }
    if(mode==1) {
    cout << "Total cycles w/o forwarding : " << total_cycles_wo_fwd << endl;
    cout << "Branch Penalty            : " << Penalties << " cycles"<< endl;
    cout << "\nStalls (no forwarding)      : " << Stalls << endl;
    }
    if(mode==2){
    cout << "Total cycles with forwarding: " << total_cycles_w_fwd << endl;
    cout << "Branch Penalty            : " << Penalties << " cycles"<<endl;
    cout << "Stalls (with forwarding)    : " << ldw_stalls << endl;
    }
    cout << "======================================================================" << endl;
}

// Your existing functions with modifications
element_t* fileToMemory(const char* filename);
element_t hexToElement(string memLine);
string opcodeToString(opcode_t opcode);

int pipesim(const char* filename) {
    element_t* memImage;
    int clk = 0;

    memImage = fileToMemory(filename);

    while(PPC < mem_size && memImage[PPC].opcode != HALT) {
        // Store current instruction in dynamic list
        uint32_t current_instruction = (memImage[PPC].opcode << 26) | 
                            (memImage[PPC].rs << 21) | 
                            (memImage[PPC].rt << 16) | 
                            (memImage[PPC].rd << 11) | 
                            (memImage[PPC].imm & 0xFFFF);
        
        if(current_instruction == 0x00000000){
            PPC++;
            clk++;
            continue;
        }
        

        instn_lst[Instn_idx] = current_instruction;                   
        // Update instruction statistics
        updateInstructionStats(memImage[PPC].opcode);
        
        // Check for hazards
        check_hazards(memImage);
        
        
        int newPC = executeInstruction(memImage[PPC]);
        
        PPC = newPC;
        Instn_idx++;
        clk++;
        
        // Add stall cycles
        if (RAWhaz[Instn_idx-1] != 0) {
            int stallCycles = (RAWhaz[Instn_idx-1] == -1) ? 2 : 1;
            for (int i = 0; i < stallCycles; i++) {
                
                clk++;
            }
        }
        
        if (branch_pen[Instn_idx-1] != 0) {
            for (int i = 0; i < 2; i++) {
                
                clk++;
            }
        }
    }

    if (PPC < mem_size && memImage[PPC].opcode == HALT) {
        
        updateInstructionStats(HALT);
    }

    cout << "\n";
    
    return EXIT_SUCCESS;
    
}

// Your existing helper functions (keeping them as they were)
element_t* fileToMemory(const char* filename){
    element_t* memImage = new element_t[CAPACITY];
    string line;
    
    fstream file;
    file.open(filename);

    if (!file.is_open()) {
        cerr << "Error: Unable to open file!" << endl;
        return 0;
    }

    while (getline(file, line) && mem_size < CAPACITY) {
        memImage[mem_size] = hexToElement(line);
        mem_size++;
    }
    file.close();
    return memImage;
}

element_t hexToElement(string memLine){
    element_t element;
    int nibble[8];
    long val = 0;

    // Convert Hex to decimal
    for (int i = 0; i < 8; i++) {
        switch (memLine[i]){
            case '0': nibble[i] = 0; break;    
            case '1': nibble[i] = 1; break;
            case '2': nibble[i] = 2; break;
            case '3': nibble[i] = 3; break;
            case '4': nibble[i] = 4; break;
            case '5': nibble[i] = 5; break;
            case '6': nibble[i] = 6; break;
            case '7': nibble[i] = 7; break;
            case '8': nibble[i] = 8; break;
            case '9': nibble[i] = 9; break;
            case 'A': nibble[i] = 10; break;
            case 'B': nibble[i] = 11; break;
            case 'C': nibble[i] = 12; break;
            case 'D': nibble[i] = 13; break;
            case 'E': nibble[i] = 14; break;
            case 'F': nibble[i] = 15; break;
            default: nibble[i] = 0;
        }
    }
    
    // combine all converted hex values into a single decimal value
    for (int i = 0; i < 8; i++) {
        val += nibble[7-i] * pow(2, 4*i);
    }    

    // Decode opcode (& optype)
    element.opcode = opcode_t((val&0xFC000000)/pow(2, 26));
    if (element.opcode == ADD || element.opcode == SUB || element.opcode == MUL ||
        element.opcode == AND || element.opcode == OR || element.opcode == XOR) {
        element.optype = RTYPE;
    } else {
        element.optype = ITYPE;
    }

    // Decode Rs & Rt
    element.rs = (val&0x03E00000)/pow(2, 21);
    element.rt = (val&0x001F0000)/pow(2, 16);

    // Decode Rd and Imm depending on the instruction type
    if (element.optype == RTYPE) {
        element.rd = (val&0x0000F800)/pow(2, 11);
        element.imm = 0;    
    } else {
        element.imm = (val&0x0000FFFF);
        // Sign extend immediate if it's negative (bit 15 set)
        if (element.imm & 0x8000) {
            element.imm |= 0xFFFF0000;
        }
        element.rd = -1;
    }

    return element;
}

string opcodeToString(opcode_t opcode) {
    switch (opcode) {
        case (ADD):     return "ADD ";
        case (ADDI):    return "ADDI";
        case (SUB):     return "SUB ";
        case (SUBI):    return "SUBI";
        case (MUL):     return "MUL ";
        case (MULI):    return "MULI";
        case (OR):      return "OR  ";
        case (ORI):     return "ORI ";
        case (AND):     return "AND ";
        case (ANDI):    return "ANDI";
        case (XOR):     return "XOR ";
        case (XORI):    return "XORI";
        case (LDW):     return "LDW ";
        case (STW):     return "STW ";
        case (BZ):      return "BZ  ";
        case (BEQ):     return "BEQ ";
        case (JR):      return "JR  ";
        case (HALT):    return "HALT";
        case (NOP):     return "NOP ";
        default:        return "????";
    }
}