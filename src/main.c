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

    // imprimir o arquivo fonte
    printf("programa fonte:\n ");
    char c = prox_char();
    while (c != EOF) {
        putchar(c);
        c = prox_char();
    }

   
    rewind(arquivo_fonte);
    c = prox_char(); 

    //testar a funcao ignora espacos e comentarios
    printf("\ntestando a funcao de ignorar espacos e comentarios:\n\n");
    while(c != EOF){
        c = ignora(c);
        if( c!=EOF ){
            putchar(c);
            c = prox_char();
        }
    }

    printf("\n");
    fclose(arquivo_fonte);
    return 0;
}