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
#include <stdlib.h>
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "ir/ir.h"
#include "intercode/intercode.h"
#include "assembly/assembly.h"

//definicoes
#define VERSION "0.1"


//protótipos de funções auxiliares
void imprimir_tokens(TokenList* tokens);
void salvar_tokens_em_arquivo(TokenList* tokens, bool verbose);
void imprimir_ast(ASTNode* arvore);
void salvar_ast(ASTNode* arvore, bool verbose);
void imprimir_ir(IR_Instruction* ir_head);
void salvar_ir_em_arquivo(IR_Instruction* ir_head, bool verbose); 
int executar_analise_lexica(const char* nome_arquivo, TokenList* lista);
static void imprimir_operando_ir(FILE* saida, IROperand* operando);
static void imprimir_uso(const char* programa);

/** @brief imprime a sintaxe aceita pela interface de linha de comando. */
static void imprimir_uso(const char* programa) {
    fprintf(stderr,
            "Uso: %s [--scan|--parse|--semantic|--gen-ir|--gen-asm] "
            "<arquivo.mcc> [-o arquivo.s]\n",
            programa);
}

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
            finalizar_scanner();
            return -1;
        }
    } while (token.tipo != END_OF_FILE);

    finalizar_scanner();
    return 0;
}

/**
 * @brief ponto de entrada principal do compilador.
 */
int main(int argc, char* argv[]) {
    const char* modo = "completo";
    const char* nome_arquivo = NULL;
    const char* saida_solicitada = NULL;
    bool modo_definido = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc || saida_solicitada != NULL) {
                imprimir_uso(argv[0]);
                return EXIT_FAILURE;
            }
            saida_solicitada = argv[i];
        } else if (strcmp(argv[i], "--scan") == 0 ||
                   strcmp(argv[i], "--parse") == 0 ||
                   strcmp(argv[i], "--semantic") == 0 ||
                   strcmp(argv[i], "--gen-ir") == 0 ||
                   strcmp(argv[i], "--gen-asm") == 0) {
            if (modo_definido) {
                fprintf(stderr, "Erro: Mais de um modo de compilacao foi informado.\n");
                return EXIT_FAILURE;
            }
            modo = argv[i];
            modo_definido = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            imprimir_uso(argv[0]);
            return EXIT_SUCCESS;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Erro: Opcao desconhecida '%s'.\n", argv[i]);
            imprimir_uso(argv[0]);
            return EXIT_FAILURE;
        } else if (nome_arquivo == NULL) {
            nome_arquivo = argv[i];
        } else {
            fprintf(stderr, "Erro: Mais de um arquivo fonte foi informado.\n");
            return EXIT_FAILURE;
        }
    }

    if (nome_arquivo == NULL) {
        fprintf(stderr, "Erro: Arquivo fonte nao especificado.\n");
        imprimir_uso(argv[0]);
        return EXIT_FAILURE;
    }
    if (saida_solicitada != NULL && strcmp(modo, "--gen-asm") != 0 && strcmp(modo, "completo") != 0) {
        fprintf(stderr, "Erro: A opcao -o so pode ser usada na geracao de assembly.\n");
        return EXIT_FAILURE;
    }
    
    //fase 1: análise léxica
    TokenList tokens;
    if (inicializar_token_list(&tokens, TOKEN_LIST) != 0) return EXIT_FAILURE;
    if (executar_analise_lexica(nome_arquivo, &tokens) != 0) {
        liberar_token_list(&tokens);
        return EXIT_FAILURE; //encerra se houver erro léxico
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
    // Determina se deve imprimir no terminal
    bool verbose_semantic = (strcmp(modo, "--semantic") == 0);
    
    // Chama a análise passando o flag de verbose (arquivo symbols.txt sempre é gerado)
    analisar_semanticamente(arvore, pilha_simbolos, verbose_semantic);
    
    printf("Fase 3 (Semantica) concluida: Codigo validado.\n");


    //modo semantic
    if (strcmp(modo, "--semantic") == 0) {
        printf("\n--- MODO DE ANALISE SEMANTICA CONCLUIDO ---\n");
        printf("Arquivo 'symbols.txt' gerado com sucesso.\n"); // Feedback visual
        liberar_arvore(arvore);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0;
    }
    
    //fase 4: geração de código intermediário
    IR_Instruction* ir_head = gerar_codigo_intermediario(arvore, pilha_simbolos);
    printf("Fase 4 (Geracao de IR) concluida.\n");
    
    //salva o arquivo de ir silenciosamente
    salvar_ir_em_arquivo(ir_head, false); 

    //modo gen-ir
    if (strcmp(modo, "--gen-ir") == 0) {
        printf("\n--- MODO DE GERACAO DE CODIGO INTERMEDIARIO ---\n");
        imprimir_ir(ir_head);
        salvar_ir_em_arquivo(ir_head, true); 

        //limpeza de memória com a ordem correta
        liberar_arvore(arvore);
        liberar_ir(ir_head); 
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return 0;
    }

    //fase 5: geração de código assembly
    //define o nome do arquivo de saída (ex: teste.cmm -> teste.s)
    char nome_arquivo_asm[MAX_PATH_LEN];
    if (saida_solicitada != NULL) {
        if (snprintf(nome_arquivo_asm, sizeof(nome_arquivo_asm), "%s", saida_solicitada) >= (int)sizeof(nome_arquivo_asm)) {
            fprintf(stderr, "Erro: Caminho de saida excede o limite suportado.\n");
            return EXIT_FAILURE;
        }
    } else {
        if (snprintf(nome_arquivo_asm, sizeof(nome_arquivo_asm), "%s", nome_arquivo) >= (int)sizeof(nome_arquivo_asm) - 2) {
            fprintf(stderr, "Erro: Caminho de entrada excede o limite suportado.\n");
            return EXIT_FAILURE;
        }
        char* ponto = strrchr(nome_arquivo_asm, '.');
        if (ponto != NULL) *ponto = '\0';
        strncat(nome_arquivo_asm, ".s", sizeof(nome_arquivo_asm) - strlen(nome_arquivo_asm) - 1);
    }

    if (gerar_assembly(ir_head, pilha_simbolos, nome_arquivo_asm) != 0) {
        liberar_arvore(arvore);
        liberar_ir(ir_head);
        destruir_pilha_tabelas(pilha_simbolos);
        liberar_token_list(&tokens);
        return EXIT_FAILURE; //encerra se houver erro na geração de assembly
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
        return 0; 
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
    
    FILE* destino = arquivo != NULL ? arquivo : stdout;
    for (int i = 0; i < indent_level; ++i) fputs("  ", destino);
    switch (no->node_type) {
        case NODE_PROGRAM: fprintf(destino, "NO_PROGRAMA\n"); break;
        case NODE_FUNCTION_DEF: fprintf(destino, "NO_DEFINICAO_FUNCAO\n"); break;
        case NODE_VAR_DECL: fprintf(destino, "NO_DECLARACAO_VARIAVEL\n"); break;
        case NODE_ASSIGN: fprintf(destino, "NO_ATRIBUICAO\n"); break;
        case NODE_IF: fprintf(destino, "NO_IF\n"); break;
        case NODE_FOR: fprintf(destino, "NO_FOR\n"); break;
        case NODE_RETURN: fprintf(destino, "NO_RETORNO\n"); break;
        case NODE_PRINT: fprintf(destino, "NO_PRINT\n"); break;
        case NODE_BLOCK: fprintf(destino, "NO_BLOCO\n"); break;
        case NODE_PARAM_LIST: fprintf(destino, "NO_LISTA_PARAMETROS\n"); break;
        case NODE_ARG_LIST: fprintf(destino, "NO_LISTA_ARGUMENTOS\n"); break;
        case NODE_ARRAY_ACCESS: fprintf(destino, "NO_ACESSO_ARRAY\n"); break;
        case NODE_CALL: fprintf(destino, "NO_CHAMADA_FUNCAO\n"); break;
        case NODE_TYPE: fprintf(destino, "NO_TIPO (%s)\n", no->data.op_type == INT ? "INT" : "CHAR"); break;
        case NODE_ID: fprintf(destino, "NO_ID (Nome: %s)\n", no->data.string_value); break;
        case NODE_INTEGER_CONST: fprintf(destino, "NO_CONST_INT (Valor: %ld)\n", no->data.int_value); break;
        case NODE_CHAR_CONST: fprintf(destino, "NO_CONST_CHAR (Valor: '%c')\n", no->data.char_value); break;
        case NODE_STRING_CONST: fprintf(destino, "NO_CONST_STRING (Valor: \"%s\")\n", no->data.string_value); break;
        case NODE_BINARY_OP: fprintf(destino, "NO_OP_BINARIA (%s)\n", token_name(no->data.op_type)); break;
        case NODE_UNARY_OP: fprintf(destino, "NO_OP_UNARIA (%s)\n", token_name(no->data.op_type)); break;
        case NODE_UNDEFINED: fprintf(destino, "NO_INSTRUCAO_VAZIA\n"); break;
        default: fprintf(destino, "NO_DESCONHECIDO (tipo: %d)\n", no->node_type); break;
    }

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

/** @brief imprime um operando da ir no fluxo informado. */
static void imprimir_operando_ir(FILE* saida, IROperand* operando) {
    if (operando == NULL) {
        fprintf(saida, "_");
        return;
    }
    switch (operando->type) {
        case OPERAND_SYMBOL:
            if (operando->data.symbol != NULL) fprintf(saida, "%s", operando->data.symbol->nome);
            else fprintf(saida, "(simbolo nulo)");
            break;
        case OPERAND_TEMP: fprintf(saida, "t%d", operando->data.temp_id); break;
        case OPERAND_CONST: fprintf(saida, "%d", operando->data.const_val); break;
        case OPERAND_LABEL:
        case OPERAND_STRING_LBL: fprintf(saida, "%s", operando->data.label_name); break;
        default: fprintf(saida, "OPERANDO_INVALIDO"); break;
    }
}

/** @brief retorna a representação textual de um operador binário. */
static const char* simbolo_operador_ir(IROpcode opcode) {
    switch (opcode) {
        case IR_ADD: return "+";
        case IR_SUB: return "-";
        case IR_MUL: return "*";
        case IR_DIV: return "/";
        case IR_MOD: return "%";
        case IR_EQ: return "==";
        case IR_NEQ: return "!=";
        case IR_LT: return "<";
        case IR_LEQ: return "<=";
        case IR_GT: return ">";
        case IR_GEQ: return ">=";
        case IR_AND: return "&&";
        case IR_OR: return "||";
        default: return "?";
    }
}

/** @brief escreve uma lista de ir no fluxo solicitado. */
static void imprimir_ir_no_fluxo(FILE* saida, IR_Instruction* ir_head) {
    fprintf(saida, "--- CODIGO INTERMEDIARIO (IR) ---\n");
    if (ir_head == NULL) fprintf(saida, "(nenhuma instrucao gerada)\n");

    for (IR_Instruction* instr = ir_head; instr != NULL; instr = instr->next) {
        if (instr->opcode != IR_LABEL) fputs("    ", saida);
        switch (instr->opcode) {
            case IR_LABEL:
                imprimir_operando_ir(saida, instr->result);
                fputc(':', saida);
                break;
            case IR_ASSIGN:
                imprimir_operando_ir(saida, instr->result);
                fputs(" := ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                break;
            case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
            case IR_EQ: case IR_NEQ: case IR_LT: case IR_LEQ: case IR_GT:
            case IR_GEQ: case IR_AND: case IR_OR:
                imprimir_operando_ir(saida, instr->result);
                fputs(" := ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                fprintf(saida, " %s ", simbolo_operador_ir(instr->opcode));
                imprimir_operando_ir(saida, instr->arg2);
                break;
            case IR_NEG:
            case IR_NOT:
                imprimir_operando_ir(saida, instr->result);
                fprintf(saida, " := %s", instr->opcode == IR_NEG ? "-" : "!");
                imprimir_operando_ir(saida, instr->arg1);
                break;
            case IR_LOAD:
            case IR_LOAD_BYTE:
                imprimir_operando_ir(saida, instr->result);
                fprintf(saida, " := %s ", instr->opcode == IR_LOAD ? "LOAD" : "LOAD_BYTE");
                imprimir_operando_ir(saida, instr->arg1);
                fputc('[', saida);
                imprimir_operando_ir(saida, instr->arg2);
                fputc(']', saida);
                break;
            case IR_STORE:
            case IR_STORE_BYTE:
                fprintf(saida, "%s ", instr->opcode == IR_STORE ? "STORE" : "STORE_BYTE");
                imprimir_operando_ir(saida, instr->result);
                fputc('[', saida);
                imprimir_operando_ir(saida, instr->arg2);
                fputs("] := ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                break;
            case IR_STRCPY:
                fputs("strcpy ", saida);
                imprimir_operando_ir(saida, instr->result);
                fputs(", ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                break;
            case IR_RETURN:
                fputs("return ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                break;
            case IR_GOTO:
                fputs("goto ", saida);
                imprimir_operando_ir(saida, instr->result);
                break;
            case IR_IF_FALSE:
                fputs("if_false ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                fputs(" goto ", saida);
                imprimir_operando_ir(saida, instr->result);
                break;
            case IR_PARAM:
                fputs("param ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                break;
            case IR_CALL:
                imprimir_operando_ir(saida, instr->result);
                fputs(" := call ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                fputs(", ", saida);
                imprimir_operando_ir(saida, instr->arg2);
                break;
            case IR_PRINT_INT:
            case IR_PRINT_CHAR:
            case IR_PRINT_STRING:
                if (instr->opcode == IR_PRINT_INT) fputs("print_int ", saida);
                if (instr->opcode == IR_PRINT_CHAR) fputs("print_char ", saida);
                if (instr->opcode == IR_PRINT_STRING) fputs("print_string ", saida);
                imprimir_operando_ir(saida, instr->arg1);
                break;
            default:
                fprintf(saida, "(opcode invalido %d)", instr->opcode);
                break;
        }
        fputc('\n', saida);
    }
    fprintf(saida, "---------------------------------\n");
}

/** @brief imprime a lista de código intermediário no terminal. */
void imprimir_ir(IR_Instruction* ir_head) {
    imprimir_ir_no_fluxo(stdout, ir_head);
}

/** @brief salva a lista de código intermediário no arquivo `ir.txt`. */
void salvar_ir_em_arquivo(IR_Instruction* ir_head, bool verbose) {
    FILE* saida = fopen("ir.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo ir.txt\n");
        return;
    }
    imprimir_ir_no_fluxo(saida, ir_head);
    fclose(saida);
    if (verbose) printf("Arquivo 'ir.txt' gerado com sucesso.\n");
}
