#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include "tokens.h"

/**
 * @file symbol_table.h
 * @brief definições de tipos e funções para a tabela de símbolos.
 *
 * este arquivo define as estruturas de dados para a tabela de símbolos,
 * que é usada para armazenar informações sobre os identificadores (variáveis,
 * funções, etc.) encontrados no código fonte. a implementação usa uma
 * pilha de tabelas para gerenciar os escopos.
 */

#define MAX_SIMBOLOS 100
#define MAX_PARAMETROS 10

/** @brief armazena informações sobre um único símbolo (variável, função, etc.). */
typedef struct {
    char nome[100];                         ///< nome do identificador.
    TokenType tipo;                         ///< tipo do símbolo (ex: INT, CHAR).
    int is_function;                        ///< flag que indica se o símbolo é uma função.
    TokenType param_tipos[MAX_PARAMETROS];  ///< armazena os tipos dos parâmetros se for uma função.
    int num_parametros;                     ///< armazena o número de parâmetros se for uma função.
    int is_array;                           ///< flag que indica se o símbolo é um array.
    int array_size;                         ///< armazena o tamanho se for um array.
    int memory_offset;                      ///< o deslocamento (offset) do símbolo na pilha de execução.
    int is_parameter;                       ///< flag que indica se o símbolo é um parâmetro.
} Simbolo;

/** @brief representa uma tabela de símbolos para um único escopo. */
typedef struct {
    Simbolo simbolos[MAX_SIMBOLOS]; ///< um array de símbolos neste escopo.
    int tamanho;                    ///< número atual de símbolos na tabela.
} TabelaSimbolos;

/** @brief representa a pilha de tabelas de símbolos para controle de escopo. */
typedef struct {
    TabelaSimbolos *tabelas[MAX_SIMBOLOS]; ///< array de ponteiros para as tabelas de cada escopo.
    int topo;                             ///< indice para o topo da pilha.
} PilhaTabelasSimbolos;


/** @brief cria e inicializa uma nova pilha de tabelas de símbolos. */
PilhaTabelasSimbolos* criar_pilha_tabelas();

/** @brief libera toda a memória alocada por uma pilha de tabelas. */
void destruir_pilha_tabelas(PilhaTabelasSimbolos *pilha);

/** @brief empilha uma nova tabela de símbolos, criando um novo escopo. */
void empilhar_tabela(PilhaTabelasSimbolos *pilha);

/** @brief desempilha a tabela de símbolos do topo, fechando o escopo atual. */
void desempilhar_tabela(PilhaTabelasSimbolos *pilha);

/** * @brief adiciona um novo símbolo à tabela que está no topo da pilha. 
 * @return Retorna um ponteiro para o símbolo criado (Simbolo*) ou NULL em caso de erro.
 */
Simbolo* adicionar_simbolo(PilhaTabelasSimbolos *pilha, const char *nome, TokenType tipo, int is_function, int is_array, int array_size, int is_parameter);

/** @brief busca por um símbolo apenas na tabela do escopo atual (topo da pilha). */
Simbolo* buscar_simbolo_no_escopo_atual(PilhaTabelasSimbolos *pilha, const char *nome);

/** @brief busca por um símbolo em todas as tabelas, do escopo atual para o global. */
Simbolo* buscar_simbolo_em_todos_escopos(PilhaTabelasSimbolos *pilha, const char *nome);

/** * @brief Gera uma representação visual formatada da tabela de símbolos.
 * @param t Ponteiro para a tabela a ser impressa.
 * @param nome_escopo Uma string descrevendo o escopo (ex: "Global", "Funcao fatorial").
 * @param out Ponteiro para o arquivo onde será escrito (pode ser um arquivo .txt ou stdout).
 */
void imprimir_tabela(TabelaSimbolos* t, const char* nome_escopo, FILE* out);

#endif // SYMBOL_TABLE_H