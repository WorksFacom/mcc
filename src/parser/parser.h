#ifndef PARSER_H
#define PARSER_H

#include "tokens.h"
#include "symbol_table.h"
#include "ast.h"

/**
 * @file parser.h
 * @brief declarações de tipos e funções para o analisador sintático (parser).
 *
 * este arquivo define a interface pública do módulo do parser. ele inclui a
 * definição da estrutura de estado 'parser', a lista de tokens e os protótipos
 * de todas as funções que compõem o parser recursivo descendente.
 */

/** @brief estrutura para armazenar dinamicamente a lista de tokens lida do scanner. */
typedef struct {
    Token *tokens;      ///< ponteiro para o array de tokens.
    int tamanho;        ///< número atual de tokens na lista.
    int capacidade;     ///< capacidade atual do array de tokens.
} TokenList;

/** @brief estrutura que mantém o estado completo do parser durante a análise. */
typedef struct {
    TokenList *lista;                   ///< ponteiro para a lista de todos os tokens do programa.
    int current;                        ///< índice do token atual que está sendo processado.
    Token *current_token;               ///< ponteiro para o token atual, para acesso rápido.
    PilhaTabelasSimbolos *symtab_stack; ///< ponteiro para a pilha de tabelas de símbolos.
} Parser;

//--- protótipos de funções de utilidade e controle ---

/**
 * @brief inicializa uma lista de tokens com uma capacidade inicial.
 * @param lista ponteiro para a TokenList a ser inicializada.
 * @param capacidade_inicial o número inicial de tokens que a lista pode armazenar.
 * @return 0 em caso de sucesso, -1 se a alocação de memória falhar.
 */
int inicializar_token_list(TokenList *lista, int capacidade_inicial);

/**
 * @brief libera a memória alocada para uma TokenList.
 * @param lista ponteiro para a TokenList a ser liberada.
 */
void liberar_token_list(TokenList *lista);

/**
 * @brief carrega tokens de um arquivo de texto para uma TokenList.
 * @param nome_arquivo o caminho para o arquivo de tokens.
 * @param lista ponteiro para a TokenList onde os tokens serão armazenados.
 * @return 0 em caso de sucesso, -1 se o arquivo não puder ser aberto ou em caso de erro de memória.
 */
int carregar_tokens(const char *nome_arquivo, TokenList *lista);

/**
 * @brief prepara a estrutura do parser para iniciar a análise a partir de uma lista de tokens.
 * @param parser ponteiro para a estrutura parser a ser inicializada.
 * @param lista ponteiro para a TokenList contendo os tokens a serem analisados.
 * @param symtab_stack ponteiro para a pilha de tabelas de símbolos.
 */
void inicializar_parser(Parser *parser, TokenList *lista, PilhaTabelasSimbolos *symtab_stack);

/**
 * @brief reporta um erro sintático e encerra o programa.
 * @param parser ponteiro para o estado do parser, para obter informações de contexto (linha, token).
 * @param msg a mensagem de erro específica a ser exibida.
 */
void error(Parser *parser, const char *msg);

/**
 * @brief converte um TokenType em uma string legível para impressão.
 * @param tipo o valor do enum TokenType a ser convertido.
 * @return uma string constante representando o nome do token.
 */
const char* token_name(TokenType tipo);

/** @brief adiciona um token ao final de uma TokenList, redimensionando se necessário. */
void adicionar_token(TokenList* lista, Token token);

//--- protótipo da função principal do parser ---

/**
 * @brief função principal que dispara o processo de análise sintática.
 * @param parser ponteiro para o estado do parser já inicializado.
 * @return um ponteiro para o nó raiz da ast gerada.
 */
ASTNode* parse(Parser *parser);

//--- protótipos das funções da gramática ---

/** @brief processa a regra 'program', o ponto de entrada da gramática. */
ASTNode* program(Parser *parser);
/** @brief processa a definição completa de uma função. */
ASTNode* function(Parser *parser);
/** @brief processa a declaração de uma variável. */
ASTNode* var_declaration(Parser *parser);
/** @brief processa um especificador de tipo (INT ou CHAR). */
ASTNode* type(Parser *parser);
/** @brief processa uma única instrução ou um bloco de instruções. */
ASTNode* statement(Parser *parser);
/** @brief processa uma expressão (incluindo relacionais, aritméticas, etc.). */
ASTNode* expression(Parser *parser);
/** @brief processa uma expressão aritmética de soma e subtração. */
ASTNode* arithmetic_expression(Parser* parser);
/** @brief processa um termo (multiplicação/divisão). */
ASTNode* term(Parser* parser);
/** @brief processa um fator (número, ID, parênteses). */
ASTNode* factor(Parser* parser);
/** @brief processa uma instrução de atribuição. */
ASTNode* assign(Parser *parser);
/** @brief processa uma instrução condicional 'if-else'. */
ASTNode* if_statement(Parser *parser);
/** @brief processa uma instrução de laço 'for'. */
ASTNode* for_statement(Parser *parser);
/** @brief processa a lista de parâmetros na definição de uma função. */
ASTNode* param_list(Parser *parser);
/** @brief processa a lista de argumentos em uma chamada de função. */
ASTNode* arg_list(Parser *parser);

#endif //PARSER_H