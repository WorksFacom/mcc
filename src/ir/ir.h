#ifndef IR_H
#define IR_H

#include "symbol_table.h"

/**
 * @file ir.h
 * @brief definições das estruturas de dados para a representação intermediária (ir).
 *
 * este arquivo define as estruturas para o código de três endereços (tac),
 * que serve como uma ponte de alto nível e independente de máquina entre o
 * front-end e o back-end gerador de assembly.
 */

//enumeração de todos os opcodes possíveis na nossa ir
typedef enum {
    //opcodes aritméticos binários: resultado = arg1 op arg2
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,

    //opcodes lógicos e de comparação (retornam 0 ou 1)
    IR_EQ,          //resultado = (arg1 == arg2)
    IR_NEQ,         //resultado = (arg1 != arg2)
    IR_LT,          //resultado = (arg1 < arg2)
    IR_LEQ,         //resultado = (arg1 <= arg2)
    IR_GT,          //resultado = (arg1 > arg2)
    IR_GEQ,         //resultado = (arg1 >= arg2)
    IR_AND,         //resultado = (arg1 && arg2)
    IR_OR,          //resultado = (arg1 || arg2)

    //opcodes unários: resultado = op arg1
    IR_NEG,         //negação aritmética (ex: -x)
    IR_NOT,         //negação lógica (ex: !x)

    //opcodes de movimentação de dados
    IR_STRCPY,
    IR_ASSIGN,      //resultado = arg1 (mov)
    IR_STORE,       //armazena em um array: resultado[arg2] = arg1
    IR_LOAD,        //carrega de um array: resultado = arg1[arg2]
    IR_LOAD_BYTE,  //leitura de 1 byte (para char)
    IR_STORE_BYTE, //escrita de 1 byte (para char)
    
    //opcodes de controle de fluxo
    IR_LABEL,       //define um rótulo: L1:
    IR_GOTO,        //pula incondicionalmente para um rótulo
    IR_IF_FALSE,    //se arg1 for falso (zero), pula para o rótulo em resultado
    
    //opcodes de chamada de função
    IR_PARAM,       //define um parâmetro para uma chamada de função
    IR_CALL,        //chama uma função e armazena o retorno em resultado
    IR_RETURN,      //retorna de uma função
    IR_PRINT_INT,     // Para imprimir um argumento do tipo int
    IR_PRINT_CHAR,    // Para imprimir um argumento do tipo char
    IR_PRINT_STRING   // Para imprimir um argumento do tipo string literal
} IROpcode;

//enumeração dos tipos de operandos
typedef enum {
    OPERAND_EMPTY,
    OPERAND_SYMBOL,     //uma variável ou parâmetro (referência à tabela de símbolos)
    OPERAND_TEMP,       //uma variável temporária gerada pelo compilador
    OPERAND_CONST,      //uma constante inteira
    OPERAND_STRING_LBL, //um rótulo para uma string literal
    OPERAND_LABEL,      //um rótulo para um ponto no código (ex: L1, L2)
} OperandType;

//estrutura para um operando
typedef struct {
    OperandType type;
    int owns_label;
    union {
        Simbolo* symbol;
        int temp_id;
        int const_val;
        char* label_name;
    } data;
    char* string_content;
} IROperand;

//estrutura para uma única instrução da nossa ir
typedef struct IR_Instruction {
    IROpcode opcode;
    IROperand* result;
    IROperand* arg1;
    IROperand* arg2;
    struct IR_Instruction* next;
} IR_Instruction;

/**
 * @brief libera toda a memória alocada por uma lista de instruções da ir.
 * @param ir_head ponteiro para a primeira instrução da lista.
 */
void liberar_ir(IR_Instruction* ir_head);

#endif //IR_H