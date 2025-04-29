/** @file tokens.h
 *  @brief Definições de tipos de tokens e estrutura de token para a linguagem Simple C.
 *
 *  Este arquivo define a enumeração dos tipos de tokens reconhecidos pelo analisador léxico
 *  e a estrutura que armazena informações sobre cada token gerado.
 */

 /** @brief Enumeração dos tipos de tokens da linguagem Simple C.
 *
 *  Define todos os tipos de tokens que o analisador léxico pode gerar, incluindo
 *  operadores, palavras reservadas, constantes e símbolos especiais.
 */
typedef enum {
    UNDEF,              // token indefinido
    ID,                 // identificador
    INTEGERCONST,       // constante inteira 
    CHARCONST,          // constante de caractere
    STRINGCONST,        // string de caracteres
    PLUS,               // operador +
    MINUS,              // operador -
    MUL,                // operador *
    DIV,                // operador /
    MOD,                // operador %
    EQ,                 // operador ==
    NEQ,                // operador !=
    LT,                 // operador <
    GT,                 // operador >
    LEQ,                // operador <=
    GEQ,                // operador >=
    AND,                // operador &&
    OR,                 // operador ||
    NOT,                // operador !
    ASSIGN,             // operador =
    SEMICOLON,          // ponto e virgula ;
    COMMA,              // virgula ,
    LPAREN,             // parêntese esquerdo (
    RPAREN,             // parentese direito )
    LBRACE,             // chave esquerda {
    RBRACE,             // chave direita }
    LBRACKET,           // colchete esquerdo [
    RBRACKET,           // colchete direito ]
    MAIN,               // palavra reservada main
    IF,                 // palavra reservada if
    ELSE,               // Palavra reservada else
    FOR,                // palavra reservada for
    RETURN,             // palavra reservada return
    INT,                // palavra reservada int
    CHAR,               // palavra reservada char
    END_OF_FILE         // fim de arquivo
} TokenType;


/** @brief Estrutura que representa um token gerado pelo analisador léxico.
 *
 *  Armazena o tipo do token, o lexema e a linha onde foi encontrado.
 */
typedef struct {
    /** @brief Tipo do token, conforme definido em TokenType. */
    TokenType tipo;
    char lexema[100];
    int linha;

} Token;


