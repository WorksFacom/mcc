/**
 * @file assembly.c
 * @brief implementação do gerador de código assembly x86-64.
 *
 * este módulo traduz a representação intermediária (ir) para o 
 * código assembly final da arquitetura x86-64, usando a sintaxe at&t.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "assembly.h"

//ponteiro para o arquivo de saída onde o assembly será escrito
static FILE* outfile;
//buffer estático global para construir strings de operandos formatados
static char asm_buffer[100];
//contador global para o número de parâmetros sendo empilhados para uma chamada
static int param_count = 0;
static IROperand* param_buffer[10]; 
static int param_buffer_count = 0;   

//================================================================================
// FUNÇÕES AUXILIARES DE GERAÇÃO
//================================================================================

/**
 * @brief escreve uma linha de assembly formatada no arquivo de saída, sem indentação.
 * @details usado para labels e diretivas (ex: .text, main:).
 * @param format a string de formato (como em printf).
 * @param ... os argumentos variáveis para a string de formato.
 */
static void asm_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(outfile, format, args);
    va_end(args);
    fprintf(outfile, "\n");
}

/**
 * @brief escreve uma linha de instrução assembly formatada, com indentação.
 * @details usado para instruções de máquina (ex: movl, addl, jmp).
 * @param format a string de formato (como em printf).
 * @param ... os argumentos variáveis para a string de formato.
 */
static void asm_instr(const char* format, ...) {
    fprintf(outfile, "    "); //indenta com 4 espaços
    va_list args;
    va_start(args, format);
    vfprintf(outfile, format, args);
    va_end(args);
    fprintf(outfile, "\n");
}

/**
 * @brief traduz um operando da ir para sua representação em assembly at&t.
 * @detalhes esta é uma função crucial que mapeia os conceitos da ir
 * (constante, símbolo, temporário) para a sintaxe específica do assembly.
 * @param op o operando da ir (ex: um OPERAND_CONST com valor 10).
 * @return uma string estática formatada para o assembly (ex: "$10", "-4(%rbp)").
 */
static char* get_operand_asm(IROperand* op) {
    if (op == NULL) {
        strcpy(asm_buffer, "NULL_OPERAND");
        return asm_buffer;
    }

    switch (op->type) {
        case OPERAND_CONST:
            //constantes (valores imediatos) são prefixadas com "$"
            sprintf(asm_buffer, "$%d", op->data.const_val);
            break;

        case OPERAND_SYMBOL:
            //símbolos (variáveis/parâmetros) são acessados por seu offset
            //calculado pelo semantic.c (ex: -4(%rbp) ou +16(%rbp))
            if (op->data.symbol == NULL) {
                sprintf(asm_buffer, "NULL_SYMBOL_ERROR");
            } else {
                sprintf(asm_buffer, "%d(%%rbp)", op->data.symbol->memory_offset);
            }
            break;
            
        case OPERAND_TEMP: {
            //temporários (t0, t1, ...) também vivem na pilha.
            //vamos calcular um offset para eles, reservando uma área
            //separada das variáveis locais (ex: a partir de -128)
            //cada temp tem 4 bytes (tamanho de um int)
            int temp_offset = -128 - (op->data.temp_id * 4);
            sprintf(asm_buffer, "%d(%%rbp)", temp_offset);
            break;
        }

        case OPERAND_LABEL:
        case OPERAND_STRING_LBL:
            //labels são usados diretamente como endereços de código ou dados
            if (op->data.label_name == NULL) {
                sprintf(asm_buffer, "NULL_LABEL_ERROR");
            } else {
                sprintf(asm_buffer, "%s", op->data.label_name);
            }
            break;

        default:
            sprintf(asm_buffer, "INVALID_OPERAND");
            break;
    }
    return asm_buffer;
}


/**
 * @brief gera o prólogo da função (cria o stack frame).
 * @details todo início de função em assembly deve configurar sua pilha.
 */
static void asm_gen_prologue() {
    asm_instr("pushq %%rbp");           //salva o ponteiro base da função anterior
    asm_instr("movq %%rsp, %%rbp");    //define o ponteiro base desta função
    //aqui alocamos espaço na pilha para todas as variáveis locais e temporários.
    //uma implementação otimizada calcularia o tamanho exato necessário.
    //para nosso compilador didático, alocamos um espaço fixo de 256 bytes.
    asm_instr("subq $256, %%rsp");
}

/**
 * @brief gera o epílogo da função (destroi o stack frame e retorna).
 * @details todo fim de função em assembly deve limpar a pilha e retornar.
 */
static void asm_gen_epilogue() {
    asm_instr("movq %%rbp, %%rsp");    //restaura o ponteiro da pilha (libera espaço local)
    asm_instr("popq %%rbp");           //restaura o ponteiro base da função anterior
    asm_instr("ret");                  //retorna da função
}

//================================================================================
// FUNÇÃO PRINCIPAL DE GERAÇÃO
//================================================================================

/**
 * @brief função principal que gera o arquivo de assembly a partir da ir.
 * @details percorre a lista encadeada de ir, traduzindo cada instrução
 * (opcode) para sua sequência correspondente de assembly x86-64 at&t.
 * @param ir_head o ponteiro para a primeira instrução da lista de ir.
 * @param pilha a pilha de tabelas de símbolos (para consultar offsets).
 * @param nome_arquivo_saida o nome do arquivo .s onde o assembly será escrito.
 * @return 0 em caso de sucesso, -1 em caso de erro (ex: falha ao abrir arquivo).
 */
int gerar_assembly(IR_Instruction* ir_head, PilhaTabelasSimbolos* pilha, const char* nome_arquivo_saida) {
    outfile = fopen(nome_arquivo_saida, "w");
    if (outfile == NULL) {
        printf("Erro: Nao foi possivel criar o arquivo de saida assembly '%s'\n", nome_arquivo_saida);
        return -1;
    }

    //define a seção de dados (somente leitura)
    asm_print(".section .rodata");
    
    //define as strings de formato que o printf usará.
    //usamos '%%' para "escapar" o % para a função 'asm_print' (vfprintf).
    asm_print(".L.str.int: .string \"%%d\\n\"");
    asm_print(".L.str.char: .string \"%%c\\n\"");
    asm_print(".L.str.str: .string \"%%s\\n\"");

    //agora, fazemos uma "pré-varredura" na ir para encontrar
    //todos os literais de string (ex: print("ola")) e defini-los.
    IR_Instruction* instr_scan = ir_head;
    while (instr_scan != NULL) {
        if (instr_scan->opcode == IR_PRINT_STRING && 
            instr_scan->arg1 != NULL && 
            instr_scan->arg1->type == OPERAND_STRING_LBL) 
        {
            const char* label_name = instr_scan->arg1->data.label_name;
            const char* string_val = instr_scan->arg1->string_content;

            if (label_name != NULL && string_val != NULL) {
                asm_print("%s: .string \"%s\"", label_name, string_val);
            }
        }
        instr_scan = instr_scan->next;
    }
    
    //linha em branco para formatação
    asm_print("");
    
    asm_print(".text"); //volta para a seção de código
    

    IR_Instruction* instr = ir_head;
    while (instr != NULL) {
        //traduz instrução por instrução
        switch (instr->opcode) {
            
            /**
             * @brief traduz a instrução ir_label.
             * @detalhes labels de função (ex: main) recebem diretivas .globl
             * e o prólogo da função. labels de desvio (ex: l0) são apenas impressos.
             */

            case IR_LABEL: { //adiciona chaves para criar um novo escopo
                //passo 1: busca o símbolo na tabela
                Simbolo* s = buscar_simbolo_em_todos_escopos(pilha, instr->result->data.label_name);
                
                //passo 2: verifica se o símbolo existe e se é uma função
                if (s != NULL && s->is_function) {
                    //é um label de função (main, soma, etc.)
                    asm_print(".globl %s", s->nome);
                    asm_print("%s:", s->nome);
                    asm_gen_prologue();
                } else {
                    //é um label de desvio (l0, l1, etc.)
                    asm_print("%s:", get_operand_asm(instr->result));
                }
                break;
            } //fecha o escopo do case
            
            /**
             * @brief traduz a instrução ir_assign (atribuição).
             * @detalhes traduz `result := arg1` (ex: x := t0).
             * usa %eax como registrador intermediário.
             */
            case IR_ASSIGN:
                //passo 1: carrega a fonte (arg1) para o registrador %eax
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                //passo 2: armazena o valor do registrador no destino (result)
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;
            
            // --- INÍCIO DA REESTRUTURAÇÃO LÓGICA ---

            //grupo 1: aritmética simples (add, sub, mul)
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
                //passo 1: carrega arg1 em %eax
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                //passo 2: carrega arg2 em %ebx
                asm_instr("movl %s, %%ebx", get_operand_asm(instr->arg2));
                
                //passo 3: opera
                if(instr->opcode == IR_ADD) asm_instr("addl %%ebx, %%eax"); //eax = eax + ebx
                if(instr->opcode == IR_SUB) asm_instr("subl %%ebx, %%eax"); //eax = eax - ebx
                if(instr->opcode == IR_MUL) asm_instr("imull %%ebx, %%eax"); //eax = eax * ebx

                //passo 4: armazena o resultado (%eax) no destino (result)
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            //grupo 2: divisão e módulo (requerem %eax e %edx)
            case IR_DIV:
            case IR_MOD:
                //passo 1: carrega o dividendo (arg1) em %eax
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                
                //passo 2: estende o sinal de %eax para %edx (necessário para idivl)
                asm_instr("cdq");
                
                //passo 3: divide edx:eax pelo divisor (arg2)
                //o divisor não pode ser %eax ou %edx
                asm_instr("idivl %s", get_operand_asm(instr->arg2));
                
                //passo 4: armazena o resultado
                if (instr->opcode == IR_DIV) {
                    //o quociente está em %eax
                    asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                } else {
                    //o resto (módulo) está em %edx
                    asm_instr("movl %%edx, %s", get_operand_asm(instr->result));
                }
                break;

            //grupo 3: comparações (resultam em 0 ou 1)
            case IR_EQ:
            case IR_NEQ:
            case IR_LT:
            case IR_LEQ:
            case IR_GT:
            case IR_GEQ:
                //passo 1: carrega arg1 em %eax
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                //passo 2: carrega arg2 em %ebx
                asm_instr("movl %s, %%ebx", get_operand_asm(instr->arg2));
                //passo 3: compara %eax com %ebx (cmpl fonte, destino)
                asm_instr("cmpl %%ebx, %%eax");
                //passo 4: zera %eax (prepara para resultado falso)
                asm_instr("movl $0, %%eax");
                
                //passo 5: define o byte %al como 1 se a condição for verdadeira
                //o 'l' em setl/sete refere-se ao registrador de 8 bits %al
                if(instr->opcode == IR_EQ)  asm_instr("sete %%al"); //set if equal
                if(instr->opcode == IR_NEQ) asm_instr("setne %%al"); //set if not equal
                if(instr->opcode == IR_LT)  asm_instr("setl %%al"); //set if less
                if(instr->opcode == IR_LEQ) asm_instr("setle %%al"); //set if less or equal
                if(instr->opcode == IR_GT)  asm_instr("setg %%al"); //set if greater
                if(instr->opcode == IR_GEQ) asm_instr("setge %%al"); //set if greater or equal
                
                //passo 6: armazena o resultado (0 ou 1) no destino
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            //grupo 4: lógica booleana (and, or)
            case IR_AND:
            case IR_OR:
                //lógica booleana (não bitwise)
                //result = (arg1 != 0) && (arg2 != 0)
                
                //passo 1: carrega arg1 em %eax e "booleaniza" (0 ou 1)
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("cmpl $0, %%eax");
                asm_instr("setne %%al"); //al = (arg1 != 0)
                
                //passo 2: carrega arg2 em %ebx e "booleaniza" (0 ou 1)
                asm_instr("movl %s, %%ebx", get_operand_asm(instr->arg2));
                asm_instr("cmpl $0, %%ebx");
                asm_instr("setne %%bl"); //bl = (arg2 != 0)
                
                //passo 3: opera os bytes (al e bl)
                if(instr->opcode == IR_AND) asm_instr("andb %%bl, %%al"); //al = al & bl
                if(instr->opcode == IR_OR)  asm_instr("orb %%bl, %%al");  //al = al | bl
                
                //passo 4: estende o byte (%al) de volta para 32 bits (%eax)
                asm_instr("movzbl %%al, %%eax");
                
                //passo 5: armazena o resultado (0 ou 1)
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            // --- FIM DA REESTRUTURAÇÃO LÓGICA ---

            /**
             * @brief traduz a instrução ir_if_false (desvio condicional).
             * @detalhes traduz `if_false arg1 goto result` (ex: if_false t0 goto l0).
             */
            case IR_IF_FALSE:
                //passo 1: compara o valor da condição (arg1) com 0
                asm_instr("cmpl $0, %s", get_operand_asm(instr->arg1));
                //passo 2: pula se for igual a 0 (je = jump if equal)
                asm_instr("je %s", get_operand_asm(instr->result));
                break;

            /**
             * @brief traduz a instrução ir_goto (desvio incondicional).
             * @detalhes traduz `goto result` (ex: goto l1).
             */
            case IR_GOTO:
                asm_instr("jmp %s", get_operand_asm(instr->result));
                break;
            
/**
             * @brief traduz a instrução ir_param (passagem de parâmetro).
             * @detalhes acumula parâmetros em um buffer para empilhar na ordem
             * reversa durante o ir_call.
             */
            case IR_PARAM:
                //acumula o parâmetro no buffer
                if (param_buffer_count < 10) {
                    param_buffer[param_buffer_count] = instr->arg1;
                }
                param_buffer_count++;
                break;
                
            /**
             * @brief traduz a instrução ir_call (chamada de função).
             * @detalhes empilha os parâmetros acumulados na ordem reversa,
             * chama a função, limpa a pilha e salva o retorno.
             */
            case IR_CALL:
                //passo 1: empilha os parâmetros na ordem reversa (da direita para a esquerda)
                for (int i = param_buffer_count - 1; i >= 0; i--) {
                    asm_instr("movl %s, %%eax", get_operand_asm(param_buffer[i]));
                    asm_instr("pushq %%rax");
                }
                
                //passo 2: chama a função (o label está em arg1)
                asm_instr("call %s", get_operand_asm(instr->arg1));
                
                //passo 3: limpa os parâmetros da pilha
                if (param_buffer_count > 0) {
                    asm_instr("addq $%d, %%rsp", param_buffer_count * 8);
                }
                
                //passo 4: reseta os contadores de parâmetro
                param_buffer_count = 0;
                param_count = 0; //(seu contador antigo, resetando por segurança)
                
                //passo 5: armazena o valor de retorno (que está em %eax) no destino
                if (instr->result) {
                    asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                }
                break;

            case IR_PRINT_INT: {
                //versão linux/gcc (system v amd64 abi):
                //arg 1 (formato) vai em %rdi
                //arg 2 (valor) vai em %rsi
                
                //1. carrega o formato em %rdi (primeiro argumento)
                asm_instr("leaq .L.str.int(%%rip), %%rdi");
                
                //2. carrega o valor em %esi (segundo argumento - parte baixa de %rsi)
                asm_instr("movl %s, %%esi", get_operand_asm(instr->arg1));
                
                //3. gcc exige %rax = 0 para printf (indica 0 args de ponto flutuante)
                asm_instr("xorl %%eax, %%eax");
                
                //4. alinha a pilha para 16 bytes (system v exige isso)
                asm_instr("subq $8, %%rsp");
                
                //5. chama printf
                asm_instr("call printf");
                
                //6. restaura a pilha
                asm_instr("addq $8, %%rsp");
                break;
            }

            case IR_PRINT_CHAR: {
                //versão linux/gcc:
                //arg 1 (formato) em %rdi
                //arg 2 (char) em %esi
                
                asm_instr("leaq .L.str.char(%%rip), %%rdi");
                asm_instr("movl %s, %%esi", get_operand_asm(instr->arg1));
                asm_instr("xorl %%eax, %%eax");
                asm_instr("subq $8, %%rsp");
                asm_instr("call printf");
                asm_instr("addq $8, %%rsp");
                break;
            }

            case IR_PRINT_STRING: {
                //versão linux/gcc:
                //arg 1 (formato "%s\n") em %rdi
                //arg 2 (ponteiro para string) em %rsi
                
                asm_instr("leaq .L.str.str(%%rip), %%rdi");
                asm_instr("leaq %s(%%rip), %%rsi", get_operand_asm(instr->arg1));
                asm_instr("xorl %%eax, %%eax");
                asm_instr("subq $8, %%rsp");
                asm_instr("call printf");
                asm_instr("addq $8, %%rsp");
                break;
            }

            /**
             * @brief traduz a instrução ir_return.
             * @detalhes traduz `return arg1` (ex: return z).
             * o valor de retorno é movido para %eax e o epílogo é gerado.
             */
            case IR_RETURN:
                if(instr->arg1) {
                    //passo 1: move o valor de retorno para %eax (convenção de chamada)
                    asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                }
                //passo 2: gera o epílogo da função
                asm_gen_epilogue();
                break;

            /**
             * @brief traduz a instrução ir_load (leitura de array).
             * @detalhes traduz `result := load arg1[arg2]` (ex: t0 := v[i]).
             * usa o endereçamento indexado do x86-64: [base + indice * escala].
             * %rax armazena o endereço base do array (arg1).
             * %ebx armazena o índice (arg2).
             * %ecx armazena o valor carregado da memória.
             */
            case IR_LOAD:
                //passo 1: carrega o endereço base do array (arg1) em %rax
                //(leaq = load effective address)
                asm_instr("leaq %s, %%rax", get_operand_asm(instr->arg1));
                //passo 2: carrega o índice (arg2) em %ebx
                asm_instr("movl %s, %%ebx", get_operand_asm(instr->arg2));
                //passo 3: carrega o valor do endereço [base + indice*4] em %ecx
                //a sintaxe at&t para isso é: offset(base, indice, escala)
                //usamos (%rax, %rbx, 4) -> %rax=base, %rbx=indice, 4=escala(bytes)
                asm_instr("movl (%%rax, %%rbx, 4), %%ecx");
                //passo 4: armazena o valor (em %ecx) no destino (result)
                asm_instr("movl %%ecx, %s", get_operand_asm(instr->result));
                break;

            /**
             * @brief traduz a instrução ir_store (escrita em array).
             * @detalhes traduz `store result[arg2] := arg1` (ex: v[i] := x).
             * %rax armazena o endereço base do array (result).
             * %ebx armazena o índice (arg2).
             * %ecx armazena o valor a ser guardado (arg1).
             */
            case IR_STORE:
                //passo 1: carrega o endereço base do array (result) em %rax
                asm_instr("leaq %s, %%rax", get_operand_asm(instr->result));
                //passo 2: carrega o índice (arg2) em %ebx
                asm_instr("movl %s, %%ebx", get_operand_asm(instr->arg2));
                //passo 3: carrega o valor a ser armazenado (arg1) em %ecx
                asm_instr("movl %s, %%ecx", get_operand_asm(instr->arg1));
                //passo 4: armazena o valor (em %ecx) no endereço [base + indice*4]
                asm_instr("movl %%ecx, (%%rax, %%rbx, 4)");
                break;

            default:
                asm_instr("# opcode da ir nao implementado: %d", instr->opcode);
                break;
        }
        instr = instr->next;
    }

    fclose(outfile);
    return 0;
}