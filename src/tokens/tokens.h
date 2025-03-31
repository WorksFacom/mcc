

typedef enum {
    UNDEF,              // token indefinido
    ID,                 // identificador
    INTEGERCONST,       // constante inteira 
    CHARCONST,          // constante de caractere 
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



typedef struct {
    
    TokenType tipo;
    char lexema[50]; 

} Token;


