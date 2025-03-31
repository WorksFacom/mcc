#include <stdio.h>
#include "scanner/scanner.h"


int main(int argc, char *argv[]) {
    if (argc < 2) {
    printf("arquivo nao encontrado\n");
    return -1;
    }

    if (inicializar_scanner(argv[1]) != 0) {
        return -1;
    }

    //imprimir o arquivo fonte
    printf("programa fonte:\n ");
    char c = prox_char();
    while (c != EOF) {
        putchar(c);
        c = prox_char();
    }

   
    rewind(arquivo_fonte);
    Token token;

    //testar o reconhecimento de todos os tokens
    printf("\n\ntestando o analisador lexico:\n");
    token = proximo_token();
    while (token.tipo != END_OF_FILE) {
        printf("token: tipo = %d, lexema = '%s'\n", token.tipo, token.lexema);//imprimir todos os tokens
        token = proximo_token();
    }
    printf("token: tipo = %d, lexema = '%s'\n", token.tipo, token.lexema);//imprimir o EOF


    printf("\n");
    fclose(arquivo_fonte);
    return 0;
}