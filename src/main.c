/**
 * @file main.c
 * @brief programa principal e orquestrador do compilador.
 *
 * este arquivo contém a função 'main' que controla o fluxo de compilação,
 * invocando as fases de análise léxica, sintática e semântica em sequência.
 * ele também lida com argumentos de linha de comando para permitir a inspeção
 * da saída de cada fase individualmente.
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "ir/ir.h"
#include "intercode/intercode.h"
#include "assembly/assembly.h"

//protótipos de funções auxiliares
void imprimir_tokens(TokenList* tokens);
void salvar_tokens_em_arquivo(TokenList* tokens, bool verbose);
void imprimir_ast(ASTNode* arvore);
void salvar_ast(ASTNode* arvore, bool verbose);
void imprimir_ir(IR_Instruction* ir_head);
void salvar_ir_em_arquivo(IR_Instruction* ir_head, bool verbose); 
int executar_analise_lexica(const char* nome_arquivo, TokenList* lista);

/**
 * @brief orquestra a fase de análise léxica.
 */
int executar_analise_lexica(const char* nome_arquivo, TokenList* lista) {
    if (inicializar_scanner(nome_arquivo) != 0) {
        return -1; //erro ao abrir o arquivo
    }

    Token token;
    do {
        token = proximo_token();
        adicionar_token(lista, token); //adiciona o token à lista em memória
        if (token.tipo == UNDEF) {
            fprintf(stderr, "Erro Lexico: Token indefinido '%s' encontrado na linha %d\n", token.lexema, token.linha);
            return -1;
        }
    } while (token.tipo != END_OF_FILE);

    return 0;
}

/**
 * @brief ponto de entrada principal do compilador.
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Erro: Arquivo fonte nao especificado.\n");
        fprintf(stderr, "Uso: %s [--scan | --parse | --semantic | --gen-ir | --gen-asm] <arquivo_fonte.cmm>\n", argv[0]);
        return -1;
    }

    //determina o modo de operação e o nome do arquivo
    char* modo = "completo";
    char* nome_arquivo = argv[1];

    if (argc > 2) {
        modo = argv[1];
        nome_arquivo = argv[2];
    }
    
    //fase 1: análise léxica
    TokenList tokens;
    inicializar_token_list(&tokens, 100);
    if (executar_analise_lexica(nome_arquivo, &tokens) != 0) {
        liberar_token_list(&tokens);
        return -1; //encerra se houver erro léxico
    }
    //salva o arquivo de tokens silenciosamente em todos os modos, exceto 'scan'
    if (strcmp(modo, "--scan") != 0) {
        salvar_tokens_em_arquivo(&tokens, false);
    }
    printf("Fase 1 (Lexica) concluida: %d tokens gerados.\n", tokens.tamanho);


    //modo scan
    if (strcmp(modo, "--scan") == 0) {
        imprimir_tokens(&tokens);
        salvar_tokens_em_arquivo(&tokens, true); //salva e imprime a mensagem de sucesso
        liberar_token_list(&tokens);
        return 0; 
    }

    //fase 2: análise sintática
    PilhaTabelasSimbolos* pilha_simbolos = criar_pilha_tabelas();
    Parser parser;
    inicializar_parser(&parser, &tokens, pilha_simbolos);
    ASTNode* arvore = parse(&parser);
    //salva o arquivo da ast silenciosamente em todos os modos, exceto 'parse'
    if (strcmp(modo, "--parse") != 0) {
        salvar_ast(arvore, false);
    }
    printf("Fase 2 (Sintatica) concluida: AST gerada.\n");

    //modo parse
    if (strcmp(modo, "--parse") == 0) {
        imprimir_ast(arvore);
        salvar_ast(arvore, true); //salva e imprime a mensagem de sucesso
        liberar_arvore(arvore);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0; 
    }

    //fase 3: análise semântica
    analisar_semanticamente(arvore, pilha_simbolos);
    printf("Fase 3 (Semantica) concluida: Codigo validado.\n");


    //modo semantic
    if (strcmp(modo, "--semantic") == 0) {
        printf("\n--- MODO DE ANALISE SEMANTICA CONCLUIDO ---\n");
        liberar_arvore(arvore);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0;
    }
    
    //fase 4: geração de código intermediário
    IR_Instruction* ir_head = gerar_codigo_intermediario(arvore, pilha_simbolos);
    printf("Fase 4 (Geracao de IR) concluida.\n");
    
    //salva o arquivo de ir silenciosamente
    salvar_ir_em_arquivo(ir_head, false); // <-- ADIÇÃO AQUI

    //modo gen-ir
    if (strcmp(modo, "--gen-ir") == 0) {
        printf("\n--- MODO DE GERACAO DE CODIGO INTERMEDIARIO ---\n");
        imprimir_ir(ir_head);
        salvar_ir_em_arquivo(ir_head, true); // <-- ADIÇÃO AQUI

        //limpeza de memória com a ordem correta
        liberar_arvore(arvore);
        liberar_ir(ir_head); // 
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0;
    }

    //fase 5: geração de código assembly
    //define o nome do arquivo de saída (ex: teste.cmm -> teste.s)
    char nome_arquivo_asm[100];
    strncpy(nome_arquivo_asm, nome_arquivo, sizeof(nome_arquivo_asm) - 2);
    char* ponto = strrchr(nome_arquivo_asm, '.');
    if (ponto) {
        *ponto = '\0'; //remove a extensão .cmm
    }
    strcat(nome_arquivo_asm, ".s"); //adiciona a extensão .s

    if (gerar_assembly(ir_head, pilha_simbolos, nome_arquivo_asm) != 0) {
        liberar_arvore(arvore);
        liberar_ir(ir_head);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return -1; //encerra se houver erro na geração de assembly
    }
    printf("Fase 5 (Geracao de Assembly) concluida: '%s' gerado.\n", nome_arquivo_asm);

    //modo gen-asm
    if (strcmp(modo, "--gen-asm") == 0) {
        printf("\n--- MODO DE GERACAO DE ASSEMBLY CONCLUIDO ---\n");
        
        //limpeza de memória com a ordem correta
        liberar_arvore(arvore);

        liberar_ir(ir_head);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0; // <-- CORREÇÃO AQUI
    }
    
    //modo completo (default)
    printf("\n--- MODO DE COMPILACAO COMPLETA ---\n");
    printf("Todas as fases de analise foram concluidas com sucesso.\n");
    
    //limpeza de memória
    liberar_arvore(arvore);
    liberar_ir(ir_head);
    destruir_pilha_tabelas(pilha_simbolos);
    liberar_token_list(&tokens);

    printf("\nCompilacao finalizada!\n");
    return 0;
}

/** @brief imprime o conteúdo de uma TokenList no console. */
void imprimir_tokens(TokenList* tokens) {
    printf("--- LISTA DE TOKENS GERADOS ---\n");
    for (int i = 0; i < tokens->tamanho; i++) {
        printf("Token: tipo = %d, lexema = '%s', linha = %d\n", 
               tokens->tokens[i].tipo, 
               tokens->tokens[i].lexema, 
               tokens->tokens[i].linha);
    }
    printf("--------------------------------\n");
}

/** @brief salva o conteúdo de uma TokenList no arquivo 'tokens.txt'. */
void salvar_tokens_em_arquivo(TokenList* tokens, bool verbose) {
    FILE *saida = fopen("tokens.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo tokens.txt\n");
        return;
    }
    for (int i = 0; i < tokens->tamanho; i++) {
        fprintf(saida, "Token: tipo = %d, lexema = '%s', linha = %d\n", 
                tokens->tokens[i].tipo, 
                tokens->tokens[i].lexema, 
                tokens->tokens[i].linha);
    }
    fclose(saida);
    if (verbose) {
        printf("Arquivo 'tokens.txt' gerado com sucesso.\n");
    }
}

/** @brief função recursiva para imprimir ou salvar a AST de forma legível. */
void processar_ast_recursivamente(ASTNode *no, int indent_level, FILE *arquivo) {
    if (no == NULL) return;
    
    char indent_str[100] = "";
    for (int i = 0; i < indent_level; ++i) {
        strcat(indent_str, "  ");
    }

    #define PRINT(format, ...) do { if (arquivo) fprintf(arquivo, format, ##__VA_ARGS__); else printf(format, ##__VA_ARGS__); } while(0)
    
    PRINT("%s", indent_str);
    switch (no->node_type) {
        case NODE_PROGRAM: PRINT("NO_PROGRAMA\n"); break;
        case NODE_FUNCTION_DEF: PRINT("NO_DEFINICAO_FUNCAO\n"); break;
        case NODE_VAR_DECL: PRINT("NO_DECLARACAO_VARIAVEL\n"); break;
        case NODE_ASSIGN: PRINT("NO_ATRIBUICAO\n"); break;
        case NODE_IF: PRINT("NO_IF\n"); break;
        case NODE_FOR: PRINT("NO_FOR\n"); break;
        case NODE_RETURN: PRINT("NO_RETORNO\n"); break;
        case NODE_PRINT: PRINT("NO_PRINT\n"); break;
        case NODE_BLOCK: PRINT("NO_BLOCO\n"); break;
        case NODE_PARAM_LIST: PRINT("NO_LISTA_PARAMETROS\n"); break;
        case NODE_ARG_LIST: PRINT("NO_LISTA_ARGUMENTOS\n"); break;
        case NODE_ARRAY_ACCESS: PRINT("NO_ACESSO_ARRAY\n"); break;
        case NODE_CALL: PRINT("NO_CHAMADA_FUNCAO\n"); break;
        case NODE_TYPE: PRINT("NO_TIPO (%s)\n", no->data.op_type == INT ? "INT" : "CHAR"); break;
        case NODE_ID: PRINT("NO_ID (Nome: %s)\n", no->data.string_value); break;
        case NODE_INTEGER_CONST: PRINT("NO_CONST_INT (Valor: %ld)\n", no->data.int_value); break;
        case NODE_CHAR_CONST: PRINT("NO_CONST_CHAR (Valor: '%c')\n", no->data.char_value); break;
        case NODE_STRING_CONST: PRINT("NO_CONST_STRING (Valor: \"%s\")\n", no->data.string_value); break; 
        case NODE_BINARY_OP: PRINT("NO_OP_BINARIA (%s)\n", token_name(no->data.op_type)); break;
        case NODE_UNDEFINED: PRINT("NO_INSTRUCAO_VAZIA\n"); break;
        default: PRINT("NO_DESCONHECIDO (tipo: %d)\n", no->node_type); break;
    }
    #undef PRINT

    ASTNode* filho = no->filho;
    while (filho != NULL) {
        processar_ast_recursivamente(filho, indent_level + 1, arquivo);
        filho = filho->proximo_irmao;
    }
}

/** @brief imprime a Árvore Sintática Abstrata (AST) no console. */
void imprimir_ast(ASTNode* arvore) {
    printf("--- ARVORE SINTATICA ABSTRATA (AST) ---\n");
    processar_ast_recursivamente(arvore, 0, NULL); //NULL no arquivo indica impressão no console
    printf("---------------------------------------\n");
}

/** @brief salva a Árvore Sintática Abstrata (AST) no arquivo 'ast.txt'. */
void salvar_ast(ASTNode* arvore, bool verbose) {
    FILE* saida = fopen("ast.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo ast.txt\n");
        return;
    }
    processar_ast_recursivamente(arvore, 0, saida);
    fclose(saida);
    if (verbose) {
        printf("Arquivo 'ast.txt' gerado com sucesso.\n");
    }
}

/** @brief imprime a lista de Código Intermediário (IR) de forma legível. */
void imprimir_ir(IR_Instruction* ir_head) {
    printf("--- CODIGO INTERMEDIARIO (IR) ---\n");
    IR_Instruction* instr = ir_head;
    if (!instr) {
        printf("(nenhuma instrucao gerada)\n");
    }
    
    //função interna auxiliar para imprimir um operando
    void print_op(IROperand* op) {
        if (op == NULL) { printf("_"); return; }
        switch(op->type) {
            case OPERAND_SYMBOL:
                if (op->data.symbol) printf("%s", op->data.symbol->nome);
                else printf("(simbolo nulo)");
                break;
            case OPERAND_TEMP: printf("t%d", op->data.temp_id); break;
            case OPERAND_CONST: printf("%d", op->data.const_val); break;
            case OPERAND_LABEL: printf("%s", op->data.label_name); break;
            case OPERAND_STRING_LBL: printf("%s", op->data.label_name); break;
            default: printf("OPERANDO_INVALIDO"); break;
        }
    }

    while(instr) {
        if (instr->opcode != IR_LABEL) {
            printf("    ");
        }
        switch(instr->opcode) {
            case IR_LABEL:
                print_op(instr->result);
                printf(":");
                break;
            case IR_ASSIGN:
                print_op(instr->result); printf(" := "); print_op(instr->arg1);
                break;
            case IR_LOAD:
                print_op(instr->result); printf(" := LOAD "); print_op(instr->arg1);
                printf("["); print_op(instr->arg2); printf("]");
                break;
            case IR_STORE:
                printf("STORE "); print_op(instr->result); printf("["); print_op(instr->arg2); printf("]");
                printf(" := "); print_op(instr->arg1);
                break;
            case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
            case IR_LT: case IR_EQ:
                print_op(instr->result); printf(" := ");
                print_op(instr->arg1);
                if (instr->opcode == IR_ADD) printf(" + ");
                else if (instr->opcode == IR_SUB) printf(" - ");
                else if (instr->opcode == IR_MUL) printf(" * ");
                else if (instr->opcode == IR_DIV) printf(" / ");
                else if (instr->opcode == IR_LT) printf(" < ");
                else if (instr->opcode == IR_EQ) printf(" == ");
                print_op(instr->arg2);
                break;
            case IR_RETURN:
                printf("return "); print_op(instr->arg1);
                break;
            case IR_GOTO:
                printf("goto "); print_op(instr->result);
                break;
            case IR_IF_FALSE:
                printf("if_false "); print_op(instr->arg1); printf(" goto "); print_op(instr->result);
                break;
            
            case IR_PARAM:
                printf("param "); print_op(instr->arg1);
                break;
            case IR_CALL:
                if (instr->result) {
                    print_op(instr->result); printf(" := ");
                }
                printf("call "); print_op(instr->arg1);
                if (instr->arg2) {
                    printf(", "); print_op(instr->arg2);
                }
                break;

            case IR_PRINT_INT:
                printf("print_int ");
                print_op(instr->arg1); //imprime o número ou o símbolo
                break;
            
            case IR_PRINT_CHAR:
                printf("print_char ");
                print_op(instr->arg1);
                break;

            case IR_PRINT_STRING:
                printf("print_string ");
                print_op(instr->arg1);
                break;
            
            
            
            default:
                printf("(opcode %d nao implementado para impressao)", instr->opcode);
                break;
        }
        printf("\n");
        instr = instr->next;
    }
    printf("---------------------------------\n");
}


/** @brief salva a lista de Código Intermediário (IR) no arquivo 'ir.txt'. */
void salvar_ir_em_arquivo(IR_Instruction* ir_head, bool verbose) {
    FILE *saida = fopen("ir.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo ir.txt\n");
        return;
    }

    fprintf(saida, "--- CODIGO INTERMEDIARIO (IR) ---\n");
    IR_Instruction* instr = ir_head;
    if (!instr) {
        fprintf(saida, "(nenhuma instrucao gerada)\n");
    }
    
    //função interna auxiliar para imprimir um operando no arquivo
    void print_op_to_file(IROperand* op) {
        if (op == NULL) { fprintf(saida, "_"); return; }
        switch(op->type) {
            case OPERAND_SYMBOL:
                if (op->data.symbol) fprintf(saida, "%s", op->data.symbol->nome);
                else fprintf(saida, "(simbolo nulo)");
                break;
            case OPERAND_TEMP: fprintf(saida, "t%d", op->data.temp_id); break;
            case OPERAND_CONST: fprintf(saida, "%d", op->data.const_val); break;
            case OPERAND_LABEL: fprintf(saida, "%s", op->data.label_name); break;
            case OPERAND_STRING_LBL: fprintf(saida, "%s", op->data.label_name); break;
            default: fprintf(saida, "OPERANDO_INVALIDO"); break;
        }
    }

    while(instr) {
        if (instr->opcode != IR_LABEL) {
            fprintf(saida, "    ");
        }
        switch(instr->opcode) {
            case IR_LABEL:
                print_op_to_file(instr->result);
                fprintf(saida, ":");
                break;
            case IR_ASSIGN:
                print_op_to_file(instr->result); fprintf(saida, " := "); print_op_to_file(instr->arg1);
                break;
            case IR_LOAD:
                print_op_to_file(instr->result); fprintf(saida, " := LOAD "); print_op_to_file(instr->arg1);
                fprintf(saida, "["); print_op_to_file(instr->arg2); fprintf(saida, "]");
                break;
            case IR_STORE:
                fprintf(saida, "STORE "); print_op_to_file(instr->result); fprintf(saida, "["); print_op_to_file(instr->arg2); fprintf(saida, "]");
                fprintf(saida, " := "); print_op_to_file(instr->arg1);
                break;
            case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
            case IR_LT: case IR_EQ:
                print_op_to_file(instr->result); fprintf(saida, " := ");
                print_op_to_file(instr->arg1);
                if (instr->opcode == IR_ADD) fprintf(saida, " + ");
                else if (instr->opcode == IR_SUB) fprintf(saida, " - ");
                else if (instr->opcode == IR_MUL) fprintf(saida, " * ");
                else if (instr->opcode == IR_DIV) fprintf(saida, " / ");
                else if (instr->opcode == IR_LT) fprintf(saida, " < ");
                else if (instr->opcode == IR_EQ) fprintf(saida, " == ");
                print_op_to_file(instr->arg2);
                break;
            case IR_RETURN:
                fprintf(saida, "return "); print_op_to_file(instr->arg1);
                break;
            case IR_GOTO:
                fprintf(saida, "goto "); print_op_to_file(instr->result);
                break;
            case IR_IF_FALSE:
                fprintf(saida, "if_false "); print_op_to_file(instr->arg1); fprintf(saida, " goto "); print_op_to_file(instr->result);
                break;
            case IR_PARAM:
                fprintf(saida, "param "); print_op_to_file(instr->arg1);
                break;
            case IR_CALL:
                if (instr->result) {
                    print_op_to_file(instr->result); fprintf(saida, " := ");
                }
                fprintf(saida, "call "); print_op_to_file(instr->arg1);
                if (instr->arg2) {
                    fprintf(saida, ", "); print_op_to_file(instr->arg2);
                }
                break;
            case IR_PRINT_INT:
                fprintf(saida, "print_int ");
                print_op_to_file(instr->arg1);
                break;
            
            case IR_PRINT_CHAR:
                fprintf(saida, "print_char ");
                print_op_to_file(instr->arg1);
                break;

            case IR_PRINT_STRING:
                fprintf(saida, "print_string ");
                print_op_to_file(instr->arg1);
                break;
            
            default:
                fprintf(saida, "(opcode %d nao implementado para impressao)", instr->opcode);
                break;
        }
        fprintf(saida, "\n");
        instr = instr->next;
    }
    fprintf(saida, "---------------------------------\n");
    
    fclose(saida);
    if (verbose) {
        printf("Arquivo 'ir.txt' gerado com sucesso.\n");
    }
}