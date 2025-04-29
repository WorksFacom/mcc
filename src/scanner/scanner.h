/** @file scanner.h
 *  @brief Definições do analisador léxico para a linguagem Simple C.
 *
 *  Este arquivo declara as funções e variáveis globais usadas pelo analisador léxico,
 *  responsável por ler o arquivo fonte e gerar tokens para o compilador.
 *  
 */

#include <stdio.h>
#include "tokens.h" 

/** @brief Arquivo de entrada do arquivo fonte.
 *
 *  Ponteiro para o arquivo aberto que contém o código Simple C a ser analisado.
 */
extern FILE* arquivo_fonte;

/** @brief Inicializa o analisador léxico com o arquivo de entrada.
 *
 *  Abre o arquivo especificado e prepara o analisador para leitura.
 *  @param teste Nome do arquivo a ser analisado.
 *  @return 0 em caso de sucesso, -1 se houver erro ao abrir o arquivo.
 */
int inicializar_scanner(const char* nome_arquivo);

/** @brief Lê o próximo caractere do arquivo de entrada.
 *
 *  @return O caractere lido ou EOF se o fim do arquivo for atingido.
 */
char prox_char();

/** @brief Ignora espaços em branco e comentários no arquivo fonte.
 *
 *  Avança a leitura até encontrar um caractere significativo, pulando espaços,
 *  quebras de linha e comentários. Atualiza a linha atual.
 *  @param c Caractere inicial a ser processado.
 *  @return Próximo caractere significativo ou EOF se o fim do arquivo for atingido.
 */
char ignora(char c);

/** @brief Gera o próximo token do arquivo fonte.
 *
 *  Lê caracteres do arquivo, ignora espaços e comentários, e identifica o próximo
 *  token (ex.: identificadores, operadores, palavras reservadas).
 *  @return Estrutura Token contendo tipo, lexema e linha do token identificado.
 */
Token proximo_token();