/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_
#define TAKEN 1
#define NOT_TAKEN 0

#include "apex_macros.h"

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int imm;
} APEX_Instruction;

// condition code struct
typedef struct condition_code
{
    int z;
    int p;
    int n;
    int valid;
} condition_code;

typedef struct forward_bus
{
    int reg;
	int value;
}forward_bus;

typedef struct BTB  // stack max 4 enteries
{
    int inst_address;
    int executed; // if it was taken and executed for the first time
    int target_address;
    int prediction_state; // Variable to store the prediction state
    int history_state; //prediction history 
    int num_executed; //number of times branch was executed irrespective of taken,  not taken
} BTB;

// Define the circular queue structure
struct CircularQueue {
    BTB data[4];
    int head, tail;
    unsigned int size;
};

struct CircularQueue_prf {
    int data[24];
    int head, tail;
    unsigned int size;
};


typedef struct Queue{
    int allocate_bit;    //if the index is free or allocated
    int fu;
    int opcode;         // opcode number of instruction
    char opcode_str_iq[128];
    int literal;         // imm value if present
    int src1_valid;      // rs1 if found
    int src1_tag;        // store physical register address obtained from rename table
    int src1_value;      // value of rs1
    int src1_arc;
    int src2_valid;      // rs2 if found
    int src2_tag;       // store physical register address obtained from renmae table
    int src2_value;     // rs2 value
    int src2_arc;
    int dest;           // rd address in physical register address obtained from rename table
    int pc;             // pc value
    int issued;         // the instruction has been issued or not
    int result_buffer;
}Queue;

enum fu_enum {
	no_fu,
    int_fu,
    a_fu,
    mul_fu1,
    b_fu
};

typedef struct broadcasting{
    int tag;
    int value;
    int arc_reg;
}broadcasting;




typedef struct rob{
    int established_entry; //entry made 
    int opcode; //opcode of instruction
    char opstrg[128];
    int pc; //pc of instruction
    int rd_physical; //physical reg index
    int rename_entry; //prev rename entry
    int rd_arc; //arc reg 
    int lsq_index; //lsq index
    int status; //means updated or not
}rob;

typedef struct physical_regs{
    int valid;
    int value;
    int arc_reg;
    int prf_pc;
}physical_regs;




/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int rs1_f;   //flag if rs1 is found
    int rs2_f;   // flag rs2 is found
    int rd;
    int imm;
    int rs1_value;
    int rs2_value;
    int result_buffer;
    int memory_address;
    int has_insn;
    int stalled;
    int entry_made_iq;
    int entry_made_pr; 
    int entry_made_rob;
    int btb_searched;
    int type_of_branch;
} CPU_Stage;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int insn_completed;            /* Instructions retired */
    int regs[REG_FILE_SIZE];       /* Integer register file */
    int regs_writing[REG_FILE_SIZE];//for knowing which register is writing currently
    int code_memory_size;          /* Number of instruction in the input file */
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int simulate;
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int fetch_from_next_cycle;
    condition_code cc;            /*condition code flag for cmp,cml {1 (+), 0 (-)}*/
    forward_bus fb;
    struct forward_bus ex_fb;  //excution stage forward bus
    struct forward_bus mem_fb; //memory stage forward bus
    int mem_address[DATA_MEMORY_SIZE];
    int data_counter;
    BTB btb; 
    struct CircularQueue btb_queue; // Circular Queue for BTB entries
    struct Queue IQ[24];
    struct Queue BQ[24];
    struct physical_regs PR[25];
    struct condition_code CC[16];
    rob rob[32];
    broadcasting broadcasting[25];
    struct CircularQueue_prf prf_free_list;
    int iq_head;
    int iq_tail;
    int rob_tail;
    int rob_head;
    int check_int_fu;
    int check_a_fu;
    int check_b_fu;
    int check_mul_fu;
    int check_no_fu;
    int pr;
    int broadcasting_rs1;
    int broadcasting_rs2;

    
    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode;
    CPU_Stage dispatch;
    CPU_Stage issue;
    CPU_Stage int_fu;
    CPU_Stage mul_fu1;
    CPU_Stage mul_fu2;
    CPU_Stage mul_fu3;
    CPU_Stage a_fu;
    CPU_Stage b_fu;
    CPU_Stage no_fu;
    CPU_Stage execute;
    CPU_Stage memory;
    CPU_Stage update;
    CPU_Stage writeback;
} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
void simulate_cpu_for_cycles(APEX_CPU *cpu, int num_cycles);
void initCircularQueue(struct CircularQueue *queue);
#endif
