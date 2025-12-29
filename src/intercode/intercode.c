/**
 * @file intercode.c
 * @brief implementação do gerador de código intermediário.
 *
 * este módulo percorre a árvore sintática abstrata (ast) e a traduz
 * para uma representação intermediária linear (ir) em formato de
 * código de três endereços.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intercode.h"

//variaveis globais estaticas para gerenciar o estado da geracao de codigo
static int temp_counter = 0;
static int label_counter = 0;
static IR_Instruction* ir_list_head = NULL;
static IR_Instruction* ir_list_tail = NULL;

//prototipos de funcoes auxiliares internas
static IROperand* criar_operando(OperandType type);
static IROperand* criar_operando_temporario();
static IROperand* criar_operando_label_novo();
static IROperand* criar_operando_label_nome(const char* nome);
static void emitir(IROpcode op, IROperand* res, IROperand* arg1, IROperand* arg2);
static void gerar_ir_no(ASTNode* no);
static IROperand* gerar_ir_expr(ASTNode* no);
static IROperand* gerar_ir_call(ASTNode* no);
static TokenType get_expr_static_type(ASTNode* expr_node);
static IROperand* copiar_operando(IROperand* op);

/** @brief aloca e cria um operando genérico. */
static IROperand* criar_operando(OperandType type) {
    IROperand* op = (IROperand*) malloc(sizeof(IROperand));
    op->type = type;
    op->owns_label = 0; //inicializa a flag
    memset(&op->data, 0, sizeof(op->data)); //zera a uniao
    return op;
}

/** @brief cria um novo operando para uma variável temporária. */
static IROperand* criar_operando_temporario() {
    IROperand* op = criar_operando(OPERAND_TEMP);
    op->data.temp_id = temp_counter++;
    return op;
}

/** @brief cria um novo operando para um rótulo gerado automaticamente. */
static IROperand* criar_operando_label_novo() {
    IROperand* op = criar_operando(OPERAND_LABEL);
    op->data.label_name = (char*) malloc(16);
    sprintf(op->data.label_name, "L%d", label_counter++);
    op->owns_label = 1; //este operando e o "dono" da string do label
    return op;
}

/** @brief cria um novo operando para um rótulo com nome específico. */
static IROperand* criar_operando_label_nome(const char* nome) {
    IROperand* op = criar_operando(OPERAND_LABEL);
    op->data.label_name = strdup(nome);
    //esta copia e "dona" da sua propria string alocada
    op->owns_label = 1; 
    return op;
}

/** @brief cria uma cópia de um operando. */
static IROperand* copiar_operando(IROperand* op) {
    if (op == NULL) return NULL;

    IROperand* copia = criar_operando(op->type);
    //copia os dados
    memcpy(&copia->data, &op->data, sizeof(op->data));
    
    //se o original era um label, criamos uma nova copia da string
    if (op->type == OPERAND_LABEL || op->type == OPERAND_STRING_LBL) {
        if (op->data.label_name) {
            copia->data.label_name = strdup(op->data.label_name);
            copia->owns_label = 1; //a copia e dona da sua propria string
        }
    }
    
    //se tiver conteúdo de string literal, duplicamos também
    if (op->string_content) {
        copia->string_content = strdup(op->string_content);
    } else {
        copia->string_content = NULL;
    }
    
    return copia;
}



/** @brief cria e anexa uma nova instrução à lista de ir. */
static void emitir(IROpcode op, IROperand* res, IROperand* arg1, IROperand* arg2) {
    IR_Instruction* instr = (IR_Instruction*) malloc(sizeof(IR_Instruction));
    instr->opcode = op;
    instr->result = res;
    instr->arg1 = arg1;
    instr->arg2 = arg2;
    instr->next = NULL;

    if (ir_list_head == NULL) {
        ir_list_head = ir_list_tail = instr;
    } else {
        ir_list_tail->next = instr;
        ir_list_tail = instr;
    }
}

/** @brief função auxiliar para gerar código para chamadas de função. */
static IROperand* gerar_ir_call(ASTNode* no) {
    //1. processa a lista de argumentos
    ASTNode* arg_list_no = no->filho->proximo_irmao;
    int arg_count = 0;
    if (arg_list_no && arg_list_no->node_type == NODE_ARG_LIST) {
        ASTNode* arg_expr = arg_list_no->filho;
        while (arg_expr) {
            IROperand* arg_op_original = gerar_ir_expr(arg_expr);
            //cria uma copia para a instrucao param
            emitir(IR_PARAM, NULL, copiar_operando(arg_op_original), NULL);
            arg_count++;
            arg_expr = arg_expr->proximo_irmao;
        }
    }
    
    //2. cria o operando para o nome da funcao (que e um label)
    IROperand* func_label = criar_operando_label_nome(no->filho->data.string_value);
    //3. cria o operando para a contagem de argumentos
    IROperand* arg_count_op = criar_operando(OPERAND_CONST);
    arg_count_op->data.const_val = arg_count;
    //4. cria um temporario para armazenar o valor de retorno
    IROperand* temp_retorno = criar_operando_temporario();

    
    //5. emite a instrucao de chamada
    emitir(IR_CALL, temp_retorno, func_label, arg_count_op);
    
    return temp_retorno; //retorna o ponteiro original do temporario
}


/**
 * @brief Retorna o tipo estático de um nó de expressão.
 * (versao simplificada do 'get_expression_type' do semantic.c)
 */
static TokenType get_expr_static_type(ASTNode* expr_node) {
    if (expr_node == NULL) return UNDEF;
    
    switch (expr_node->node_type) {
        //tipos literais
        case NODE_INTEGER_CONST: return INT;
        case NODE_CHAR_CONST:    return CHAR;
        case NODE_STRING_CONST:  return STRINGCONST;
        
        //tipo de uma variavel
        case NODE_ID: {
            // ALTERAÇÃO: Usa o ponteiro 'symbol' salvo na AST, em vez de buscar na pilha
            Simbolo* s = (Simbolo*)expr_node->symbol;
            if (s) {
                //Se for um array de CHAR, o tipo estático para impressão deve ser STRINGCONST
                if (s->is_array && s->tipo == CHAR) {
                    return STRINGCONST;
                }
                return s->tipo; //o tipo (int ou char) do simbolo
            }
            return UNDEF;
        }
        
        //tipo de um acesso a array (e o tipo base do array)
        case NODE_ARRAY_ACCESS: {
             // O símbolo está no filho (o ID do array)
             ASTNode* id = expr_node->filho;
             Simbolo* s = (Simbolo*)id->symbol;
             if (s) return s->tipo; //ex: 'int v[10]' -> int
             return UNDEF;
        }

        //na sua linguagem, operacoes e chamadas de funcao parecem retornar int
        case NODE_BINARY_OP:
        case NODE_CALL:
            return INT; 
            
        default:
            return UNDEF;
    }
}


/** @brief função recursiva que percorre a ast e gera a ir para nós de expressão. */
static IROperand* gerar_ir_expr(ASTNode* no) {
    if (no == NULL) return NULL;

    switch (no->node_type) {
        case NODE_INTEGER_CONST: {
            IROperand* constante = criar_operando(OPERAND_CONST);
            constante->data.const_val = no->data.int_value;
            return constante;
        }

        case NODE_CHAR_CONST: {
            IROperand* constante_char = criar_operando(OPERAND_CONST);
            //armazena o char como seu valor ascii (int).
            //o parser armazena o char em data.char_value.
            constante_char->data.const_val = (int)no->data.char_value;
            return constante_char;
        }

        case NODE_STRING_CONST: {
            IROperand* string_label = criar_operando_label_novo(); 
            string_label->type = OPERAND_STRING_LBL;
            
            //armazena o conteudo fora da uniao
            string_label->string_content = strdup(no->data.string_value);
            
            //nao mexe no data.symbol!
            return string_label;
        }

        case NODE_ID: {
            //acesso direto ao símbolo salvo no nó
            Simbolo* s = (Simbolo*)no->symbol;
            if (s == NULL) {
                //se chegou aqui sem símbolo, é erro grave do semantic, mas não deve ocorrer
                fprintf(stderr, "ERRO FATAL (IR): simbolo '%s' nao linkado na AST.\n", no->data.string_value);
                return NULL;
            }
            IROperand* var = criar_operando(OPERAND_SYMBOL);
            var->data.symbol = s;
            return var;
        }

        case NODE_ARRAY_ACCESS: {
            // O símbolo está no nó filho (ID)
            ASTNode* id_no = no->filho;
            Simbolo* s = (Simbolo*)id_no->symbol;
            
            IROperand* base = criar_operando(OPERAND_SYMBOL);
            base->data.symbol = s;
            
            IROperand* index = gerar_ir_expr(no->filho->proximo_irmao);
            IROperand* temp = criar_operando_temporario();
            
            //se for array de CHAR, usa LOAD_BYTE ---
            if (s && s->tipo == CHAR) {
                emitir(IR_LOAD_BYTE, temp, base, index);
            } else {
                emitir(IR_LOAD, temp, base, index);
            }
            // -----------------------------------------------------
            
            return temp;
        }
        case NODE_BINARY_OP: {
            IROperand* arg1 = gerar_ir_expr(no->filho);
            IROperand* arg2 = gerar_ir_expr(no->filho->proximo_irmao);
            IROperand* temp = criar_operando_temporario();
            IROpcode op = UNDEF;
            switch(no->data.op_type) {
                case PLUS: op = IR_ADD; break; case MINUS: op = IR_SUB; break;
                case MUL: op = IR_MUL; break; case DIV: op = IR_DIV; break;
                case MOD: op = IR_MOD; break; case LT: op = IR_LT; break;
                case LEQ: op = IR_LEQ; break; case GT: op = IR_GT; break;
                case GEQ: op = IR_GEQ; break; case EQ: op = IR_EQ; break;
                case NEQ: op = IR_NEQ; break; case AND: op = IR_AND; break;
                case OR: op = IR_OR; break; default: break;
            }
            emitir(op, temp, arg1, arg2);
            return temp;
        }
        
        case NODE_CALL: {
            //uma chamada de funcao usada em uma expressao (ex: z = soma(x,y))
            //gera o codigo de chamada e retorna o temporario com o resultado.
            return gerar_ir_call(no);
        }

        default: return NULL;
    }
}

/** @brief função recursiva principal que percorre a ast e gera a ir para instruções. */
static void gerar_ir_no(ASTNode* no) {
    if (no == NULL) return;

    switch (no->node_type) {
        case NODE_PROGRAM:
        case NODE_BLOCK: {
            
            ASTNode* filho = no->filho;
            while (filho != NULL) {
                gerar_ir_no(filho);
                filho = filho->proximo_irmao;
            }
            
            break;
        }
        case NODE_FUNCTION_DEF: {
            IROperand* func_label = criar_operando_label_nome(no->filho->proximo_irmao->data.string_value);
            emitir(IR_LABEL, func_label, NULL, NULL);
            ASTNode* corpo_funcao = no->filho;
            while(corpo_funcao && corpo_funcao->node_type != NODE_BLOCK) {
                corpo_funcao = corpo_funcao->proximo_irmao;
            }
            gerar_ir_no(corpo_funcao);
            break;
        }
        case NODE_VAR_DECL: {
            ASTNode* assign_no = no->filho;
            while(assign_no && assign_no->node_type != NODE_ASSIGN) {
                assign_no = assign_no->proximo_irmao;
            }
            if (assign_no) {
                gerar_ir_no(assign_no);
            }
            break;
        }
        case NODE_ASSIGN: {
            ASTNode* lado_esquerdo = no->filho;
            ASTNode* lado_direito = lado_esquerdo->proximo_irmao;
            
            // --- DETECÇÃO DE ATRIBUIÇÃO DE STRING (IR_STRCPY) ---
            TokenType t_esq = get_expr_static_type(lado_esquerdo);
            TokenType t_dir = get_expr_static_type(lado_direito);

            if (t_esq == STRINGCONST && t_dir == STRINGCONST) {
                // Caso Especial: nome = "string";
                IROperand* src = gerar_ir_expr(lado_direito); // Gera o label da string
                
                Simbolo* s = (Simbolo*)lado_esquerdo->symbol; // Pega do nó ID
                IROperand* dest = criar_operando(OPERAND_SYMBOL);
                dest->data.symbol = s;
                
                // Emite nova instrução IR_STRCPY (destino, origem)
                emitir(IR_STRCPY, dest, copiar_operando(src), NULL);
                
                break; // Sai do case, pois já tratamos
            }
            // ----------------------------------------------------
            
            //src e o ponteiro original (ex: t0)
            IROperand* src = gerar_ir_expr(lado_direito); 

            if (lado_esquerdo->node_type == NODE_ID) {
                Simbolo* s = (Simbolo*)lado_esquerdo->symbol;
                IROperand* dest = criar_operando(OPERAND_SYMBOL);
                dest->data.symbol = s;
                //usa uma copia de 'src' (t0) para esta instrucao
                emitir(IR_ASSIGN, dest, copiar_operando(src), NULL); 
            } 
            else if (lado_esquerdo->node_type == NODE_ARRAY_ACCESS) {
                ASTNode* id = lado_esquerdo->filho;
                Simbolo* s = (Simbolo*)id->symbol;
                
                IROperand* base = criar_operando(OPERAND_SYMBOL);
                base->data.symbol = s;
                IROperand* index = gerar_ir_expr(lado_esquerdo->filho->proximo_irmao);
                
                // --- CORRECAO: Se for CHAR, usa STORE_BYTE ---
                if (s && s->tipo == CHAR) {
                    emitir(IR_STORE_BYTE, base, copiar_operando(src), index); 
                } else {
                    emitir(IR_STORE, base, copiar_operando(src), index); 
                }
                // ---------------------------------------------
            }
            break;
        }
        case NODE_RETURN: {
            IROperand* ret_val = gerar_ir_expr(no->filho);
            //usa uma copia do valor de retorno
            emitir(IR_RETURN, NULL, copiar_operando(ret_val), NULL);
            break;
        }
        case NODE_IF: {
            ASTNode* cond_no = no->filho;
            ASTNode* then_no = cond_no->proximo_irmao;
            ASTNode* else_no = then_no ? then_no->proximo_irmao : NULL;

            IROperand* label_else = criar_operando_label_novo();
            IROperand* label_fim_if = criar_operando_label_novo();
            
            IROperand* cond_result = gerar_ir_expr(cond_no);
            
            //usa uma copia do resultado da condicao
            emitir(IR_IF_FALSE, copiar_operando(label_else), copiar_operando(cond_result), NULL);
            
            gerar_ir_no(then_no);
            emitir(IR_GOTO, copiar_operando(label_fim_if), NULL, NULL);

            emitir(IR_LABEL, label_else, NULL, NULL); //usa o original
            if (else_no) {
                gerar_ir_no(else_no);
            }
            
            emitir(IR_LABEL, label_fim_if, NULL, NULL); //usa o original
            break;
        }
        case NODE_FOR: {
            ASTNode* init_no = no->filho;
            ASTNode* cond_no = init_no ? init_no->proximo_irmao : NULL;
            ASTNode* incr_no = cond_no ? cond_no->proximo_irmao : NULL;
            ASTNode* body_no = incr_no ? incr_no->proximo_irmao : NULL;

            IROperand* label_inicio_loop = criar_operando_label_novo();
            IROperand* label_fim_loop = criar_operando_label_novo();

            gerar_ir_no(init_no);
            emitir(IR_LABEL, label_inicio_loop, NULL, NULL); //usa o original

            if (cond_no && cond_no->node_type != NODE_UNDEFINED) {
                IROperand* cond_result = gerar_ir_expr(cond_no);
                //usa copias
                emitir(IR_IF_FALSE, copiar_operando(label_fim_loop), copiar_operando(cond_result), NULL);
            }

            gerar_ir_no(body_no);
            gerar_ir_no(incr_no);

            emitir(IR_GOTO, copiar_operando(label_inicio_loop), NULL, NULL); //usa copia
            emitir(IR_LABEL, label_fim_loop, NULL, NULL); //usa o original
            break;
        }

        case NODE_PRINT: {
            ASTNode* arg_node = no->filho;
            if (arg_node == NULL) break; //'print()' sem argumento, nao faz nada

            //1. descobre o tipo estatico do argumento (int, char, etc.)
            TokenType arg_type = get_expr_static_type(arg_node);
            
            //2. gera a ir para o argumento (ex: z, 'a', "ola")
            IROperand* arg_op = gerar_ir_expr(arg_node);
            if (arg_op == NULL) break; //falha ao gerar expressao

            //3. emite o opcode de print correto baseado no tipo
            switch (arg_type) {
                case INT:
                case INTEGERCONST: //se for uma variavel int ou um literal int
                    //usa a copia, por seguranca (como em ir_assign)
                    emitir(IR_PRINT_INT, NULL, copiar_operando(arg_op), NULL);
                    break;
                    
                case CHAR:
                case CHARCONST: //se for uma variavel char ou um literal char
                    emitir(IR_PRINT_CHAR, NULL, copiar_operando(arg_op), NULL);
                    break;
                    
                case STRINGCONST: 
                    //Usamos copiar_operando para suportar tanto literais quanto variáveis
                    emitir(IR_PRINT_STRING, NULL, copiar_operando(arg_op), NULL);
                    break;
                    
                default:
                    //nao faz nada se tentar printar um tipo desconhecido (ex: um array)
                    break;
            }
            
            break;
        }
        case NODE_CALL: {
            //este e um 'call' onde o valor de retorno nao e usado.
            //(ex: uma funcao que so tem efeitos colaterais)
            gerar_ir_call(no);
            break;
        }

        default:
            break;
    }
}

/** @brief função pública que inicia a geração de código intermediário. */
IR_Instruction* gerar_codigo_intermediario(ASTNode* raiz, PilhaTabelasSimbolos* pilha) {
    temp_counter = 0;
    label_counter = 0;
    ir_list_head = ir_list_tail = NULL;
    

    gerar_ir_no(raiz);
    
    return ir_list_head;
}