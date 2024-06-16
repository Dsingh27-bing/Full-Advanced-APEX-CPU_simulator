/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
//final dimple 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"
#include "info_print.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

typedef struct {
    int prediction_state;
    int history_state;
} PredictionResult;

void predict_and_update_btb(BTB *btb_entry, int outcome, int type, PredictionResult *result) {
    
    int prediction_state = btb_entry->prediction_state;
    int history_prediction = btb_entry->history_state;
    /* Update prediction state based on the outcome and type */
    if (outcome == TAKEN && type == 0) //bnz bp
    {
        if (history_prediction == 2)
            history_prediction = 3;
        else if (history_prediction == 1)
            history_prediction = 2;
        else if (history_prediction == 3)
            history_prediction = 3;
        else if (history_prediction == 0)
            history_prediction = 1;
    } 
    else if (outcome == NOT_TAKEN && type == 0) 
    {
        if (history_prediction == 1)
            history_prediction = 0;
        else if (history_prediction == 3)
            history_prediction = 2;
        else if (history_prediction == 0)
            history_prediction = 0;
        else if (history_prediction == 2)
            history_prediction = 1;
    } 
    else if (outcome == TAKEN && type == 1) // bz bnp
    {
        if (history_prediction == 2)
            history_prediction = 3;
        else if (history_prediction == 1)
            history_prediction = 2;
        else if (history_prediction == 3)
            history_prediction = 3;
        else if (history_prediction == 0)
            history_prediction = 1;
        
    } 
    else if (outcome == NOT_TAKEN && type == 1) 
    {
        if (history_prediction == 1)
            history_prediction = 0;
        else if (history_prediction == 3)
            history_prediction = 2;
        else if (history_prediction == 0)
            history_prediction = 0;
        else if (history_prediction == 2)
            history_prediction = 1;
    }

    if((history_prediction == 0 || history_prediction == 1 ) && type == 1) //bz bnp
    {
        prediction_state = 0;
    }
    else if((history_prediction == 3 || history_prediction == 2) && type == 1)
    {
        prediction_state = 1;
    }
    else if((history_prediction == 3  || history_prediction == 2) && type == 0) //bnz bp
    {
        prediction_state = 1;
    }
    else if((history_prediction == 0 || history_prediction == 1) && type == 0)
    {
        prediction_state = 0;
    }

    /* Update the BTB entry with the new prediction state */
    btb_entry->prediction_state = prediction_state;
    btb_entry->history_state = history_prediction;

    result->prediction_state = prediction_state;
    result->history_state = history_prediction;

}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_JALR:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }


        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_LOADP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        case OPCODE_STOREP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BN:
        case OPCODE_BNN:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }


        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }
        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }
        case OPCODE_CML:
        case OPCODE_JUMP:
        {
            printf("%s,R%d,#%d", stage->opcode_str,stage->rs1,stage->imm);
            break;
        }
        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d", stage->opcode_str,stage->rs1,stage->rs2);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{

    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n \n \n");
    printf("----------\n%s\n----------\n", "FLAGS (+,-,0):");
    printf("\n Zero flag = %d", cpu->cc.z);
    printf("\n Positive flag = %d", cpu->cc.p);
    printf("\n Negative flag = %d", cpu->cc.n);
    printf("\n");
    printf("\n \n \n");
    printf("----------\n%s\n----------\n", "MEMORY:");
    for (int i =0; i<cpu->data_counter; ++i){
        printf("\nMemory[%d]= %d\n", cpu->mem_address[i],cpu->data_memory[cpu->mem_address[i]]);
    }
    printf("\n\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */

static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;
    if (cpu->fetch.has_insn)
    {    
        cpu->fetch.btb_searched = 0;
        
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE && cpu->fetch.btb_searched ==0)
        {
            
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;
        if(cpu->fetch.stalled == 0 )
        {  
        
          /* Index into code memory using this pc and copy all instruction fields
           * into fetch latch  */
          current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
          strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
          cpu->fetch.opcode = current_ins->opcode;
          cpu->fetch.rd = current_ins->rd;
          cpu->fetch.rs1 = current_ins->rs1;
          cpu->fetch.rs2 = current_ins->rs2;
          cpu->fetch.imm = current_ins->imm;
          for (int i = 0; i < cpu->btb_queue.size; i++) 
            {   
                int index = (cpu->btb_queue.head + i) % cpu->btb_queue.size;
                if (cpu->pc == cpu->btb_queue.data[index].inst_address && cpu->btb_queue.data[index].num_executed > 0 && cpu->fetch.type_of_branch== 0 && cpu->btb_queue.data[index].prediction_state == 1 && cpu->btb_queue.data[index].target_address != 0) 
                {   
                    
                    
                    cpu->fetch.btb_searched = 1;
                    cpu->pc = cpu->btb_queue.data[index].target_address;
                    cpu->decode = cpu->fetch; //sending branch to decode so that fetch is updated to target address

                    if(cpu->fetch.stalled == 0)
                    {

                        /* Stop fetching new instructions if HALT is fetched */
                        if (cpu->fetch.opcode == OPCODE_HALT)
                        {
                            cpu->fetch.has_insn = FALSE;
                        }
                    }
                    if (ENABLE_DEBUG_MESSAGES)
                    {
                        print_stage_content("Fetch", &cpu->fetch);
                    }

                    return;
                
                }
                else if (cpu->pc == cpu->btb_queue.data[index].inst_address && cpu->btb_queue.data[index].num_executed > 0 && cpu->fetch.type_of_branch== 1 && cpu->btb_queue.data[index].prediction_state == 1 && cpu->btb_queue.data[index].target_address != 0) 
                {   
                    
                    cpu->fetch.btb_searched = 1;
                    cpu->pc = cpu->btb_queue.data[index].target_address;
                    cpu->decode = cpu->fetch; //sending branch to decode so that fetch is updated to target address
                    
                    if(cpu->fetch.stalled == 0)
                    {

                        /* Stop fetching new instructions if HALT is fetched */
                        if (cpu->fetch.opcode == OPCODE_HALT)
                        {
                            cpu->fetch.has_insn = FALSE;
                        }
                    }
                    if (ENABLE_DEBUG_MESSAGES)
                    {
                        print_stage_content("Fetch", &cpu->fetch);
                    }

                    return;
                
                }

                
            }
        }
            
        if(cpu->fetch.stalled == 0 && cpu->fetch.btb_searched==0)
        {
            /* Update PC for next instruction */
            cpu->pc += 4;

            /* Copy data from fetch latch to decode latch*/

              cpu->decode = cpu->fetch;
        }

        if(cpu->fetch.stalled == 0)
        {

            /* Stop fetching new instructions if HALT is fetched */
            if (cpu->fetch.opcode == OPCODE_HALT)
            {
                cpu->fetch.has_insn = FALSE;
            }
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
        print_stage_content("Fetch", &cpu->fetch);
        }
    }
    
    
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu) // we will only create pd here for rename table .    we will create entries in issue, ROB, LSQ from here.
{

    if (cpu->decode.has_insn && cpu->decode.stalled==0)
    {


        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_MOVC:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                
                int i = get_free_prf(cpu);
                int entry = set_free_prf(cpu, i, cpu->decode.rd, -1,cpu->decode.pc);
                
                if(entry ==1)
                {     
                    cpu->decode.stalled=0;
                    cpu->decode.entry_made_pr =1;
                    break;
                }
            
            }
            
            case OPCODE_JALR:
            case OPCODE_LOAD:
            {
              

                if (cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->ex_fb.value;
                    cpu->decode.rs1_f = 1;
                  }

                if (cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->mem_fb.value;
                    cpu->decode.rs1_f = 1;
                  }
                  if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.rs1_f = 1;
                    }
                if(cpu->decode.rs1_f){
                    cpu->regs_writing[cpu->decode.rd] = 1;
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }}
            case OPCODE_LOADP:
            {

                if (cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->ex_fb.value;
                    cpu->decode.rs1_f = 1;
                    }

                if (cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0){
                    cpu->decode.rs1_value = cpu->mem_fb.value;
                    cpu->decode.rs1_f = 1;
                    }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs1_f = 1;
                  }
                if(cpu->decode.rs1_f){
                    cpu->regs_writing[cpu->decode.rs1] = 1;
                    cpu->regs_writing[cpu->decode.rd] = 1;
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

                }

            case OPCODE_CML:
            case OPCODE_JUMP:
                {

                if (cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->ex_fb.value;
                    cpu->decode.rs1_f = 1;}
                if (cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->mem_fb.value;
                    cpu->decode.rs1_f = 1;}
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0 ){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs1_f = 1;
                  }
                if(cpu->decode.rs1_f){
                    cpu->regs_writing[cpu->decode.rs1]=1;
                    cpu->decode.stalled = 0;
                    break;
                }

                else{
                  cpu->decode.stalled = 1;
                  break;
                }

                }

            case OPCODE_STORE:
            {
                if( cpu->decode.rs1 == cpu->ex_fb.reg && cpu->decode.rs1_f ==0)
                {
                    cpu->decode.rs1_value=cpu->ex_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->ex_fb.reg && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value=cpu->ex_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if(cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0)
                {
                    cpu->decode.rs1_value=cpu->mem_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if( cpu->decode.rs2== cpu->mem_fb.reg && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value=cpu->mem_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs1_f = 1;
                }
                if(cpu->regs_writing[cpu->decode.rs2] ==0 && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.rs2_f = 1;
                }
                if (cpu->decode.rs1_f==1 && cpu->decode.rs2_f==1)
                {
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }
            }
            case OPCODE_STOREP:
            {
                if(cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 )
                {
                    cpu->decode.rs1_value=cpu->ex_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->ex_fb.reg && cpu->decode.rs2_f ==0  )
                {
                    cpu->decode.rs2_value=cpu->ex_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if(cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0  )
                {
                    cpu->decode.rs1_value=cpu->mem_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->mem_fb.reg && cpu->decode.rs2_f ==0  )
                {
                    cpu->decode.rs2_value=cpu->mem_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0 ){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs1_f = 1;
                }
                if(cpu->regs_writing[cpu->decode.rs2] ==0 && cpu->decode.rs2_f ==0 )  // loadp ne pkda hai to ye kaese hua??
                {
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.rs2_f = 1;
                }

                if (cpu->decode.rs1_f==1 && cpu->decode.rs2_f==1)
                {
                    
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }
            }

            case OPCODE_CMP:
            {


                if(cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 )
                {
                    cpu->decode.rs1_value=cpu->ex_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->ex_fb.reg && cpu->decode.rs2_f ==0 )
                {
                    cpu->decode.rs2_value=cpu->ex_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if(cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0 )
                {
                    cpu->decode.rs1_value=cpu->mem_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->mem_fb.reg && cpu->decode.rs2_f ==0 )
                {
                    cpu->decode.rs2_value=cpu->mem_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs1_f = 1;
                }
                if(cpu->regs_writing[cpu->decode.rs2] ==0 && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.rs2_f = 1;
                }

                if (cpu->decode.rs1_f==1 && cpu->decode.rs2_f==1)
                {
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }
            }

            case OPCODE_BNZ:
            case OPCODE_BP:
            {
                
                if (cpu->btb_queue.size < 4 && cpu->decode.btb_searched == 0) 
                {   
                    
                    // The queue is not full, add a new entry
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }

                    if (!inst_address_exists)
                    {
                        // Add a new entry
                        cpu->btb_queue.data[cpu->btb_queue.tail].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[cpu->btb_queue.tail].prediction_state =1;
                        cpu->btb_queue.data[cpu->btb_queue.tail].history_state =3;
                        cpu->btb_queue.data[cpu->btb_queue.tail].executed =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].num_executed=0;
                        cpu->btb_queue.size++;
                        cpu->btb_queue.tail = (cpu->btb_queue.tail + 1) % 4; // Update tail
                    }
                    
                    
                    break;  
                } 
                else if (cpu->btb_queue.size >= 4 && cpu->decode.btb_searched == 0)
                {
                    
                    // The queue is full, replace the oldest entry (FIFO)
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }
                    if (!inst_address_exists)
                    {
                        // Replace the oldest entry
                        int replaced_index = cpu->btb_queue.head;
                        cpu->btb_queue.data[replaced_index].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[replaced_index].target_address = 0; // Reset target_address
                        cpu->btb_queue.data[replaced_index].prediction_state = 1; // Reset prediction_state
                        cpu->btb_queue.data[replaced_index].executed = 0;        // Reset executed
                        cpu->btb_queue.data[replaced_index].history_state = 3;
                        cpu->btb_queue.data[replaced_index].num_executed =0;

                        // Update head
                        cpu->btb_queue.head = (cpu->btb_queue.head + 1) % 4;
                    }
                            
                    break;
                    
                }
             
            }
            case OPCODE_BNP:
            case OPCODE_BZ:
            {
                
                 
                if (cpu->btb_queue.size < 4 && cpu->decode.btb_searched == 0) 
                {   
                    
                    // The queue is not full, add a new entry
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }

                    if (!inst_address_exists)
                    {
                        // Add a new entry
                        cpu->btb_queue.data[cpu->btb_queue.tail].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[cpu->btb_queue.tail].prediction_state =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].history_state =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].executed =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].num_executed=0;
                        cpu->btb_queue.size++;
                        cpu->btb_queue.tail = (cpu->btb_queue.tail + 1) % 4; // Update tail
                    }
                    
                    break;  
                } 
                else if (cpu->btb_queue.size >= 4 && cpu->decode.btb_searched == 0)
                {
                    
                   
                    // The queue is full, replace the oldest entry (FIFO)
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }
                    if (!inst_address_exists)
                    {
                        // Replace the oldest entry
                        int replaced_index = cpu->btb_queue.head;
                        cpu->btb_queue.data[replaced_index].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[replaced_index].target_address = 0; // Reset target_address
                        cpu->btb_queue.data[replaced_index].prediction_state = 0; // Reset prediction_state
                        cpu->btb_queue.data[replaced_index].executed = 0;        // Reset executed
                        cpu->btb_queue.data[replaced_index].history_state = 0;
                        cpu->btb_queue.data[replaced_index].num_executed =0;

                        // Update head
                        cpu->btb_queue.head = (cpu->btb_queue.head + 1) % 4;
                    }
      
                    break;
                    
                }

            }
            case OPCODE_HALT:
            {
                cpu->decode.stalled=0; //HALT should move as is
                cpu->decode.entry_made_pr=1;
                break;
            }

        }

        /* Copy data from decode latch to execute latch*/

        if (cpu->decode.stalled == 0 && cpu->decode.entry_made_pr == 1)
        {
        
            cpu->dispatch = cpu->decode;
            cpu->fetch.stalled = 0;
            
        }
        else{
          cpu->fetch.stalled = 1;
          cpu->decode.stalled =1;
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
            // print_prf(cpu);

        }
    }
    
    

}


static void
APEX_dispatch(APEX_CPU *cpu)
{
    
    if (cpu->dispatch.has_insn && cpu->dispatch.stalled==0)
    {
        switch (cpu->dispatch.opcode)
        {
            
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
                
                if(cpu->dispatch.rs1_f==0) //rs1 found in prf and issue queue 
                {
                    int rs1_prf_index;
                    rs1_prf_index= search_prf(cpu, cpu->dispatch.rs1,cpu->dispatch.pc);
                    int rs1_issue_queue_idx = search_issue_queue_pc(cpu, cpu->dispatch.pc);
                    if(rs1_prf_index != -2 && rs1_issue_queue_idx==-2)
                    {
                        cpu->dispatch.entry_made_iq=entry_issue_queue(cpu, 0, cpu->dispatch.opcode, cpu->dispatch.imm,  1, rs1_prf_index, cpu->PR[rs1_prf_index].value, 0, 0, 0, cpu->dispatch.rd, cpu->dispatch.pc, cpu->dispatch.rs1, cpu->dispatch.rs2, cpu->dispatch.opcode_str ) ;
                        cpu->dispatch.rs1_f=1;
                    }
                    else if(rs1_prf_index!=-2 && rs1_issue_queue_idx !=-2)
                    {
                        cpu->dispatch.rs1_f=1;
                    } 
                    
                }
                if(cpu->dispatch.rs1_f==0 && cpu->dispatch.rs2_f==0) //rs2 found in prf and issue queue
                {
                   
                    int rs2_prf_index= search_prf(cpu, cpu->dispatch.rs2, cpu->dispatch.pc);
                    int rs2_issue_queue_idx = search_issue_queue_pc(cpu, cpu->dispatch.pc);
                    if(rs2_prf_index != -2 && rs2_issue_queue_idx ==-2)
                    {
                        cpu->dispatch.entry_made_iq=entry_issue_queue(cpu,0, cpu->dispatch.opcode, cpu->dispatch.imm,  0, 0, 0, 1, rs2_prf_index, cpu->PR[rs2_prf_index].value, cpu->dispatch.rd, cpu->dispatch.pc, cpu->dispatch.rs1, cpu->dispatch.rs2, cpu->dispatch.opcode_str ) ;
                        cpu->dispatch.rs2_f=1;
                    }
                    else if(rs2_prf_index!=-2 && rs2_issue_queue_idx !=-2)
                    {
                         
                        cpu->dispatch.rs2_f=1;
                    }
                    
                }
                if(cpu->dispatch.rs1_f==1 && cpu->dispatch.rs2_f==0) //rs1 exist in issue queue we update rs2 
                {
                    int rs2_prf_index= search_prf(cpu, cpu->dispatch.rs2, cpu->dispatch.pc);
                    int rs1_prf_index= search_prf(cpu, cpu->dispatch.rs1, cpu->dispatch.pc);
                    int rs1_found_issue_queue = search_issue_queue(cpu, rs1_prf_index, cpu->dispatch.pc);
                    //int rs2_found_issue_queu = search_issue_queue(cpu, rs2_prf_index,cpu->dispatch.pc);
                    if(rs2_prf_index != -2 && rs1_found_issue_queue !=-2 )
                    {
                        //cpu->dispatch.entry_made_iq=entry_issue_queue(cpu, cpu->dispatch.opcode, cpu->dispatch.imm,  0, 0, 0, 1, rs2_prf_index, cpu->PR[rs2_prf_index].value, cpu->dispatch.rd, cpu->dispatch.pc ) ;
                        cpu->dispatch.entry_made_iq=update_issue_queue(cpu, rs2_prf_index, rs1_prf_index, rs1_found_issue_queue);
                        cpu->dispatch.rs2_f=1;
                    }
                    
                }
                if(cpu->dispatch.rs2_f==1 && cpu->dispatch.rs1_f==0) //rs2 exist in issue queue we update rs1
                {
                    int rs2_prf_index= search_prf(cpu, cpu->dispatch.rs2, cpu->dispatch.pc);
                    int rs1_prf_index= search_prf(cpu, cpu->dispatch.rs1, cpu->dispatch.pc);
                    //int rs1_found_issue_queue = search_issue_queue(cpu, rs1_prf_index,cpu->dispatch.pc);
                    int rs2_found_issue_queue = search_issue_queue(cpu, rs2_prf_index, cpu->dispatch.pc);
                    if(rs1_prf_index != -2 && rs2_found_issue_queue !=-2 )
                    {
                        //cpu->dispatch.entry_made_iq=entry_issue_queue(cpu, cpu->dispatch.opcode, cpu->dispatch.imm,  0, 0, 0, 1, rs2_prf_index, cpu->PR[rs2_prf_index].value, cpu->dispatch.rd, cpu->dispatch.pc ) ;
                        cpu->dispatch.entry_made_iq=update_issue_queue(cpu, rs1_prf_index, rs2_prf_index, rs2_found_issue_queue);
                        cpu->dispatch.rs1_f=1;
                    }
                    
                }
                
                
                int rd_prf_index = search_prf_broadcast(cpu, cpu->dispatch.rd, cpu->dispatch.pc); //current physical reg
                int prd_prev = search_prf_prev_rd(cpu, cpu->dispatch.rd, cpu->dispatch.pc);


                cpu->dispatch.entry_made_rob = entry_rob(cpu, cpu->dispatch.opcode, cpu->dispatch.pc, rd_prf_index ,prd_prev, cpu->dispatch.rd, 0, cpu->dispatch.opcode_str );

                if(cpu->dispatch.entry_made_rob==1)
                {
                    cpu->regs_writing[cpu->dispatch.rd]=1;
                    break;
                }
                else{
                    cpu->dispatch.stalled=1;
                    break;
                }
                
            }
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_JALR:
            case OPCODE_LOAD:
            {
            

                if (cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->ex_fb.value;
                    cpu->decode.rs1_f = 1;
                }

                if (cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->mem_fb.value;
                    cpu->decode.rs1_f = 1;
                }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.rs1_f = 1;
                    }
                if(cpu->decode.rs1_f){
                    cpu->regs_writing[cpu->decode.rd] = 1;
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                cpu->decode.stalled = 1;
                break;
                }}
            case OPCODE_LOADP:
            {

                if (cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->ex_fb.value;
                    cpu->decode.rs1_f = 1;
                    }

                if (cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0){
                    cpu->decode.rs1_value = cpu->mem_fb.value;
                    cpu->decode.rs1_f = 1;
                    }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs1_f = 1;
                }
                if(cpu->decode.rs1_f){
                    cpu->regs_writing[cpu->decode.rs1] = 1;
                    cpu->regs_writing[cpu->decode.rd] = 1;
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                cpu->decode.stalled = 1;
                break;
                }

                }

            case OPCODE_CML:
            case OPCODE_JUMP:
                {

                if (cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->ex_fb.value;
                    cpu->decode.rs1_f = 1;}
                if (cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0 ){
                    cpu->decode.rs1_value = cpu->mem_fb.value;
                    cpu->decode.rs1_f = 1;}
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0 ){
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs1_f = 1;
                }
                if(cpu->decode.rs1_f){
                    cpu->regs_writing[cpu->decode.rs1]=1;
                    cpu->decode.stalled = 0;
                    break;
                }

                else{
                cpu->decode.stalled = 1;
                break;
                }

                }

            case OPCODE_STORE:
            {
                if( cpu->decode.rs1 == cpu->ex_fb.reg && cpu->decode.rs1_f ==0)
                {
                    cpu->decode.rs1_value=cpu->ex_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->ex_fb.reg && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value=cpu->ex_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if(cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0)
                {
                    cpu->decode.rs1_value=cpu->mem_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if( cpu->decode.rs2== cpu->mem_fb.reg && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value=cpu->mem_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs1_f = 1;
                }
                if(cpu->regs_writing[cpu->decode.rs2] ==0 && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.rs2_f = 1;
                }
                if (cpu->decode.rs1_f==1 && cpu->decode.rs2_f==1)
                {
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                cpu->decode.stalled = 1;
                break;
                }
            }
            case OPCODE_STOREP:
            {
                if(cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 )
                {
                    cpu->decode.rs1_value=cpu->ex_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->ex_fb.reg && cpu->decode.rs2_f ==0  )
                {
                    cpu->decode.rs2_value=cpu->ex_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if(cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0  )
                {
                    cpu->decode.rs1_value=cpu->mem_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->mem_fb.reg && cpu->decode.rs2_f ==0  )
                {
                    cpu->decode.rs2_value=cpu->mem_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0 ){
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs1_f = 1;
                }
                if(cpu->regs_writing[cpu->decode.rs2] ==0 && cpu->decode.rs2_f ==0 )  // loadp ne pkda hai to ye kaese hua??
                {
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.rs2_f = 1;
                }

                if (cpu->decode.rs1_f==1 && cpu->decode.rs2_f==1)
                {
                    
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                cpu->decode.stalled = 1;
                break;
                }
            }

            case OPCODE_CMP:
            {


                if(cpu->decode.rs1== cpu->ex_fb.reg && cpu->decode.rs1_f ==0 )
                {
                    cpu->decode.rs1_value=cpu->ex_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->ex_fb.reg && cpu->decode.rs2_f ==0 )
                {
                    cpu->decode.rs2_value=cpu->ex_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if(cpu->decode.rs1== cpu->mem_fb.reg && cpu->decode.rs1_f ==0 )
                {
                    cpu->decode.rs1_value=cpu->mem_fb.value;
                    cpu->decode.rs1_f =1;
                }
                if(cpu->decode.rs2== cpu->mem_fb.reg && cpu->decode.rs2_f ==0 )
                {
                    cpu->decode.rs2_value=cpu->mem_fb.value;
                    cpu->decode.rs2_f =1;
                }
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->decode.rs1_f ==0){
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs1_f = 1;
                }
                if(cpu->regs_writing[cpu->decode.rs2] ==0 && cpu->decode.rs2_f ==0)
                {
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->decode.rs2_f = 1;
                }

                if (cpu->decode.rs1_f==1 && cpu->decode.rs2_f==1)
                {
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                cpu->decode.stalled = 1;
                break;
                }
            }

            case OPCODE_MOVC:
            {
                
                cpu->dispatch.entry_made_iq=entry_issue_queue(cpu, 1,cpu->dispatch.opcode, cpu->dispatch.imm,  0, 0, 0, 0, 0, 0, cpu->dispatch.rd, cpu->dispatch.pc, cpu->dispatch.rs1, cpu->dispatch.rs2 , cpu->dispatch.opcode_str) ;
                int rd_prf_index = search_prf_broadcast(cpu, cpu->dispatch.rd, cpu->dispatch.pc);
                int prd_prev = search_prf_prev_rd(cpu, cpu->dispatch.rd, cpu->dispatch.pc);
                cpu->dispatch.entry_made_rob = entry_rob(cpu, cpu->dispatch.opcode, cpu->dispatch.pc, rd_prf_index ,prd_prev, cpu->dispatch.rd, 0 , cpu->dispatch.opcode_str); 
                if(cpu->dispatch.entry_made_iq==1 && cpu->dispatch.entry_made_rob ==1)
                {
                    /* MOVC doesn't have register operands */
                    cpu->regs_writing[cpu->dispatch.rd] = 1;
                    break;
                }
                else
                {
                    cpu->dispatch.stalled=1;
                    break;
                }
                
            }
            case OPCODE_BNZ:
            case OPCODE_BP:
            {
                
                if (cpu->btb_queue.size < 4 && cpu->decode.btb_searched == 0) 
                {   
                    
                    // The queue is not full, add a new entry
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }

                    if (!inst_address_exists)
                    {
                        // Add a new entry
                        cpu->btb_queue.data[cpu->btb_queue.tail].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[cpu->btb_queue.tail].prediction_state =1;
                        cpu->btb_queue.data[cpu->btb_queue.tail].history_state =3;
                        cpu->btb_queue.data[cpu->btb_queue.tail].executed =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].num_executed=0;
                        cpu->btb_queue.size++;
                        cpu->btb_queue.tail = (cpu->btb_queue.tail + 1) % 4; // Update tail
                    }
                    
                    
                    break;  
                } 
                else if (cpu->btb_queue.size >= 4 && cpu->decode.btb_searched == 0)
                {
                    
                    // The queue is full, replace the oldest entry (FIFO)
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }
                    if (!inst_address_exists)
                    {
                        // Replace the oldest entry
                        int replaced_index = cpu->btb_queue.head;
                        cpu->btb_queue.data[replaced_index].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[replaced_index].target_address = 0; // Reset target_address
                        cpu->btb_queue.data[replaced_index].prediction_state = 1; // Reset prediction_state
                        cpu->btb_queue.data[replaced_index].executed = 0;        // Reset executed
                        cpu->btb_queue.data[replaced_index].history_state = 3;
                        cpu->btb_queue.data[replaced_index].num_executed =0;

                        // Update head
                        cpu->btb_queue.head = (cpu->btb_queue.head + 1) % 4;
                    }
                            
                    break;
                    
                }
            
            }
            case OPCODE_BNP:
            case OPCODE_BZ:
            {
                
                
                if (cpu->btb_queue.size < 4 && cpu->decode.btb_searched == 0) 
                {   
                    
                    // The queue is not full, add a new entry
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }

                    if (!inst_address_exists)
                    {
                        // Add a new entry
                        cpu->btb_queue.data[cpu->btb_queue.tail].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[cpu->btb_queue.tail].prediction_state =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].history_state =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].executed =0;
                        cpu->btb_queue.data[cpu->btb_queue.tail].num_executed=0;
                        cpu->btb_queue.size++;
                        cpu->btb_queue.tail = (cpu->btb_queue.tail + 1) % 4; // Update tail
                    }
                    
                    break;  
                } 
                else if (cpu->btb_queue.size >= 4 && cpu->decode.btb_searched == 0)
                {
                    
                
                    // The queue is full, replace the oldest entry (FIFO)
                    int inst_address_exists = 0;
                    for (int i = 0; i < cpu->btb_queue.size; i++)
                    {
                        if (cpu->decode.pc == cpu->btb_queue.data[i].inst_address)
                        {
                            inst_address_exists = 1;
                            break;
                        }
                    }
                    if (!inst_address_exists)
                    {
                        // Replace the oldest entry
                        int replaced_index = cpu->btb_queue.head;
                        cpu->btb_queue.data[replaced_index].inst_address = cpu->decode.pc;
                        cpu->btb_queue.data[replaced_index].target_address = 0; // Reset target_address
                        cpu->btb_queue.data[replaced_index].prediction_state = 0; // Reset prediction_state
                        cpu->btb_queue.data[replaced_index].executed = 0;        // Reset executed
                        cpu->btb_queue.data[replaced_index].history_state = 0;
                        cpu->btb_queue.data[replaced_index].num_executed =0;

                        // Update head
                        cpu->btb_queue.head = (cpu->btb_queue.head + 1) % 4;
                    }
    
                    break;
                    
                }

            }
            case OPCODE_HALT:
            {
                cpu->dispatch.entry_made_iq=entry_issue_queue(cpu, 1, cpu->dispatch.opcode, 0,  0, 0, 0, 0, 0, 0, 0, cpu->dispatch.pc, cpu->dispatch.rs1, cpu->dispatch.rs2, cpu->dispatch.opcode_str );//HALT should move as is
                cpu->dispatch.entry_made_rob=entry_rob(cpu, cpu->dispatch.opcode, cpu->dispatch.pc, -1,-1,-1,-1, cpu->dispatch.opcode_str);
                break;
            }

        }

        /* Copy data from decode latch to execute latch*/
        if (cpu->dispatch.stalled == 0  && cpu->dispatch.entry_made_rob==1)
        {
            cpu->issue = cpu->dispatch;
            cpu->dispatch.has_insn = FALSE;
            
        }
        else
        {
        cpu->decode.stalled=1;
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Dispatch/RF", &cpu->dispatch);
            // print_prf(cpu);
            // print_issue_queue(cpu);
            // print_rob(cpu);
        }
    }
}


static void
APEX_issue(APEX_CPU *cpu)
{
    if (cpu->issue.has_insn)
    {
        int j= cpu->rob_head;
        int k= search_issue_queue_pc(cpu,cpu->issue.pc);
    while(j<= cpu->rob_tail)
    {
   
    if(k!=-2 && cpu->IQ[j].pc!=-1 && cpu->IQ[j].issued!=1 && cpu->rob[j].status==0)
    {
        cpu->issue.has_insn = TRUE;
        cpu->issue.pc= cpu->IQ[j].pc;
        cpu->issue.rs1= cpu->IQ[j].src1_arc;
        cpu->issue.rs2= cpu->IQ[j].src2_arc;
        cpu->issue.imm = cpu->IQ[j].literal;
        cpu->issue.rd=cpu->IQ[j].dest;
        cpu->issue.opcode= cpu->IQ[j].opcode;
        strcpy(cpu->issue.opcode_str, cpu->IQ[j].opcode_str_iq);
        cpu->issue.result_buffer= cpu->IQ[j].result_buffer;
        switch (cpu->issue.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
                
                int iq_index = search_issue_queue_pc(cpu, cpu->issue.pc);
                int rob_index= search_rob_pc(cpu, cpu->issue.pc);
                int rs1_prf_index= search_prf(cpu, cpu->issue.rs1, cpu->issue.pc);
                int rs2_prf_index= search_prf(cpu, cpu->issue.rs2, cpu->issue.pc);
                
                // case where both src are missing and available in prf 
                //searching in PRF values are present renaming done
                if(iq_index == -2 && rs1_prf_index !=-2 && rs2_prf_index !=-2 ) 
                {
                    entry_issue_queue(cpu, 0, cpu->issue.opcode, 0, 1, rs1_prf_index, cpu->PR[rs1_prf_index].value, 1, rs2_prf_index, cpu->PR[rs2_prf_index].value, cpu->issue.rd, cpu->issue.pc, cpu->issue.rs1, cpu->issue.rs2, cpu->issue.opcode_str);
                    
                }
                

                int iq_index_again = search_issue_queue_pc(cpu, cpu->issue.pc);
                // case where one of the src is missing  and found in  prf
                if(iq_index_again != -2 && cpu->IQ[iq_index_again].allocate_bit==0 && (cpu->IQ[iq_index_again].pc != -1 || cpu->IQ[iq_index_again].pc == cpu->issue.pc))   
                {
                    
                    if(cpu->IQ[iq_index_again].src1_tag!=rs2_prf_index && rs2_prf_index!=-2)
                    {
                    update_iq(cpu,1,rs2_prf_index, cpu->PR[rs2_prf_index].value);
                    }
                    if(cpu->IQ[iq_index_again].src2_valid!=rs1_prf_index && rs1_prf_index!=-2)
                    {
                    update_iq(cpu, 1,rs1_prf_index, cpu->PR[rs1_prf_index].value);
                    }
                }

                
                int iq_index_brd1 = search_issue_queue_pc(cpu, cpu->issue.pc);
                int broadcast1 = search_broadcasting_bus(cpu, cpu->issue.rs1);
                int broadcast2= search_broadcasting_bus(cpu, cpu->issue.rs2);
                //one of the src reg value is found in broadcast tag bus
                //getting the broadcast value for either of the two regs
                if(iq_index_brd1!= -2  && (cpu->IQ[iq_index_brd1].pc != -1 || cpu->IQ[iq_index_brd1].pc == cpu->issue.pc)) 
                {
                    
                    if(cpu->IQ[iq_index_brd1].src1_arc==cpu->broadcasting[broadcast2].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast2].tag, 0);
                        if(cpu->IQ[iq_index_brd1].src1_arc==cpu->PR[rs1_prf_index].arc_reg)
                        {
                            cpu->PR[rs1_prf_index].valid=1;
                        }
                        if(cpu->IQ[iq_index_brd1].src1_arc==cpu->PR[rs2_prf_index].arc_reg)
                        {
                            cpu->PR[rs2_prf_index].valid=1;
                        }
                        cpu->broadcasting_rs1=1;
                    }
                    if(cpu->IQ[iq_index_brd1].src2_arc==cpu->broadcasting[broadcast2].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast2].tag, 0);
                        if(cpu->IQ[iq_index_brd1].src2_arc==cpu->PR[rs1_prf_index].arc_reg)
                        {
                            cpu->PR[rs1_prf_index].valid=1;
                        }
                        if(cpu->IQ[iq_index_brd1].src2_arc==cpu->PR[rs2_prf_index].arc_reg)
                        {
                            cpu->PR[rs2_prf_index].valid=1;
                        }
                        cpu->broadcasting_rs2=1;
                    }
                    if(cpu->IQ[iq_index_brd1].src1_arc==cpu->broadcasting[broadcast1].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast1].tag, 0);
                        if(cpu->IQ[iq_index_brd1].src1_arc==cpu->PR[rs1_prf_index].arc_reg)
                        {
                            cpu->PR[rs1_prf_index].valid=1;
                        }
                        if(cpu->IQ[iq_index_brd1].src1_arc==cpu->PR[rs2_prf_index].arc_reg)
                        {
                            cpu->PR[rs2_prf_index].valid=1;
                        }
                        cpu->broadcasting_rs1=1;
                    }
                    if(cpu->IQ[iq_index_brd1].src2_arc==cpu->broadcasting[broadcast1].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast1].tag, 0);
                        if(cpu->IQ[iq_index_brd1].src2_arc==cpu->PR[rs1_prf_index].arc_reg)
                        {
                            cpu->PR[rs1_prf_index].valid=1;
                        }
                        if(cpu->IQ[iq_index_brd1].src2_arc==cpu->PR[rs2_prf_index].arc_reg)
                        {
                            cpu->PR[rs2_prf_index].valid=1;
                        }
                        cpu->broadcasting_rs2=1;
                    }
                    
                }
                int broadcst1 = search_broadcasting_bus(cpu, cpu->issue.rs1);
                int broadcst2= search_broadcasting_bus(cpu, cpu->issue.rs2);
                int iq_index_brd2 = search_issue_queue_pc(cpu, cpu->issue.pc);
                //if both regs are found in broadcast
                if(iq_index_brd2 == -2 && broadcst1!=-2 && broadcst2!=-2) //getting the broadcast value for  two regs
                {

                    entry_issue_queue(cpu, 0, cpu->issue.opcode, 0, 1, cpu->broadcasting[broadcst1].tag, cpu->broadcasting[broadcst1].value, 1, cpu->broadcasting[broadcst2].tag, cpu->broadcasting[broadcst2].value, cpu->issue.rd, cpu->issue.pc, cpu->issue.rs1, cpu->issue.rs2, cpu->issue.opcode_str);
                    
                }
                
                
                int pr1= search_prf_cpu(cpu, cpu->issue.rs1, cpu->issue.pc);
                int pr2 = search_prf_cpu(cpu, cpu->issue.rs2, cpu->issue.pc);
                update_allocate_iq(cpu,pr1, pr2, iq_index_brd2);
                int iq_index_again2 = search_issue_queue_pc(cpu, cpu->issue.pc);

                
                if(cpu->IQ[iq_index_again2].allocate_bit==1 && cpu->rob[rob_index].established_entry==1)
                {
                    cpu->issue.entry_made_iq=1;
                    cpu->issue.entry_made_rob=1;
                }
                break;

            }
            case OPCODE_MUL:
            {
                int iq_index = search_issue_queue_pc(cpu, cpu->issue.pc);
                int rob_index= search_rob_pc(cpu, cpu->issue.pc);
                int rs1_prf_index= search_prf(cpu, cpu->issue.rs1, cpu->issue.pc);
                int rs2_prf_index= search_prf(cpu, cpu->issue.rs2, cpu->issue.pc);
                
                // case where both src are missing and available in prf 
                //searching in PRF values are present renaming done
                if(iq_index == -2 && rs1_prf_index !=-2 && rs2_prf_index !=-2 ) 
                {
                    entry_issue_queue(cpu, 0, cpu->issue.opcode, 0, 1, rs1_prf_index, cpu->PR[rs1_prf_index].value, 1, rs2_prf_index, cpu->PR[rs2_prf_index].value, cpu->issue.rd, cpu->issue.pc, cpu->issue.rs1, cpu->issue.rs2, cpu->issue.opcode_str);
                    
                }
                

                int iq_index_again = search_issue_queue_pc(cpu, cpu->issue.pc);
                // case where one of the src is missing  and found in  prf
                if(iq_index_again != -2 && cpu->IQ[iq_index_again].allocate_bit==0 && (cpu->IQ[iq_index_again].pc != -1 || cpu->IQ[iq_index_again].pc == cpu->issue.pc))   
                {
                    
                    if(cpu->IQ[iq_index_again].src1_tag!=rs2_prf_index && rs2_prf_index!=-2)
                    {
                    update_iq(cpu, 1,rs2_prf_index, cpu->PR[rs2_prf_index].value);
                    }
                    if(cpu->IQ[iq_index_again].src2_valid!=rs1_prf_index && rs1_prf_index!=-2)
                    {
                    update_iq(cpu, 1,rs1_prf_index, cpu->PR[rs1_prf_index].value);
                    }
                }

                
                int iq_index_brd1 = search_issue_queue_pc(cpu, cpu->issue.pc);
                int broadcast1 = search_broadcasting_bus(cpu, cpu->issue.rs1);
                int broadcast2= search_broadcasting_bus(cpu, cpu->issue.rs2);
                //one of the src reg value is found in broadcast tag bus
                //getting the broadcast value for either of the two regs
                if(iq_index_brd1!= -2 && cpu->IQ[iq_index_brd1].allocate_bit==0 && (cpu->IQ[iq_index_brd1].pc != -1 || cpu->IQ[iq_index_brd1].pc == cpu->issue.pc)) 
                {
                    
                    if(cpu->IQ[iq_index_brd1].src1_arc==cpu->broadcasting[broadcast2].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast2].tag, 0);
                        cpu->broadcasting_rs1=1;
                    }
                    if(cpu->IQ[iq_index_brd1].src2_arc==cpu->broadcasting[broadcast2].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast2].tag, 0);
                        cpu->broadcasting_rs2=1;
                    }
                    if(cpu->IQ[iq_index_brd1].src1_arc==cpu->broadcasting[broadcast1].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast1].tag, 0);
                        cpu->broadcasting_rs1=1;
                    }
                    if(cpu->IQ[iq_index_brd1].src2_arc==cpu->broadcasting[broadcast1].arc_reg )
                    {
                        update_iq(cpu, 1,cpu->broadcasting[broadcast1].tag, 0);
                        cpu->broadcasting_rs2=1;
                    }
                }
                int broadcst1 = search_broadcasting_bus(cpu, cpu->issue.rs1);
                int broadcst2= search_broadcasting_bus(cpu, cpu->issue.rs2);
                int iq_index_brd2 = search_issue_queue_pc(cpu, cpu->issue.pc);
                //if both regs are found in broadcast
                if(iq_index_brd2 == -2 && broadcst1!=-2 && broadcst2!=-2) //getting the broadcast value for  two regs
                {

                    entry_issue_queue(cpu, 0, cpu->issue.opcode, 0, 1, cpu->broadcasting[broadcst1].tag, cpu->broadcasting[broadcst1].value, 1, cpu->broadcasting[broadcst2].tag, cpu->broadcasting[broadcst2].value, cpu->issue.rd, cpu->issue.pc, cpu->issue.rs1, cpu->issue.rs2, cpu->issue.opcode_str);
                    
                }
                
                update_allocate_iq(cpu,rs1_prf_index, rs2_prf_index, iq_index_brd2);
                
                int iq_index_again2 = search_issue_queue_pc(cpu, cpu->issue.pc);

                
                if(cpu->IQ[iq_index_again2].allocate_bit==1 && cpu->rob[rob_index].established_entry==1)
                {
                    cpu->issue.entry_made_iq=1;
                    cpu->issue.entry_made_rob=1;
                }
                break;
                

            }
            case OPCODE_MOVC:
            {
                int iq_index = search_issue_queue_pc(cpu, cpu->issue.pc);
                int rob_index= search_rob_pc(cpu, cpu->issue.pc);
                if(cpu->IQ[iq_index].allocate_bit==1 && cpu->rob[rob_index].established_entry==1 && cpu->IQ[iq_index].pc != -1)
                {
                    cpu->issue.entry_made_iq=1;
                    cpu->issue.entry_made_rob=1;
                }
                break;
            }

                
            
    
            case OPCODE_HALT:
            {
                // cpu->issue.stalled=0; //HALT should move as is
                // cpu->execute = cpu->issue;
                int iq_index = search_issue_queue_pc(cpu, cpu->issue.pc);
                int rob_index= search_rob_pc(cpu, cpu->issue.pc);
                if(cpu->IQ[iq_index].allocate_bit==1 && cpu->rob[rob_index].established_entry==1 && cpu->IQ[iq_index].pc != -1)
                {
                    cpu->issue.entry_made_iq=1;
                    cpu->issue.entry_made_rob=1;
                }
                break;
            }
        }
    
       
            
        /* Copy data from execute latch to memory latch*/
        //if (cpu->issue.stalled==0 &&cpu->issue.entry_made_iq==1 && cpu->issue.entry_made_rob==1 )
        if(cpu->issue.stalled==0)
        {
            int i = cpu->iq_head;
            
            while(i < cpu->iq_tail)
            {
                printf("\n head of iq = %d", i);
                
                if (cpu->IQ[i].allocate_bit == 1 && cpu->IQ[i].pc!=-1 && cpu->IQ[i].issued==-1 && search_issue_queue_pc(cpu,cpu->IQ[i].pc )!=-2 && search_rob_pc(cpu, cpu->IQ[i].pc)!=-2)
                {                       
                    
                    if(cpu->IQ[i].fu == int_fu && cpu->check_int_fu==0 && cpu->IQ[i].pc==  cpu->issue.pc)
                    {
                        update_issue(cpu, i);
                        cpu->check_int_fu = 1;
                        //cpu->int_fu = cpu->issue;
                        cpu->int_fu.has_insn = TRUE;
                        cpu->int_fu.pc= cpu->IQ[i].pc;
                        cpu->int_fu.rs1= cpu->IQ[i].src1_arc;
                        cpu->int_fu.rs1_value=cpu->IQ[i].src1_value;
                        cpu->int_fu.rs2= cpu->IQ[i].src2_arc;
                        cpu->int_fu.rs2_value= cpu->IQ[i].src2_value;
                        cpu->int_fu.imm = cpu->IQ[i].literal;
                        cpu->int_fu.rd=cpu->IQ[i].dest;
                        cpu->int_fu.opcode= cpu->IQ[i].opcode;
                        cpu->int_fu.imm=cpu->IQ[i].literal;
                        strcpy(cpu->int_fu.opcode_str, cpu->IQ[i].opcode_str_iq);
                        cpu->int_fu.result_buffer= cpu->IQ[i].result_buffer;
                    }

                    if(cpu->IQ[i].fu == mul_fu1 && cpu->check_mul_fu==0 && cpu->IQ[i].pc==  cpu->issue.pc )
                    {
                        update_issue(cpu, i);
                        cpu->check_mul_fu = 1;
                        //cpu->mul_fu1 = cpu->issue;
                        cpu->mul_fu1.has_insn = TRUE;
                        cpu->mul_fu1.pc= cpu->IQ[i].pc;
                        cpu->mul_fu1.rs1= cpu->IQ[i].src1_arc;
                        cpu->mul_fu1.rs1_value=cpu->IQ[i].src1_value;
                        cpu->mul_fu1.rs2= cpu->IQ[i].src2_arc;
                        cpu->int_fu.rs2_value= cpu->IQ[i].src2_value;
                        cpu->mul_fu1.imm = cpu->IQ[i].literal;
                        cpu->mul_fu1.rd=cpu->IQ[i].dest;
                        cpu->mul_fu1.opcode= cpu->IQ[i].opcode;
                        strcpy(cpu->mul_fu1.opcode_str, cpu->IQ[i].opcode_str_iq);
                        cpu->mul_fu1.result_buffer= cpu->IQ[i].result_buffer;
                    }

                    if(cpu->IQ[i].fu == a_fu && cpu->check_a_fu==0 && cpu->IQ[i].pc==  cpu->issue.pc)
                    {
                        update_issue(cpu, i);
                        cpu->check_a_fu = 1;
                        cpu->a_fu = cpu->issue;
                    }

                    if(cpu->IQ[i].fu == b_fu && cpu->check_b_fu==0 && cpu->IQ[i].pc==  cpu->issue.pc)
                    {
                        update_issue(cpu, i);
                        cpu->check_b_fu = 1;
                        cpu->b_fu = cpu->issue;
                    }

                    if(cpu->IQ[i].fu == no_fu && cpu->check_no_fu==0 && cpu->IQ[i].pc==  cpu->issue.pc)
                    {
                        update_issue(cpu, i);
                        cpu->check_no_fu=1;
                        //cpu->no_fu = cpu->issue;
                        cpu->no_fu.has_insn = TRUE;
                        cpu->no_fu.pc= cpu->IQ[i].pc;
                        cpu->no_fu.rs1= cpu->IQ[i].src1_arc;
                        cpu->no_fu.rs2= cpu->IQ[i].src2_arc;
                        cpu->no_fu.imm = cpu->IQ[i].literal;
                        cpu->no_fu.rd=cpu->IQ[i].dest;
                        cpu->no_fu.opcode= cpu->IQ[i].opcode;
                        strcpy(cpu->no_fu.opcode_str, cpu->IQ[i].opcode_str_iq);
                        cpu->no_fu.result_buffer= cpu->IQ[i].result_buffer;

                    }
                }
                
                i++;
            }
            cpu->issue.has_insn = FALSE;
        }
        else
        {
            cpu->dispatch.stalled= 1;
        }
        
        if (ENABLE_DEBUG_MESSAGES)
        {

            print_stage_content("Issue ", &cpu->issue);
            // print_issue_queue(cpu);
            // print_prf(cpu);
            // // print_broadcst(cpu);
            // // print_rob(cpu);
        }
    }
    j++;
    }
    
    }
}



/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */

static void
APEX_no_fu(APEX_CPU *cpu)
{
     if (cpu->no_fu.has_insn)
    {
        switch (cpu->no_fu.opcode)
        {

          case OPCODE_HALT:
          {
            break;
          }
        }

         if (cpu->rob_head == search_rob_pc(cpu, cpu->no_fu.pc)){
            if(cpu->update.has_insn == FALSE){
                cpu->update = cpu->no_fu;
                int idx_issue = search_issue_queue_pc(cpu, cpu->no_fu.pc);
                free_issue_queue(cpu, idx_issue);
                cpu->no_fu.has_insn = FALSE;
                cpu->check_no_fu=0;
            }
            
        }
        if (cpu->rob_head+1 == search_rob_pc(cpu, cpu->no_fu.pc) && (get_rob_entry_status(cpu)==1 || get_rob_entry_status(cpu)==2) ){
            if(cpu->update.has_insn == FALSE){
                cpu->update = cpu->no_fu;
                int idx_issue = search_issue_queue_pc(cpu, cpu->no_fu.pc);
                free_issue_queue(cpu, idx_issue);
                cpu->no_fu.has_insn = FALSE;
                cpu->check_no_fu=0;
            }
            
        }
        if (cpu->rob_head+2 == search_rob_pc(cpu, cpu->int_fu.pc) && cpu->rob[cpu->rob_head+1].status==1 &&  cpu->rob[cpu->rob_head].status==2 )
        {
           if(cpu->update.has_insn == FALSE){
            cpu->update = cpu->no_fu;
            int idx_issue = search_issue_queue_pc(cpu, cpu->no_fu.pc);
            free_issue_queue(cpu, idx_issue);
            cpu->no_fu.has_insn = FALSE;
            cpu->check_no_fu=0;

        }
            
        }
        

         if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("NO-FU", &cpu->no_fu);
        } 
    }
}

static void
APEX_int_fu(APEX_CPU *cpu)
{
    if (cpu->int_fu.has_insn)
    {
        cpu->check_int_fu=1;
        
        /* int_fu logic based on instruction type */
        switch (cpu->int_fu.opcode)
        {

          case OPCODE_ADD:
          {
            if(cpu->regs_writing[cpu->int_fu.rd] == 0){
              cpu->regs_writing[cpu->int_fu.rd] = 1;
            }
            int rd_prf_index= search_prf_broadcast(cpu, cpu->int_fu.rd, cpu->int_fu.pc);
            cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
            cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;

            int rs1_prf_index= search_prf(cpu, cpu->int_fu.rs1, cpu->int_fu.pc);
            int rs2_prf_index= search_prf(cpu, cpu->int_fu.rs2, cpu->int_fu.pc);
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            if(cpu->broadcasting_rs1==1)
            {
                cpu->IQ[iq_index].src1_value=cpu->PR[rs1_prf_index].value;

            }
            if(cpu->broadcasting_rs2==1)
            {
                
                cpu->IQ[iq_index].src2_value=cpu->PR[rs2_prf_index].value;
            }
            cpu->int_fu.result_buffer=cpu->IQ[iq_index].src1_value + cpu->IQ[iq_index].src2_value;


            cpu->broadcasting[rd_prf_index].value=cpu->int_fu.result_buffer;

              /* Set the zero flag based on the result buffer */
              if(cpu->int_fu.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->int_fu.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->int_fu.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
            
              break;
          }
          case OPCODE_ADDL:
          {

            if(cpu->regs_writing[cpu->int_fu.rd] == 0){
              cpu->regs_writing[cpu->int_fu.rd] = 1;
            }
            //int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
              cpu->int_fu.result_buffer
                  = cpu->int_fu.rs1_value + cpu->int_fu.imm;
                cpu->ex_fb.reg= cpu->int_fu.rd;
              cpu->ex_fb.value = cpu->int_fu.result_buffer;
              /* Set the zero flag based on the result buffer */
              if(cpu->int_fu.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->int_fu.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->int_fu.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
              break;
          }
          case OPCODE_SUB:
          {
            if(cpu->regs_writing[cpu->int_fu.rd] == 0){
              cpu->regs_writing[cpu->int_fu.rd] = 1;
            }

            int rd_prf_index= search_prf_broadcast(cpu, cpu->int_fu.rd, cpu->int_fu.pc);
            cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
            cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;

            int rs1_prf_index= search_prf(cpu, cpu->int_fu.rs1, cpu->int_fu.pc);
            int rs2_prf_index= search_prf(cpu, cpu->int_fu.rs2, cpu->int_fu.pc);
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            if(cpu->broadcasting_rs1==1)
            {
                cpu->IQ[iq_index].src1_value=cpu->PR[rs1_prf_index].value;
            }
            if(cpu->broadcasting_rs2==1)
            {
                cpu->IQ[iq_index].src2_value=cpu->PR[rs2_prf_index].value;
            }

            cpu->int_fu.result_buffer=cpu->IQ[iq_index].src1_value - cpu->IQ[iq_index].src2_value;


            cpu->broadcasting[rd_prf_index].value=cpu->int_fu.result_buffer;

                

              /* Set the zero flag based on the result buffer */
             if(cpu->int_fu.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->int_fu.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->int_fu.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
              break;
          }
          case OPCODE_SUBL:
          {
            if(cpu->regs_writing[cpu->int_fu.rd] == 0){
              cpu->regs_writing[cpu->int_fu.rd] = 1;
            }
            //int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
              cpu->int_fu.result_buffer
                  = cpu->int_fu.rs1_value - cpu->int_fu.imm;
                   cpu->ex_fb.reg= cpu->int_fu.rd;
              cpu->ex_fb.value = cpu->int_fu.result_buffer;

              /* Set the zero flag based on the result buffer */
              if(cpu->int_fu.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->int_fu.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->int_fu.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
              break;
          }
          
          case OPCODE_AND:
          {
            if(cpu->regs_writing[cpu->int_fu.rd] == 0){
              cpu->regs_writing[cpu->int_fu.rd] = 1;
            }
            int rd_prf_index= search_prf_broadcast(cpu, cpu->int_fu.rd, cpu->int_fu.pc);
            cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
            cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;

            int rs1_prf_index= search_prf(cpu, cpu->int_fu.rs1, cpu->int_fu.pc);
            int rs2_prf_index= search_prf(cpu, cpu->int_fu.rs2, cpu->int_fu.pc);
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            if(cpu->broadcasting_rs1==1)
            {
                cpu->IQ[iq_index].src1_value=cpu->broadcasting[rs1_prf_index].value;
            }
            if(cpu->broadcasting_rs2==1)
            {
                cpu->IQ[iq_index].src2_value=cpu->broadcasting[rs2_prf_index].value;
            }


            cpu->int_fu.result_buffer=cpu->IQ[iq_index].src1_value & cpu->IQ[iq_index].src2_value;


            cpu->broadcasting[rd_prf_index].value=cpu->int_fu.result_buffer;


              /* Set the zero flag based on the result buffer */
              if(cpu->int_fu.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->int_fu.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->int_fu.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
              break;
          }
          case OPCODE_OR:
          {
            if(cpu->regs_writing[cpu->int_fu.rd] == 0){
              cpu->regs_writing[cpu->int_fu.rd] = 1;
            }
            int rd_prf_index= search_prf_broadcast(cpu, cpu->int_fu.rd, cpu->int_fu.pc);
            cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
            cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;

            int rs1_prf_index= search_prf(cpu, cpu->int_fu.rs1, cpu->int_fu.pc);
            int rs2_prf_index= search_prf(cpu, cpu->int_fu.rs2, cpu->int_fu.pc);
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            if(cpu->broadcasting_rs1==1)
            {
                cpu->IQ[iq_index].src1_value=cpu->broadcasting[rs1_prf_index].value;
            }
            if(cpu->broadcasting_rs2==1)
            {
                cpu->IQ[iq_index].src2_value=cpu->broadcasting[rs2_prf_index].value;
            }


            cpu->int_fu.result_buffer=cpu->IQ[iq_index].src1_value | cpu->IQ[iq_index].src2_value;


            cpu->broadcasting[rd_prf_index].value=cpu->int_fu.result_buffer;

              /* Set the zero flag based on the result buffer */
              if(cpu->int_fu.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->int_fu.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->int_fu.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
              break;
          }
          case OPCODE_XOR:
          {
            if(cpu->regs_writing[cpu->int_fu.rd] == 0){
              cpu->regs_writing[cpu->int_fu.rd] = 1;
            }
            int rd_prf_index= search_prf_broadcast(cpu, cpu->int_fu.rd, cpu->int_fu.pc);
            cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
            cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;

            int rs1_prf_index= search_prf(cpu, cpu->int_fu.rs1, cpu->int_fu.pc);
            int rs2_prf_index= search_prf(cpu, cpu->int_fu.rs2, cpu->int_fu.pc);
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            if(cpu->broadcasting_rs1==1)
            {
                cpu->IQ[iq_index].src1_value=cpu->broadcasting[rs1_prf_index].value;
            }
            if(cpu->broadcasting_rs2==1)
            {
                cpu->IQ[iq_index].src2_value=cpu->broadcasting[rs2_prf_index].value;
            }


            cpu->int_fu.result_buffer=cpu->IQ[iq_index].src1_value ^ cpu->IQ[iq_index].src2_value;


            cpu->broadcasting[rd_prf_index].value=cpu->int_fu.result_buffer;

              /* Set the zero flag based on the result buffer */
              if(cpu->int_fu.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->int_fu.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->int_fu.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
              break;
          }
            case OPCODE_MOVC:
            {
              if(cpu->regs_writing[cpu->int_fu.rd] == 0){
                cpu->regs_writing[cpu->int_fu.rd] = 1;
              }
             
                //cpu->int_fu.result_buffer = cpu->int_fu.imm + 0;
                
                int rd_prf_index= search_prf_broadcast(cpu, cpu->int_fu.rd, cpu->int_fu.pc);
                cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
                cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;

                int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
                cpu->int_fu.result_buffer= cpu->IQ[iq_index].literal +0;
                cpu->PR[rd_prf_index].value= cpu->int_fu.result_buffer;
                break;
            }
         
        }
        
        /* Copy data from execute latch to memory latch*/
        if (cpu->rob_head == search_rob_pc(cpu, cpu->int_fu.pc)){
             if(cpu->update.has_insn == FALSE){
            cpu->update = cpu->int_fu;
            cpu->update.result_buffer=cpu->int_fu.result_buffer;
            cpu->update.has_insn = TRUE;
            cpu->int_fu.has_insn = FALSE;
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            free_issue_queue(cpu, iq_index);
            cpu->check_int_fu=0;
            

        }
            
        }
        if (cpu->rob_head+1 == search_rob_pc(cpu, cpu->int_fu.pc) && (get_rob_entry_status(cpu)==1 || get_rob_entry_status(cpu)==2)  ){
           if(cpu->update.has_insn == FALSE){
            cpu->update = cpu->int_fu;
            cpu->update.result_buffer=cpu->int_fu.result_buffer;
            cpu->update.has_insn = TRUE;
            cpu->int_fu.has_insn = FALSE;
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            free_issue_queue(cpu, iq_index);
            cpu->check_int_fu=0;

        }
        }
        if (cpu->rob_head+2 == search_rob_pc(cpu, cpu->int_fu.pc) && cpu->rob[cpu->rob_head+1].status==1 &&  cpu->rob[cpu->rob_head].status==2 ){
           if(cpu->update.has_insn == FALSE){
            cpu->update = cpu->int_fu;
            cpu->update.result_buffer=cpu->int_fu.result_buffer;
            cpu->update.has_insn = TRUE;
            cpu->int_fu.has_insn = FALSE;
            int iq_index = search_issue_queue_pc(cpu, cpu->int_fu.pc);
            free_issue_queue(cpu, iq_index);
            cpu->check_int_fu=0;

        }
            
        }
        

         if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("INT-FU", &cpu->int_fu);
            // print_prf(cpu);
            // print_broadcst(cpu);
            // print_issue_queue(cpu);
            // print_rob(cpu);
        }
    
    }
    return;
}



static void
APEX_mul_fu1(APEX_CPU *cpu)
{
    if (cpu->mul_fu1.has_insn)
    {

        /* int_fu logic based on instruction type */
        switch (cpu->mul_fu1.opcode)
        {

          case OPCODE_MUL:
          {
            cpu->check_mul_fu=1;
          }
        }
        /* Copy data from execute latch to memory latch*/
        cpu->mul_fu2 = cpu->mul_fu1;
        cpu->mul_fu1.has_insn = FALSE;
        cpu->check_mul_fu=0;
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("MUL-FU1", &cpu->mul_fu1);
            // print_issue_queue(cpu);
        }
    }
}

static void
APEX_mul_fu2(APEX_CPU *cpu)
{
    if (cpu->mul_fu2.has_insn)
    {

        /* int_fu logic based on instruction type */
        switch (cpu->mul_fu2.opcode)
        {

          case OPCODE_MUL:
          {
            cpu->check_mul_fu=1;
            if(cpu->regs_writing[cpu->mul_fu2.rd] == 0){
              cpu->regs_writing[cpu->mul_fu2.rd] = 1;
            }

            int rd_prf_index= search_prf_broadcast(cpu, cpu->mul_fu2.rd, cpu->mul_fu2.pc);
            cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
            cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;
          }
        }
        /* Copy data from execute latch to memory latch*/
        cpu->mul_fu3 = cpu->mul_fu2;
        cpu->mul_fu2.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("MUL-FU2", &cpu->mul_fu2);
        }
    }
}

static void
APEX_mul_fu3(APEX_CPU *cpu)
{
    if (cpu->mul_fu3.has_insn)
    {

        /* mul_fu3 logic based on instruction type */
        switch (cpu->mul_fu3.opcode)
        {

          case OPCODE_MUL:
          {
            cpu->check_mul_fu=1;

            if(cpu->regs_writing[cpu->mul_fu3.rd] == 0){
              cpu->regs_writing[cpu->mul_fu3.rd] = 1;
            }

            int rd_prf_index= search_prf_broadcast(cpu, cpu->mul_fu3.rd, cpu->mul_fu3.pc);
            cpu->broadcasting[rd_prf_index].tag = rd_prf_index;
            cpu->broadcasting[rd_prf_index].arc_reg=cpu->PR[rd_prf_index].arc_reg;

            int rs1_prf_index= search_prf(cpu, cpu->mul_fu3.rs1, cpu->mul_fu3.pc);
            int rs2_prf_index= search_prf(cpu, cpu->mul_fu3.rs2, cpu->mul_fu3.pc);
            int iq_index = search_issue_queue_pc(cpu, cpu->mul_fu3.pc);
            if(cpu->broadcasting_rs1==1)
            {
                cpu->IQ[iq_index].src1_value=cpu->PR[rs1_prf_index].value;
            }
            if(cpu->broadcasting_rs2==1)
            {
                cpu->IQ[iq_index].src2_value=cpu->PR[rs2_prf_index].value;
            }

            cpu->mul_fu3.result_buffer=cpu->IQ[iq_index].src1_value * cpu->IQ[iq_index].src2_value;


            cpu->broadcasting[rd_prf_index].value=cpu->mul_fu3.result_buffer;
            
            if(cpu->mul_fu3.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->mul_fu3.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->mul_fu3.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
               
            break;
          }
        }

         if (cpu->rob_head == search_rob_pc(cpu, cpu->mul_fu3.pc)){

             if(cpu->update.has_insn == FALSE){
            cpu->update = cpu->mul_fu3;
            cpu->update.result_buffer=cpu->mul_fu3.result_buffer;
            cpu->update.has_insn = TRUE;
            cpu->mul_fu3.has_insn = FALSE;
            int iq_index = search_issue_queue_pc(cpu, cpu->mul_fu3.pc);
            free_issue_queue(cpu, iq_index);
            cpu->check_mul_fu=0;

        }
            
        }
        if (cpu->rob_head+1 == search_rob_pc(cpu, cpu->mul_fu3.pc) && (get_rob_entry_status(cpu)==1 || get_rob_entry_status(cpu)==2)  ){

           if(cpu->update.has_insn == FALSE){
            cpu->update = cpu->mul_fu3;
            cpu->update.result_buffer=cpu->mul_fu3.result_buffer;
            cpu->update.has_insn = TRUE;
            cpu->mul_fu3.has_insn = FALSE;
             cpu->mul_fu1.has_insn = FALSE;
            cpu->mul_fu2.has_insn = FALSE;
            int iq_index = search_issue_queue_pc(cpu, cpu->mul_fu3.pc);
            free_issue_queue(cpu, iq_index);
            cpu->check_mul_fu=0;
            

        }
            
        }
        if (cpu->rob_head+2 == search_rob_pc(cpu, cpu->mul_fu3.pc) && cpu->rob[cpu->rob_head+1].status==1 &&  cpu->rob[cpu->rob_head].status==2 ){
           if(cpu->update.has_insn == FALSE){
            cpu->update = cpu->mul_fu3;
            cpu->update.result_buffer=cpu->mul_fu3.result_buffer;
            cpu->update.has_insn = TRUE;
            cpu->mul_fu3.has_insn = FALSE;
             cpu->mul_fu1.has_insn = FALSE;
            cpu->mul_fu2.has_insn = FALSE;
            int iq_index = search_issue_queue_pc(cpu, cpu->mul_fu3.pc);
            free_issue_queue(cpu, iq_index);
            cpu->check_mul_fu=0;

        }
            
        }
       
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("MUL-FU3", &cpu->mul_fu3);
        }
    }
}
static void
APEX_update(APEX_CPU *cpu)
{
    if(cpu->update.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->update.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
                int rd_prf_index= update_prf(cpu, cpu->update.rd);
                cpu->broadcasting[rd_prf_index].value=cpu->update.result_buffer;
                cpu->PR[rd_prf_index].value=cpu->update.result_buffer;
                cpu->PR[rd_prf_index].valid=1;
                int rd_prf=search_prf(cpu, cpu->update.rd, cpu->update.pc);
                int iq_index = search_issue_queue(cpu, rd_prf, cpu->update.pc);
                if(cpu->IQ[iq_index].src1_tag==rd_prf)
                {
                    cpu->IQ[iq_index].src1_valid=1;
                    cpu->IQ[iq_index].src1_value=cpu->update.result_buffer;
                }
                if(cpu->IQ[iq_index].src2_tag==rd_prf)
                {
                    cpu->IQ[iq_index].src2_valid=1;
                    cpu->IQ[iq_index].src2_value=cpu->update.result_buffer;
                }
                int rob_idx = search_rob_pc(cpu,cpu->update.pc );
                cpu->rob[rob_idx].status=1;


                break;
            }
            case OPCODE_MOVC:
            {
                int rd_prf_index= update_prf(cpu, cpu->update.rd);

                cpu->broadcasting[rd_prf_index].value=cpu->update.result_buffer;
                cpu->PR[rd_prf_index].value=cpu->update.result_buffer;
                cpu->PR[rd_prf_index].valid=1;
                int rd_prf=search_prf(cpu, cpu->update.rd, cpu->update.pc);
                int iq_index = search_issue_queue(cpu, rd_prf, cpu->update.pc);
                if(cpu->IQ[iq_index].src1_tag==rd_prf)
                {
                    cpu->IQ[iq_index].src1_valid=1;
                    cpu->IQ[iq_index].src1_value=cpu->update.result_buffer;
                }
                if(cpu->IQ[iq_index].src2_tag==rd_prf)
                {
                    cpu->IQ[iq_index].src2_valid=1;
                    cpu->IQ[iq_index].src2_value=cpu->update.result_buffer;
                }
                int rob_idx = search_rob_pc(cpu,cpu->update.pc );
                cpu->rob[rob_idx].status=1;
                break;
            }

            case OPCODE_HALT:
            {
                int rob_idx = search_rob_pc(cpu,cpu->update.pc );
                cpu->rob[rob_idx].status=1;
                break;
            }
        
        }
        
        cpu->writeback = cpu->update;
        cpu->update.has_insn = FALSE;
        

         if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Update", &cpu->update);
            // print_prf(cpu);
            // print_issue_queue(cpu);
            // print_rob(cpu);
        }
    }
    return;
}

static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {        
        
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
          case OPCODE_ADD:
          case OPCODE_ADDL:
          case OPCODE_SUB:
          case OPCODE_SUBL:
          case OPCODE_MUL:
          case OPCODE_AND:
          case OPCODE_OR:
          case OPCODE_XOR:
            {

                int rob_index = cpu->rob_head;
                cpu->rob[rob_index].status=2; //in writeback
                cpu->regs[cpu->rob[rob_index].rd_arc]= cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->rob[rob_index].rd_arc]=0;
                

                int free_prf_index = cpu->rob[rob_index].rename_entry;
                free_prf(cpu, free_prf_index);

                int brdcast_idx= search_broadcasting_bus(cpu,cpu->writeback.rd );
                free_broadcast_bus(cpu, brdcast_idx);
                
                free_rob(cpu, rob_index);
                //cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                //cpu->regs_writing[cpu->writeback.rd] = 0;
                
                
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->writeback.rd] = 0;

                break;
            }
            case OPCODE_LOADP:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regs[cpu->writeback.rs1]=cpu->writeback.rs1_value;
                cpu->regs_writing[cpu->writeback.rd] = 0;
                cpu->regs_writing[cpu->writeback.rs1] =0;
                break;
            }
            case OPCODE_STOREP:
            {
                cpu->regs[cpu->writeback.rs2]=cpu->writeback.rs2_value;
                cpu->regs_writing[cpu->writeback.rs2] = 0;
                
                break;
            }

            case OPCODE_MOVC:
            {
                int rob_index = cpu->rob_head;
                cpu->rob[rob_index].status=2; //in writeback
                cpu->regs[cpu->rob[rob_index].rd_arc]= cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->rob[rob_index].rd_arc]=0;
                
                int free_prf_index = cpu->rob[rob_index].rename_entry;
                free_prf(cpu,free_prf_index );
                
                int brdcast_idx= search_broadcasting_bus(cpu,cpu->writeback.rd );
                free_broadcast_bus(cpu, brdcast_idx);

                free_rob(cpu, rob_index);
                //cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                //cpu->regs_writing[cpu->writeback.rd] = 0;
                

                break;
            }
            case OPCODE_JALR:
            {
                cpu->regs[cpu->writeback.rd]= cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->writeback.rd] = 0;
                break;
            }
        }


        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;
        
         if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }
        if (cpu->writeback.opcode == OPCODE_HALT)
            {
                /* Stop the APEX simulator */
                return TRUE;
            }
        

    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    cpu->data_counter = 0;
    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    memset(&cpu->btb_queue, 0, sizeof(cpu->btb_queue)); // Initialize to all zeros
    cpu->btb_queue.head = 0;
    cpu->btb_queue.tail = 0;
    cpu->btb_queue.size = 0;
    for (i = 0; i < 24; ++i)
    {
        cpu->IQ[i].allocate_bit = -1;
        cpu->IQ[i].opcode = -1;
        cpu->IQ[i].literal = -1;
        cpu->IQ[i].src1_valid = -1;
        cpu->IQ[i].src1_tag = -1;
        cpu->IQ[i].src1_value = -1;
        cpu->IQ[i].src2_valid = -1;
        cpu->IQ[i].src2_tag = -1;
        cpu->IQ[i].src2_value = -1;
        cpu->IQ[i].dest = -1;
        cpu->IQ[i].pc = -1;
        cpu->IQ[i].issued=-1;
        cpu->IQ[i].src1_arc=-1;
        cpu->IQ[i].src2_arc=-1;

        cpu->BQ[i].allocate_bit = -1;
        cpu->BQ[i].opcode = -1;
        cpu->BQ[i].literal = -1;
        cpu->BQ[i].src1_valid = -1;
        cpu->BQ[i].src1_tag = -1;
        cpu->BQ[i].src1_value = -1;
        cpu->BQ[i].src2_valid = -1;
        cpu->BQ[i].src2_tag = -1;
        cpu->BQ[i].src2_value = -1;
        cpu->BQ[i].dest = -1;
        cpu->BQ[i].pc = -1;
        cpu->BQ[i].issued=-1;
    }
    for (int j = 0; j < 25; ++j)
    {
        cpu->PR[j].valid=0;
        cpu->PR[j].value=-1;
        cpu->PR[j].arc_reg=-1;
        cpu->broadcasting[j].arc_reg=-1;
        cpu->broadcasting[j].tag=-1;
        cpu->broadcasting[j].value=-1;
    }
    cpu->iq_head=0;
    cpu->iq_tail=0;
    cpu->rob_head=0;
    cpu->rob_tail=0;
    cpu->check_int_fu = 0;
    cpu->check_a_fu = 0;
    cpu->check_b_fu = 0;
    cpu->check_mul_fu=0;
    cpu->pr = 0;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void simulate_cpu_for_cycles(APEX_CPU *cpu, int num_cycles) {
    for (int cycle = 1; cycle <= num_cycles; cycle++) {
        if (ENABLE_DEBUG_MESSAGES) {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cycle);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu)) {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cycle, cpu->insn_completed);
            break;
        }

        // APEX_memory(cpu);
        // APEX_execute(cpu);
        APEX_update(cpu);
        APEX_mul_fu3(cpu);
        APEX_mul_fu2(cpu);
        APEX_mul_fu1(cpu);
        APEX_no_fu(cpu);
        APEX_int_fu(cpu);
        APEX_issue(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

    }
}

void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;
    cpu->clock=1;
    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }
        
        // APEX_memory(cpu);
        // APEX_execute(cpu);
        APEX_writeback(cpu);
        APEX_update(cpu);
        APEX_mul_fu3(cpu);
        APEX_mul_fu2(cpu);
        APEX_mul_fu1(cpu);
        APEX_no_fu(cpu);
        APEX_int_fu(cpu);
        APEX_issue(cpu);
        APEX_dispatch(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        
        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}
