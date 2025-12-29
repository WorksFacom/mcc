#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdbool.h> // Necessário para o tipo 'bool'
#include "ast.h"
#include "symbol_table.h"

/**
 * @file semantic.h
 * @brief declarações para o analisador semântico.
 *
 * define a interface da fase de análise semântica, cuja principal função
 * é percorrer a ast para validar as regras de significado da linguagem,
 * como tipos, escopos e declarações.
 */

/**
 * @brief função principal que inicia a análise semântica da ast.
 *
 * esta função dispara o processo de percorrer a árvore (tree walking)
 * para validar o programa.
 * @param raiz o nó raiz da árvore sintática abstrata gerada pelo parser.
 * @param pilha a pilha da tabela de símbolos, para gerenciamento de escopo e contexto.
 * @param verbose se true, imprime a tabela de símbolos no terminal (stdout) além de gerar o arquivo.
 */
void analisar_semanticamente(ASTNode* raiz, PilhaTabelasSimbolos* pilha, bool verbose);

#endif //SEMANTIC_H