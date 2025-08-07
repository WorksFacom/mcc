#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

/**
 * @file semantic.c
 * @brief implementação do analisador semântico.
 *
 * este módulo percorre a árvore sintática abstrata (ast) para realizar
 * checagens de tipo, de declaração e de escopo. ele utiliza a
 * tabela de símbolos como uma estrutura de dados auxiliar para manter o contexto
 * do código, como as variáveis, arrays e funções declaradas em cada escopo.
 */

//protótipos de funções estáticas
static void analisar_no(ASTNode* no, PilhaTabelasSimbolos* pilha, Simbolo* escopo_funcao_atual);
static TokenType get_expression_type(ASTNode* expr_node, PilhaTabelasSimbolos* pilha, int* string_len);

/** @brief função auxiliar para reportar erros semânticos e encerrar a compilação. */
static void erro_semantico(const char* msg, int linha) {
    fprintf(stderr, "Erro Semantico na linha %d: %s\n", linha, msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief determina o tipo de uma expressão e, se for uma string, seu tamanho.
 * @param expr_node o nó raiz da expressão a ser analisada.
 * @param pilha a pilha da tabela de símbolos para consulta de tipos.
 * @param string_len ponteiro para um inteiro que receberá o tamanho da string (se aplicável).
 * @return o TokenType correspondente ao tipo da expressão.
 */
static TokenType get_expression_type(ASTNode* expr_node, PilhaTabelasSimbolos* pilha, int* string_len) {
    if (expr_node == NULL) return UNDEF;
    
    if (string_len) *string_len = 0;

    switch (expr_node->node_type) {
        case NODE_INTEGER_CONST:
            return INT;

        case NODE_CHAR_CONST:
            return CHAR;
        
        case NODE_STRING_CONST:
            if (string_len) {
                *string_len = strlen(expr_node->data.string_value) + 1;
            }
            return STRINGCONST;

        case NODE_ID: {
            Simbolo* simbolo = buscar_simbolo_em_todos_escopos(pilha, expr_node->data.string_value);
            if (simbolo == NULL) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Variavel '%s' nao foi declarada.", expr_node->data.string_value);
                erro_semantico(msg, expr_node->linha);
            }
            if (simbolo->is_array) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Nome de array '%s' nao pode ser usado como uma variavel simples.", simbolo->nome);
                erro_semantico(msg, expr_node->linha);
            }
            if (simbolo->is_function) {
                char msg[200];
                snprintf(msg, sizeof(msg), "'%s' e uma funcao e nao pode ser usada como variavel.", simbolo->nome);
                erro_semantico(msg, expr_node->linha);
            }
            return simbolo->tipo;
        }
        
        case NODE_ARRAY_ACCESS: {
            ASTNode* id_no = expr_node->filho;
            Simbolo* simbolo = buscar_simbolo_em_todos_escopos(pilha, id_no->data.string_value);
            if (simbolo == NULL) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Array '%s' nao foi declarado.", id_no->data.string_value);
                erro_semantico(msg, id_no->linha);
            }
            if (!simbolo->is_array) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Variavel '%s' nao e um array e nao pode ser indexada.", simbolo->nome);
                erro_semantico(msg, id_no->linha);
            }
            return simbolo->tipo;
        }
        
        case NODE_BINARY_OP: {
            TokenType tipo_esq = get_expression_type(expr_node->filho, pilha, NULL);
            TokenType tipo_dir = get_expression_type(expr_node->filho->proximo_irmao, pilha, NULL);
            if (tipo_esq == INT && tipo_dir == INT) return INT;
            erro_semantico("Tipos incompativeis em operacao binaria (esperado INT).", expr_node->linha);
            return UNDEF;
        }

        case NODE_CALL: {
            ASTNode* id_no = expr_node->filho;
            Simbolo* func_simbolo = buscar_simbolo_em_todos_escopos(pilha, id_no->data.string_value);

            if (func_simbolo == NULL) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Funcao '%s' nao foi declarada.", id_no->data.string_value);
                erro_semantico(msg, id_no->linha);
            }
            if (!func_simbolo->is_function) {
                char msg[200];
                snprintf(msg, sizeof(msg), "'%s' nao e uma funcao e nao pode ser chamada.", func_simbolo->nome);
                erro_semantico(msg, id_no->linha);
            }

            ASTNode* arg_list_no = id_no->proximo_irmao;
            int arg_count = 0;
            if (arg_list_no && arg_list_no->node_type == NODE_ARG_LIST) {
                ASTNode* expr_arg = arg_list_no->filho;
                while (expr_arg) {
                    if (arg_count >= func_simbolo->num_parametros) {
                        erro_semantico("Numero excessivo de argumentos na chamada da funcao.", expr_arg->linha);
                    }
                    TokenType tipo_arg = get_expression_type(expr_arg, pilha, NULL);
                    if (tipo_arg != func_simbolo->param_tipos[arg_count]) {
                        erro_semantico("Tipo de argumento incompativel na chamada da funcao.", expr_arg->linha);
                    }
                    arg_count++;
                    expr_arg = expr_arg->proximo_irmao;
                }
            }
            
            if (arg_count < func_simbolo->num_parametros) {
                erro_semantico("Numero insuficiente de argumentos na chamada da funcao.", expr_node->linha);
            }

            return func_simbolo->tipo;
        }
        default:
            erro_semantico("Tipo de no inesperado dentro de uma expressao.", expr_node->linha);
            return UNDEF;
    }
}


/** @brief função recursiva que percorre a árvore para análise semântica. */
static void analisar_no(ASTNode* no, PilhaTabelasSimbolos* pilha, Simbolo* escopo_funcao_atual) {
    if (no == NULL) return;

    //ações de pré-ordem
    switch (no->node_type) {
        case NODE_BLOCK:
            empilhar_tabela(pilha);
            break;
            
        case NODE_FUNCTION_DEF: {
            ASTNode* tipo_retorno_no = no->filho;
            ASTNode* id_no = tipo_retorno_no->proximo_irmao;
            char* nome_func = id_no->data.string_value;
            TokenType tipo_retorno = tipo_retorno_no->data.op_type;

            if (buscar_simbolo_no_escopo_atual(pilha, nome_func)) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Funcao '%s' ja foi declarada.", nome_func);
                erro_semantico(msg, id_no->linha);
            }
            adicionar_simbolo(pilha, nome_func, tipo_retorno, 1, 0, 0);
            escopo_funcao_atual = buscar_simbolo_no_escopo_atual(pilha, nome_func);
            
            //lógica de descida especial para funções
            empilhar_tabela(pilha); //escopo dos parâmetros e corpo
            ASTNode* filho = id_no->proximo_irmao; //começa a partir dos parâmetros
            while (filho != NULL) {
                analisar_no(filho, pilha, escopo_funcao_atual);
                filho = filho->proximo_irmao;
            }
            desempilhar_tabela(pilha); //sai do escopo da função
            return; //impede a descida genérica do final da função
        }

        case NODE_VAR_DECL: {
            ASTNode* tipo_no = no->filho;
            ASTNode* id_no = tipo_no->proximo_irmao;
            char* nome_var = id_no->data.string_value;
            TokenType tipo_var = tipo_no->data.op_type;
            
            int is_array = 0;
            int array_size = 0;
            ASTNode* proximo_no = id_no->proximo_irmao;
            
            if (proximo_no && proximo_no->node_type == NODE_INTEGER_CONST) {
                is_array = 1;
                array_size = proximo_no->data.int_value;
                proximo_no = proximo_no->proximo_irmao;
            }

            if (buscar_simbolo_no_escopo_atual(pilha, nome_var)) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Variavel ou parametro '%s' ja foi declarado neste escopo.", nome_var);
                erro_semantico(msg, id_no->linha);
            }
            adicionar_simbolo(pilha, nome_var, tipo_var, 0, is_array, array_size);

            if (escopo_funcao_atual && pilha->topo == 1) { 
                 if (escopo_funcao_atual->num_parametros < MAX_PARAMETROS) {
                     escopo_funcao_atual->param_tipos[escopo_funcao_atual->num_parametros] = tipo_var;
                     escopo_funcao_atual->num_parametros++;
                 }
            }

            if (proximo_no && proximo_no->node_type == NODE_ASSIGN) {
                int string_len = 0;
                TokenType tipo_expr = get_expression_type(proximo_no->filho->proximo_irmao, pilha, &string_len);

                if (is_array) {
                    if (tipo_var == CHAR && tipo_expr == STRINGCONST) {
                        if (string_len > array_size) {
                            char msg[200];
                            snprintf(msg, sizeof(msg), "String e muito grande para o array '%s' (tamanho %d, necessario %d).", nome_var, array_size, string_len);
                            erro_semantico(msg, proximo_no->linha);
                        }
                    } else {
                        erro_semantico("Arrays do tipo 'int' nao podem ser inicializados com string, e arrays 'char' so com string literal.", proximo_no->linha);
                    }
                } else {
                    if (tipo_var != tipo_expr) {
                        erro_semantico("Tipos incompativeis na inicializacao da variavel.", proximo_no->linha);
                    }
                }
            }
            return;
        }

        case NODE_ARRAY_ACCESS: {
            ASTNode* id_no = no->filho;
            ASTNode* index_expr_no = id_no->proximo_irmao;
            
            Simbolo* simbolo = buscar_simbolo_em_todos_escopos(pilha, id_no->data.string_value);
            if (!simbolo) {
                 char msg[200];
                 snprintf(msg, sizeof(msg), "Array '%s' nao foi declarado.", id_no->data.string_value);
                 erro_semantico(msg, id_no->linha);
            }
            if (!simbolo->is_array) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Variavel '%s' nao e um array e nao pode ser indexada.", simbolo->nome);
                erro_semantico(msg, id_no->linha);
            }
            if (get_expression_type(index_expr_no, pilha, NULL) != INT) {
                erro_semantico("Indice de array deve ser do tipo inteiro.", index_expr_no->linha);
            }

            if (index_expr_no->node_type == NODE_INTEGER_CONST) {
                long index_val = index_expr_no->data.int_value;
                if (index_val >= simbolo->array_size) {
                    char msg[200];
                    snprintf(msg, sizeof(msg), "Indice de array (%ld) fora dos limites do array '%s' (tamanho %d).", index_val, simbolo->nome, simbolo->array_size);
                    erro_semantico(msg, index_expr_no->linha);
                }
            }
            break;
        }
        
        case NODE_ASSIGN: {
            TokenType tipo_esq = get_expression_type(no->filho, pilha, NULL);
            TokenType tipo_dir = get_expression_type(no->filho->proximo_irmao, pilha, NULL);
            if (tipo_esq != tipo_dir) {
                erro_semantico("Atribuicao de tipos incompativeis.", no->linha);
            }
            break;
        }
        
        case NODE_RETURN: {
            if (escopo_funcao_atual == NULL) {
                erro_semantico("Comando 'return' encontrado fora de uma funcao.", no->linha);
            }
            TokenType tipo_retornado = (no->filho) ? get_expression_type(no->filho, pilha, NULL) : UNDEF;
            
            if (escopo_funcao_atual->tipo != tipo_retornado) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Tipo de retorno incompativel para a funcao '%s'.", escopo_funcao_atual->nome);
                erro_semantico(msg, no->linha);
            }
            break;
        }
        default: break;
    }

    //visita recursiva padrão
    ASTNode* filho = no->filho;
    while (filho != NULL) {
        analisar_no(filho, pilha, escopo_funcao_atual);
        filho = filho->proximo_irmao;
    }

    //ações de pós-ordem
    if (no->node_type == NODE_BLOCK) {
        desempilhar_tabela(pilha);
    }
}


/** @brief função principal que dispara a análise semântica. */
void analisar_semanticamente(ASTNode* raiz, PilhaTabelasSimbolos* pilha) {
    if (raiz == NULL) {
        printf("Aviso: Arvore sintatica vazia. Nada para analisar.\n");
        return;
    }
    analisar_no(raiz, pilha, NULL);
    printf("Analise Semantica concluida com sucesso.\n");
}