/** @file main.c
 *  @brief Programa principal do compilador da linguagem Simple C.
 *
 *  Este arquivo contém a função principal que inicializa o analisador léxico,
 *  lê o arquivo fonte, e gera a lista de tokens em um arquivo de saída.
 */

#include <stdio.h>
#include "scanner/scanner.h"

/** @brief Função principal do compilador Simple C.
 *
 *  Verifica argumentos, inicializa o analisador léxico, imprime o codigo fonte,
 *  e gera tokens no arquivo tokens.txt. Interrompe se encontrar um token indefinido.
 *  @param argc número de argumentos da linha de comando.
 *  @param argv vetor de strings com os argumentos.
 *  @return 0 em caso de sucesso, -1 se houver erro (ex.: arquivo inválido ou token indefinido).
 */
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


    FILE *saida = fopen("tokens.txt", "w");
    if (saida == NULL) {
        printf("erro ao criar arquivo de saida tokens.txt\n");
        fclose(arquivo_fonte);
        return -1;
    }

    rewind(arquivo_fonte);
    Token token;

    //imprimir todos os tokens no arquivo tokens.txt
    printf("\n\ntestando o analisador lexico:\n");
    token = proximo_token();
    while (token.tipo != END_OF_FILE) {
        //se encontrar um token UNDEF, imprime ele e encerra
        if (token.tipo == UNDEF) {
            fprintf(stderr, "Erro: Token indefinido '%s' encontrado na linha %d\n", token.lexema, token.linha);
            fclose(saida);
            fclose(arquivo_fonte);
            return -1; 
        }

        fprintf(saida, "Token: tipo = %d, lexema = '%s', linha = %d\n", token.tipo, token.lexema,token.linha);//imprimir todos os tokens
        token = proximo_token();
    }
    fprintf(saida, "Token: tipo = %d, lexema = '%s', linha = %d\n", token.tipo, token.lexema,token.linha);//imprimir o EOF

    printf("\n");
    fclose(saida);
    fclose(arquivo_fonte);
    return 0;
}