#include "info_print.h"
#include <stdio.h>
#include <string.h>

// //head>tail  that means rob/issue queue is empty

// void print_issue_queue(APEX_CPU *cpu) {
    
//     printf("\nPrinting IQ Entries:\n");
//     printf("| Allocate bit |  fu | Literal | Src1 Valid | Src1 Tag | Src1 Value  | Src2 Valid | Src2 Tag | Src2 Value  | Dest |   PC   | Issued |\n");
//     printf("|--------------|-----|---------|------------|----------|-------------|------------|----------|-------------|------|--------|--------|\n");

//     for (int i = 0; i < cpu->iq_tail; i++) {
//         printf("| %12d | %3d | %7d | %10d | %8d | %11d | %10d | %8d | %11d | %4d | %6d | %6d |\n",
//         cpu->IQ[i].allocate_bit, cpu->IQ[i].fu, cpu->IQ[i].literal,
//         cpu->IQ[i].src1_valid, cpu->IQ[i].src1_tag, cpu->IQ[i].src1_value,
//         cpu->IQ[i].src2_valid, cpu->IQ[i].src2_tag, cpu->IQ[i].src2_value,
//         cpu->IQ[i].dest, cpu->IQ[i].pc, cpu->IQ[i].issued);  
//     }
//     return;
// }

void print_issue_queue(APEX_CPU *cpu) {
    
    printf("\nPrinting IQ Entries:\n");
    printf("| Allocate bit |  fu | Literal | Src1 Valid | Src1 Tag | Src1 Value  | Src2 Valid | Src2 Tag | Src2 Value  | Dest |   PC   | Src1 Arc | Src2 Arc | Issued |\n");
    printf("|--------------|-----|---------|------------|----------|-------------|------------|----------|-------------|------|--------|----------|----------|--------|\n");

    for (int i = 0; i < cpu->iq_tail; i++) {
        printf("| %12d | %3d | %7d | %10d | %8d | %11d | %10d | %8d | %11d | %4d | %6d | %8d | %8d | %6d |\n",
        cpu->IQ[i].allocate_bit, cpu->IQ[i].fu, cpu->IQ[i].literal,
        cpu->IQ[i].src1_valid, cpu->IQ[i].src1_tag, cpu->IQ[i].src1_value,
        cpu->IQ[i].src2_valid, cpu->IQ[i].src2_tag, cpu->IQ[i].src2_value,
        cpu->IQ[i].dest, cpu->IQ[i].pc, cpu->IQ[i].src1_arc, cpu->IQ[i].src2_arc, cpu->IQ[i].issued);  
    }
    return;
}



void print_prf(APEX_CPU *cpu) {
    printf("\n+------+-------+----------+------------+");
    printf("\n|  PR  | Valid |   Value  | Arc Reg    |");
    printf("\n+------+-------+----------+------------+");

    for (int j = 0; j < 25; j++) {
        printf("\n|  P%-2d |   %d   | %-8d | %-10d |", j, cpu->PR[j].valid, cpu->PR[j].value, cpu->PR[j].arc_reg);
        
    }

    printf("\n+------+-------+----------+------------+\n");
}

void print_broadcst(APEX_CPU *cpu) {
    printf("\n+-----+----------+------------+");
    printf("\n| Tag |   Value  | Arc Reg    |");
    printf("\n+-----+----------+------------+");

    for (int j = 0; j < 25; j++) {
        printf("\n|   %2d  |   %6d   |   %8d   |", cpu->broadcasting[j].tag, cpu->broadcasting[j].value, cpu->broadcasting[j].arc_reg);
    }

    printf("\n+-----+----------+---------+\n");
}


// print rob struct values

void print_rob(APEX_CPU *cpu){
    printf("\nPrinting ROB Entries:\n");

    printf("| Established Entry | Opcode |   PC   | Physical Dest | Rename Entry | Architectural Dest | LSQ Index |\n");
    printf("|-------------------|--------|--------|---------------|--------------|--------------------|-----------|\n");

    for (int i = 0; i < cpu->rob_tail; i++) {
        printf("| %17d | %6d | %6d | %13d | %12d | %18d | %9d |\n",
        cpu->rob[i].established_entry, cpu->rob[i].opcode, cpu->rob[i].pc,
        cpu->rob[i].rd_physical, cpu->rob[i].rename_entry, cpu->rob[i].rd_arc,
        cpu->rob[i].lsq_index);  
    }
}




// ------ prf free list ------

int get_free_prf(APEX_CPU *cpu){
    for (int i = 0; i < 25; i++) {
        if (cpu->PR[i].valid == 0 && cpu->PR[i].arc_reg == -1 && cpu->PR[i].value==-1) {
            return i;
        }
    }
    return -1;
}

int set_free_prf(APEX_CPU *cpu, int prf_index, int arc_reg, int value, int pc_prf){
    cpu->PR[prf_index].valid = 0;
    cpu->PR[prf_index].value = value;
    cpu->PR[prf_index].arc_reg = arc_reg;
    cpu->PR[prf_index].prf_pc= pc_prf;
    return 1;
}

void free_prf(APEX_CPU *cpu, int prf_index)
{
    cpu->PR[prf_index].valid=0;
    cpu->PR[prf_index].value=-1;
    cpu->PR[prf_index].arc_reg=-1;
    cpu->PR[prf_index].prf_pc=-1;
}

int search_prf(APEX_CPU *cpu, int arc_reg, int pr_pc)
{
    for(int i = 24; i >= 0; i--)
    {
        if(cpu->PR[i].arc_reg == arc_reg && cpu->PR[i].prf_pc!= pr_pc)
        {
            //return cpu->PR[i].value;
            return i;
        }
    }
    return -2;
}

int search_prf_cpu(APEX_CPU *cpu, int arc_reg, int pc_prf)
{
    for(int i = 24; i >= 0; i--)
    {
        if(cpu->PR[i].arc_reg == arc_reg && cpu->PR[i].prf_pc!= pc_prf)
        {
            //return cpu->PR[i].value;
            return i;
        }
    }
    return -2;
}
int search_prf_broadcast(APEX_CPU *cpu, int arc_reg, int pr_pc)
{
    for(int i = 24; i >= 0; i--)
    {
        if(cpu->PR[i].arc_reg == arc_reg && cpu->PR[i].prf_pc== pr_pc)
        {
            //return cpu->PR[i].value;
            return i;
        }
    }
    return -2;
}

int search_prf_prev_rd(APEX_CPU *cpu, int arc_reg, int pr_pc)
{
    int count = 0;

    for (int i = 24; i >= 0; i--)
    {
        if (cpu->PR[i].arc_reg == arc_reg && cpu->PR[i].prf_pc== pr_pc)
        {
            count++;

            // Check if this is the second occurrence
            if (count == 2)
            {
                return i;
            }
        }
    }

    return -2; // If the second occurrence is not found
}
int update_prf(APEX_CPU *cpu, int rd)
{
    for(int i=0;i<25;i++)
    {
        if(cpu->PR[i].arc_reg==rd){
            if(cpu->PR[i].valid==0)
            {
                return i;
            }
        }
    }

    return -2;
}

//----------------CC------------------------------------------------------------------------------------



// ------- issue queue ----------------------------------------------------------------

void increment_iq_head(APEX_CPU *cpu){
    cpu->iq_head = (cpu->iq_head + 1);
}

void increment_iq_tail(APEX_CPU *cpu){
    cpu->iq_tail = (cpu->iq_tail + 1);
}

int opcode_to_fu(int opcode){
    int fu;
    switch (opcode)
    {
    case OPCODE_ADD:
    case OPCODE_ADDL:
    case OPCODE_SUB:
    case OPCODE_SUBL:
    case OPCODE_XOR:
    case OPCODE_OR:
    case OPCODE_AND:
        fu = int_fu;
        break;
    case OPCODE_MOVC:
        fu = int_fu;
        break;
    case OPCODE_MUL:
        fu= mul_fu1;
        break;
    default:
        fu = no_fu;
        break;
    }
    return fu;
}

void free_issue_queue(APEX_CPU *cpu, int iss_idx)
{
    cpu->IQ[iss_idx].allocate_bit=-1;
    cpu->IQ[iss_idx].opcode = -1;
    cpu->IQ[iss_idx].literal = -1;
    cpu->IQ[iss_idx].src1_valid = -1;
    cpu->IQ[iss_idx].src1_tag = -1;
    cpu->IQ[iss_idx].src1_value = -1;
    cpu->IQ[iss_idx].src2_valid = -1;
    cpu->IQ[iss_idx].src2_tag = -1;
    cpu->IQ[iss_idx].src2_value = -1;
    cpu->IQ[iss_idx].dest = -1;
    cpu->IQ[iss_idx].pc = -1;
    cpu->IQ[iss_idx].issued=-1;
    cpu->IQ[iss_idx].src1_arc=-1;
    cpu->IQ[iss_idx].src2_arc=-1;
    strcpy(cpu->IQ[iss_idx].opcode_str_iq, "empty");
    cpu->IQ[iss_idx].result_buffer=-1;
    increment_iq_head(cpu);

}
int entry_issue_queue(APEX_CPU *cpu, int allo_bit, int opcode, int literal, int src1_valid, int src1_tag, int src1_value, int src2_valid, int src2_tag, int src2_value, int dest, int pc, int sr1_ar, int sr2_ar, char strg[128]){
        int i = cpu->iq_tail;
        if (cpu->IQ[i].allocate_bit == -1) {
            cpu->IQ[i].allocate_bit = allo_bit;
            cpu->IQ[i].opcode = opcode;
            cpu->IQ[i].fu = opcode_to_fu(opcode);
            cpu->IQ[i].literal = literal;
            cpu->IQ[i].src1_valid = src1_valid;
            cpu->IQ[i].src1_tag = src1_tag;
            cpu->IQ[i].src1_value = src1_value;
            cpu->IQ[i].src2_valid = src2_valid;
            cpu->IQ[i].src2_tag = src2_tag;
            cpu->IQ[i].src2_value = src2_value;
            cpu->IQ[i].dest = dest;
            cpu->IQ[i].pc = pc;
            cpu->IQ[i].src1_arc= sr1_ar;
            cpu->IQ[i].src2_arc=sr2_ar;
            strcpy(cpu->IQ[i].opcode_str_iq, strg);
            cpu->IQ[i].result_buffer=-1;
            increment_iq_tail(cpu);
            return 1;
        }
    return -1;
}

int search_issue_queue(APEX_CPU *cpu, int pregister, int pc_iq)
{
    for(int i=0; i<24; i++)
    {
        if((cpu->IQ[i].src1_tag == pregister && cpu->IQ[i].src1_valid==1 && cpu->IQ[i].pc==pc_iq) || (cpu->IQ[i].src2_tag == pregister && cpu->IQ[i].src2_valid ==1&& cpu->IQ[i].pc==pc_iq))
        {
            return i;
        }
    }
    return -2;
}

int search_issue_queue_pc(APEX_CPU *cpu, int pc)
{
    for(int i=0; i<24; i++)
    {
        if(cpu->IQ[i].pc == pc) 
        {
            return i;
        }
    }
    return -2;
}



//the below function is for situation when we have one register already available and the second one is also found in prf
int update_issue_queue(APEX_CPU *cpu, int prf_index2, int prf_index1, int issue_index){

    for (int i=0; i<cpu->iq_tail; i++)
    {
        if(cpu->IQ[i].src1_tag== prf_index1 && cpu->IQ[i].src1_valid==1 && cpu->PR[prf_index1].valid==1)
        {
            cpu->IQ[i].allocate_bit=1;
            cpu->IQ[i].src2_valid=1;
            cpu->IQ[i].src2_tag = prf_index2;
            cpu->IQ[i].src2_value = cpu->PR[prf_index2].value;
            return 1;
        }
        else if(cpu->IQ[i].src2_tag== prf_index1 && cpu->IQ[i].src2_valid==1 && cpu->PR[prf_index1].valid==1)
        {
            cpu->IQ[i].allocate_bit=1;
            cpu->IQ[i].src1_valid=1;
            cpu->IQ[i].src1_tag = prf_index2;
            cpu->IQ[i].src1_value = cpu->PR[prf_index2].value;
            return 1;
        }
    }
    return -1;
}

void update_iq(APEX_CPU *cpu,  int valid, int tag, int value)
{
    for (int i=0; i<cpu->iq_tail; i++)
    {
        if(cpu->IQ[i].src1_tag!= tag)
        {
            cpu->IQ[i].src2_valid=valid;
            cpu->IQ[i].src2_tag = tag;
            cpu->IQ[i].src2_value = value;
        }
        else{

            cpu->IQ[i].src1_valid=valid;
            cpu->IQ[i].src1_tag = tag;
            cpu->IQ[i].src1_value = value;
        }
    }
}

void update_allocate_iq(APEX_CPU *cpu, int pr1, int pr2, int iq_indx)
{
    if(((cpu->IQ[iq_indx].src1_arc== cpu->PR[pr1].arc_reg && cpu->PR[pr1].valid == 1) || 
    (cpu->IQ[iq_indx].src1_arc== cpu->PR[pr2].arc_reg && cpu->PR[pr1].valid == 1 )) && 
    ((cpu->IQ[iq_indx].src2_arc== cpu->PR[pr1].arc_reg && cpu->PR[pr1].valid == 1) || 
    (cpu->IQ[iq_indx].src2_arc== cpu->PR[pr2].arc_reg && cpu->PR[pr1].valid == 1 )))
    {
        cpu->IQ[iq_indx].allocate_bit=1;
    }
}
void update_issue(APEX_CPU *cpu, int indx)
{
    cpu->IQ[indx].issued=1;
}


// ------- ROB ----------------------------------------------------------------------

void increment_rob_tail(APEX_CPU *cpu){
    cpu->rob_tail = cpu->rob_tail + 1;
}

void increment_rob_head(APEX_CPU *cpu){
    cpu->rob_head = cpu->rob_head + 1;
}

void free_rob(APEX_CPU *cpu, int head)
{
    cpu->rob[head].established_entry=-1;
    cpu->rob[head].opcode=-1;
    cpu->rob[head].pc=-1;
    cpu->rob[head].rd_physical=-1;
    cpu->rob[head].rename_entry=-1;
    cpu->rob[head].rd_arc=-1;
    cpu->rob[head].lsq_index=-1;
    cpu->rob[head].status= -1;
    strcpy(cpu->rob[head].opstrg,"empty");
    increment_rob_head(cpu);
}
int entry_rob(APEX_CPU *cpu, int opcode, int pc, int rd_physical, int rename_entry, int rd_arc, int lsq_index, char  op_string[128]){
    int i = cpu->rob_tail;
    if (cpu->rob[i].established_entry == 0) {
        cpu->rob[i].established_entry = 1;
        cpu->rob[i].opcode = opcode;
        cpu->rob[i].pc = pc;
        cpu->rob[i].rd_physical = rd_physical;
        cpu->rob[i].rename_entry = rename_entry;
        cpu->rob[i].rd_arc = rd_arc;
        cpu->rob[i].lsq_index = lsq_index;
        cpu->rob[i].status= 0;
        strcpy(cpu->rob[i].opstrg, op_string);
        increment_rob_tail(cpu);
        return 1;
    }
    
    return -1;
}
int search_rob(APEX_CPU *cpu, int arc_rd)
{
    for (int i=0; i<32;i++)
    {
        if(cpu->rob[i].rd_arc == arc_rd)
        {
            return i;
        }
    }
    return -2;
}

int search_rob_pc(APEX_CPU *cpu, int pc_rob)
{
    for (int i=0; i<32;i++)
    {
        if(cpu->rob[i].pc == pc_rob)
        {
            return i;
        }
    }
    return -2;
}

int get_rob_entry_status(APEX_CPU *cpu)
{
    if(cpu->rob_head<cpu->rob_tail){
        return cpu->rob[cpu->rob_head].status;
    }
   return -1;
}
// ----------------------- broadcast functions ------------------------------------------

int search_broadcasting_bus(APEX_CPU *cpu, int src_reg)
{
    for (int i=0; i<25; i++)
    {
        if(cpu->broadcasting[i].arc_reg==src_reg)
        {
            return i;
        }
    }
    return -2;
}

void free_broadcast_bus(APEX_CPU *cpu, int indx)
{
    cpu->broadcasting[indx].arc_reg =-1;
    cpu->broadcasting[indx].tag=-1;
    cpu->broadcasting[indx].value=-1;
}