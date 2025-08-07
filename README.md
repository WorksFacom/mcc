# Compilador Didático para Linguagem Simple-C

Este é um compilador com fins acadêmicos para uma sublinguagem simples baseada em C. O projeto foi desenvolvido em C e implementa as três fases principais do front-end de um compilador moderno:

1. Análise Léxica: Converte o código-fonte em uma sequência de tokens.
2. Análise Sintática: Valida a gramática, reporta erros de sintaxe e constrói uma Árvore Sintática Abstrata (AST).
3. Análise Semântica: Valida as regras de significado da linguagem, como tipos, escopos e declarações, utilizando uma Tabela de Símbolos.

## Estrutura de Diretórios

O código fonte está organizado de forma modular dentro da pasta `src/`:

- src/main.c: Ponto de entrada e orquestrador do compilador.
- src/scanner/: Módulo do Analisador Léxico (Scanner).
- src/parser/: Módulo do Analisador Sintático (Parser).
- src/ast/: Definições e funções para a Árvore Sintática Abstrata.
- src/semantic/: Módulo do Analisador Semântico.
- src/symbol_table/: Implementação da Tabela de Símbolos.
- src/tokens/: Definição dos tipos de tokens da linguagem.

## Como Compilar e Executar

O projeto utiliza um Makefile para automatizar todo o processo de compilação e execução das diferentes fases de análise.

### 1. Compilar o Projeto

Para compilar todos os módulos e gerar o executável final (compilador), execute o seguinte comando no terminal, na raiz do projeto:

make

Isso irá criar o arquivo compilador.exe 

### 2. Executar as Fases de Análise

O Makefile está configurado para usar o arquivo teste.cmm como entrada padrão. Você pode rodar e inspecionar a saída de cada fase com os seguintes comandos:

Análise Léxica:
Roda o scanner, imprime os tokens na tela e salva em tokens.txt

make scan

Análise Sintática:
Roda as fases léxica e sintática, imprime a AST na tela e salva em ast.txt

make parse

Análise Semântica:
Roda as três fases de análise e reporta sucesso ou o primeiro erro semântico encontrado

make semantic

Execução Completa:
Roda todo o front-end do compilador

make run

### 3. Usando um Arquivo de Teste Diferente

Você pode facilmente especificar outro arquivo de entrada para os testes diretamente na linha de comando, da seguinte forma:

make scan INPUT=meu_outro_teste.cmm  
make parse INPUT=meu_outro_teste.cmm
make semantic INPUT=meu_outro_teste.cmm
make run INPUT=meu_outro_teste.cmm

### 4. Limpeza

Para remover todos os arquivos gerados pela compilação (.o e o executável), use:

make clean

## Documentação

A documentação do código-fonte está no formato Doxygen.

Gerar a Documentação:
Para gerar a documentação em HTML, certifique-se de ter o Doxygen instalado e execute o seguinte comando na raiz do projeto:

doxygen

(Isso pressupõe que seu arquivo de configuração se chama Doxyfile)

Visualizar a Documentação:
Após a geração, uma nova pasta chamada docs será criada, dentro dela vai ter outra pasta chamada html. Para visualizar, abra o seguinte arquivo no seu navegador de internet:

docs/html/index.html

## Compilação Manual

Caso prefira compilar manualmente sem o Makefile, o comando completo que inclui todos os módulos do projeto é:

gcc -g -Wall -Isrc/scanner -Isrc/parser -Isrc/ast -Isrc/tokens -Isrc/symbol_table -Isrc/semantic src/main.c src/scanner/scanner.c src/parser/parser.c src/ast/ast.c src/symbol_table/symbol_table.c src/semantic/semantic.c -o compilador


Para Rodar a Análise Léxica:

./compilador --scan teste.cmm


Para Rodar a Análise Sintática:

./compilador --parse teste.cmm


Para Rodar a Análise Semântica

./compilador --semantic teste.cmm


Para Rodar a Compilação Completa

./compilador teste.cmm
