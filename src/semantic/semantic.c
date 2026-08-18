/**
 * @file semantic.c
 * @brief implementação do analisador semântico em múltiplas passagens.
 *
 * a primeira passagem registra todas as funções. a segunda valida corpos,
 * escopos, tipos e retornos, além de atribuir offsets únicos por função.
 */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

typedef struct {
    PilhaTabelasSimbolos* pilha;
    Simbolo* funcao;
    int bytes_locais;
    int proximo_parametro;
} ContextoSemantico;

static FILE* arquivo_simbolos = NULL;
static bool modo_verbose = false;

//================================================================================
// DIAGNÓSTICOS E TAMANHOS
//================================================================================

/** @brief reporta um erro semântico e encerra a compilação. */
static void erro_semantico(const char* mensagem, int linha) {
    fprintf(stderr, "Erro Semantico na linha %d: %s\n", linha, mensagem);
    exit(EXIT_FAILURE);
}

/** @brief alinha um deslocamento para o próximo múltiplo de quatro. */
static int alinhar_4(int valor) {
    return (valor + 3) & ~3;
}

/** @brief alinha o frame para chamadas que exigem múltiplos de 16 bytes. */
static int alinhar_16(int valor) {
    return (valor + 15) & ~15;
}

/** @brief calcula o tamanho de armazenamento de um símbolo local. */
static int tamanho_simbolo(const Simbolo* simbolo) {
    if (simbolo->is_array) {
        int tamanho_elemento = simbolo->tipo == CHAR ? 1 : 4;
        return simbolo->array_size * tamanho_elemento;
    }

    //escalares int e char usam quatro bytes, conforme o modelo didático do livro
    return 4;
}

/** @brief imprime uma tabela no arquivo e, opcionalmente, no terminal. */
static void registrar_tabela(TabelaSimbolos* tabela, const char* titulo) {
    if (arquivo_simbolos != NULL) {
        imprimir_tabela(tabela, titulo, arquivo_simbolos);
    }
    if (modo_verbose) {
        imprimir_tabela(tabela, titulo, stdout);
    }
}

//================================================================================
// PRIMEIRA PASSAGEM: DECLARAÇÕES GLOBAIS
//================================================================================

/** @brief registra todas as assinaturas antes de analisar qualquer corpo. */
static void registrar_funcoes(ASTNode* raiz, PilhaTabelasSimbolos* pilha) {
    for (ASTNode* funcao = raiz->filho; funcao != NULL; funcao = funcao->proximo_irmao) {
        if (funcao->node_type != NODE_FUNCTION_DEF) continue;

        ASTNode* tipo = funcao->filho;
        ASTNode* id = tipo->proximo_irmao;
        if (buscar_simbolo_no_escopo_atual(pilha, id->data.string_value) != NULL) {
            char mensagem[256];
            snprintf(mensagem, sizeof(mensagem), "Funcao '%s' ja foi declarada.", id->data.string_value);
            erro_semantico(mensagem, id->linha);
        }

        Simbolo* simbolo = adicionar_simbolo(pilha, id->data.string_value,
                                              tipo->data.op_type, 1, 0, 0, 0);
        if (simbolo == NULL) {
            erro_semantico("Nao foi possivel registrar a funcao.", id->linha);
        }
        id->symbol = simbolo;

        ASTNode* parametros = id->proximo_irmao;
        if (parametros != NULL && parametros->node_type == NODE_PARAM_LIST) {
            for (ASTNode* parametro = parametros->filho;
                 parametro != NULL;
                 parametro = parametro->proximo_irmao) {
                if (simbolo->num_parametros >= MAX_PARAMETROS) {
                    erro_semantico("Numero maximo de parametros excedido.", parametro->linha);
                }
                simbolo->param_tipos[simbolo->num_parametros++] = parametro->filho->data.op_type;
            }
        }
    }

    Simbolo* principal = buscar_simbolo_no_escopo_atual(pilha, "main");
    if (principal == NULL || !principal->is_function) {
        erro_semantico("Programa deve declarar a funcao main.", 0);
    }
    if (principal->tipo != INT || principal->num_parametros != 0) {
        erro_semantico("Funcao main deve possuir retorno int e nenhum parametro.", 0);
    }
}

//================================================================================
// SEGUNDA PASSAGEM: TIPOS, ESCOPOS E LAYOUT
//================================================================================

static TokenType obter_tipo_expressao(ASTNode* expressao, ContextoSemantico* contexto, int* tamanho_string);
static void analisar_instrucao(ASTNode* no, ContextoSemantico* contexto);

/** @brief associa um identificador ao símbolo visível no ponto de uso. */
static Simbolo* resolver_simbolo(ASTNode* id, ContextoSemantico* contexto) {
    Simbolo* simbolo = buscar_simbolo_em_todos_escopos(contexto->pilha, id->data.string_value);
    if (simbolo == NULL) {
        char mensagem[256];
        snprintf(mensagem, sizeof(mensagem), "Identificador '%s' nao foi declarado.", id->data.string_value);
        erro_semantico(mensagem, id->linha);
    }
    id->symbol = simbolo;
    return simbolo;
}

/** @brief valida uma chamada e anota seu tipo de retorno. */
static TokenType obter_tipo_chamada(ASTNode* chamada, ContextoSemantico* contexto) {
    ASTNode* id = chamada->filho;
    Simbolo* funcao = resolver_simbolo(id, contexto);
    if (!funcao->is_function) {
        char mensagem[256];
        snprintf(mensagem, sizeof(mensagem), "'%s' nao e uma funcao.", funcao->nome);
        erro_semantico(mensagem, id->linha);
    }

    ASTNode* lista = id->proximo_irmao;
    ASTNode* argumento = lista != NULL ? lista->filho : NULL;
    int quantidade = 0;
    while (argumento != NULL) {
        if (quantidade >= funcao->num_parametros) {
            erro_semantico("Numero excessivo de argumentos na chamada da funcao.", argumento->linha);
        }
        TokenType tipo = obter_tipo_expressao(argumento, contexto, NULL);
        if (tipo != funcao->param_tipos[quantidade]) {
            erro_semantico("Tipo de argumento incompativel na chamada da funcao.", argumento->linha);
        }
        quantidade++;
        argumento = argumento->proximo_irmao;
    }

    if (quantidade != funcao->num_parametros) {
        erro_semantico("Numero insuficiente de argumentos na chamada da funcao.", chamada->linha);
    }

    chamada->inferred_type = funcao->tipo;
    return funcao->tipo;
}

/** @brief determina e anota o tipo de uma expressão validada. */
static TokenType obter_tipo_expressao(ASTNode* expressao, ContextoSemantico* contexto, int* tamanho_string) {
    if (expressao == NULL) return UNDEF;
    if (tamanho_string != NULL) *tamanho_string = 0;

    switch (expressao->node_type) {
        case NODE_INTEGER_CONST:
            expressao->inferred_type = INT;
            return INT;

        case NODE_CHAR_CONST:
            expressao->inferred_type = CHAR;
            return CHAR;

        case NODE_STRING_CONST:
            expressao->inferred_type = STRINGCONST;
            if (tamanho_string != NULL) {
                size_t tamanho = strlen(expressao->data.string_value) + 1;
                if (tamanho > INT_MAX) {
                    erro_semantico("Literal de string excede o limite suportado.", expressao->linha);
                }
                *tamanho_string = (int)tamanho;
            }
            return STRINGCONST;

        case NODE_ID: {
            Simbolo* simbolo = resolver_simbolo(expressao, contexto);
            if (simbolo->is_function) {
                char mensagem[256];
                snprintf(mensagem, sizeof(mensagem), "'%s' e uma funcao e nao pode ser usada como variavel.", simbolo->nome);
                erro_semantico(mensagem, expressao->linha);
            }
            if (simbolo->is_array) {
                if (simbolo->tipo != CHAR) {
                    erro_semantico("Array int deve ser usado com um indice.", expressao->linha);
                }
                expressao->inferred_type = STRINGCONST;
                return STRINGCONST;
            }
            expressao->inferred_type = simbolo->tipo;
            return simbolo->tipo;
        }

        case NODE_ARRAY_ACCESS: {
            ASTNode* id = expressao->filho;
            Simbolo* simbolo = resolver_simbolo(id, contexto);
            if (!simbolo->is_array) {
                erro_semantico("Somente arrays podem ser indexados.", id->linha);
            }
            if (obter_tipo_expressao(id->proximo_irmao, contexto, NULL) != INT) {
                erro_semantico("Indice de array deve ser do tipo inteiro.", expressao->linha);
            }
            expressao->inferred_type = simbolo->tipo;
            return simbolo->tipo;
        }

        case NODE_UNARY_OP:
            if (obter_tipo_expressao(expressao->filho, contexto, NULL) != INT) {
                erro_semantico("Operador unario espera uma expressao inteira.", expressao->linha);
            }
            expressao->inferred_type = INT;
            return INT;

        case NODE_BINARY_OP: {
            TokenType esquerdo = obter_tipo_expressao(expressao->filho, contexto, NULL);
            TokenType direito = obter_tipo_expressao(expressao->filho->proximo_irmao, contexto, NULL);
            if (esquerdo != INT || direito != INT) {
                erro_semantico("Tipos incompativeis em operacao binaria (esperado INT).", expressao->linha);
            }
            expressao->inferred_type = INT;
            return INT;
        }

        case NODE_CALL:
            return obter_tipo_chamada(expressao, contexto);

        default:
            erro_semantico("No inesperado dentro de uma expressao.", expressao->linha);
            return UNDEF;
    }
}

/** @brief reserva um offset único no frame da função atual. */
static void atribuir_offset_local(Simbolo* simbolo, ContextoSemantico* contexto) {
    int tamanho = tamanho_simbolo(simbolo);
    int alinhamento = (!simbolo->is_array || simbolo->tipo == INT) ? 4 : 1;
    if (alinhamento == 4) contexto->bytes_locais = alinhar_4(contexto->bytes_locais);
    contexto->bytes_locais += tamanho;
    simbolo->memory_offset = -contexto->bytes_locais;
}

/** @brief valida uma atribuição já representada na ast. */
static void analisar_atribuicao(ASTNode* atribuicao, ContextoSemantico* contexto) {
    ASTNode* esquerdo = atribuicao->filho;
    ASTNode* direito = esquerdo->proximo_irmao;
    TokenType tipo_esquerdo = obter_tipo_expressao(esquerdo, contexto, NULL);
    int tamanho_string = 0;
    TokenType tipo_direito = obter_tipo_expressao(direito, contexto, &tamanho_string);

    if (tipo_esquerdo != tipo_direito) {
        erro_semantico("Atribuicao de tipos incompativeis.", atribuicao->linha);
    }

    if (tipo_esquerdo == STRINGCONST) {
        if (esquerdo->node_type != NODE_ID || direito->node_type != NODE_STRING_CONST) {
            erro_semantico("Arrays char so podem receber uma string literal.", atribuicao->linha);
        }
        Simbolo* destino = (Simbolo*)esquerdo->symbol;
        if (tamanho_string > destino->array_size) {
            char mensagem[256];
            snprintf(mensagem, sizeof(mensagem),
                     "String e muito grande para o array '%s' (capacidade %d, necessario %d).",
                     destino->nome, destino->array_size, tamanho_string);
            erro_semantico(mensagem, atribuicao->linha);
        }
    }
}

/** @brief declara um parâmetro ou variável e atribui seu endereço no frame. */
static void analisar_declaracao(ASTNode* declaracao, ContextoSemantico* contexto, bool parametro) {
    ASTNode* tipo = declaracao->filho;
    ASTNode* id = tipo->proximo_irmao;
    ASTNode* seguinte = id->proximo_irmao;
    int is_array = 0;
    int array_size = 0;

    if (seguinte != NULL && seguinte->node_type == NODE_INTEGER_CONST) {
        if (seguinte->data.int_value <= 0 || seguinte->data.int_value > INT_MAX) {
            erro_semantico("Tamanho de array deve ser um inteiro positivo.", seguinte->linha);
        }
        is_array = 1;
        array_size = (int)seguinte->data.int_value;
        seguinte = seguinte->proximo_irmao;
    }

    if (parametro && is_array) {
        erro_semantico("Arrays nao podem ser usados como parametros nesta versao.", id->linha);
    }
    if (parametro && seguinte != NULL && seguinte->node_type == NODE_ASSIGN) {
        erro_semantico("Parametros nao podem possuir inicializador.", id->linha);
    }
    if (buscar_simbolo_no_escopo_atual(contexto->pilha, id->data.string_value) != NULL) {
        char mensagem[256];
        snprintf(mensagem, sizeof(mensagem), "'%s' ja foi declarado neste escopo.", id->data.string_value);
        erro_semantico(mensagem, id->linha);
    }

    Simbolo* simbolo = adicionar_simbolo(contexto->pilha, id->data.string_value,
                                          tipo->data.op_type, 0, is_array,
                                          array_size, parametro);
    if (simbolo == NULL) {
        erro_semantico("Nao foi possivel adicionar o simbolo a tabela.", id->linha);
    }
    id->symbol = simbolo;

    if (parametro) {
        simbolo->memory_offset = 16 + contexto->proximo_parametro * 8;
        contexto->proximo_parametro++;
    } else {
        atribuir_offset_local(simbolo, contexto);
    }

    if (seguinte != NULL && seguinte->node_type == NODE_ASSIGN) {
        seguinte->filho->symbol = simbolo;
        seguinte->filho->inferred_type = is_array && simbolo->tipo == CHAR ? STRINGCONST : simbolo->tipo;
        analisar_atribuicao(seguinte, contexto);
    }
}

/** @brief analisa um bloco, criando escopo quando necessário. */
static void analisar_bloco(ASTNode* bloco, ContextoSemantico* contexto, bool criar_escopo) {
    if (criar_escopo) empilhar_tabela(contexto->pilha);

    for (ASTNode* instrucao = bloco->filho;
         instrucao != NULL;
         instrucao = instrucao->proximo_irmao) {
        analisar_instrucao(instrucao, contexto);
    }

    if (criar_escopo) {
        char titulo[80];
        snprintf(titulo, sizeof(titulo), "Bloco Interno (Linha %d)", bloco->linha);
        registrar_tabela(contexto->pilha->tabelas[contexto->pilha->topo], titulo);
        desempilhar_tabela(contexto->pilha);
    }
}

/** @brief analisa uma instrução e liga todos os símbolos utilizados. */
static void analisar_instrucao(ASTNode* no, ContextoSemantico* contexto) {
    if (no == NULL || no->node_type == NODE_UNDEFINED) return;

    switch (no->node_type) {
        case NODE_BLOCK:
            analisar_bloco(no, contexto, true);
            break;

        case NODE_VAR_DECL:
            analisar_declaracao(no, contexto, false);
            break;

        case NODE_ASSIGN:
            analisar_atribuicao(no, contexto);
            break;

        case NODE_IF: {
            ASTNode* condicao = no->filho;
            if (obter_tipo_expressao(condicao, contexto, NULL) != INT) {
                erro_semantico("Condicao do if deve ser inteira.", condicao->linha);
            }
            analisar_instrucao(condicao->proximo_irmao, contexto);
            if (condicao->proximo_irmao != NULL) {
                analisar_instrucao(condicao->proximo_irmao->proximo_irmao, contexto);
            }
            break;
        }

        case NODE_FOR: {
            empilhar_tabela(contexto->pilha);
            ASTNode* inicializacao = no->filho;
            ASTNode* condicao = inicializacao->proximo_irmao;
            ASTNode* incremento = condicao->proximo_irmao;
            ASTNode* corpo = incremento->proximo_irmao;
            analisar_instrucao(inicializacao, contexto);
            if (condicao->node_type != NODE_UNDEFINED &&
                obter_tipo_expressao(condicao, contexto, NULL) != INT) {
                erro_semantico("Condicao do for deve ser inteira.", condicao->linha);
            }
            analisar_instrucao(incremento, contexto);
            analisar_instrucao(corpo, contexto);
            char titulo[80];
            snprintf(titulo, sizeof(titulo), "Escopo do For (Linha %d)", no->linha);
            registrar_tabela(contexto->pilha->tabelas[contexto->pilha->topo], titulo);
            desempilhar_tabela(contexto->pilha);
            break;
        }

        case NODE_RETURN: {
            if (no->filho == NULL) {
                erro_semantico("Funcao int ou char deve retornar um valor.", no->linha);
            }
            TokenType tipo = obter_tipo_expressao(no->filho, contexto, NULL);
            if (tipo != contexto->funcao->tipo) {
                erro_semantico("Tipo de retorno incompativel com a funcao.", no->linha);
            }
            break;
        }

        case NODE_PRINT: {
            TokenType tipo = obter_tipo_expressao(no->filho, contexto, NULL);
            if (tipo != INT && tipo != CHAR && tipo != STRINGCONST) {
                erro_semantico("Argumento invalido para print.", no->linha);
            }
            break;
        }

        case NODE_CALL:
            obter_tipo_chamada(no, contexto);
            break;

        case NODE_ARRAY_ACCESS:
        case NODE_BINARY_OP:
        case NODE_UNARY_OP:
        case NODE_ID:
            obter_tipo_expressao(no, contexto, NULL);
            break;

        default:
            break;
    }
}

//================================================================================
// ANÁLISE DE FLUXO DE RETORNO
//================================================================================

/** @brief informa se uma instrução garante retorno em todos os seus caminhos. */
static bool instrucao_retorna(const ASTNode* no) {
    if (no == NULL) return false;
    if (no->node_type == NODE_RETURN) return true;

    if (no->node_type == NODE_IF) {
        const ASTNode* bloco_then = no->filho != NULL ? no->filho->proximo_irmao : NULL;
        const ASTNode* bloco_else = bloco_then != NULL ? bloco_then->proximo_irmao : NULL;
        return bloco_else != NULL && instrucao_retorna(bloco_then) && instrucao_retorna(bloco_else);
    }

    if (no->node_type == NODE_BLOCK) {
        for (const ASTNode* filho = no->filho; filho != NULL; filho = filho->proximo_irmao) {
            if (instrucao_retorna(filho)) return true;
        }
    }
    return false;
}

/** @brief analisa parâmetros e corpo de uma função registrada. */
static void analisar_funcao(ASTNode* no, PilhaTabelasSimbolos* pilha) {
    ASTNode* id = no->filho->proximo_irmao;
    ASTNode* seguinte = id->proximo_irmao;
    ASTNode* parametros = NULL;
    ASTNode* corpo = seguinte;
    if (seguinte != NULL && seguinte->node_type == NODE_PARAM_LIST) {
        parametros = seguinte;
        corpo = seguinte->proximo_irmao;
    }

    ContextoSemantico contexto = {
        .pilha = pilha,
        .funcao = (Simbolo*)id->symbol,
        .bytes_locais = 0,
        .proximo_parametro = 0
    };

    empilhar_tabela(pilha);
    if (parametros != NULL) {
        for (ASTNode* parametro = parametros->filho;
             parametro != NULL;
             parametro = parametro->proximo_irmao) {
            analisar_declaracao(parametro, &contexto, true);
        }
    }
    analisar_bloco(corpo, &contexto, false);

    if (!instrucao_retorna(corpo)) {
        char mensagem[256];
        snprintf(mensagem, sizeof(mensagem),
                 "Funcao '%s' pode terminar sem retornar um valor.", contexto.funcao->nome);
        erro_semantico(mensagem, id->linha);
    }

    contexto.funcao->local_size = alinhar_4(contexto.bytes_locais);
    contexto.funcao->frame_size = alinhar_16(contexto.funcao->local_size);

    char titulo[128];
    snprintf(titulo, sizeof(titulo), "Funcao: %s", contexto.funcao->nome);
    registrar_tabela(pilha->tabelas[pilha->topo], titulo);
    desempilhar_tabela(pilha);
}

/** @brief executa as passagens da análise semântica. */
void analisar_semanticamente(ASTNode* raiz, PilhaTabelasSimbolos* pilha, bool verbose) {
    if (raiz == NULL || pilha == NULL) {
        fprintf(stderr, "Erro: Arvore ou tabela de simbolos invalida.\n");
        exit(EXIT_FAILURE);
    }

    modo_verbose = verbose;
    arquivo_simbolos = fopen("symbols.txt", "w");
    if (arquivo_simbolos == NULL) {
        perror("Aviso: Nao foi possivel criar symbols.txt");
    }

    registrar_funcoes(raiz, pilha);
    for (ASTNode* funcao = raiz->filho; funcao != NULL; funcao = funcao->proximo_irmao) {
        analisar_funcao(funcao, pilha);
    }
    registrar_tabela(pilha->tabelas[0], "GLOBAL");

    if (arquivo_simbolos != NULL) fclose(arquivo_simbolos);
    arquivo_simbolos = NULL;
}
