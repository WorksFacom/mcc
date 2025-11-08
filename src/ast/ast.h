#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include "tokens.h" 

/**
 * @file ast.h
 * @brief definições das estruturas de dados para a árvore sintática abstrata (ast).
 *
 * este arquivo estabelece a fundação para a representação intermediária do código
 * fonte. a ast gerada pelo analisador sintático é a principal estrutura de dados
 * que será consumida pelas fases subsequentes do compilador, como a análise
 * semântica e a geração de código.
 */

/**
 * @brief enumeração de todos os tipos de nós possíveis na árvore sintática abstrata.
 *
 * cada valor nesta enumeração corresponde a uma construção gramatical ou a um
 * elemento léxico da linguagem, permitindo que o analisador semântico identifique
 * e processe cada parte da árvore de forma específica.
 */
typedef enum {
    NODE_UNDEFINED,         ///< nó indefinido ou para uso em estados de erro.
    NODE_PROGRAM,           ///< nó raiz da ast, representa o programa inteiro.
    NODE_FUNCTION_DEF,      ///< representa a definição completa de uma função.
    NODE_VAR_DECL,          ///< representa a declaração de uma variável.
    NODE_ASSIGN,            ///< representa uma operação de atribuição (ex: x = 5).
    NODE_IF,                ///< representa uma estrutura condicional 'if-else'.
    NODE_FOR,               ///< representa um laço 'for'.
    NODE_RETURN,            ///< representa um comando 'return'.
    NODE_PRINT,             ///< representa um comando 'print'.
    NODE_CALL,              ///< representa uma chamada de função.
    NODE_BINARY_OP,         ///< representa uma operação binária (ex: +, -, *, <, ==).
    NODE_UNARY_OP,          ///< representa uma operação unária (ex: -, !).
    NODE_ID,                ///< representa um identificador (nome de variável ou função).
    NODE_INTEGER_CONST,     ///< representa uma constante do tipo inteiro.
    NODE_CHAR_CONST,        ///< representa uma constante do tipo caractere.
    NODE_STRING_CONST,      ///< representa uma constante do tipo string.
    NODE_BLOCK,             ///< representa um bloco de comandos delimitado por { ... }.
    NODE_PARAM_LIST,        ///< representa a lista de parâmetros na definição de uma função.
    NODE_ARG_LIST,          ///< representa a lista de argumentos em uma chamada de função.
    NODE_ARRAY_ACCESS,      ///< representa o acesso a um elemento de um array (ex: v[i]).
    NODE_TYPE               ///< nó simples para armazenar um tipo de dado (int, char).
} NodeType;

/**
 * @brief estrutura de um nó individual da árvore sintática abstrata (ast).
 *
 * esta é a estrutura central da ast. cada nó contém seu tipo, ponteiros para
 * formar a hierarquia da árvore (filhos e irmãos) e uma união de dados para
 * armazenar informações específicas, como o valor de uma constante ou o nome
 * de um identificador.
 */
typedef struct ASTNode {
    /** @brief o tipo do nó, conforme definido na enumeração NodeType. */
    NodeType node_type;

    /** @brief a linha do código fonte onde este nó (ou sua construção) se originou. */
    int linha; 

    /** @brief ponteiro para o primeiro nó filho na hierarquia da árvore. */
    struct ASTNode *filho;
    
    /** @brief ponteiro para o próximo nó no mesmo nível hierárquico (irmão). */
    struct ASTNode *proximo_irmao;

    /** @brief união para armazenar dados específicos do nó de forma eficiente. */
    union {
        long int_value;      ///< usado por nós do tipo NODE_INTEGER_CONST.
        char char_value;     ///< usado por nós do tipo NODE_CHAR_CONST.
        char* string_value;  ///< usado por nós NODE_ID e NODE_STRING_CONST (requer alocação dinâmica).
        TokenType op_type;   ///< usado por NODE_BINARY_OP e NODE_TYPE para guardar o tipo de operador ou de dado.
    } data;

} ASTNode;


/**
 * @brief aloca e inicializa um novo nó para a ast.
 * @param type o tipo do nó a ser criado (da enumeração NodeType).
 * @param linha a linha do código fonte correspondente a este nó.
 * @return um ponteiro para o ASTNode recém-criado e inicializado. a função encerra o programa se a alocação de memória falhar.
 */
ASTNode* criar_no(NodeType type, int linha);

/**
 * @brief adiciona um nó 'filho' a um nó 'pai' na ast.
 *
 * a função insere o nó filho no final da lista de filhos do nó pai, mantendo
 * a ordem de inserção, que geralmente corresponde à ordem de aparição no código.
 * @param pai o nó que receberá o novo filho.
 * @param filho o nó a ser adicionado como filho.
 */
void adicionar_filho(ASTNode* pai, ASTNode* filho);

/**
 * @brief libera recursivamente toda a memória alocada para uma sub-árvore da ast.
 *
 * esta função deve ser chamada no nó raiz da árvore para limpar toda a memória
 * utilizada pela ast ao final da compilação. ela navega em pós-ordem para garantir
 * que os filhos sejam liberados antes dos pais.
 * @param no o nó raiz da sub-árvore a ser liberada.
 */
void liberar_arvore(ASTNode* no);

#endif //AST_H