#include <stdlib.h>
#include <string.h>
#include "ir.h"

/**
 * @file ir.c
 * @brief implementação de funções de utilidade para a representação intermediária.
 *
 * este arquivo contém funções auxiliares para o módulo da ir, como a
 * função para gerenciar e liberar a memória alocada para a lista de instruções.
 */

/**
 * @brief libera toda a memória alocada por uma lista de instruções da ir.
 *
 * percorre a lista encadeada de instruções, liberando cada operando,
 * os dados internos (como nomes de labels) e a própria instrução, evitando
 * vazamentos de memória.
 *
 * @param ir_head ponteiro para a primeira instrução da lista.
 */
void liberar_ir(IR_Instruction* ir_head) {
    IR_Instruction* atual = ir_head;
    IR_Instruction* proximo;
    
    while (atual != NULL) {
        proximo = atual->next;
        
        //libera result
        if (atual->result) {
            //só libera label_name se for LABEL/STRING_LBL e for o dono
            if ((atual->result->type == OPERAND_LABEL || atual->result->type == OPERAND_STRING_LBL)
                && atual->result->owns_label
                && atual->result->data.label_name) {
                free(atual->result->data.label_name);
            }
            //sempre libera a estrutura do operando
            if (atual->result->type == OPERAND_STRING_LBL && atual->result->string_content) {
            free(atual->result->string_content);
            }
            free(atual->result);
        }
        
        //libera arg1
        if (atual->arg1) {
            //só libera label_name se for LABEL/STRING_LBL e for o dono
            if ((atual->arg1->type == OPERAND_LABEL || atual->arg1->type == OPERAND_STRING_LBL)
                && atual->arg1->owns_label
                && atual->arg1->data.label_name) {
                free(atual->arg1->data.label_name);
            }
            if (atual->arg1->type == OPERAND_STRING_LBL && atual->arg1->string_content) {
                free(atual->arg1->string_content);
            }
            free(atual->arg1);
        }
        
        //libera arg2
        if (atual->arg2) {
            //só libera label_name se for LABEL/STRING_LBL e for o dono
            if ((atual->arg2->type == OPERAND_LABEL || atual->arg2->type == OPERAND_STRING_LBL)
                && atual->arg2->owns_label
                && atual->arg2->data.label_name) {
                free(atual->arg2->data.label_name);
            }
            //libera string_content se ele existir
            if (atual->arg2->type == OPERAND_STRING_LBL && atual->arg2->string_content) {
                free(atual->arg2->string_content);
            }
            free(atual->arg2);
        }
        
        //libera a instrução
        free(atual);
        atual = proximo;
    }
}