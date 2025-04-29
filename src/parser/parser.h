#include "tokens.h"

typedef struct {
    Token *tokens;
    int tamanho;
    int capacidade;
} TokenList;

int inicializar_token_list(TokenList *lista, int capacidade_inicial);
void liberar_token_list(TokenList *lista);
int carregar_tokens(const char *nome_arquivo, TokenList *lista);
