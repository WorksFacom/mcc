#ifndef CONFIG_H
#define CONFIG_H

// --- Configurações do Scanner e Tokens ---

#define MAX_INT_LEN      12   // 10 dígitos + 1 sinal + 1 nulo = 12. Arredondando para dar folga.
#define MAX_ID_LEN       20   // Tamanho máximo de um identificador (nome de variável/função)
#define MAX_STR_LEN      256  // Tamanho máximo de uma string literal "..."
#define MAX_TOKEN_LEN    256  //define o tamanho do buffer do Token como o MAIOR dentre eles


// --- Configurações main ---
#define MAX_PATH_LEN     256   //para nomes de arquivos e caminhos do sistema
#define MAX_INDENT_SIZE  256  //suporta até 128 níveis de aninhamento
#define TOKEN_LIST  100   // Capacidade inicial da lista de tokens

// --- Configurações do Parser e AST ---
#define MAX_CHILDREN     10    // Número máximo de filhos por nó (se sua AST usar vetor estático)

// --- Configurações da Tabela de Símbolos ---
#define SYMBOL_TABLE_SIZE 211  // Tamanho da tabela hash (números primos são melhores para hash)
#define SCOPE_STACK_SIZE  20   // Profundidade máxima de escopos aninhados
#define MAX_MSG_LEN      512   // Tamanho seguro para mensagens de erro e títulos que contêm nomes de variáveis
#define MAX_SIMBOLOS 100
#define MAX_PARAMETROS 10

// --- Configurações Gerais ---
#define VERSION "0.1"

#endif // CONFIG_H