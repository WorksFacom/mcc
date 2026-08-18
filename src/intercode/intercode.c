/**
 * @file intercode.c
 * @brief implementação do gerador de código intermediário.
 *
 * este módulo percorre a ast validada e a traduz para uma representação
 * intermediária linear em formato de código de três endereços.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intercode.h"

//================================================================================
// ESTADO DA GERAÇÃO
//================================================================================

static int temp_counter = 0;
static int label_counter = 0;
static Simbolo* funcao_atual = NULL;
static IR_Instruction* ir_list_head = NULL;
static IR_Instruction* ir_list_tail = NULL;

//================================================================================
// FUNÇÕES AUXILIARES
//================================================================================

static IROperand* criar_operando(OperandType type);
static IROperand* criar_operando_temporario(void);
static IROperand* criar_operando_label_novo(void);
static IROperand* criar_operando_label_nome(const char* nome);
static void emitir(IROpcode op, const IROperand* res, const IROperand* arg1, const IROperand* arg2);
static void gerar_ir_no(ASTNode* no);
static IROperand* gerar_ir_expr(ASTNode* no);
static IROperand* gerar_ir_call(ASTNode* no);

/** @brief encerra a compilação quando uma alocação da ir falha. */
static void erro_fatal_ir(void) {
    fprintf(stderr, "Erro fatal: Falha ao alocar memoria para a IR.\n");
    exit(EXIT_FAILURE);
}

/** @brief cria uma cópia portável de uma string. */
static char* duplicar_string(const char* texto) {
    size_t tamanho = strlen(texto) + 1;
    char* copia = malloc(tamanho);
    if (copia == NULL) erro_fatal_ir();
    memcpy(copia, texto, tamanho);
    return copia;
}

/** @brief alinha um tamanho para o próximo múltiplo de 16 bytes. */
static int alinhar_16(int tamanho) {
    return (tamanho + 15) & ~15;
}

/** @brief aloca e inicializa completamente um operando. */
static IROperand* criar_operando(OperandType type) {
    IROperand* op = calloc(1, sizeof(*op));
    if (op == NULL) erro_fatal_ir();
    op->type = type;
    return op;
}

/** @brief cria um temporário na região exclusiva do frame atual. */
static IROperand* criar_operando_temporario(void) {
    IROperand* op = criar_operando(OPERAND_TEMP);
    op->data.temp_id = temp_counter++;

    if (funcao_atual != NULL) {
        op->stack_offset = -(funcao_atual->local_size + (op->data.temp_id + 1) * 4);
    }
    return op;
}

/** @brief cria um rótulo interno único. */
static IROperand* criar_operando_label_novo(void) {
    char nome[32];
    snprintf(nome, sizeof(nome), ".L%d", label_counter++);
    return criar_operando_label_nome(nome);
}

/** @brief cria um operando de rótulo com nome próprio. */
static IROperand* criar_operando_label_nome(const char* nome) {
    IROperand* op = criar_operando(OPERAND_LABEL);
    op->data.label_name = duplicar_string(nome);
    op->owns_label = 1;
    return op;
}

/**
 * @brief adiciona uma instrução com cópias próprias de seus operandos.
 *
 * cada instrução é proprietária dos operandos armazenados nela. os operandos
 * recebidos permanecem pertencendo ao chamador e podem ser reutilizados.
 */
static void emitir(IROpcode op, const IROperand* res, const IROperand* arg1, const IROperand* arg2) {
    IR_Instruction* instr = calloc(1, sizeof(*instr));
    if (instr == NULL) erro_fatal_ir();

    instr->opcode = op;
    instr->result = copiar_operando(res);
    instr->arg1 = copiar_operando(arg1);
    instr->arg2 = copiar_operando(arg2);

    if ((res != NULL && instr->result == NULL) ||
        (arg1 != NULL && instr->arg1 == NULL) ||
        (arg2 != NULL && instr->arg2 == NULL)) {
        liberar_operando(instr->result);
        liberar_operando(instr->arg1);
        liberar_operando(instr->arg2);
        free(instr);
        erro_fatal_ir();
    }

    if (ir_list_head == NULL) {
        ir_list_head = ir_list_tail = instr;
    } else {
        ir_list_tail->next = instr;
        ir_list_tail = instr;
    }
}

/** @brief cria um operando constante. */
static IROperand* criar_constante(int valor) {
    IROperand* constante = criar_operando(OPERAND_CONST);
    constante->data.const_val = valor;
    return constante;
}

/** @brief cria um operando associado a um símbolo validado. */
static IROperand* criar_simbolo(Simbolo* simbolo) {
    IROperand* operando = criar_operando(OPERAND_SYMBOL);
    operando->data.symbol = simbolo;
    return operando;
}

//================================================================================
// EXPRESSÕES E CHAMADAS
//================================================================================

/** @brief gera parâmetros e chamada sem misturar chamadas aninhadas. */
static IROperand* gerar_ir_call(ASTNode* no) {
    IROperand* argumentos[MAX_PARAMETROS] = {0};
    int quantidade = 0;
    ASTNode* arg_list = no->filho->proximo_irmao;

    //avalia todas as chamadas aninhadas antes de emitir parâmetros externos
    if (arg_list != NULL && arg_list->node_type == NODE_ARG_LIST) {
        ASTNode* argumento = arg_list->filho;
        while (argumento != NULL) {
            if (quantidade >= MAX_PARAMETROS) {
                fprintf(stderr, "Erro fatal: Limite de parametros excedido na IR.\n");
                exit(EXIT_FAILURE);
            }
            argumentos[quantidade++] = gerar_ir_expr(argumento);
            argumento = argumento->proximo_irmao;
        }
    }

    for (int i = 0; i < quantidade; i++) {
        emitir(IR_PARAM, NULL, argumentos[i], NULL);
        liberar_operando(argumentos[i]);
    }

    IROperand* nome_funcao = criar_operando_label_nome(no->filho->data.string_value);
    IROperand* numero_argumentos = criar_constante(quantidade);
    IROperand* retorno = criar_operando_temporario();
    emitir(IR_CALL, retorno, nome_funcao, numero_argumentos);
    liberar_operando(nome_funcao);
    liberar_operando(numero_argumentos);
    return retorno;
}

/** @brief gera curto-circuito para os operadores lógicos. */
static IROperand* gerar_ir_logico(ASTNode* no) {
    IROperand* resultado = criar_operando_temporario();
    IROperand* constante_zero = criar_constante(0);
    IROperand* constante_um = criar_constante(1);
    IROperand* label_secundario = criar_operando_label_novo();
    IROperand* label_falso = criar_operando_label_novo();
    IROperand* label_fim = criar_operando_label_novo();
    IROperand* esquerdo = gerar_ir_expr(no->filho);

    if (no->data.op_type == AND) {
        emitir(IR_IF_FALSE, label_falso, esquerdo, NULL);
        liberar_operando(esquerdo);

        IROperand* direito = gerar_ir_expr(no->filho->proximo_irmao);
        emitir(IR_IF_FALSE, label_falso, direito, NULL);
        liberar_operando(direito);
        emitir(IR_ASSIGN, resultado, constante_um, NULL);
        emitir(IR_GOTO, label_fim, NULL, NULL);
    } else {
        emitir(IR_IF_FALSE, label_secundario, esquerdo, NULL);
        liberar_operando(esquerdo);
        emitir(IR_ASSIGN, resultado, constante_um, NULL);
        emitir(IR_GOTO, label_fim, NULL, NULL);
        emitir(IR_LABEL, label_secundario, NULL, NULL);

        IROperand* direito = gerar_ir_expr(no->filho->proximo_irmao);
        emitir(IR_IF_FALSE, label_falso, direito, NULL);
        liberar_operando(direito);
        emitir(IR_ASSIGN, resultado, constante_um, NULL);
        emitir(IR_GOTO, label_fim, NULL, NULL);
    }

    emitir(IR_LABEL, label_falso, NULL, NULL);
    emitir(IR_ASSIGN, resultado, constante_zero, NULL);
    emitir(IR_LABEL, label_fim, NULL, NULL);

    liberar_operando(constante_zero);
    liberar_operando(constante_um);
    liberar_operando(label_secundario);
    liberar_operando(label_falso);
    liberar_operando(label_fim);
    return resultado;
}

/** @brief gera a ir correspondente a uma expressão. */
static IROperand* gerar_ir_expr(ASTNode* no) {
    if (no == NULL) return NULL;

    switch (no->node_type) {
        case NODE_INTEGER_CONST:
            return criar_constante((int)no->data.int_value);

        case NODE_CHAR_CONST:
            return criar_constante((unsigned char)no->data.char_value);

        case NODE_STRING_CONST: {
            IROperand* string_label = criar_operando_label_novo();
            string_label->type = OPERAND_STRING_LBL;
            string_label->string_content = duplicar_string(no->data.string_value);
            return string_label;
        }

        case NODE_ID:
            if (no->symbol == NULL) {
                fprintf(stderr, "Erro fatal: Simbolo '%s' nao foi ligado a AST.\n", no->data.string_value);
                exit(EXIT_FAILURE);
            }
            return criar_simbolo((Simbolo*)no->symbol);

        case NODE_ARRAY_ACCESS: {
            ASTNode* id = no->filho;
            Simbolo* simbolo = (Simbolo*)id->symbol;
            IROperand* base = criar_simbolo(simbolo);
            IROperand* indice = gerar_ir_expr(id->proximo_irmao);
            IROperand* resultado = criar_operando_temporario();
            emitir(simbolo->tipo == CHAR ? IR_LOAD_BYTE : IR_LOAD,
                   resultado, base, indice);
            liberar_operando(base);
            liberar_operando(indice);
            return resultado;
        }

        case NODE_UNARY_OP: {
            IROperand* argumento = gerar_ir_expr(no->filho);
            IROperand* resultado = criar_operando_temporario();
            emitir(no->data.op_type == MINUS ? IR_NEG : IR_NOT,
                   resultado, argumento, NULL);
            liberar_operando(argumento);
            return resultado;
        }

        case NODE_BINARY_OP: {
            if (no->data.op_type == AND || no->data.op_type == OR) {
                return gerar_ir_logico(no);
            }

            IROperand* esquerdo = gerar_ir_expr(no->filho);
            IROperand* direito = gerar_ir_expr(no->filho->proximo_irmao);
            IROperand* resultado = criar_operando_temporario();
            IROpcode opcode;

            switch (no->data.op_type) {
                case PLUS: opcode = IR_ADD; break;
                case MINUS: opcode = IR_SUB; break;
                case MUL: opcode = IR_MUL; break;
                case DIV: opcode = IR_DIV; break;
                case MOD: opcode = IR_MOD; break;
                case EQ: opcode = IR_EQ; break;
                case NEQ: opcode = IR_NEQ; break;
                case LT: opcode = IR_LT; break;
                case LEQ: opcode = IR_LEQ; break;
                case GT: opcode = IR_GT; break;
                case GEQ: opcode = IR_GEQ; break;
                default:
                    fprintf(stderr, "Erro fatal: Operador binario invalido na IR.\n");
                    exit(EXIT_FAILURE);
            }

            emitir(opcode, resultado, esquerdo, direito);
            liberar_operando(esquerdo);
            liberar_operando(direito);
            return resultado;
        }

        case NODE_CALL:
            return gerar_ir_call(no);

        default:
            return NULL;
    }
}

//================================================================================
// INSTRUÇÕES
//================================================================================

/** @brief gera a ir correspondente a um nó de instrução. */
static void gerar_ir_no(ASTNode* no) {
    if (no == NULL || no->node_type == NODE_UNDEFINED) return;

    switch (no->node_type) {
        case NODE_PROGRAM:
        case NODE_BLOCK: {
            for (ASTNode* filho = no->filho; filho != NULL; filho = filho->proximo_irmao) {
                gerar_ir_no(filho);
            }
            break;
        }

        case NODE_FUNCTION_DEF: {
            ASTNode* id = no->filho->proximo_irmao;
            funcao_atual = (Simbolo*)id->symbol;
            temp_counter = 0;

            IROperand* label = criar_operando_label_nome(id->data.string_value);
            emitir(IR_LABEL, label, NULL, NULL);
            liberar_operando(label);

            ASTNode* corpo = id->proximo_irmao;
            if (corpo != NULL && corpo->node_type == NODE_PARAM_LIST) {
                corpo = corpo->proximo_irmao;
            }
            gerar_ir_no(corpo);

            if (funcao_atual != NULL) {
                funcao_atual->frame_size = alinhar_16(funcao_atual->local_size + temp_counter * 4);
            }
            funcao_atual = NULL;
            break;
        }

        case NODE_VAR_DECL: {
            ASTNode* filho = no->filho;
            while (filho != NULL && filho->node_type != NODE_ASSIGN) {
                filho = filho->proximo_irmao;
            }
            gerar_ir_no(filho);
            break;
        }

        case NODE_ASSIGN: {
            ASTNode* esquerdo = no->filho;
            ASTNode* direito = esquerdo->proximo_irmao;

            if (esquerdo->inferred_type == STRINGCONST && direito->inferred_type == STRINGCONST) {
                IROperand* destino = criar_simbolo((Simbolo*)esquerdo->symbol);
                IROperand* origem = gerar_ir_expr(direito);
                emitir(IR_STRCPY, destino, origem, NULL);
                liberar_operando(destino);
                liberar_operando(origem);
                break;
            }

            IROperand* origem = gerar_ir_expr(direito);
            if (esquerdo->node_type == NODE_ID) {
                IROperand* destino = criar_simbolo((Simbolo*)esquerdo->symbol);
                emitir(IR_ASSIGN, destino, origem, NULL);
                liberar_operando(destino);
            } else {
                ASTNode* id = esquerdo->filho;
                Simbolo* simbolo = (Simbolo*)id->symbol;
                IROperand* base = criar_simbolo(simbolo);
                IROperand* indice = gerar_ir_expr(id->proximo_irmao);
                emitir(simbolo->tipo == CHAR ? IR_STORE_BYTE : IR_STORE,
                       base, origem, indice);
                liberar_operando(base);
                liberar_operando(indice);
            }
            liberar_operando(origem);
            break;
        }

        case NODE_RETURN: {
            IROperand* valor = gerar_ir_expr(no->filho);
            emitir(IR_RETURN, NULL, valor, NULL);
            liberar_operando(valor);
            break;
        }

        case NODE_IF: {
            ASTNode* condicao = no->filho;
            ASTNode* bloco_then = condicao->proximo_irmao;
            ASTNode* bloco_else = bloco_then != NULL ? bloco_then->proximo_irmao : NULL;
            IROperand* label_else = criar_operando_label_novo();
            IROperand* label_fim = criar_operando_label_novo();
            IROperand* resultado = gerar_ir_expr(condicao);

            emitir(IR_IF_FALSE, bloco_else != NULL ? label_else : label_fim, resultado, NULL);
            liberar_operando(resultado);
            gerar_ir_no(bloco_then);

            if (bloco_else != NULL) {
                emitir(IR_GOTO, label_fim, NULL, NULL);
                emitir(IR_LABEL, label_else, NULL, NULL);
                gerar_ir_no(bloco_else);
            }
            emitir(IR_LABEL, label_fim, NULL, NULL);
            liberar_operando(label_else);
            liberar_operando(label_fim);
            break;
        }

        case NODE_FOR: {
            ASTNode* inicializacao = no->filho;
            ASTNode* condicao = inicializacao->proximo_irmao;
            ASTNode* incremento = condicao->proximo_irmao;
            ASTNode* corpo = incremento->proximo_irmao;
            IROperand* label_inicio = criar_operando_label_novo();
            IROperand* label_fim = criar_operando_label_novo();

            gerar_ir_no(inicializacao);
            emitir(IR_LABEL, label_inicio, NULL, NULL);
            if (condicao->node_type != NODE_UNDEFINED) {
                IROperand* resultado = gerar_ir_expr(condicao);
                emitir(IR_IF_FALSE, label_fim, resultado, NULL);
                liberar_operando(resultado);
            }
            gerar_ir_no(corpo);
            gerar_ir_no(incremento);
            emitir(IR_GOTO, label_inicio, NULL, NULL);
            emitir(IR_LABEL, label_fim, NULL, NULL);
            liberar_operando(label_inicio);
            liberar_operando(label_fim);
            break;
        }

        case NODE_PRINT: {
            IROperand* argumento = gerar_ir_expr(no->filho);
            IROpcode opcode = IR_PRINT_INT;
            if (no->filho->inferred_type == CHAR) opcode = IR_PRINT_CHAR;
            if (no->filho->inferred_type == STRINGCONST) opcode = IR_PRINT_STRING;
            emitir(opcode, NULL, argumento, NULL);
            liberar_operando(argumento);
            break;
        }

        case NODE_CALL: {
            IROperand* retorno = gerar_ir_call(no);
            liberar_operando(retorno);
            break;
        }

        default:
            break;
    }
}

/** @brief inicia a geração da lista de código intermediário. */
IR_Instruction* gerar_codigo_intermediario(ASTNode* raiz, PilhaTabelasSimbolos* pilha) {
    (void)pilha;
    temp_counter = 0;
    label_counter = 0;
    funcao_atual = NULL;
    ir_list_head = ir_list_tail = NULL;
    gerar_ir_no(raiz);
    return ir_list_head;
}
