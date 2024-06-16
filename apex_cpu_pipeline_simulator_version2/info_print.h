#include "apex_macros.h"
#include "apex_cpu.h"




// ----------------- freelist methods fetch ----------------- //
void print_prf(APEX_CPU *cpu);

int get_free_prf(APEX_CPU *cpu);
int set_free_prf(APEX_CPU *cpu, int prf, int arch_reg, int value, int pc_prf);
void free_prf(APEX_CPU *cpu, int prf_index);
int search_prf(APEX_CPU *cpu, int arc_reg, int pr_pc);
int search_prf_broadcast(APEX_CPU *cpu, int arc_reg, int pr_pc);
int search_prf_cpu(APEX_CPU *cpu, int arc_reg, int pr_pc);
int search_prf_prev_rd(APEX_CPU *cpu, int arc_reg, int pr_pc);
int update_prf(APEX_CPU *cpu, int rd);

// ----------------- issue queue ----------------- //

int entry_issue_queue(APEX_CPU *cpu, int allo_bit, int opcode, int literal, int src1_valid, int src1_tag, int src1_value, int src2_valid, int src2_tag, int src2_value, int dest, int pc, int sr1_ar, int sr2_ar, char strg[128]);
void free_issue_queue(APEX_CPU *cpu, int iss_idx);
void print_issue_queue(APEX_CPU *cpu);

void increment_iq_head(APEX_CPU *cpu);
void increment_iq_tail(APEX_CPU *cpu);
int search_issue_queue(APEX_CPU *cpu, int pregister, int pc_iq);
int search_issue_queue_pc(APEX_CPU *cpu, int pc);
int update_issue_queue(APEX_CPU *cpu, int prf_index2, int prf_index1, int issue_index);
void update_iq(APEX_CPU *cpu, int valid, int tag, int value);
void update_allocate_iq(APEX_CPU *cpu, int pr1, int pr2, int iq_indx);
void update_issue(APEX_CPU *cpu, int indx);

// ----------------- ROB ----------------- //
void print_rob(APEX_CPU *cpu);
void free_rob(APEX_CPU *cpu, int head);
int entry_rob(APEX_CPU *cpu, int opcode, int pc, int rd_physical, int rename_entry, int rd_arc, int lsq_index, char op_string[128]);
int search_rob(APEX_CPU *cpu, int arc_rd);

int search_rob_pc(APEX_CPU *cpu, int pc_rob);
void increment_rob_tail(APEX_CPU *cpu);
void increment_rob_head(APEX_CPU *cpu);
int get_rob_entry_status(APEX_CPU *cpu);

// ------------braodcast--------
void print_broadcst(APEX_CPU *cpu);
int search_broadcasting_bus(APEX_CPU *cpu, int src_reg);

void free_broadcast_bus(APEX_CPU *cpu, int indx);