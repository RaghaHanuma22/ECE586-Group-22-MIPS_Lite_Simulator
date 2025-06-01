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
int pregisters[32] = {0}; // Changed to 32 registers (including R0)
const int pr0 = 0;

// Statistics tracking variables (similar to mips_hazards.c)
int ArithmeticInst = 0;
int LogicalInst = 0;
int MemoryInst = 0;
int ControlInst = 0;
int TotalInstructions = 0;
int Stalls = 0;
int FrwdStalls = 0;
int Penalties = 0;
int Instn_idx = 0; // Current instruction index
bool DReg[32] = {false}; // Track which registers were modified
vector<int> DynInstList(MAX_INSTRUCTIONS); // Dynamic instruction list
vector<int> RawHazDict(MAX_INSTRUCTIONS, 0); // RAW hazard tracking
vector<int> Flag2Dict(MAX_INSTRUCTIONS, 0); // Branch penalty tracking
vector<bool> BranchFlag(MAX_INSTRUCTIONS, false); // Branch instruction tracking

// Memory tracking structure
struct MemEntry {
    int address;
    int value;
};
vector<MemEntry> MemDict;
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
            ArithmeticInst++;
            break;
        case OR: case ORI: case AND: case ANDI: case XOR: case XORI:
            LogicalInst++;
            break;
        case LDW: case STW:
            MemoryInst++;
            break;
        case BZ: case BEQ: case JR:
            ControlInst++;
            break;
        default:
            break;
    }
    TotalInstructions++;
}
/*
bool isDataHazard(element_t current, element_t previous) {
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

void stallsInjection(element_t* memImage) {
    if (Instn_idx == 0) return;

    element_t curr = memImage[Instn_idx];
    
    // Check 1-instruction back for hazards
    if (Instn_idx >= 1) {
        element_t prev1 = memImage[Instn_idx - 1];
        if (isDataHazard(curr, prev1)) {
            if (prev1.opcode == LDW || prev1.opcode == STW) {
                FrwdStalls++; //Stall counter when we are fowarding but instn stalls because of LDW|STW
            }
            Stalls += 2; //No forwarding stalls
            RawHazDict[Instn_idx] = -1;
            return;
        }
    }

    // Check 2-instructions back for hazards
    if (Instn_idx >= 2) {
        element_t prev2 = memImage[Instn_idx - 2];
        if (isDataHazard(curr, prev2)) {
            Stalls += 1;
            RawHazDict[Instn_idx] = -2;
            return;
        }
    }
}

void hazardsChecker(element_t* memImage) {
    if (Instn_idx == 0) return;
    
    // Check for branch penalties
    if (Instn_idx >= 1 && BranchFlag[Instn_idx-1]) {
        Penalties += 2;
        Flag2Dict[Instn_idx] = -1;
        return;
    }
    if (Instn_idx >= 2 && BranchFlag[Instn_idx-2]) {
        stallsInjection(memImage);
        return;
    }
    stallsInjection(memImage);
}*/


bool isDataHazard(element_t current, element_t previous) {
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

void stallsInjection(element_t* memImage, int currentPC) {
    if (currentPC == 0) return;

    element_t curr = memImage[currentPC];
    
    // Check 1-instruction back for hazards
    if (currentPC >= 1) {
        element_t prev1 = memImage[currentPC - 1];
        if (isDataHazard(curr, prev1)) {
            // WITHOUT forwarding: Always need 2 stalls for 1-instruction separation
            if (prev1.opcode == LDW) {
                // Load-use hazard: even with forwarding, still needs 1 stall
                FrwdStalls += 1; // Stall counter for forwarding case
                Stalls += 2;     // Stalls for no-forwarding case
                RawHazDict[Instn_idx] = -1; // Indicates 2-cycle stall
            } else {
                // Regular ALU operation hazard
                Stalls += 2;     // Without forwarding: 2 stalls needed
                RawHazDict[Instn_idx] = -1; // Indicates 2-cycle stall
                // With forwarding: no additional stalls needed (FrwdStalls += 0)
            }
            return;
        }
    }

    // Check 2-instructions back for hazards
    if (currentPC >= 2) {
        element_t prev2 = memImage[currentPC - 2];
        if (isDataHazard(curr, prev2)) {
            // With 2-instruction separation, the hazard resolves naturally
            // No stalls needed even without forwarding
            return;
        }
    }
}

void hazardsChecker(element_t* memImage) {
    if (PPC == 0) return;
    
    // Check for branch penalties
    if (PPC >= 1 && BranchFlag[PPC-1]) {
        Penalties += 2;
        Flag2Dict[Instn_idx] = -1;
        return;
    }
    if (PPC >= 2 && BranchFlag[PPC-2]) {
        stallsInjection(memImage, PPC);
        return;
    }
    stallsInjection(memImage, PPC);
}

int executeInstruction(element_t instr) {
    int result = 0;
    
    switch(instr.opcode) {
        case ADD:
            result = pregisters[instr.rs] + pregisters[instr.rt];
            pregisters[instr.rd] = result;
            DReg[instr.rd] = true;
            break;
        case ADDI:
            result = pregisters[instr.rs] + instr.imm;
            pregisters[instr.rt] = result;
            DReg[instr.rt] = true;
            break;
        case SUB:
            result = pregisters[instr.rs] - pregisters[instr.rt];
            pregisters[instr.rd] = result;
            DReg[instr.rd] = true;
            break;
        case SUBI:
            result = pregisters[instr.rs] - instr.imm;
            pregisters[instr.rt] = result;
            DReg[instr.rt] = true;
            break;
        case MUL:
            result = pregisters[instr.rs] * pregisters[instr.rt];
            pregisters[instr.rd] = result;
            DReg[instr.rd] = true;
            break;
        case MULI:
            result = pregisters[instr.rs] * instr.imm;
            pregisters[instr.rt] = result;
            DReg[instr.rt] = true;
            break;
        case OR:
            result = pregisters[instr.rs] | pregisters[instr.rt];
            pregisters[instr.rd] = result;
            DReg[instr.rd] = true;
            break;
        case ORI:
            result = pregisters[instr.rs] | instr.imm;
            pregisters[instr.rt] = result;
            DReg[instr.rt] = true;
            break;
        case AND:
            result = pregisters[instr.rs] & pregisters[instr.rt];
            pregisters[instr.rd] = result;
            DReg[instr.rd] = true;
            break;
        case ANDI:
            result = pregisters[instr.rs] & instr.imm;
            pregisters[instr.rt] = result;
            DReg[instr.rt] = true;
            break;
        case XOR:
            result = pregisters[instr.rs] ^ pregisters[instr.rt];
            pregisters[instr.rd] = result;
            DReg[instr.rd] = true;
            break;
        case XORI:
            result = pregisters[instr.rs] ^ instr.imm;
            pregisters[instr.rt] = result;
            DReg[instr.rt] = true;
            break;
        case LDW:
            {
                int address = pregisters[instr.rs] + instr.imm;
                result = memory[address/4];
                pregisters[instr.rt] = result;
                DReg[instr.rt] = true;
            }
            break;
        case STW:
            {
                int address = pregisters[instr.rs] + instr.imm;
                memory[address/4] = pregisters[instr.rt];
                // Track memory stores
                MemEntry entry = {address, pregisters[instr.rt]};
                MemDict.push_back(entry);
                memEntryCount++;
            }
            break;
        case BZ:
            BranchFlag[Instn_idx] = true;
            if (pregisters[instr.rs] == 0)
                return PPC + instr.imm;
            return PPC + 1;
        case BEQ:
            BranchFlag[Instn_idx] = true;
            if (pregisters[instr.rs] == pregisters[instr.rt])
                return PPC + instr.imm;
            return PPC + 1;
        case JR:
            BranchFlag[Instn_idx] = true;
            return pregisters[instr.rs] / 4; // Convert to word address
        case HALT:
            return PPC;
        default:
            return PPC + 1;
    }
    return PPC + 1;
}

// Display function (adapted from mips_hazards.c)
void display() {
    
    cout << "======================================================================" << endl;
    cout << "                     Simulation Summary" << endl;
    cout << "======================================================================" << endl;
    cout << "Total instructions       : " << TotalInstructions << endl;
    cout << "  Arithmetic             : " << ArithmeticInst << endl;
    cout << "  Logical                : " << LogicalInst << endl;
    cout << "  Memory (LD/ST)         : " << MemoryInst << endl;
    cout << "  Control (branches,jr)  : " << ControlInst << endl;
    cout << "----------------------------------------------------------------------" << endl;

    cout << "----------------------------------------------------------------------" << endl;

    cout << "----------------------------------------------------------------------" << endl;
}
/*
void pipe_stats() {
    // Calculate hazard statistics
    int hazardCount = 0, single = 0, dbl = 0, branchCount = 0;
    for (int i = 0; i < Instn_idx; i++) {
        if (RawHazDict[i] != 0) {
            hazardCount++;
            if (RawHazDict[i] == -2) single++;
            else if (RawHazDict[i] == -1) dbl++;
        }
        if (Flag2Dict[i] != 0) branchCount++;
    }
    
    double avgStalls = hazardCount ? ((double)Stalls)/hazardCount : 0.0;
    double avgBranchPenalty = branchCount ? ((double)Penalties)/branchCount : 0.0;
    int totalCyclesNoFwd = Instn_idx + 5 + Stalls + Penalties;
    int totalCyclesFwd = Instn_idx + 5 + FrwdStalls + Penalties;
    double speedup = totalCyclesNoFwd > 0 ? (double)totalCyclesNoFwd / totalCyclesFwd : 1.0;

    cout << "\nStalls (no forwarding)      : " << Stalls << endl;
    cout << "  avg  cycles/hazard        : " << fixed << setprecision(5) << avgStalls << endl;
    cout << "  single-cycle stalls       : " << single << endl;
    cout << "  double-cycle stalls       : " << dbl << endl;
    cout << "Branch penalties            : " << Penalties << " cycles (in " << branchCount << " branches)" << endl;
    cout << "  avg cycles/branch         : " << fixed << setprecision(5) << avgBranchPenalty << endl;
    cout << "Stalls (with forwarding)    : " << FrwdStalls << endl;
    cout << "Total cycles w/o forwarding : " << totalCyclesNoFwd << endl;
    cout << "Total cycles with forwarding: " << totalCyclesFwd << endl;
    cout << "Speedup                     : " << fixed << setprecision(5) << speedup << "×" << endl;
    //cout <<"Final PPC                    : " <<PPC <<endl;
    cout << "======================================================================" << endl;
}*/

void pipe_stats() {
    // Calculate hazard statistics
    int hazardCount = 0, single = 0, dbl = 0, branchCount = 0;
    for (int i = 0; i < Instn_idx; i++) {
        if (RawHazDict[i] != 0) {
            hazardCount++;
            // Note: We removed 2-instruction separation stalls (-2), 
            // so single should be 0 now
            if (RawHazDict[i] == -2) single++;      // Should be 0 with corrected logic
            else if (RawHazDict[i] == -1) dbl++;    // 2-cycle stalls for 1-instruction separation
        }
        if (Flag2Dict[i] != 0) branchCount++;
    }
    
    // With corrected logic: hazardCount should equal dbl (all remaining hazards are 2-cycle)
    double avgStalls = hazardCount ? ((double)Stalls)/hazardCount : 0.0;
    double avgBranchPenalty = branchCount ? ((double)Penalties)/branchCount : 0.0;
    
    // Total cycles calculation:
    // Base execution: Instn_idx instructions + 5 pipeline latency
    int totalCyclesNoFwd = Instn_idx + 5 + Stalls + Penalties;
    int totalCyclesFwd = Instn_idx + 5 + FrwdStalls + Penalties;
    double speedup = totalCyclesFwd > 0 ? (double)totalCyclesNoFwd / totalCyclesFwd : 1.0;

    cout << "\nStalls (no forwarding)      : " << Stalls << endl;
    cout << "  avg  cycles/hazard        : " << fixed << setprecision(5) << avgStalls << endl;
    cout << "  single-cycle stalls       : " << single << endl;
    cout << "  double-cycle stalls       : " << dbl << endl;
    cout << "Branch penalties            : " << Penalties << " cycles (in " << branchCount << " branches)" << endl;
    cout << "  avg cycles/branch         : " << fixed << setprecision(5) << avgBranchPenalty << endl;
    cout << "Stalls (with forwarding)    : " << FrwdStalls << endl;
    cout << "Total cycles w/o forwarding : " << totalCyclesNoFwd << endl;
    cout << "Total cycles with forwarding: " << totalCyclesFwd << endl;
    cout << "Speedup                     : " << fixed << setprecision(5) << speedup << "×" << endl;
    //cout <<"Final PPC                    : " <<PPC <<endl;
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

    cout << "\n=== Pipeline Execution Trace ===" << endl;
    cout << "| Cycle | Instruction | PC   | Action" << endl;
    cout << "-------------------------------------" << endl;

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
        

        DynInstList[Instn_idx] = current_instruction;                   
        // Update instruction statistics
        updateInstructionStats(memImage[PPC].opcode);
        
        // Check for hazards
        hazardsChecker(memImage);
        
        // Execute instruction
        cout << "| " << setw(5) << clk << " | " << setw(11) << opcodeToString(memImage[PPC].opcode) 
             << " | " << setw(4) << (PPC * 4) << " | ";
        
        int newPC = executeInstruction(memImage[PPC]);
        
        if (newPC != PPC + 1) {
            cout << "Branch taken to PC=" << (newPC * 4);
        } else {
            cout << "Normal execution";
        }
        cout << endl;
        
        PPC = newPC;
        Instn_idx++;
        clk++;
        
        // Add stall cycles
        if (RawHazDict[Instn_idx-1] != 0) {
            int stallCycles = (RawHazDict[Instn_idx-1] == -1) ? 2 : 1;
            for (int i = 0; i < stallCycles; i++) {
                cout << "| " << setw(5) << clk << " | " << setw(11) << "STALL" 
                     << " | " << setw(4) << "----" << " | RAW Hazard detected" << endl;
                clk++;
            }
        }
        
        if (Flag2Dict[Instn_idx-1] != 0) {
            for (int i = 0; i < 2; i++) {
                cout << "| " << setw(5) << clk << " | " << setw(11) << "STALL" 
                     << " | " << setw(4) << "----" << " | Branch penalty" << endl;
                clk++;
            }
        }
    }

    if (PPC < mem_size && memImage[PPC].opcode == HALT) {
        cout << "| " << setw(5) << clk << " | " << setw(11) << "HALT" 
             << " | " << setw(4) << (PPC * 4) << " | Program terminated" << endl;
        updateInstructionStats(HALT);
    }

    cout << "\n";
    //display();
    
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