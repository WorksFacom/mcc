#ifndef INTERCODE_H
#define INTERCODE_H

#include "../ast/ast.h"
#include "../ir/ir.h"
#include "../symbol_table/symbol_table.h"

/**
 * @file intercode.h
 * @brief definições para o gerador de código intermediário.
 *
 * este arquivo define a interface pública do módulo responsável por
 * traduzir a árvore sintática abstrata (ast) para a representação
 * intermediária (ir) em formato de código de três endereços.
 */

/**
 * @brief função principal que gera a lista de instruções da ir a partir da ast.
 *
 * esta função percorre a ast (que já foi validada semanticamente) e
 * gera uma lista encadeada de instruções de três endereços (ir_instruction)
 * que representa a lógica do programa.
 *
 * @param raiz o nó raiz da árvore sintática abstrata.
 * @param pilha a pilha de tabelas de símbolos, já preenchida pela análise semântica.
 * @return um ponteiro para a primeira instrução na lista de código intermediário gerado.
 */
IR_Instruction* gerar_codigo_intermediario(ASTNode* raiz, PilhaTabelasSimbolos* pilha);

#endif //INTERCODE_H