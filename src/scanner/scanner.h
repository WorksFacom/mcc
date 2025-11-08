#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "tokens.h" 

/**
 * @file scanner.h
 * @brief definições do analisador léxico para a linguagem simple c.
 *
 * este arquivo declara as funções e variáveis globais usadas pelo analisador léxico,
 * responsável por ler o arquivo fonte e gerar tokens para o compilador.
 */

/** @brief arquivo de entrada do arquivo fonte.
 *
 * ponteiro para o arquivo aberto que contém o código simple c a ser analisado.
 */
extern FILE* arquivo_fonte;
/** @brief contador da linha atual no arquivo fonte.
 *
 * rastreia a linha atual para relatar erros com precisão.
 */
extern int linha_atual;

/**
 * @brief inicializa o analisador léxico com o arquivo de entrada.
 *
 * abre o arquivo especificado e prepara o analisador para leitura.
 * @param nome_arquivo nome do arquivo a ser analisado.
 * @return 0 em caso de sucesso, -1 se houver erro ao abrir o arquivo.
 */
int inicializar_scanner(const char* nome_arquivo);

/**
 * @brief gera o próximo token do arquivo fonte.
 *
 * lê caracteres do arquivo, ignora espaços e comentários, e identifica o próximo
 * token (ex.: identificadores, operadores, palavras reservadas).
 * @return estrutura token contendo tipo, lexema e linha do token identificado.
 */
Token proximo_token();

#endif //SCANNER_H