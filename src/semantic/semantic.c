#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "semantic.h"

/**
 * @file semantic.c
 * @brief implementação do analisador semântico.
 *
 * este módulo percorre a árvore sintática abstrata (ast) para realizar
 * checagens de tipo, de declaração e de escopo. ele utiliza a
 * tabela de símbolos e calcula os deslocamentos de memória para o back-end.
 * Além disso, gera uma representação visual da tabela de símbolos.
 */

// Variáveis para controle da saída visual da tabela de símbolos
static FILE* arquivo_simbolos = NULL;
static bool modo_verbose = false;

//protótipos de funções estáticas
static void analisar_no(ASTNode* no, PilhaTabelasSimbolos* pilha, Simbolo* escopo_funcao_atual, bool is_param_decl);
static TokenType get_expression_type(ASTNode* expr_node, PilhaTabelasSimbolos* pilha, int* string_len);
static int get_symbol_size(Simbolo* s);
static void calcular_offsets(TabelaSimbolos* tabela);

/** @brief função auxiliar para reportar erros semânticos e encerrar a compilação. */
static void erro_semantico(const char* msg, int linha) {
    fprintf(stderr, "Erro Semantico na linha %d: %s\n", linha, msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief calcula o tamanho em bytes de um símbolo para alocação na pilha.
 * @param s o símbolo a ser medido.
 * @return o tamanho em bytes do símbolo.
 */
static int get_symbol_size(Simbolo* s) {
    //se for CHAR, o tamanho base é 1 byte. Se for INT, é 4 bytes.
    int tipo_base_size = (s->tipo == CHAR) ? 1 : 4;
    
    if (s->is_array) {
        return s->array_size * tipo_base_size;
    }
    return tipo_base_size;
}

/**
 * @brief percorre uma tabela de um escopo e calcula os offsets de memória.
 *
 * esta função é chamada ao final da análise de um escopo de função.
 * ela faz duas passagens:
 * 1. Calcula offsets POSITIVOS para parâmetros (ex: [rbp+16], [rbp+24]).
 * 2. Calcula offsets NEGATIVOS para variáveis locais (ex: [rbp-4], [rbp-8]).
 * @param tabela a tabela de símbolos do escopo da função.
 */
static void calcular_offsets(TabelaSimbolos* tabela) {
    //padrão da convenção de chamada 'System V AMD64 ABI' (usada pelo Linux/macOS)
    //os primeiros 6 argumentos inteiros/ponteiros vão para registradores.
    //argumentos adicionais e parâmetros na stack começam em [rbp+16].
    //vamos usar +16 para o primeiro parâmetro na pilha para simplificar.
    //(em uma implementação real, os 6 primeiros seriam tratados de forma diferente)
    
    //começamos o offset dos parâmetros em +16
    int param_offset = 16; 
    int local_offset = 0;

    //passagem 1: calcular offsets de parâmetros
    //(iteramos na ordem de declaração para que o primeiro parâmetro tenha o menor offset)
    for (int i = 0; i < tabela->tamanho; i++) {
        Simbolo* s = &tabela->simbolos[i];
        if (s->is_function) continue; //ignora sub-funções

        if (s->is_parameter) {
            s->memory_offset = param_offset;
            //assumindo que cada parâmetro ocupa 8 bytes na pilha (padrão 64-bit)
            param_offset += 8; 
        }
    }
    
    //passagem 2: calcular offsets de variáveis locais
    //(iteramos na ordem de declaração)
    for (int i = 0; i < tabela->tamanho; i++) {
        Simbolo* s = &tabela->simbolos[i];
        if (s->is_function) continue; //ignora sub-funções

        if (!s->is_parameter) { //se for uma variável local
            int tamanho_simbolo = get_symbol_size(s);
            local_offset -= tamanho_simbolo; //pilha cresce para baixo
            s->memory_offset = local_offset;
        }
    }
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
        case NODE_INTEGER_CONST: return INT;
        case NODE_CHAR_CONST: return CHAR;
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
            
            // --- LINKAGEM: SALVA O PONTEIRO DO SIMBOLO NO NO DA AST ---
            expr_node->symbol = simbolo;
            // ----------------------------------------------------------
            
            if (simbolo->is_array) {
                // Se for array de CHAR, permitimos uso como variável simples (ponteiro/string)
                if (simbolo->tipo == CHAR) {
                    return STRINGCONST; 
                } 
                // Se for array de INT (ou outro), mantemos a proibição
                else {
                    char msg[200];
                    snprintf(msg, sizeof(msg), "O array '%s' (tipo int) nao pode ser usado sem indice (apenas arrays char/strings).", simbolo->nome);
                    erro_semantico(msg, expr_node->linha);
                }
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
            
            // --- LINKAGEM: SALVA O SIMBOLO NO FILHO (ID) ---
            id_no->symbol = simbolo;
            // -----------------------------------------------

            if (!simbolo->is_array) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Variavel '%s' nao e um array e nao pode ser indexada.", simbolo->nome);
                erro_semantico(msg, id_no->linha);
            }
            
            // Recursão para validar o índice (e linkar símbolos dentro dele)
            if (get_expression_type(id_no->proximo_irmao, pilha, NULL) != INT) {
                erro_semantico("Indice de array deve ser do tipo inteiro.", expr_node->linha);
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
            
            // --- LINKAGEM: SALVA O SIMBOLO DA FUNCAO ---
            id_no->symbol = func_simbolo;
            // -------------------------------------------

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
static void analisar_no(ASTNode* no, PilhaTabelasSimbolos* pilha, Simbolo* escopo_funcao_atual, bool is_param_decl) {
    if (no == NULL) return;

    bool novo_escopo = false;
    Simbolo* proxima_funcao_escopo = escopo_funcao_atual;

    //ações de pré-ordem
    switch (no->node_type) {
        case NODE_BLOCK:
            empilhar_tabela(pilha);
            novo_escopo = true;
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
            
            // --- LINKAGEM: Captura o símbolo e salva no nó ---
            Simbolo* s = adicionar_simbolo(pilha, nome_func, tipo_retorno, 1, 0, 0, 0); //0 = não é parâmetro
            id_no->symbol = s;
            // -------------------------------------------------
            
            //define o escopo da função atual para os nós filhos
            proxima_funcao_escopo = s;
            
            //1. cria o escopo da função (para params e locais)
            empilhar_tabela(pilha);
            novo_escopo = true; 

            //2. itera manualmente nos filhos
            ASTNode* child = no->filho;
            ASTNode* block_node = NULL;
            
            while(child) {
                if (child->node_type == NODE_BLOCK) {
                    //3. encontra o bloco, mas não o analisa ainda.
                    block_node = child;
                } else {
                    //4. analisa os nós que vêm antes do bloco (type, id, param_list)
                    bool proximo_e_param = (child->node_type == NODE_PARAM_LIST);
                    analisar_no(child, pilha, proxima_funcao_escopo, proximo_e_param);
                }
                child = child->proximo_irmao;
            }
            
            //5. agora, analisa os *filhos* do bloco, mas não o bloco em si.
            //isso força as 'var_decl' a caírem no escopo atual (o da função)
            if (block_node) {
                ASTNode* block_child = block_node->filho;
                while (block_child) {
                    //'false' porque não são mais declarações de parâmetros
                    analisar_no(block_child, pilha, proxima_funcao_escopo, false); 
                    block_child = block_child->proximo_irmao;
                }
            }

            //6. ações de pós-ordem (só para este nó)
            if (novo_escopo) {
                calcular_offsets(pilha->tabelas[pilha->topo]);
                
                // --- IMPRESSÃO DA TABELA DE SÍMBOLOS (FUNÇÃO) ---
                if (arquivo_simbolos) {
                    char titulo[100];
                    sprintf(titulo, "Funcao: %s", nome_func);
                    imprimir_tabela(pilha->tabelas[pilha->topo], titulo, arquivo_simbolos);
                    if (modo_verbose) {
                        imprimir_tabela(pilha->tabelas[pilha->topo], titulo, stdout);
                    }
                }
                // -----------------------------------------------

                //nota: não desempilhamos aqui para manter a memória viva para o backend.
                //a limpeza real ocorre no final do programa.
                desempilhar_tabela(pilha);
            }
            
            //7. retorna para impedir o loop recursivo padrão no final de analisar_no
            return; 
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
            
            // --- LINKAGEM: Captura o símbolo e salva no nó ---
            Simbolo* s = adicionar_simbolo(pilha, nome_var, tipo_var, 0, is_array, array_size, is_param_decl);
            id_no->symbol = s;
            // -------------------------------------------------

            if (is_param_decl) { 
                 if (escopo_funcao_atual->num_parametros < MAX_PARAMETROS) {
                     escopo_funcao_atual->param_tipos[escopo_funcao_atual->num_parametros] = tipo_var;
                     escopo_funcao_atual->num_parametros++;
                 }
            }

            if (proximo_no && proximo_no->node_type == NODE_ASSIGN) {
                int string_len = 0;
                // Importante: Chama get_expression_type para o lado direito para garantir a linkagem dos símbolos lá!
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
            return; //retorna para não visitar os filhos de var_decl
        }
        
        // --- CORREÇÃO: Forçar linkagem em estruturas de controle ---
        case NODE_IF: {
            ASTNode* cond = no->filho;
            get_expression_type(cond, pilha, NULL); // Linka símbolos na condição
            break; 
        }
        case NODE_FOR: {
            ASTNode* init = no->filho;
            ASTNode* cond = init->proximo_irmao;
            ASTNode* inc = cond->proximo_irmao;
            
            // Linka símbolos na condição e incremento
            if (cond && cond->node_type != NODE_UNDEFINED) get_expression_type(cond, pilha, NULL);
            if (inc) {
                 if (inc->node_type == NODE_ASSIGN) {
                     get_expression_type(inc->filho, pilha, NULL);
                     get_expression_type(inc->filho->proximo_irmao, pilha, NULL);
                 } else {
                     get_expression_type(inc, pilha, NULL);
                 }
            }
            break;
        }
        case NODE_RETURN: {
            if (no->filho) get_expression_type(no->filho, pilha, NULL);
            // Continua a verificação semântica padrão abaixo
            if (escopo_funcao_atual == NULL) {
                erro_semantico("Comando 'return' encontrado fora de uma funcao.", no->linha);
            }
            TokenType tipo_retornado = (no->filho) ? get_expression_type(no->filho, pilha, NULL) : UNDEF;
            if (escopo_funcao_atual->tipo != tipo_retornado) {
                char msg[200];
                snprintf(msg, sizeof(msg), "Tipo de retorno incompativel para a funcao '%s'.", escopo_funcao_atual->nome);
                erro_semantico(msg, no->linha);
            }
            return; // Já analisado
        }
        case NODE_PRINT: {
            if (no->filho) get_expression_type(no->filho, pilha, NULL);
            // Continua verificação abaixo
            ASTNode* expr_no = no->filho;
            if (expr_no == NULL) {
                erro_semantico("Comando 'print' espera um argumento para imprimir.", no->linha);
            }
            TokenType tipo_expr = get_expression_type(expr_no, pilha, NULL);
            if (tipo_expr != INT && tipo_expr != CHAR && tipo_expr != STRINGCONST) {
                erro_semantico("Argumento invalido para 'print'. So e possivel imprimir int, char ou uma string literal.", expr_no->linha);
            }
            return;
        }
        case NODE_ASSIGN: {
            // Garante linkagem em ambos os lados
            get_expression_type(no->filho, pilha, NULL);
            get_expression_type(no->filho->proximo_irmao, pilha, NULL);
            
            TokenType tipo_esq = get_expression_type(no->filho, pilha, NULL);
            TokenType tipo_dir = get_expression_type(no->filho->proximo_irmao, pilha, NULL);
            if (tipo_esq != tipo_dir) {
                erro_semantico("Atribuicao de tipos incompativeis.", no->linha);
            }
            return;
        }
        case NODE_CALL: {
            get_expression_type(no, pilha, NULL);
            return;
        }
        case NODE_ARRAY_ACCESS: {
            get_expression_type(no, pilha, NULL);
            return;
        }
        // -----------------------------------------------------------

        default: break;
    }

    //lógica de descida na árvore (unificada)
    ASTNode* filho = no->filho;
    while (filho != NULL) {
        //define a flag 'is_param_decl' corretamente ANTES de visitar o filho
        bool proximo_e_param = (no->node_type == NODE_FUNCTION_DEF && filho->node_type == NODE_PARAM_LIST) ||
                               (no->node_type == NODE_PARAM_LIST);
                               
        analisar_no(filho, pilha, proxima_funcao_escopo, proximo_e_param);
        filho = filho->proximo_irmao;
    }

    //ações de pós-ordem (unificada)
    if (novo_escopo) {
        //calcula os offsets para as variáveis deste escopo
        calcular_offsets(pilha->tabelas[pilha->topo]);
        
        // --- IMPRESSÃO DE BLOCOS ANÔNIMOS (IF/FOR/WHILE) ---
        // Apenas se for um bloco puro (não função) e tivermos arquivo aberto
        if (no->node_type == NODE_BLOCK && arquivo_simbolos) {
            char titulo[50];
            sprintf(titulo, "Bloco Interno (Linha %d)", no->linha);
            imprimir_tabela(pilha->tabelas[pilha->topo], titulo, arquivo_simbolos);
            if (modo_verbose) {
                imprimir_tabela(pilha->tabelas[pilha->topo], titulo, stdout);
            }
        }
        // ---------------------------------------------------
        
        desempilhar_tabela(pilha); 

    }
}


/** @brief função principal que dispara a análise semântica. */
void analisar_semanticamente(ASTNode* raiz, PilhaTabelasSimbolos* pilha, bool verbose) {
    if (raiz == NULL) {
        printf("Aviso: Arvore sintatica vazia. Nada para analisar.\n");
        return;
    }

    // Configura os arquivos e modo verbose
    modo_verbose = verbose;
    arquivo_simbolos = fopen("symbols.txt", "w");
    if (!arquivo_simbolos) {
        perror("Aviso: Nao foi possivel criar symbols.txt");
    }

    analisar_no(raiz, pilha, NULL, false);
    
    // --- IMPRIME O ESCOPO GLOBAL (BASE DA PILHA) AO FINAL ---
    if (arquivo_simbolos && pilha->topo >= 0) {
        imprimir_tabela(pilha->tabelas[0], "GLOBAL", arquivo_simbolos);
        if (modo_verbose) {
            imprimir_tabela(pilha->tabelas[0], "GLOBAL", stdout);
        }
    }
    // --------------------------------------------------------

    if (arquivo_simbolos) {
        fclose(arquivo_simbolos);
        arquivo_simbolos = NULL;
    }
}