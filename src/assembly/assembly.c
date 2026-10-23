/**
 * @file assembly.c
 * @brief tradução da ir para assembly x86-64 em sintaxe AT&T.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assembly.h"

static FILE* outfile = NULL;
static char asm_buffer[100];
static IROperand* param_buffer[MAX_PARAMETROS];
static int param_buffer_count = 0;

//================================================================================
// FUNÇÕES AUXILIARES DE EMISSÃO
//================================================================================

/** @brief escreve uma diretiva ou rótulo sem indentação. */
static void asm_print(const char* formato, ...) {
    va_list args;
    va_start(args, formato);
    vfprintf(outfile, formato, args);
    va_end(args);
    fputc('\n', outfile);
}

/** @brief escreve uma instrução com a indentação padrão. */
static void asm_instr(const char* formato, ...) {
    fputs("    ", outfile);
    va_list args;
    va_start(args, formato);
    vfprintf(outfile, formato, args);
    va_end(args);
    fputc('\n', outfile);
}

/** @brief converte um operando da ir para a sintaxe do montador. */
static const char* get_operand_asm(const IROperand* op) {
    if (op == NULL) return "NULL_OPERAND";

    switch (op->type) {
        case OPERAND_CONST:
            snprintf(asm_buffer, sizeof(asm_buffer), "$%d", op->data.const_val);
            break;
        case OPERAND_SYMBOL:
            if (op->data.symbol == NULL) return "NULL_SYMBOL_ERROR";
            snprintf(asm_buffer, sizeof(asm_buffer), "%d(%%rbp)", op->data.symbol->memory_offset);
            break;
        case OPERAND_TEMP:
            snprintf(asm_buffer, sizeof(asm_buffer), "%d(%%rbp)", op->stack_offset);
            break;
        case OPERAND_LABEL:
        case OPERAND_STRING_LBL:
            if (op->data.label_name == NULL) return "NULL_LABEL_ERROR";
            snprintf(asm_buffer, sizeof(asm_buffer), "%s", op->data.label_name);
            break;
        default:
            return "INVALID_OPERAND";
    }
    return asm_buffer;
}

/** @brief gera o prólogo usando o tamanho calculado para a função. */
static void asm_gen_prologue(int frame_size) {
    asm_instr("pushq %%rbp");
    asm_instr("movq %%rsp, %%rbp");
    //o tamanho inclui variáveis, temporários e alinhamento de 16 bytes
    if (frame_size > 0) asm_instr("subq $%d, %%rsp", frame_size);
}

/** @brief restaura o frame e retorna à função chamadora. */
static void asm_gen_epilogue(void) {
    asm_instr("movq %%rbp, %%rsp");
    asm_instr("popq %%rbp");
    asm_instr("ret");
}

/** @brief escreve um literal usando escapes válidos para o montador. */
static void emitir_string_assembly(const char* label, const char* conteudo) {
    fprintf(outfile, "%s: .string \"", label);
    for (const unsigned char* p = (const unsigned char*)conteudo; *p != '\0'; p++) {
        switch (*p) {
            case '\\': fputs("\\\\", outfile); break;
            case '"': fputs("\\\"", outfile); break;
            case '\n': fputs("\\n", outfile); break;
            case '\t': fputs("\\t", outfile); break;
            default:
                if (isprint(*p)) fputc(*p, outfile);
                else fprintf(outfile, "\\%03o", *p);
                break;
        }
    }
    fputs("\"\n", outfile);
}

/** @brief emite uma comparação que produz zero ou um. */
static void emitir_comparacao(IROpcode opcode, const IR_Instruction* instr) {
    asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
    asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
    asm_instr("cmpl %%r10d, %%eax");
    asm_instr("movl $0, %%eax");
    if (opcode == IR_EQ) asm_instr("sete %%al");
    if (opcode == IR_NEQ) asm_instr("setne %%al");
    if (opcode == IR_LT) asm_instr("setl %%al");
    if (opcode == IR_LEQ) asm_instr("setle %%al");
    if (opcode == IR_GT) asm_instr("setg %%al");
    if (opcode == IR_GEQ) asm_instr("setge %%al");
    asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
}

/** @brief emite uma chamada interna com argumentos passados pela pilha. */
static int emitir_chamada(const IR_Instruction* instr) {
    if (instr->arg2 == NULL || instr->arg2->type != OPERAND_CONST ||
        instr->arg2->data.const_val != param_buffer_count) {
        fprintf(stderr, "Erro: Sequencia de parametros inconsistente na IR.\n");
        return -1;
    }

    //o padding precede os argumentos e não altera seus offsets no destino
    int padding = param_buffer_count % 2 != 0 ? 8 : 0;
    if (padding != 0) asm_instr("subq $8, %%rsp");
    for (int i = param_buffer_count - 1; i >= 0; i--) {
        asm_instr("movl %s, %%eax", get_operand_asm(param_buffer[i]));
        asm_instr("pushq %%rax");
    }
    asm_instr("call %s", get_operand_asm(instr->arg1));

    int bytes = param_buffer_count * 8 + padding;
    if (bytes > 0) asm_instr("addq $%d, %%rsp", bytes);
    param_buffer_count = 0;

    if (instr->result != NULL) {
        asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
    }
    return 0;
}

//================================================================================
// FUNÇÃO PRINCIPAL DE GERAÇÃO
//================================================================================

/** @brief gera o arquivo assembly correspondente à lista de ir. */
int gerar_assembly(IR_Instruction* ir_head, PilhaTabelasSimbolos* pilha,
                   const char* nome_arquivo_saida) {
    outfile = fopen(nome_arquivo_saida, "w");
    if (outfile == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel criar o arquivo de saida assembly '%s'\n",
                nome_arquivo_saida);
        return -1;
    }
    param_buffer_count = 0;

    asm_print(".section .rodata");
    asm_print(".L.str.int: .string \"%%d\\n\"");
    asm_print(".L.str.char: .string \"%%c\\n\"");
    asm_print(".L.str.str: .string \"%%s\\n\"");

    for (IR_Instruction* instr = ir_head; instr != NULL; instr = instr->next) {
        if ((instr->opcode == IR_PRINT_STRING || instr->opcode == IR_STRCPY) &&
            instr->arg1 != NULL && instr->arg1->type == OPERAND_STRING_LBL &&
            instr->arg1->data.label_name != NULL && instr->arg1->string_content != NULL) {
            emitir_string_assembly(instr->arg1->data.label_name, instr->arg1->string_content);
        }
    }

    asm_print("");
    asm_print(".text");

    for (IR_Instruction* instr = ir_head; instr != NULL; instr = instr->next) {
        switch (instr->opcode) {
            case IR_LABEL: {
                Simbolo* simbolo = buscar_simbolo_em_todos_escopos(
                    pilha, instr->result->data.label_name);
                if (simbolo != NULL && simbolo->is_function) {
                    asm_print(".globl %s", simbolo->nome);
                    asm_print("%s:", simbolo->nome);
                    asm_gen_prologue(simbolo->frame_size);
                } else {
                    asm_print("%s:", get_operand_asm(instr->result));
                }
                break;
            }

            case IR_ASSIGN:
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
                if (instr->opcode == IR_ADD) asm_instr("addl %%r10d, %%eax");
                if (instr->opcode == IR_SUB) asm_instr("subl %%r10d, %%eax");
                if (instr->opcode == IR_MUL) asm_instr("imull %%r10d, %%eax");
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            case IR_DIV:
            case IR_MOD:
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("cdq");
                asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
                asm_instr("idivl %%r10d");
                asm_instr("movl %s, %s",
                          instr->opcode == IR_DIV ? "%eax" : "%edx",
                          get_operand_asm(instr->result));
                break;

            case IR_EQ:
            case IR_NEQ:
            case IR_LT:
            case IR_LEQ:
            case IR_GT:
            case IR_GEQ:
                emitir_comparacao(instr->opcode, instr);
                break;

            case IR_NEG:
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("negl %%eax");
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            case IR_NOT:
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("cmpl $0, %%eax");
                asm_instr("movl $0, %%eax");
                asm_instr("sete %%al");
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            case IR_AND:
            case IR_OR:
                //mantido para ir importada; a geração atual usa curto-circuito
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("cmpl $0, %%eax");
                asm_instr("setne %%al");
                asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
                asm_instr("cmpl $0, %%r10d");
                asm_instr("setne %%r10b");
                if (instr->opcode == IR_AND) asm_instr("andb %%r10b, %%al");
                else asm_instr("orb %%r10b, %%al");
                asm_instr("movzbl %%al, %%eax");
                asm_instr("movl %%eax, %s", get_operand_asm(instr->result));
                break;

            case IR_IF_FALSE:
                asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                asm_instr("cmpl $0, %%eax");
                asm_instr("je %s", get_operand_asm(instr->result));
                break;

            case IR_GOTO:
                asm_instr("jmp %s", get_operand_asm(instr->result));
                break;

            case IR_PARAM:
                if (param_buffer_count >= MAX_PARAMETROS) {
                    fprintf(stderr, "Erro: Limite de parametros excedido no backend.\n");
                    fclose(outfile);
                    outfile = NULL;
                    return -1;
                }
                param_buffer[param_buffer_count++] = instr->arg1;
                break;

            case IR_CALL:
                if (emitir_chamada(instr) != 0) {
                    fclose(outfile);
                    outfile = NULL;
                    return -1;
                }
                break;

            case IR_PRINT_INT:
                asm_instr("leaq .L.str.int(%%rip), %%rdi");
                asm_instr("movl %s, %%esi", get_operand_asm(instr->arg1));
                asm_instr("xorl %%eax, %%eax");
                asm_instr("call printf");
                break;

            case IR_PRINT_CHAR:
                asm_instr("leaq .L.str.char(%%rip), %%rdi");
                asm_instr("movl %s, %%esi", get_operand_asm(instr->arg1));
                asm_instr("xorl %%eax, %%eax");
                asm_instr("call printf");
                break;

            case IR_PRINT_STRING:
                asm_instr("leaq .L.str.str(%%rip), %%rdi");
                if (instr->arg1->type == OPERAND_SYMBOL) {
                    asm_instr("leaq %s, %%rsi", get_operand_asm(instr->arg1));
                } else {
                    asm_instr("leaq %s(%%rip), %%rsi", get_operand_asm(instr->arg1));
                }
                asm_instr("xorl %%eax, %%eax");
                asm_instr("call printf");
                break;

            case IR_STRCPY:
                asm_instr("leaq %s, %%rdi", get_operand_asm(instr->result));
                asm_instr("leaq %s(%%rip), %%rsi", get_operand_asm(instr->arg1));
                asm_instr("call strcpy");
                break;

            case IR_RETURN:
                if (instr->arg1 != NULL) {
                    asm_instr("movl %s, %%eax", get_operand_asm(instr->arg1));
                }
                asm_gen_epilogue();
                break;

            case IR_LOAD:
                asm_instr("leaq %s, %%rax", get_operand_asm(instr->arg1));
                asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
                asm_instr("movl (%%rax, %%r10, 4), %%ecx");
                asm_instr("movl %%ecx, %s", get_operand_asm(instr->result));
                break;

            case IR_LOAD_BYTE:
                asm_instr("leaq %s, %%rax", get_operand_asm(instr->arg1));
                asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
                asm_instr("movzbl (%%rax, %%r10, 1), %%ecx");
                asm_instr("movl %%ecx, %s", get_operand_asm(instr->result));
                break;

            case IR_STORE:
                asm_instr("leaq %s, %%rax", get_operand_asm(instr->result));
                asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
                asm_instr("movl %s, %%ecx", get_operand_asm(instr->arg1));
                asm_instr("movl %%ecx, (%%rax, %%r10, 4)");
                break;

            case IR_STORE_BYTE:
                asm_instr("leaq %s, %%rax", get_operand_asm(instr->result));
                asm_instr("movl %s, %%r10d", get_operand_asm(instr->arg2));
                asm_instr("movl %s, %%ecx", get_operand_asm(instr->arg1));
                asm_instr("movb %%cl, (%%rax, %%r10, 1)");
                break;

            default:
                fprintf(stderr, "Erro: Opcode da IR nao implementado: %d\n", instr->opcode);
                fclose(outfile);
                outfile = NULL;
                return -1;
        }
    }

    fclose(outfile);
    outfile = NULL;
    return 0;
}
