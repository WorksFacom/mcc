#include <stdio.h>
#include "parser.h"
#include "ast.h"

/**
 * @file test_parser.c
 * @brief Programa de teste (driver) para o Analisador Sintático.
 *
 * Este programa não faz parte do compilador final, mas serve como uma ferramenta
 * de desenvolvimento para validar a fase de análise sintática. Ele opera de forma
 * independente, seguindo estes passos:
 * 1. Carrega a lista de tokens do arquivo 'tokens.txt'.
 * 2. Invoca o parser para construir a Árvore Sintática Abstrata (AST).
 * 3. Imprime a AST de forma legível no console para verificação visual.
 * 4. Libera toda a memória alocada.
 */


/**
 * @brief Imprime recursivamente a estrutura da Árvore Sintática Abstrata no console.
 *
 * Esta função percorre a árvore em pré-ordem, usando a indentação para exibir
 * claramente a hierarquia entre os nós (pais, filhos e irmãos). É a principal
 * ferramenta para verificar visualmente se o parser construiu a árvore corretamente.
 *
 * @param no O nó da AST a partir do qual a impressão deve começar.
 * @param indent_level O nível de profundidade atual na árvore, usado para calcular a indentação.
 */
void imprimir_ast(ASTNode *no, int indent_level) {
    if (no == NULL) {
        return;
    }

    // Imprime a indentação para visualização da hierarquia
    for (int i = 0; i < indent_level; ++i) {
        printf("  ");
    }

    // Imprime o tipo do nó e dados extras, se houver
    switch (no->node_type) {
        case NODE_PROGRAM: printf("PROGRAM\n"); break;
        case NODE_FUNCTION_DEF: printf("FUNCTION_DEF\n"); break;
        case NODE_VAR_DECL: printf("VAR_DECL\n"); break;
        case NODE_ASSIGN: printf("ASSIGN\n"); break;
        case NODE_IF: printf("IF\n"); break;
        case NODE_FOR: printf("FOR\n"); break;
        case NODE_RETURN: printf("RETURN\n"); break;
        case NODE_CALL: printf("CALL\n"); break;
        case NODE_BLOCK: printf("BLOCK\n"); break;
        case NODE_PARAM_LIST: printf("PARAM_LIST\n"); break;
        case NODE_ARRAY_ACCESS: printf("ARRAY_ACCESS\n"); break;
        case NODE_TYPE: printf("TYPE (%s)\n", no->data.op_type == INT ? "INT" : "CHAR"); break;
        case NODE_ID: printf("ID (Nome: %s)\n", no->data.string_value); break;
        case NODE_INTEGER_CONST: printf("INTEGER_CONST (Valor: %ld)\n", no->data.int_value); break;
        case NODE_CHAR_CONST: printf("CHAR_CONST (Valor: %c)\n", no->data.char_value); break;
        case NODE_STRING_CONST: printf("STRING_CONST (Valor: %s)\n", no->data.string_value); break;
        case NODE_BINARY_OP: printf("BINARY_OP (%s)\n", token_name(no->data.op_type)); break;
        case NODE_UNDEFINED: printf("EMPTY_STATEMENT\n"); break;
        default: printf("Noh desconhecido: %d\n", no->node_type); break;
    }

    // Chama a si mesma recursivamente para todos os filhos do nó atual
    ASTNode* filho = no->filho;
    while (filho != NULL) {
        imprimir_ast(filho, indent_level + 1);
        filho = filho->proximo_irmao;
    }
}


/**
 * @brief Ponto de entrada principal para o programa de teste do parser.
 * * Orquestra todo o fluxo de teste: carregamento de tokens, execução do parser,
 * impressão da AST e limpeza da memória.
 * * @param argc Número de argumentos da linha de comando (não utilizado).
 * @param argv Vetor de strings com os argumentos da linha de comando (não utilizado).
 * @return 0 em caso de sucesso, ou um código de erro em caso de falha.
 */
int main(int argc, char *argv[]) {
    printf("--- Iniciando Teste do Analisador Sintatico ---\n");

    // 1. Carrega os tokens do arquivo gerado pelo analisador léxico
    TokenList list;
    if (inicializar_token_list(&list, 100) != 0) {
        return -1;
    }
    printf("Carregando tokens de 'tokens.txt'...\n");
    if (carregar_tokens("tokens.txt", &list) != 0) {
        liberar_token_list(&list);
        return -1;
    }
    printf("%d tokens carregados.\n\n", list.tamanho);

    // 2. Prepara o parser
    PilhaTabelasSimbolos *symtab_stack = criar_pilha_tabelas(); // Necessário para a inicialização
    Parser parser;
    inicializar_parser(&parser, &list, symtab_stack);

    // 3. Executa o parser para gerar a AST
    printf("--- Executando Analise Sintatica ---\n");
    ASTNode* arvore_sintatica = parse(&parser); // Chama a função principal do parser
    printf("-----------------------------------\n\n");

    // 4. Se a AST foi gerada com sucesso, imprime para verificação
    if (arvore_sintatica != NULL) {
        printf("--- Imprimindo Arvore Sintatica Abstrata (AST) ---\n");
        imprimir_ast(arvore_sintatica, 0);
        printf("--------------------------------------------------\n\n");

        // 5. Libera a memória alocada para a AST para verificar se não há memory leaks
        printf("Liberando memoria da AST...\n");
        liberar_arvore(arvore_sintatica);
        printf("Memoria liberada.\n");
    } else {
        printf("A arvore sintatica nao foi gerada devido a um erro.\n");
    }

    // 6. Libera as outras estruturas
    liberar_token_list(&list);
    destruir_pilha_tabelas(symtab_stack);

    printf("\n--- Teste do Analisador Sintatico Finalizado ---\n");
    return 0;
}