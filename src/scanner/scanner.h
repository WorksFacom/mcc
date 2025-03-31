#include <stdio.h>
#include "tokens.h" 

extern FILE* arquivo_fonte;

int inicializar_scanner(const char* nome_arquivo);
char prox_char();
char ignora(char c);
Token proximo_token();