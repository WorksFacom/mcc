#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "../ir/ir.h"
#include "../symbol_table/symbol_table.h"

/**
 * @file assembly.h
 * @brief definições para o gerador de código assembly x86-64.
 *
 * este arquivo define a interface pública do módulo responsável por
 * traduzir a representação intermediária (ir) para o código
 * assembly final da arquitetura x86-64 (sintaxe at&t).
 */

/**
 * @brief função principal que gera o arquivo de assembly a partir da ir.
 *
 * esta função percorre a lista encadeada de instruções da ir e
 * escreve o código assembly x86-64 correspondente em um arquivo de saída.
 *
 * @param ir_head o ponteiro para a primeira instrução da lista de ir.
 * @param pilha a pilha de tabelas de símbolos (para consultar offsets).
 * @param nome_arquivo_saida o nome do arquivo .s onde o assembly será escrito.
 * @return 0 em caso de sucesso, -1 em caso de erro (ex: falha ao abrir arquivo).
 */
int gerar_assembly(IR_Instruction* ir_head, PilhaTabelasSimbolos* pilha, const char* nome_arquivo_saida);

#endif //ASSEMBLY_H