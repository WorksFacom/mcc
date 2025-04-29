#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

int inicializar_token_list(TokenList *lista, int capacidade_inicial) {
    lista->tokens = malloc(capacidade_inicial * sizeof(Token));
    if (lista->tokens == NULL) {
        printf("Erro: Falha ao alocar memória para lista de tokens\n");
        return -1;
    }
    lista->tamanho = 0;
    lista->capacidade = capacidade_inicial;
    return 0;
}

void liberar_token_list(TokenList *lista) {
    free(lista->tokens);
    lista->tokens = NULL;
    lista->tamanho = 0;
    lista->capacidade = 0;
}

int carregar_tokens(const char *nome_arquivo, TokenList *lista) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        printf("Erro: Não foi possível abrir o arquivo %s\n", nome_arquivo);
        return -1;
    }

    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        int tipo, linha_num;
        char lexema[100];

        if (sscanf(linha, "Token: tipo = %d, lexema = '%[^']', linha = %d", &tipo, lexema, &linha_num) != 3) {
            continue;
        }

        if (lista->tamanho >= lista->capacidade) {
            lista->capacidade *= 2;
            Token *nova_lista = realloc(lista->tokens, lista->capacidade * sizeof(Token));
            if (nova_lista == NULL) {
                printf("Erro: Falha ao redimensionar lista de tokens\n");
                fclose(arquivo);
                return -1;
            }
            lista->tokens = nova_lista;
        }

        lista->tokens[lista->tamanho].tipo = (TokenType)tipo;
        strncpy(lista->tokens[lista->tamanho].lexema, lexema, 50);
        lista->tokens[lista->tamanho].linha = linha_num;
        lista->tamanho++;
    }

    fclose(arquivo);
    return 0;
}