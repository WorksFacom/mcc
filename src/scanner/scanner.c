/** @file scanner.c
 *  @brief Implementação do analisador léxico para a linguagem Simple C.
 *  
 *  Este arquivo contém as funções para ler o arquivo fonte, ignorar espaços e comentários,
 *  e gerar tokens a partir de um arquivo de entrada. O analisador léxico é a primeira
 *  etapa do compilador, convertendo o código em uma sequência de tokens.
 */


#include <stdio.h>
#include <ctype.h>
#include "scanner.h"
#include "string.h"

/** @brief Arquivo de entrada do arquivo fonte.
 *  
 *  Ponteiro para o arquivo aberto que contém o código Simple C a ser analisado.
 */
FILE *arquivo_fonte;
/** @brief Contador da linha atual no arquivo fonte.
 *  
 *  Rastreia a linha atual para relatar erros com precisão, inicializado em 1.
 */
int linha_atual = 1;

/** @brief Lê o próximo caractere do arquivo de entrada.
 *  
 *  @return O caractere lido ou EOF se o fim do arquivo for atingido.
 */
char prox_char(){
    return fgetc(arquivo_fonte);
}

/** @brief Inicializa o analisador léxico com o arquivo de entrada.
 *  
 *  Abre o arquivo especificado e prepara o analisador para leitura.
 *  @param teste Nome do arquivo cmm a ser analisado.
 *  @return 0 em caso de sucesso, -1 se houver erro ao abrir o arquivo.
 */
int inicializar_scanner(const char *nome_arquivo) {
    arquivo_fonte=fopen(nome_arquivo,"r");
    if(arquivo_fonte == NULL){
        printf("erro ao abrir o arquivo %s\n",nome_arquivo);
        return -1;
    }
    linha_atual = 1;
    return 0; 
}

/** @brief Ignora espaços em branco e comentários no código-fonte.
 *  
 *  Avança a leitura até encontrar um caractere significativo, pulando espaços,
 *  quebras de linha e comentários Atualiza a linha atual.
 *  @param c Caractere inicial a ser processado.
 *  @return Próximo caractere significativo ou EOF se o fim do arquivo for atingido.
 */
char ignora(char c) {
    while(c != EOF) {
        while (isspace(c) && c!=EOF) {
            if (c == '\n') 
                linha_atual++;
            c = prox_char();
        }
    if(c == '/'){
        c = prox_char();
        if (c == '/') {

            while(c != '\n' && c!=EOF)
                c = prox_char();
            if(c== '\n'){
                linha_atual++;
                c = prox_char();
                }
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
                    } else {
                        if (c == '\n') 
                            linha_atual++;
                        c = prox_char();
                    }
                }
            } else {
                ungetc(c, arquivo_fonte); //devolve o caractere / se não for comentario, pois estava pegando e não reconhecendo o DIV
                return '/'; 
            }
        } else {
            break;
        }
    }
    return c;  
}

/** @brief Gera o próximo token do arquivo fonte.
 *  
 *  Lê caracteres do arquivo, ignora espaços e comentários, e identifica o próximo
 *  token (ex.: identificadores, operadores, palavras reservadas). Define o tipo,
 *  lexema e linha do token.
 *  @return Estrutura Token contendo tipo, lexema e linha do token identificado.
 */
Token proximo_token() {
    Token token; 
    token.tipo = UNDEF;
    token.lexema[0] = '\0';

    char c = ignora(prox_char());  
    token.linha = linha_atual;

    if (c == EOF) {
        token.tipo = END_OF_FILE;
        strcpy(token.lexema, "EOF");
        return token;
    }

    //reconhecer operadores de dois caracteres
    if (c == '=') {
        c = prox_char();  
        if (c == '=') {
            token.tipo = EQ; 
            strcpy(token.lexema, "==");
            return token;
        } 
        ungetc(c, arquivo_fonte);
        token.tipo = ASSIGN; 
        strcpy(token.lexema, "="); 
        return token;
    } else if (c == '!') {
        c = prox_char();  
        if (c == '=') {
            token.tipo = NEQ; 
            strcpy(token.lexema, "!=");
            return token;
        }
        ungetc(c, arquivo_fonte); 
        token.tipo = NOT;
        strcpy(token.lexema, "!");
        return token;
    } else if (c == '<') {
        c = prox_char();  
        if (c == '=') {
            token.tipo = LEQ; 
            strcpy(token.lexema, "<=");
            return token;
        } 
        ungetc(c, arquivo_fonte);
        token.tipo = LT; 
        strcpy(token.lexema, "<");
        return token;
    } else if (c == '>') {
        c = prox_char();
        if (c == '=') { 
            token.tipo = GEQ;
            strcpy(token.lexema, ">=");
            return token;
        }
        ungetc(c, arquivo_fonte); 
        token.tipo = GT;
        strcpy(token.lexema, ">");
        return token;
    } else if (c == '&') {
        c = prox_char();
        if (c == '&') { 
            token.tipo = AND; 
            strcpy(token.lexema, "&&");
            return token;
        } 
        ungetc(c, arquivo_fonte);
        token.tipo = UNDEF; 
        strcpy(token.lexema, "&");
        return token;
    } else if (c == '|') {
        c = prox_char(); 
        if (c == '|') {
            token.tipo = OR;
            strcpy(token.lexema, "||");
            return token;
        }
        ungetc(c, arquivo_fonte); 
        token.tipo = UNDEF;
        strcpy(token.lexema, "|");
        return token;
    }
    //reconhece operadores de um caractere
    else if (c == '+') { 
        token.tipo = PLUS; 
        strcpy(token.lexema, "+"); 
        return token; 
    } 
    else if (c == '-') { 
        token.tipo = MINUS; 
        strcpy(token.lexema, "-");
        return token; 
    } 
    else if (c == '*') { 
        token.tipo = MUL; 
        strcpy(token.lexema, "*"); 
        return token; 
    } 
    else if (c == '/') { 
        token.tipo = DIV;
        strcpy(token.lexema, "/");
        return token; 
    } else if (c == '%') {
        token.tipo = MOD;
        strcpy(token.lexema, "%");
        return token;
    } 
    else if (c == ';') {
        token.tipo = SEMICOLON;
        strcpy(token.lexema, ";"); 
        return token; 
    } 
    else if (c == ',') {
        token.tipo = COMMA;
        strcpy(token.lexema, ","); 
        return token;
    } 
    else if (c == '(') {
        token.tipo = LPAREN; 
        strcpy(token.lexema, "("); 
        return token; 
    }
    else if (c == ')') {
        token.tipo = RPAREN; 
        strcpy(token.lexema, ")"); 
        return token; 
    } 
    else if (c == '{') {
        token.tipo = LBRACE; 
        strcpy(token.lexema, "{"); 
        return token; 
    }
    else if (c == '}') { 
        token.tipo = RBRACE;
        strcpy(token.lexema, "}"); 
        return token; 
    } 
    else if (c == '[') {
        token.tipo = LBRACKET; 
        strcpy(token.lexema, "["); 
        return token;
    } 
    else if (c == ']') {
        token.tipo = RBRACKET;
        strcpy(token.lexema, "]");
        return token; 
    }

    //reconhecer constantes de caractere 
    if (c == '\'') {
        char buffer[4]; 
        buffer[0] = c;
        c = prox_char(); 
        if (c != EOF && c != '\'') { 
            buffer[1] = c; 
            c = prox_char(); 
            if (c == '\'') {
                buffer[2] = c; 
                buffer[3] = '\0'; 
                token.tipo = CHARCONST; 
                strcpy(token.lexema, buffer); 
                return token;
            } 
        } 
        ungetc(c, arquivo_fonte); 
        token.tipo = UNDEF;
        strcpy(token.lexema, "'"); 
        return token;
    }

    //reconhecer identificadores e palavras reservadas
    if (isalpha(c)) {
        char buffer[50]; 
        int i = 0; 
        buffer[i++] = c;
        c = prox_char(); 
        while (isalnum(c) && i < 49) { 
            buffer[i++] = c; 
            c = prox_char(); 
        } 
        buffer[i] = '\0';

        if (strcmp(buffer, "main") == 0) 
            token.tipo = MAIN; 
        else if (strcmp(buffer, "if") == 0) 
            token.tipo = IF; 
        else if (strcmp(buffer, "else") == 0) 
            token.tipo = ELSE;
        else if (strcmp(buffer, "for") == 0) 
            token.tipo = FOR; 
        else if (strcmp(buffer, "return") == 0) 
            token.tipo = RETURN; 
        else if (strcmp(buffer, "int") == 0) 
            token.tipo = INT;
        else if (strcmp(buffer, "char") == 0) 
            token.tipo = CHAR; 
        else 
            token.tipo = ID;

        strcpy(token.lexema, buffer);
        ungetc(c, arquivo_fonte); 
        return token;
    }

    
        //reconhcer strings
        if (c == '"') {
            char buffer[100]; 
            int i = 0;
            buffer[i++] = c; 
            c = prox_char();
            while (c != '"' && c != EOF && i < 99) { 
                buffer[i++] = c;
                c = prox_char();
            }
            if (c == '"') {
                buffer[i++] = c;
                buffer[i] = '\0';
                token.tipo = STRINGCONST;
                strcpy(token.lexema, buffer);
                return token;
            }
            
            ungetc(c, arquivo_fonte);
            token.tipo = UNDEF;
            strcpy(token.lexema, "\"");
            return token;
        }

    //reconhecer numeros inteiro
    if (isdigit(c) || c == '-') {
        char buffer[10]; 
        int i = 0; 
        buffer[i++] = c;
        c = prox_char(); 
        while (isdigit(c) && i < 9) { 
            buffer[i++] = c; 
            c = prox_char(); 
        }
        buffer[i] = '\0'; 
        token.tipo = INTEGERCONST;
        strcpy(token.lexema, buffer); 
        ungetc(c, arquivo_fonte);
        return token;
    }

    //caractere não reconhecido
    token.lexema[0] = c;
    token.lexema[1] = '\0';
    return token;
}
