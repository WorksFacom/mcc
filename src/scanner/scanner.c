/**
 * @file scanner.c
 * @brief implementação do analisador léxico para a linguagem Micro C.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "scanner.h"

FILE* arquivo_fonte = NULL;
int linha_atual = 1;

static int erro_comentario = 0;

/** @brief lê um byte preservando o valor especial EOF. */
static int prox_char(void) {
    return fgetc(arquivo_fonte);
}

/** @brief devolve um byte válido ao fluxo de entrada. */
static void devolver_char(int c) {
    if (c != EOF) ungetc(c, arquivo_fonte);
}

/** @brief cria um token inicializado para a linha atual. */
static Token criar_token(void) {
    Token token = {0};
    token.tipo = UNDEF;
    token.linha = linha_atual;
    return token;
}

/** @brief define uma mensagem curta para um token inválido. */
static Token token_erro(const char* mensagem, int linha) {
    Token token = criar_token();
    token.linha = linha;
    snprintf(token.lexema, sizeof(token.lexema), "%s", mensagem);
    return token;
}

/** @brief inicializa o scanner com um novo arquivo-fonte. */
int inicializar_scanner(const char* nome_arquivo) {
    finalizar_scanner();
    arquivo_fonte = fopen(nome_arquivo, "r");
    if (arquivo_fonte == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", nome_arquivo);
        return -1;
    }
    linha_atual = 1;
    erro_comentario = 0;
    return 0;
}

/** @brief fecha o arquivo-fonte e reinicializa o estado do scanner. */
void finalizar_scanner(void) {
    if (arquivo_fonte != NULL) fclose(arquivo_fonte);
    arquivo_fonte = NULL;
    linha_atual = 1;
    erro_comentario = 0;
}

/** @brief ignora espaços e comentários, retornando o primeiro byte útil. */
static int ignorar_espacos_e_comentarios(void) {
    int c = prox_char();

    while (1) {
        while (c != EOF && isspace((unsigned char)c)) {
            if (c == '\n') linha_atual++;
            c = prox_char();
        }

        if (c != '/') return c;

        int seguinte = prox_char();
        if (seguinte == '/') {
            while ((c = prox_char()) != '\n' && c != EOF) {
                //consome o comentário até o final da linha
            }
            if (c == '\n') linha_atual++;
            c = prox_char();
            continue;
        }

        if (seguinte == '*') {
            int anterior = 0;
            c = prox_char();
            while (c != EOF && !(anterior == '*' && c == '/')) {
                if (c == '\n') linha_atual++;
                anterior = c;
                c = prox_char();
            }
            if (c == EOF) {
                erro_comentario = 1;
                return EOF;
            }
            c = prox_char();
            continue;
        }

        devolver_char(seguinte);
        return c;
    }
}

/** @brief classifica um identificador ou palavra reservada. */
static Token ler_identificador(int primeiro, int linha) {
    Token token = criar_token();
    token.linha = linha;
    size_t tamanho = 0;
    int c = primeiro;

    while (c != EOF && (isalnum((unsigned char)c) || c == '_')) {
        if (tamanho + 1 < sizeof(token.lexema)) token.lexema[tamanho] = (char)c;
        tamanho++;
        c = prox_char();
    }
    devolver_char(c);

    if (tamanho > MAX_ID_LEN) {
        return token_erro("Identificador excede o limite", linha);
    }
    token.lexema[tamanho] = '\0';

    if (strcmp(token.lexema, "main") == 0) token.tipo = MAIN;
    else if (strcmp(token.lexema, "if") == 0) token.tipo = IF;
    else if (strcmp(token.lexema, "else") == 0) token.tipo = ELSE;
    else if (strcmp(token.lexema, "for") == 0) token.tipo = FOR;
    else if (strcmp(token.lexema, "return") == 0) token.tipo = RETURN;
    else if (strcmp(token.lexema, "int") == 0) token.tipo = INT;
    else if (strcmp(token.lexema, "char") == 0) token.tipo = CHAR;
    else if (strcmp(token.lexema, "print") == 0) token.tipo = PRINT;
    else token.tipo = ID;
    return token;
}

/** @brief lê uma constante inteira sem incorporar seu sinal. */
static Token ler_inteiro(int primeiro, int linha) {
    Token token = criar_token();
    token.linha = linha;
    size_t tamanho = 0;
    int c = primeiro;

    while (c != EOF && isdigit((unsigned char)c)) {
        if (tamanho + 1 < sizeof(token.lexema)) token.lexema[tamanho] = (char)c;
        tamanho++;
        c = prox_char();
    }
    devolver_char(c);

    if (tamanho > (size_t)(MAX_INT_LEN - 2)) {
        return token_erro("Constante inteira excede o limite", linha);
    }
    token.lexema[tamanho] = '\0';
    token.tipo = INTEGERCONST;
    return token;
}

/** @brief converte uma sequência de escape em seu byte correspondente. */
static int decodificar_escape(int c, int* valido) {
    *valido = 1;
    switch (c) {
        case '0': return '\0';
        case 'n': return '\n';
        case 't': return '\t';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '"';
        default:
            *valido = 0;
            return c;
    }
}

/** @brief lê uma constante de caractere, incluindo escapes conhecidos. */
static Token ler_caractere(int linha) {
    Token token = criar_token();
    token.linha = linha;
    int c = prox_char();
    int valor = c;
    int valido = 1;

    if (c == '\\') valor = decodificar_escape(prox_char(), &valido);
    int fechamento = prox_char();
    if (!valido || c == EOF || c == '\n' || fechamento != '\'') {
        if (c == '\n' || fechamento == '\n') linha_atual++;
        return token_erro("Constante de caractere invalida", linha);
    }

    token.tipo = CHARCONST;
    token.lexema[0] = (char)valor;
    token.lexema[1] = '\0';
    return token;
}

/** @brief lê e decodifica uma string literal. */
static Token ler_string(int linha) {
    Token token = criar_token();
    token.linha = linha;
    size_t tamanho = 0;
    int c = prox_char();

    while (c != '"' && c != '\n' && c != EOF) {
        int valor = c;
        if (c == '\\') {
            int valido = 1;
            int escape = prox_char();
            valor = decodificar_escape(escape, &valido);
            if (!valido || valor == '\0') {
                return token_erro("Escape invalido em string", linha);
            }
        }

        if (tamanho + 1 >= sizeof(token.lexema) || tamanho + 1 >= MAX_STR_LEN) {
            while (c != '"' && c != '\n' && c != EOF) c = prox_char();
            if (c == '\n') linha_atual++;
            return token_erro("String excede o limite", linha);
        }
        token.lexema[tamanho++] = (char)valor;
        c = prox_char();
    }

    if (c != '"') {
        if (c == '\n') linha_atual++;
        return token_erro("String nao terminada", linha);
    }
    token.lexema[tamanho] = '\0';
    token.tipo = STRINGCONST;
    return token;
}

/** @brief gera o próximo token do arquivo-fonte. */
Token proximo_token(void) {
    int c = ignorar_espacos_e_comentarios();
    int linha = linha_atual;
    Token token = criar_token();
    token.linha = linha;

    if (erro_comentario) return token_erro("Comentario de bloco nao terminado", linha);
    if (c == EOF) {
        token.tipo = END_OF_FILE;
        snprintf(token.lexema, sizeof(token.lexema), "EOF");
        return token;
    }
    if (isalpha((unsigned char)c) || c == '_') return ler_identificador(c, linha);
    if (isdigit((unsigned char)c)) return ler_inteiro(c, linha);
    if (c == '\'') return ler_caractere(linha);
    if (c == '"') return ler_string(linha);

    int seguinte;
    switch (c) {
        case '=':
            seguinte = prox_char();
            if (seguinte == '=') { token.tipo = EQ; strcpy(token.lexema, "=="); }
            else { devolver_char(seguinte); token.tipo = ASSIGN; strcpy(token.lexema, "="); }
            break;
        case '!':
            seguinte = prox_char();
            if (seguinte == '=') { token.tipo = NEQ; strcpy(token.lexema, "!="); }
            else { devolver_char(seguinte); token.tipo = NOT; strcpy(token.lexema, "!"); }
            break;
        case '<':
            seguinte = prox_char();
            if (seguinte == '=') { token.tipo = LEQ; strcpy(token.lexema, "<="); }
            else { devolver_char(seguinte); token.tipo = LT; strcpy(token.lexema, "<"); }
            break;
        case '>':
            seguinte = prox_char();
            if (seguinte == '=') { token.tipo = GEQ; strcpy(token.lexema, ">="); }
            else { devolver_char(seguinte); token.tipo = GT; strcpy(token.lexema, ">"); }
            break;
        case '&':
            seguinte = prox_char();
            if (seguinte == '&') { token.tipo = AND; strcpy(token.lexema, "&&"); }
            else { devolver_char(seguinte); token.lexema[0] = '&'; }
            break;
        case '|':
            seguinte = prox_char();
            if (seguinte == '|') { token.tipo = OR; strcpy(token.lexema, "||"); }
            else { devolver_char(seguinte); token.lexema[0] = '|'; }
            break;
        case '+': token.tipo = PLUS; strcpy(token.lexema, "+"); break;
        case '-': token.tipo = MINUS; strcpy(token.lexema, "-"); break;
        case '*': token.tipo = MUL; strcpy(token.lexema, "*"); break;
        case '/': token.tipo = DIV; strcpy(token.lexema, "/"); break;
        case '%': token.tipo = MOD; strcpy(token.lexema, "%"); break;
        case ';': token.tipo = SEMICOLON; strcpy(token.lexema, ";"); break;
        case ',': token.tipo = COMMA; strcpy(token.lexema, ","); break;
        case '(': token.tipo = LPAREN; strcpy(token.lexema, "("); break;
        case ')': token.tipo = RPAREN; strcpy(token.lexema, ")"); break;
        case '{': token.tipo = LBRACE; strcpy(token.lexema, "{"); break;
        case '}': token.tipo = RBRACE; strcpy(token.lexema, "}"); break;
        case '[': token.tipo = LBRACKET; strcpy(token.lexema, "["); break;
        case ']': token.tipo = RBRACKET; strcpy(token.lexema, "]"); break;
        default:
            token.lexema[0] = (char)c;
            token.lexema[1] = '\0';
            break;
    }
    return token;
}
