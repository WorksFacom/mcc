#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/**
 * @file ast.c
 * @brief implementação das funções auxiliares para manipulação da árvore sintática abstrata (ast).
 *
 * este arquivo contém a lógica para criar, conectar e destruir os nós da ast.
 * essas funções são a base para que o analisador sintático possa construir a árvore
 * de forma eficiente e para que a memória seja gerenciada corretamente ao final
 * do processo.
 */

/**
 * @brief aloca e inicializa um novo nó para a ast.
 * @param type o tipo do nó a ser criado (da enumeração NodeType).
 * @param linha a linha do código fonte correspondente a este nó.
 * @return um ponteiro para o ASTNode recém-criado e inicializado. a função encerra o programa se a alocação de memória falhar.
 */
ASTNode* criar_no(NodeType type, int linha) {
    //aloca memória para a estrutura do nó
    ASTNode* no = (ASTNode*) malloc(sizeof(ASTNode));
    if (no == NULL) {
        fprintf(stderr, "Erro fatal: Falha ao alocar memoria para no da AST.\n");
        exit(EXIT_FAILURE);
    }

    //inicializa os campos do nó com valores padrão
    no->node_type = type;
    no->linha = linha;
    no->filho = NULL;
    no->proximo_irmao = NULL;

    //limpa a união de dados para evitar valores de lixo
    memset(&no->data, 0, sizeof(no->data));

    return no;
}

/**
 * @brief adiciona um nó 'filho' a um nó 'pai' na ast.
 *
 * a função insere o nó filho no final da lista de filhos do nó pai, mantendo
 * a ordem de inserção, que geralmente corresponde à ordem de aparição no código.
 * @param pai o nó que receberá o novo filho.
 * @param filho o nó a ser adicionado como filho.
 */
void adicionar_filho(ASTNode* pai, ASTNode* filho) {
    //verificação de segurança para evitar falhas de segmentação
    if (pai == NULL || filho == NULL) {
        return; 
    }

    //se o pai ainda não tem filhos, este se torna o primeiro
    if (pai->filho == NULL) {
        pai->filho = filho;
    } else {
        //se já existem filhos, percorre a lista de irmãos até o final
        ASTNode* irmao_atual = pai->filho;
        while (irmao_atual->proximo_irmao != NULL) {
            irmao_atual = irmao_atual->proximo_irmao;
        }
        //adiciona o novo filho no final da lista
        irmao_atual->proximo_irmao = filho;
    }
}

/**
 * @brief libera recursivamente toda a memória alocada para uma sub-árvore da ast.
 *
 * esta função deve ser chamada no nó raiz da árvore para limpar toda a memória
 * utilizada pela ast ao final da compilação. ela navega em pós-ordem para garantir
 * que os filhos sejam liberados antes dos pais.
 * @param no o nó raiz da sub-árvore a ser liberada.
 */
void liberar_arvore(ASTNode* no) {
    if (no == NULL) {
        return;
    }

    //1. libera recursivamente todos os filhos da sub-árvore atual
    liberar_arvore(no->filho);
    //2. libera recursivamente todos os irmãos do nó atual
    liberar_arvore(no->proximo_irmao);

    //3. libera dados alocados dinamicamente dentro do próprio nó
    //(ex: strings para nomes de identificadores)
    if (no->node_type == NODE_ID || no->node_type == NODE_STRING_CONST) {
        if (no->data.string_value != NULL) {
            free(no->data.string_value);
        }
    }
    
    //4. finalmente, libera a memória da estrutura do nó
    free(no);
}