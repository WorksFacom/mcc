#include <stdio.h>
#include <ctype.h>
#include "scanner.h"

FILE *arquivo_fonte;

char prox_char(){
    return fgetc(arquivo_fonte);
}

int inicializar_scanner(const char *nome_arquivo) {
    arquivo_fonte=fopen(nome_arquivo,"r");
    if(arquivo_fonte == NULL){
        printf("erro ao abrir o arquivo %s\n",nome_arquivo);
        return -1;
    }
    return 0; 
}

char ignora(char c) {
    while(c != EOF) {
        while (isspace(c) && c!=EOF) {
            c = prox_char();
        }
    if(c == '/'){
        c = prox_char();
        if (c == '/') {

            while(c != '\n' && c!=EOF)
                c = prox_char();
            if(c== '\n')
                c = prox_char();
            }
            else if(c == '*') {

                c = prox_char();
                while(c != EOF) {
                    if(c == '*') {
                        c = prox_char();
                        if(c=='/'){
                            c = prox_char();
                            break;
                        }
                    }else c = prox_char();
                }
            }else{
                break;
            }
        }
        break; 
    }
    return c;
}
