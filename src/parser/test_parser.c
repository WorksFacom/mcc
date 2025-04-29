#include <stdio.h>
#include "parser.h"

int main() {
    TokenList lista;
    if (inicializar_token_list(&lista, 10) != 0) {
        return -1;
    }

    if (carregar_tokens("tokens.txt", &lista) != 0) {
        liberar_token_list(&lista);
        return -1;
    }

    printf("Tokens lidos:\n");
    for (int i = 0; i < lista.tamanho; i++) {
        printf("Token %d: tipo = %d, lexema = '%s', linha = %d\n",
               i, lista.tokens[i].tipo, lista.tokens[i].lexema, lista.tokens[i].linha);
    }

    liberar_token_list(&lista);
    return 0;
}