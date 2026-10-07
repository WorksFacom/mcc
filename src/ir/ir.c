#include <stdlib.h>
#include <string.h>
#include "ir.h"

/** @brief cria uma cópia portável de uma string. */
static char* duplicar_string(const char* texto) {
    size_t tamanho = strlen(texto) + 1;
    char* copia = malloc(tamanho);
    if (copia != NULL) memcpy(copia, texto, tamanho);
    return copia;
}

/** @brief cria uma cópia profunda e independente de um operando. */
IROperand* copiar_operando(const IROperand* operando) {
    if (operando == NULL) return NULL;

    IROperand* copia = calloc(1, sizeof(*copia));
    if (copia == NULL) return NULL;

    copia->type = operando->type;
    copia->data = operando->data;
    copia->stack_offset = operando->stack_offset;

    if ((operando->type == OPERAND_LABEL || operando->type == OPERAND_STRING_LBL) &&
        operando->data.label_name != NULL) {
        copia->data.label_name = duplicar_string(operando->data.label_name);
        if (copia->data.label_name == NULL) {
            free(copia);
            return NULL;
        }
        copia->owns_label = 1;
    }

    if (operando->string_content != NULL) {
        copia->string_content = duplicar_string(operando->string_content);
        if (copia->string_content == NULL) {
            liberar_operando(copia);
            return NULL;
        }
    }

    return copia;
}

/** @brief libera um operando e todos os dados pertencentes a ele. */
void liberar_operando(IROperand* operando) {
    if (operando == NULL) return;

    if ((operando->type == OPERAND_LABEL || operando->type == OPERAND_STRING_LBL) &&
        operando->owns_label && operando->data.label_name != NULL) {
        free(operando->data.label_name);
    }
    free(operando->string_content);
    free(operando);
}

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
        
        liberar_operando(atual->result);
        liberar_operando(atual->arg1);
        liberar_operando(atual->arg2);
        
        //libera a instrução
        free(atual);
        atual = proximo;
    }
}
