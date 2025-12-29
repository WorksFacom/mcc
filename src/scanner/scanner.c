/**
 * @file scanner.c
 * @brief implementação do analisador léxico para a linguagem simple c.
 *
 * este arquivo contém as funções para ler o arquivo fonte, ignorar espaços e comentários,
 * e gerar tokens a partir de um arquivo de entrada. o analisador léxico é a primeira
 * etapa do compilador, convertendo o código em uma sequência de tokens.
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

/** @brief arquivo de entrada do arquivo fonte.
 *
 * ponteiro para o arquivo aberto que contém o código simple c a ser analisado.
 */
FILE *arquivo_fonte;
/** @brief contador da linha atual no arquivo fonte.
 *
 * rastreia a linha atual para relatar erros com precisão, inicializado em 1.
 */
int linha_atual = 1;

//funções auxiliares internas (não precisam estar no .h)
static char prox_char();
static char ignora_espacos_e_comentarios();

/** @brief lê o próximo caractere do arquivo de entrada. */
static char prox_char() {
    return fgetc(arquivo_fonte);
}

/** @brief inicializa o analisador léxico com o arquivo de entrada. */
int inicializar_scanner(const char *nome_arquivo) {
    arquivo_fonte = fopen(nome_arquivo,"r");
    if(arquivo_fonte == NULL){
        printf("erro ao abrir o arquivo %s\n",nome_arquivo);
        return -1;
    }
    linha_atual = 1;
    return 0; 
}

/** @brief avança no arquivo até encontrar um caractere que não seja espaço ou comentário. */
static char ignora_espacos_e_comentarios() {
    char c = prox_char();
    while (1) {
        //ignora espaços em branco
        while (isspace(c)) {
            if (c == '\n') linha_atual++;
            c = prox_char();
        }

        //verifica comentários
        if (c == '/') {
            char next_c = prox_char();
            if (next_c == '/') { //comentário de linha única
                do {
                    c = prox_char();
                } while (c != '\n' && c != EOF);
            } else if (next_c == '*') { //comentário de múltiplas linhas
                char prev_c = '\0';
                do {
                    prev_c = c;
                    c = prox_char();
                    if (c == '\n') linha_atual++;
                } while (!(prev_c == '*' && c == '/') && c != EOF);
                c = prox_char(); //consome o char após o '*/'
            } else {
                //não é um comentário, devolve o caractere e retorna a barra
                ungetc(next_c, arquivo_fonte);
                return c;
            }
        } else {
            //não é espaço nem comentário, retorna o caractere encontrado
            return c;
        }
    }
}

/** @brief gera o próximo token do arquivo fonte. */
Token proximo_token() {
    Token token;
    token.tipo = UNDEF;
    token.lexema[0] = '\0';

    char c = ignora_espacos_e_comentarios();
    token.linha = linha_atual;

    //fim de arquivo
    if (c == EOF) {
        token.tipo = END_OF_FILE;
        strcpy(token.lexema, "EOF");
        return token;
    }

    //identificadores e palavras-chave
    if (isalpha(c) || c == '_') {
        char buffer[101];
        int i = 0;
        do {
            buffer[i++] = c;
            c = prox_char();
        } while ((isalnum(c) || c == '_') && i < 100);
        ungetc(c, arquivo_fonte);
        buffer[i] = '\0';

        if (strcmp(buffer, "main") == 0) token.tipo = MAIN;
        else if (strcmp(buffer, "if") == 0) token.tipo = IF;
        else if (strcmp(buffer, "else") == 0) token.tipo = ELSE;
        else if (strcmp(buffer, "for") == 0) token.tipo = FOR;
        else if (strcmp(buffer, "return") == 0) token.tipo = RETURN;
        else if (strcmp(buffer, "int") == 0) token.tipo = INT;
        else if (strcmp(buffer, "char") == 0) token.tipo = CHAR;
        else if (strcmp(buffer, "print") == 0) token.tipo = PRINT;
        else token.tipo = ID;
        strcpy(token.lexema, buffer);
        return token;
    }

    //números inteiros (positivos)
    if (isdigit(c)) {
        char buffer[101];
        int i = 0;
        do {
            buffer[i++] = c;
            c = prox_char();
        } while (isdigit(c) && i < 100);
        ungetc(c, arquivo_fonte);
        buffer[i] = '\0';
        token.tipo = INTEGERCONST;
        strcpy(token.lexema, buffer);
        return token;
    }

    //constantes de caractere (ATUALIZADO PARA SUPORTAR ESCAPE \0, \n, etc)
    if (c == '\'') {
        char char_val = 0;
        c = prox_char();
        
        if (c == '\\') {
            // Sequência de escape detectada
            char next = prox_char();
            if (next == '0') char_val = '\0';
            else if (next == 'n') char_val = '\n';
            else if (next == 't') char_val = '\t';
            else if (next == '\\') char_val = '\\';
            else if (next == '\'') char_val = '\'';
            else char_val = next; // default
        } else {
            // Caractere normal
            char_val = c;
        }

        // Verifica fechamento
        if (prox_char() == '\'') {
            token.tipo = CHARCONST;
            token.lexema[0] = char_val;
            token.lexema[1] = '\0';
            return token;
        }
        
        // Se chegou aqui, é erro (não fechou aspas)
        strcpy(token.lexema, "'"); 
        token.tipo = UNDEF;
        return token;
    }

    //reconhecer strings
    if (c == '"') {
        char buffer[101];
        int i = 0;
        c = prox_char(); //pula a aspa de abertura
        
        while (c != '"' && c != '\n' && c != EOF && i < 100) {
            buffer[i++] = c;
            c = prox_char();
        }
        buffer[i] = '\0'; //finaliza a string

        if (c == '"') { //encontrou a aspa de fechamento
            token.tipo = STRINGCONST;
            strcpy(token.lexema, buffer);
        } else { //string não terminada
            token.tipo = UNDEF;
            strcpy(token.lexema, "String nao terminada");
        }
        return token;
    }
    
    //operadores e outros símbolos
    char next_c;
    switch(c) {
        case '=':
            next_c = prox_char();
            if (next_c == '=') { token.tipo = EQ; strcpy(token.lexema, "=="); }
            else { ungetc(next_c, arquivo_fonte); token.tipo = ASSIGN; strcpy(token.lexema, "="); }
            break;
        case '!':
            next_c = prox_char();
            if (next_c == '=') { token.tipo = NEQ; strcpy(token.lexema, "!="); }
            else { ungetc(next_c, arquivo_fonte); token.tipo = NOT; strcpy(token.lexema, "!"); }
            break;
        case '<':
            next_c = prox_char();
            if (next_c == '=') { token.tipo = LEQ; strcpy(token.lexema, "<="); }
            else { ungetc(next_c, arquivo_fonte); token.tipo = LT; strcpy(token.lexema, "<"); }
            break;
        case '>':
            next_c = prox_char();
            if (next_c == '=') { token.tipo = GEQ; strcpy(token.lexema, ">="); }
            else { ungetc(next_c, arquivo_fonte); token.tipo = GT; strcpy(token.lexema, ">"); }
            break;
        case '-':
            next_c = prox_char();
            if (isdigit(next_c)) { //é um número negativo
                char buffer[101];
                buffer[0] = c;
                int i = 1;
                do {
                    buffer[i++] = next_c;
                    next_c = prox_char();
                } while (isdigit(next_c) && i < 100);
                ungetc(next_c, arquivo_fonte);
                buffer[i] = '\0';
                token.tipo = INTEGERCONST;
                strcpy(token.lexema, buffer);
            } else { //é o operador MINUS
                ungetc(next_c, arquivo_fonte);
                token.tipo = MINUS;
                strcpy(token.lexema, "-");
            }
            break;
        // suporte a operadores lógicos && e ||
        case '&':
            next_c = prox_char();
            if (next_c == '&') { 
                token.tipo = AND; 
                strcpy(token.lexema, "&&"); 
            } else { 
                ungetc(next_c, arquivo_fonte); 
                token.tipo = UNDEF; 
                token.lexema[0] = '&'; token.lexema[1] = '\0';
            }
            break;
        case '|':
            next_c = prox_char();
            if (next_c == '|') { 
                token.tipo = OR; 
                strcpy(token.lexema, "||"); 
            } else { 
                ungetc(next_c, arquivo_fonte); 
                token.tipo = UNDEF;
                token.lexema[0] = '|'; token.lexema[1] = '\0';
            }
            break;
        case '+': strcpy(token.lexema, "+"); token.tipo = PLUS; break;
        case '*': strcpy(token.lexema, "*"); token.tipo = MUL; break;
        case '/': strcpy(token.lexema, "/"); token.tipo = DIV; break;
        case '%': strcpy(token.lexema, "%"); token.tipo = MOD; break;
        case ';': strcpy(token.lexema, ";"); token.tipo = SEMICOLON; break;
        case ',': strcpy(token.lexema, ","); token.tipo = COMMA; break;
        case '(': strcpy(token.lexema, "("); token.tipo = LPAREN; break;
        case ')': strcpy(token.lexema, ")"); token.tipo = RPAREN; break;
        case '{': strcpy(token.lexema, "{"); token.tipo = LBRACE; break;
        case '}': strcpy(token.lexema, "}"); token.tipo = RBRACE; break;
        case '[': strcpy(token.lexema, "["); token.tipo = LBRACKET; break;
        case ']': strcpy(token.lexema, "]"); token.tipo = RBRACKET; break;
        default:
            token.lexema[0] = c;
            token.lexema[1] = '\0';
            token.tipo = UNDEF;
            break;
    }

    return token;
}