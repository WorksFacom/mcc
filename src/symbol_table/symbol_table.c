#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

/**
 * @file symbol_table.c
 * @brief implementação das funções de gerenciamento da tabela de símbolos.
 *
 * contém a lógica para criar, destruir e manipular a pilha de tabelas de
 * símbolos, permitindo a correta gestão de escopos aninhados.
 */

/**
 * @brief cria e inicializa uma nova pilha de tabelas de símbolos.
 *
 * aloca memória para a pilha, inicializa o topo e já empilha a primeira
 * tabela, que representará o escopo global.
 * @return ponteiro para a nova pilha de tabelas de símbolos.
 */
PilhaTabelasSimbolos* criar_pilha_tabelas() {
    PilhaTabelasSimbolos *pilha = malloc(sizeof(PilhaTabelasSimbolos));
    if (pilha == NULL) {
        printf("Erro: Falha ao alocar memória para pilha de tabelas\n");
        exit(EXIT_FAILURE);
    }
    pilha->topo = -1;
    empilhar_tabela(pilha); //empilha a tabela de escopo global
    return pilha;
}

/**
 * @brief libera toda a memória alocada por uma pilha de tabelas.
 * @param pilha ponteiro para a pilha a ser destruída.
 */
void destruir_pilha_tabelas(PilhaTabelasSimbolos *pilha) {
    while (pilha->topo >= 0) {
        desempilhar_tabela(pilha);
    }
    free(pilha);
}

/**
 * @brief empilha uma nova tabela de símbolos, criando um novo escopo.
 * @param pilha ponteiro para a pilha onde a nova tabela será adicionada.
 */
void empilhar_tabela(PilhaTabelasSimbolos *pilha) {
    if (pilha->topo + 1 >= MAX_SIMBOLOS) {
        printf("Erro: Pilha de tabelas cheia\n");
        exit(EXIT_FAILURE);
    }
    TabelaSimbolos *nova_tabela = malloc(sizeof(TabelaSimbolos));
    if (nova_tabela == NULL) {
        printf("Erro: Falha ao alocar memória para nova tabela\n");
        exit(EXIT_FAILURE);
    }
    nova_tabela->tamanho = 0;
    pilha->tabelas[++pilha->topo] = nova_tabela;
}

/** @brief desempilha a tabela de símbolos do topo, fechando o escopo atual. */
void desempilhar_tabela(PilhaTabelasSimbolos *pilha) {
    if (pilha->topo < 0) {
        printf("Erro: Pilha de tabelas vazia\n");
        exit(EXIT_FAILURE);
    }
    
    //NÃO libera a memória aqui. Apenas decrementa o ponteiro do topo.
    //a limpeza de memória será feita por destruir_pilha_tabelas() no main.c
    pilha->topo--;
}

/**
 * @brief adiciona um novo símbolo à tabela que está no topo da pilha.
 * @param pilha a pilha de tabelas de símbolos.
 * @param nome o nome do símbolo a ser adicionado.
 * @param tipo o tipo do símbolo (INT, CHAR, etc.).
 * @param is_function flag que indica se é uma função.
 * @param is_array flag que indica se é um array.
 * @param array_size o tamanho do array (se aplicável).
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int adicionar_simbolo(PilhaTabelasSimbolos *pilha, const char *nome, TokenType tipo, int is_function, int is_array, int array_size, int is_parameter) {
    if (pilha->topo < 0) {
        printf("Erro: Nenhuma tabela de símbolos disponível\n");
        return -1;
    }
    TabelaSimbolos *tabela = pilha->tabelas[pilha->topo];
    if (tabela->tamanho >= MAX_SIMBOLOS) {
        printf("Erro: Tabela de símbolos cheia\n");
        return -1;
    }
    Simbolo *s = &tabela->simbolos[tabela->tamanho];
    strncpy(s->nome, nome, 100);
    s->tipo = tipo;
    s->is_function = is_function;
    s->num_parametros = 0;
    //salva as novas informações do array
    s->is_array = is_array;
    s->array_size = array_size;

    s->is_parameter = is_parameter;
    s->memory_offset = 0; //será calculado pelo semantic.c
    
    tabela->tamanho++;
    return 0;
}
/**
 * @brief busca por um símbolo apenas na tabela do escopo atual (topo da pilha).
 * @param pilha a pilha de tabelas de símbolos.
 * @param nome o nome do símbolo a ser buscado.
 * @return ponteiro para o Simbolo se encontrado, caso contrário NULL.
 */
Simbolo* buscar_simbolo_no_escopo_atual(PilhaTabelasSimbolos *pilha, const char *nome) {
    if (pilha->topo < 0) {
        return NULL;
    }
    TabelaSimbolos *tabela = pilha->tabelas[pilha->topo];
    for (int i = 0; i < tabela->tamanho; i++) {
        if (strcmp(tabela->simbolos[i].nome, nome) == 0) {
            return &tabela->simbolos[i];
        }
    }
    return NULL;
}

/**
 * @brief busca por um símbolo em todas as tabelas, do escopo atual para o global.
 * @param pilha a pilha de tabelas de símbolos.
 * @param nome o nome do símbolo a ser buscado.
 * @return ponteiro para o primeiro Simbolo encontrado, caso contrário NULL.
 */
Simbolo* buscar_simbolo_em_todos_escopos(PilhaTabelasSimbolos *pilha, const char *nome) {
    //itera da tabela do topo (escopo local) para a base (escopo global)
    for (int i = pilha->topo; i >= 0; i--) {
        TabelaSimbolos *tabela = pilha->tabelas[i];
        for (int j = 0; j < tabela->tamanho; j++) {
            if (strcmp(tabela->simbolos[j].nome, nome) == 0) {
                return &tabela->simbolos[j];
            }
        }
    }
    return NULL;
}