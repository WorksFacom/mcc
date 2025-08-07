/**
 * @file main.c
 * @brief programa principal e orquestrador do compilador.
 *
 * este arquivo contém a função 'main' que controla o fluxo de compilação,
 * invocando as fases de análise léxica, sintática e semântica em sequência.
 * ele também lida com argumentos de linha de comando para permitir a inspeção
 * da saída de cada fase individualmente.
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "semantic/semantic.h"

//protótipos de funções auxiliares
void imprimir_tokens(TokenList* tokens);
void salvar_tokens_em_arquivo(TokenList* tokens, bool verbose);
void imprimir_ast(ASTNode* arvore);
void salvar_ast(ASTNode* arvore, bool verbose);
int executar_analise_lexica(const char* nome_arquivo, TokenList* lista);

/**
 * @brief orquestra a fase de análise léxica.
 *
 * encapsula a lógica de inicializar o scanner, chamar a função proximo_token()
 * em um loop até o fim do arquivo e preencher uma TokenList com os resultados.
 *
 * @param nome_arquivo o arquivo fonte a ser analisado.
 * @param lista a lista de tokens que será preenchida.
 * @return 0 em caso de sucesso, -1 em caso de erro léxico.
 */
int executar_analise_lexica(const char* nome_arquivo, TokenList* lista) {
    if (inicializar_scanner(nome_arquivo) != 0) {
        return -1; //erro ao abrir o arquivo
    }

    Token token;
    do {
        token = proximo_token();
        adicionar_token(lista, token); //adiciona o token à lista em memória
        if (token.tipo == UNDEF) {
            fprintf(stderr, "Erro Lexico: Token indefinido '%s' encontrado na linha %d\n", token.lexema, token.linha);
            return -1;
        }
    } while (token.tipo != END_OF_FILE);

    return 0;
}

/**
 * @brief ponto de entrada principal do compilador.
 *
 * orquestra todo o fluxo de compilação e lida com os argumentos de
 * linha de comando para controlar a execução das fases.
 *
 * @param argc número de argumentos da linha de comando.
 * @param argv vetor de argumentos da linha de comando.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Erro: Arquivo fonte nao especificado.\n");
        fprintf(stderr, "Uso: %s [--scan | --parse | --semantic] <arquivo_fonte.cmm>\n", argv[0]);
        return -1;
    }

    //determina o modo de operação e o nome do arquivo
    char* modo = "completo";
    char* nome_arquivo = argv[1];

    if (argc > 2) {
        modo = argv[1];
        nome_arquivo = argv[2];
    }
    
    //fase 1: análise léxica
    TokenList tokens;
    inicializar_token_list(&tokens, 100);
    if (executar_analise_lexica(nome_arquivo, &tokens) != 0) {
        liberar_token_list(&tokens);
        return -1; //encerra se houver erro léxico
    }
    //salva o arquivo de tokens silenciosamente em todos os modos, exceto 'scan'
    if (strcmp(modo, "--scan") != 0) {
        salvar_tokens_em_arquivo(&tokens, false);
    }
    printf("Fase 1 (Lexica) concluida: %d tokens gerados.\n", tokens.tamanho);


    //modo scan
    if (strcmp(modo, "--scan") == 0) {
        imprimir_tokens(&tokens);
        salvar_tokens_em_arquivo(&tokens, true); //salva e imprime a mensagem de sucesso
        liberar_token_list(&tokens);
        return 0; 
    }

    //fase 2: análise sintática
    PilhaTabelasSimbolos* pilha_simbolos = criar_pilha_tabelas();
    Parser parser;
    inicializar_parser(&parser, &tokens, pilha_simbolos);
    ASTNode* arvore = parse(&parser);
    //salva o arquivo da ast silenciosamente em todos os modos, exceto 'parse'
    if (strcmp(modo, "--parse") != 0) {
        salvar_ast(arvore, false);
    }
    printf("Fase 2 (Sintatica) concluida: AST gerada.\n");

    //modo parse
    if (strcmp(modo, "--parse") == 0) {
        imprimir_ast(arvore);
        salvar_ast(arvore, true); //salva e imprime a mensagem de sucesso
        liberar_arvore(arvore);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0; 
    }

    //fase 3: análise semântica
    analisar_semanticamente(arvore, pilha_simbolos);
    printf("Fase 3 (Semantica) concluida: Codigo validado.\n");


    //modo semantic
    if (strcmp(modo, "--semantic") == 0) {
        printf("\n--- MODO DE ANALISE SEMANTICA CONCLUIDO ---\n");
        liberar_arvore(arvore);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0;
    }
    
    //modo completo (default)
    printf("\n--- MODO DE COMPILACAO COMPLETA ---\n");
    printf("Todas as fases de analise foram concluidas com sucesso.\n");
    
    //limpeza de memória
    liberar_arvore(arvore);
    destruir_pilha_tabelas(pilha_simbolos);
    liberar_token_list(&tokens);

    printf("\nCompilacao finalizada!\n");
    return 0;
}

/** @brief imprime o conteúdo de uma TokenList no console. */
void imprimir_tokens(TokenList* tokens) {
    printf("--- LISTA DE TOKENS GERADOS ---\n");
    for (int i = 0; i < tokens->tamanho; i++) {
        printf("Token: tipo = %d, lexema = '%s', linha = %d\n", 
               tokens->tokens[i].tipo, 
               tokens->tokens[i].lexema, 
               tokens->tokens[i].linha);
    }
    printf("--------------------------------\n");
}

/** @brief salva o conteúdo de uma TokenList no arquivo 'tokens.txt'. */
void salvar_tokens_em_arquivo(TokenList* tokens, bool verbose) {
    FILE *saida = fopen("tokens.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo tokens.txt\n");
        return;
    }
    for (int i = 0; i < tokens->tamanho; i++) {
        fprintf(saida, "Token: tipo = %d, lexema = '%s', linha = %d\n", 
                tokens->tokens[i].tipo, 
                tokens->tokens[i].lexema, 
                tokens->tokens[i].linha);
    }
    fclose(saida);
    if (verbose) {
        printf("Arquivo 'tokens.txt' gerado com sucesso.\n");
    }
}

/** @brief função recursiva para imprimir ou salvar a AST de forma legível. */
void processar_ast_recursivamente(ASTNode *no, int indent_level, FILE *arquivo) {
    if (no == NULL) return;
    
    char indent_str[100] = "";
    for (int i = 0; i < indent_level; ++i) {
        strcat(indent_str, "  ");
    }

    #define PRINT(format, ...) do { if (arquivo) fprintf(arquivo, format, ##__VA_ARGS__); else printf(format, ##__VA_ARGS__); } while(0)
    
    PRINT("%s", indent_str);
    switch (no->node_type) {
        case NODE_PROGRAM: PRINT("NO_PROGRAMA\n"); break;
        case NODE_FUNCTION_DEF: PRINT("NO_DEFINICAO_FUNCAO\n"); break;
        case NODE_VAR_DECL: PRINT("NO_DECLARACAO_VARIAVEL\n"); break;
        case NODE_ASSIGN: PRINT("NO_ATRIBUICAO\n"); break;
        case NODE_IF: PRINT("NO_IF\n"); break;
        case NODE_FOR: PRINT("NO_FOR\n"); break;
        case NODE_RETURN: PRINT("NO_RETORNO\n"); break;
        case NODE_BLOCK: PRINT("NO_BLOCO\n"); break;
        case NODE_PARAM_LIST: PRINT("NO_LISTA_PARAMETROS\n"); break;
        case NODE_ARG_LIST: PRINT("NO_LISTA_ARGUMENTOS\n"); break;
        case NODE_ARRAY_ACCESS: PRINT("NO_ACESSO_ARRAY\n"); break;
        case NODE_CALL: PRINT("NO_CHAMADA_FUNCAO\n"); break;
        case NODE_TYPE: PRINT("NO_TIPO (%s)\n", no->data.op_type == INT ? "INT" : "CHAR"); break;
        case NODE_ID: PRINT("NO_ID (Nome: %s)\n", no->data.string_value); break;
        case NODE_INTEGER_CONST: PRINT("NO_CONST_INT (Valor: %ld)\n", no->data.int_value); break;
        case NODE_CHAR_CONST: PRINT("NO_CONST_CHAR (Valor: '%c')\n", no->data.char_value); break;
        case NODE_STRING_CONST: PRINT("NO_CONST_STRING (Valor: \"%s\")\n", no->data.string_value); break; 
        case NODE_BINARY_OP: PRINT("NO_OP_BINARIA (%s)\n", token_name(no->data.op_type)); break;
        case NODE_UNDEFINED: PRINT("NO_INSTRUCAO_VAZIA\n"); break;
        default: PRINT("NO_DESCONHECIDO (tipo: %d)\n", no->node_type); break;
    }
    #undef PRINT

    ASTNode* filho = no->filho;
    while (filho != NULL) {
        processar_ast_recursivamente(filho, indent_level + 1, arquivo);
        filho = filho->proximo_irmao;
    }
}

/** @brief imprime a Árvore Sintática Abstrata (AST) no console. */
void imprimir_ast(ASTNode* arvore) {
    printf("--- ARVORE SINTATICA ABSTRATA (AST) ---\n");
    processar_ast_recursivamente(arvore, 0, NULL); //NULL no arquivo indica impressão no console
    printf("---------------------------------------\n");
}

/** @brief salva a Árvore Sintática Abstrata (AST) no arquivo 'ast.txt'. */
void salvar_ast(ASTNode* arvore, bool verbose) {
    FILE* saida = fopen("ast.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo ast.txt\n");
        return;
    }
    processar_ast_recursivamente(arvore, 0, saida);
    fclose(saida);
    if (verbose) {
        printf("Arquivo 'ast.txt' gerado com sucesso.\n");
    }
}