#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/**
 * @file parser.c
 * @brief implementação do analisador sintático (parser) da linguagem.
 *
 * este arquivo contém a lógica para a análise sintática, que é a segunda fase
 * do compilador. ele recebe uma lista de tokens, verifica se a sequência obedece 
 * às regras gramaticais da linguagem e constrói uma árvore sintática abstrata (ast) 
 * para representar a estrutura do código, usando a abordagem de parser recursivo descendente.
 */

//================================================================================
//SEÇÃO DE FUNÇÕES DE UTILIDADE
//================================================================================

/** @brief converte um TokenType em uma string legível. */
const char* token_name(TokenType tipo) {
    switch (tipo) {
        case UNDEF: return "UNDEF"; case ID: return "ID"; case INTEGERCONST: return "INTEGERCONST";
        case CHARCONST: return "CHARCONST"; case STRINGCONST: return "STRINGCONST"; case PLUS: return "PLUS";
        case MINUS: return "MINUS"; case MUL: return "MUL"; case DIV: return "DIV"; case MOD: return "MOD";
        case EQ: return "EQ"; case NEQ: return "NEQ"; case LT: return "LT"; case GT: return "GT";
        case LEQ: return "LEQ"; case GEQ: return "GEQ"; case AND: return "AND"; case OR: return "OR";
        case NOT: return "NOT"; case ASSIGN: return "ASSIGN"; case SEMICOLON: return "SEMICOLON";
        case COMMA: return "COMMA"; case LPAREN: return "LPAREN"; case RPAREN: return "RPAREN";
        case LBRACE: return "LBRACE"; case RBRACE: return "RBRACE"; case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET"; case MAIN: return "MAIN"; case IF: return "IF";
        case ELSE: return "ELSE"; case FOR: return "FOR"; case RETURN: return "RETURN";
        case INT: return "INT"; case CHAR: return "CHAR"; case END_OF_FILE: return "END_OF_FILE";
        default: return "TOKEN_DESCONHECIDO";
    }
}

/** @brief inicializa uma lista de tokens. */
int inicializar_token_list(TokenList *lista, int capacidade_inicial) {
    lista->tokens = malloc(capacidade_inicial * sizeof(Token));
    if (lista->tokens == NULL) {
        printf("Erro: Falha ao alocar memoria para lista de tokens\n");
        return -1;
    }
    lista->tamanho = 0;
    lista->capacidade = capacidade_inicial;
    return 0;
}

/** @brief libera a memória de uma TokenList. */
void liberar_token_list(TokenList *lista) {
    free(lista->tokens);
    lista->tokens = NULL;
    lista->tamanho = 0;
    lista->capacidade = 0;
}

/** @brief carrega tokens de um arquivo para uma TokenList. */
int carregar_tokens(const char *nome_arquivo, TokenList *lista) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) { return -1; }
    char linha_str[256];
    while (fgets(linha_str, sizeof(linha_str), arquivo)) {
        int tipo, linha_num;
        char lexema[100];
        if (sscanf(linha_str, "Token: tipo = %d, lexema = '%[^']', linha = %d", &tipo, lexema, &linha_num) != 3) { continue; }
        if (lista->tamanho >= lista->capacidade) {
            lista->capacidade *= 2;
            Token *nova_lista = realloc(lista->tokens, lista->capacidade * sizeof(Token));
            if (nova_lista == NULL) { fclose(arquivo); return -1; }
            lista->tokens = nova_lista;
        }
        lista->tokens[lista->tamanho].tipo = (TokenType)tipo;
        strncpy(lista->tokens[lista->tamanho].lexema, lexema, 100);
        lista->tokens[lista->tamanho].linha = linha_num;
        lista->tamanho++;
    }
    fclose(arquivo);
    return 0;
}

/** @brief adiciona um token ao final de uma TokenList, redimensionando se necessário. */
void adicionar_token(TokenList* lista, Token token) {
    if (lista->tamanho >= lista->capacidade) {
        lista->capacidade *= 2;
        Token* nova_lista = realloc(lista->tokens, lista->capacidade * sizeof(Token));
        if (nova_lista == NULL) {
            fprintf(stderr, "Erro fatal: Falha ao redimensionar a lista de tokens.\n");
            exit(EXIT_FAILURE);
        }
        lista->tokens = nova_lista;
    }
    lista->tokens[lista->tamanho] = token;
    lista->tamanho++;
}

//================================================================================
//SEÇÃO DO NÚCLEO DO PARSER
//================================================================================

/** @brief prepara o parser para iniciar a análise. */
void inicializar_parser(Parser *parser, TokenList *lista, PilhaTabelasSimbolos *symtab_stack) {
    parser->lista = lista;
    parser->current = 0;
    parser->current_token = lista->tamanho > 0 ? &lista->tokens[0] : NULL;
    parser->symtab_stack = symtab_stack;
}

/** @brief função principal que dispara a análise sintática. */
ASTNode* parse(Parser *parser) {
    ASTNode* root = program(parser);
    printf("Analise Sintatica concluida. AST gerada com sucesso.\n");
    return root;
}

/** @brief consome o token atual e avança para o próximo. */
void advance(Parser *parser) {
    if (parser->current < parser->lista->tamanho) {
        parser->current++;
        parser->current_token = parser->current < parser->lista->tamanho ? &parser->lista->tokens[parser->current] : NULL;
    } else {
        parser->current_token = NULL;
    }
}

/** @brief valida o token atual e avança, ou lança um erro. */
void match(Parser *parser, TokenType expected) {
    if (parser->current_token && parser->current_token->tipo == expected) {
        advance(parser);
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "Token inesperado. Esperado: %s", token_name(expected));
        error(parser, msg);
    }
}

/** @brief reporta um erro sintático e encerra. */
void error(Parser *parser, const char *msg) {
    int linha = parser->current_token ? parser->current_token->linha : 0;
    const char *lexema = parser->current_token ? parser->current_token->lexema : "N/A";
    const char *tipo_token_str = parser->current_token ? token_name(parser->current_token->tipo) : "N/A";
    
    fprintf(stderr, "Erro Sintatico na linha %d: %s\n", linha, msg);
    fprintf(stderr, "Token atual: '%s' (Tipo: %s)\n", lexema, tipo_token_str);
    exit(EXIT_FAILURE);
}

//================================================================================
//SEÇÃO DAS FUNÇÕES DA GRAMÁTICA (RECURSIVE DESCENT)
//================================================================================

//declaração antecipada, pois as funções de expressão se chamam mutuamente
ASTNode* expression(Parser *parser);
ASTNode* arithmetic_expression(Parser* parser);
ASTNode* arg_list(Parser *parser);

/** @brief processa a regra 'factor' (números, IDs, parênteses, chamadas de função e acesso a array). */
ASTNode* factor(Parser *parser) {
    int line = parser->current_token->linha;
    if (parser->current_token->tipo == INTEGERCONST) {
        ASTNode* node = criar_no(NODE_INTEGER_CONST, line);
        node->data.int_value = atol(parser->current_token->lexema);
        match(parser, INTEGERCONST);
        return node;
    }
    if (parser->current_token->tipo == CHARCONST) {
        ASTNode* node = criar_no(NODE_CHAR_CONST, line);
        if (strlen(parser->current_token->lexema) > 0) { node->data.char_value = parser->current_token->lexema[0]; }
        match(parser, CHARCONST);
        return node;
    }
    if (parser->current_token->tipo == STRINGCONST) {
        ASTNode* node = criar_no(NODE_STRING_CONST, line);
        node->data.string_value = strdup(parser->current_token->lexema);
        match(parser, STRINGCONST);
        return node;
    }
    if (parser->current_token->tipo == ID) {
        ASTNode* id_node = criar_no(NODE_ID, line);
        id_node->data.string_value = strdup(parser->current_token->lexema);
        match(parser, ID);

        //verifica se é uma chamada de função
        if (parser->current_token && parser->current_token->tipo == LPAREN) {
            match(parser, LPAREN);
            ASTNode* call_node = criar_no(NODE_CALL, line);
            adicionar_filho(call_node, id_node);
            adicionar_filho(call_node, arg_list(parser));
            match(parser, RPAREN);
            return call_node;
        } 
        //verifica se é um acesso a array
        else if (parser->current_token && parser->current_token->tipo == LBRACKET) {
            match(parser, LBRACKET);
            ASTNode* array_access_node = criar_no(NODE_ARRAY_ACCESS, line);
            adicionar_filho(array_access_node, id_node);
            adicionar_filho(array_access_node, expression(parser));
            match(parser, RBRACKET);
            return array_access_node;
        }
        
        //se não for nenhum dos casos acima, é apenas uma variável.
        return id_node;
    }
    if (parser->current_token->tipo == LPAREN) {
        match(parser, LPAREN);
        ASTNode* node = expression(parser);
        match(parser, RPAREN);
        return node;
    }
    error(parser, "Fator invalido em expressao (esperado numero, variavel, string ou '(').");
    return NULL;
}

/** @brief processa um termo (multiplicação/divisão). */
ASTNode* term(Parser *parser) {
    ASTNode* node = factor(parser);
    while (parser->current_token && (parser->current_token->tipo == MUL || parser->current_token->tipo == DIV || parser->current_token->tipo == MOD)) {
        TokenType op = parser->current_token->tipo;
        int line = parser->current_token->linha;
        match(parser, op);
        ASTNode* op_node = criar_no(NODE_BINARY_OP, line);
        op_node->data.op_type = op;
        adicionar_filho(op_node, node);
        adicionar_filho(op_node, factor(parser));
        node = op_node;
    }
    return node;
}

/** @brief processa uma expressão aritmética (soma/subtração). */
ASTNode* arithmetic_expression(Parser* parser) {
    ASTNode* node = term(parser);
    while (parser->current_token && (parser->current_token->tipo == PLUS || parser->current_token->tipo == MINUS)) {
        TokenType op = parser->current_token->tipo;
        int line = parser->current_token->linha;
        match(parser, op);
        ASTNode* op_node = criar_no(NODE_BINARY_OP, line);
        op_node->data.op_type = op;
        adicionar_filho(op_node, node);
        adicionar_filho(op_node, term(parser));
        node = op_node;
    }
    return node;
}

/** @brief processa uma expressão (incluindo relacionais). */
ASTNode* expression(Parser *parser) {
    ASTNode* node = arithmetic_expression(parser);
    while (parser->current_token && (parser->current_token->tipo == LT || parser->current_token->tipo == LEQ ||
                                     parser->current_token->tipo == GT || parser->current_token->tipo == GEQ ||
                                     parser->current_token->tipo == EQ || parser->current_token->tipo == NEQ)) {
        TokenType op = parser->current_token->tipo;
        int line = parser->current_token->linha;
        match(parser, op);
        ASTNode* op_node = criar_no(NODE_BINARY_OP, line);
        op_node->data.op_type = op;
        adicionar_filho(op_node, node);
        adicionar_filho(op_node, arithmetic_expression(parser));
        node = op_node;
    }
    return node;
}

/** @brief processa uma lista de argumentos em uma chamada de função. */
ASTNode* arg_list(Parser *parser) {
    ASTNode* args_node = criar_no(NODE_ARG_LIST, parser->current_token ? parser->current_token->linha : 0);
    
    if (parser->current_token->tipo != RPAREN) {
        while (1) {
            adicionar_filho(args_node, expression(parser));
            if (parser->current_token->tipo == COMMA) {
                match(parser, COMMA);
            } else {
                break;
            }
        }
    }
    return args_node;
}

/** @brief processa a regra 'program', ponto de entrada da gramática. */
ASTNode* program(Parser *parser) {
    ASTNode* prog_node = criar_no(NODE_PROGRAM, 0);
    while (parser->current_token && parser->current_token->tipo != END_OF_FILE) {
        if (parser->current_token->tipo == INT || parser->current_token->tipo == CHAR) {
            adicionar_filho(prog_node, function(parser));
        } else {
            error(parser, "Esperado definicao de funcao (deve iniciar com 'int' ou 'char')");
        }
    }
    match(parser, END_OF_FILE);
    return prog_node;
}

/** @brief processa a definição completa de uma função. */
ASTNode* function(Parser *parser) {
    int func_line = parser->current_token->linha;
    ASTNode* func_node = criar_no(NODE_FUNCTION_DEF, func_line);
    adicionar_filho(func_node, type(parser));
    ASTNode* id_node = criar_no(NODE_ID, func_line);
    id_node->data.string_value = strdup(parser->current_token->lexema);
    adicionar_filho(func_node, id_node);
    if (parser->current_token->tipo == ID) match(parser, ID); else match(parser, MAIN);
    match(parser, LPAREN);
    if (parser->current_token->tipo != RPAREN) {
        adicionar_filho(func_node, param_list(parser));
    }
    match(parser, RPAREN);
    adicionar_filho(func_node, statement(parser));
    return func_node;
}

/** @brief processa a lista de parâmetros de uma função. */
ASTNode* param_list(Parser *parser) {
    ASTNode* params_node = criar_no(NODE_PARAM_LIST, parser->current_token->linha);
    do {
        adicionar_filho(params_node, var_declaration(parser));
        if (parser->current_token->tipo == COMMA) {
            match(parser, COMMA);
        } else { break; }
    } while (parser->current_token->tipo == INT || parser->current_token->tipo == CHAR);
    return params_node;
}

/** @brief processa um especificador de tipo. */
ASTNode* type(Parser *parser) {
    int line = parser->current_token->linha;
    ASTNode* type_node = criar_no(NODE_TYPE, line);
    if (parser->current_token && (parser->current_token->tipo == INT || parser->current_token->tipo == CHAR)) {
        type_node->data.op_type = parser->current_token->tipo;
        advance(parser);
    } else {
        error(parser, "Tipo invalido (esperado INT ou CHAR)");
    }
    return type_node;
}

/** @brief processa a declaração de uma variável. */
ASTNode* var_declaration(Parser *parser) {
    int line = parser->current_token->linha;
    ASTNode* decl_node = criar_no(NODE_VAR_DECL, line);
    adicionar_filho(decl_node, type(parser));
    ASTNode* id_node = criar_no(NODE_ID, line);
    id_node->data.string_value = strdup(parser->current_token->lexema);
    adicionar_filho(decl_node, id_node);
    match(parser, ID);
    if (parser->current_token->tipo == LBRACKET) {
        match(parser, LBRACKET);
        if (parser->current_token->tipo == INTEGERCONST) {
            ASTNode* size_node = criar_no(NODE_INTEGER_CONST, line);
            size_node->data.int_value = atol(parser->current_token->lexema);
            adicionar_filho(decl_node, size_node);
            match(parser, INTEGERCONST);
        }
        match(parser, RBRACKET);
    }
    if (parser->current_token->tipo == ASSIGN) {
        match(parser, ASSIGN);
        ASTNode* assign_node = criar_no(NODE_ASSIGN, line);
        ASTNode* id_clone_node = criar_no(NODE_ID, id_node->linha);
        id_clone_node->data.string_value = strdup(id_node->data.string_value);
        adicionar_filho(assign_node, id_clone_node);
        adicionar_filho(assign_node, expression(parser));
        adicionar_filho(decl_node, assign_node);
    }
    return decl_node;
}

/** @brief processa uma única instrução ou um bloco. */
ASTNode* statement(Parser *parser) {
    if (!parser->current_token) error(parser, "Fim de arquivo inesperado, esperando uma instrucao.");
    int line = parser->current_token->linha;
    switch (parser->current_token->tipo) {
        case LBRACE: { 
            match(parser, LBRACE);
            ASTNode* block_node = criar_no(NODE_BLOCK, line);
            while (parser->current_token && parser->current_token->tipo != RBRACE) {
                adicionar_filho(block_node, statement(parser));
            }
            match(parser, RBRACE);
            return block_node;
        }
        case INT: case CHAR: {
            ASTNode* decl = var_declaration(parser);
            match(parser, SEMICOLON);
            return decl;
        }
        case IF: return if_statement(parser);
        case FOR: return for_statement(parser);
        case ID: {
            ASTNode* node = assign(parser);
            match(parser, SEMICOLON);
            return node;
        }
        case RETURN: {
            match(parser, RETURN);
            ASTNode* return_node = criar_no(NODE_RETURN, line);
            if (parser->current_token->tipo != SEMICOLON) {
                adicionar_filho(return_node, expression(parser));
            }
            match(parser, SEMICOLON);
            return return_node;
        }
        default:
            error(parser, "Instrucao invalida ou inesperada.");
            return NULL; 
    }
}

/** @brief processa uma instrução de atribuição. */
ASTNode* assign(Parser *parser) {
    int line = parser->current_token->linha;
    ASTNode* id_node = criar_no(NODE_ID, line);
    id_node->data.string_value = strdup(parser->current_token->lexema);
    match(parser, ID);
    ASTNode* left_node = id_node; 
    if(parser->current_token && parser->current_token->tipo == LBRACKET) {
        match(parser, LBRACKET);
        ASTNode* array_node = criar_no(NODE_ARRAY_ACCESS, line);
        adicionar_filho(array_node, left_node); 
        adicionar_filho(array_node, expression(parser)); 
        left_node = array_node;
        match(parser, RBRACKET);
    }
    match(parser, ASSIGN);
    ASTNode* assign_node = criar_no(NODE_ASSIGN, line);
    adicionar_filho(assign_node, left_node);
    adicionar_filho(assign_node, expression(parser));
    return assign_node;
}

/** @brief processa uma instrução 'if-else'. */
ASTNode* if_statement(Parser *parser) {
    int line = parser->current_token->linha;
    match(parser, IF);
    ASTNode* if_node = criar_no(NODE_IF, line);
    match(parser, LPAREN);
    adicionar_filho(if_node, expression(parser)); 
    match(parser, RPAREN);
    adicionar_filho(if_node, statement(parser)); 
    if (parser->current_token && parser->current_token->tipo == ELSE) {
        match(parser, ELSE);
        adicionar_filho(if_node, statement(parser));
    }
    return if_node;
}

/** @brief processa uma instrução de laço 'for'. */
ASTNode* for_statement(Parser *parser) {
    int line = parser->current_token->linha;
    match(parser, FOR);
    ASTNode* for_node = criar_no(NODE_FOR, line);
    match(parser, LPAREN);
    if (parser->current_token->tipo != SEMICOLON) {
        if (parser->current_token->tipo == INT || parser->current_token->tipo == CHAR) {
             adicionar_filho(for_node, var_declaration(parser));
        } else {
             adicionar_filho(for_node, assign(parser));
        }
    } else {
        adicionar_filho(for_node, criar_no(NODE_UNDEFINED, line));
    }
    match(parser, SEMICOLON);
    if (parser->current_token->tipo != SEMICOLON) {
        adicionar_filho(for_node, expression(parser));
    } else {
        adicionar_filho(for_node, criar_no(NODE_UNDEFINED, line));
    }
    match(parser, SEMICOLON);
    if (parser->current_token->tipo != RPAREN) {
        adicionar_filho(for_node, assign(parser));
    } else {
        adicionar_filho(for_node, criar_no(NODE_UNDEFINED, line));
    }
    match(parser, RPAREN);
    adicionar_filho(for_node, statement(parser));
    return for_node;
}