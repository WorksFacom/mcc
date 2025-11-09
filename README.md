# Compilador Didático para Linguagem Micro C

Este é um compilador com fins acadêmicos para uma sublinguagem simplificada baseada em C.  
O projeto foi desenvolvido em **C** e implementa a **pipeline completa de compilação**,  
desde o código-fonte até a geração de um executável binário.

## Fases Implementadas

1. **Análise Léxica (Scanner):** Converte o código-fonte em uma sequência de tokens.  
2. **Análise Sintática (Parser):** Valida a gramática, reporta erros e constrói a Árvore Sintática Abstrata (AST).  
3. **Análise Semântica (Semantic):** Verifica tipos, escopos e declarações utilizando uma Tabela de Símbolos e calcula os offsets de memória.  
4. **Geração de Código Intermediário (Intercode):** Traduz a AST validada para um Código de Três Endereços (TAC), linear e independente de máquina.  
5. **Geração de Código Assembly (Assembly):** Traduz o código intermediário para Assembly x86-64 (sintaxe AT&T).  

O executável final é obtido através da **linkedição** do Assembly com a biblioteca padrão C (usando `gcc`).

---

## Estrutura de Diretórios

O código-fonte está organizado de forma modular dentro da pasta `src/`:

- `src/main.c`: Ponto de entrada do compilador.  
- `src/tokens/`: Definição dos tipos de tokens (`tokens.h`).  
- `src/scanner/`: Implementação do Analisador Léxico.  
- `src/parser/`: Implementação do Analisador Sintático.  
- `src/ast/`: Definições e funções da Árvore Sintática Abstrata.  
- `src/symbol_table/`: Implementação da Tabela de Símbolos.  
- `src/semantic/`: Implementação do Analisador Semântico.  
- `src/ir/`: Definições da Representação Intermediária (IR).  
- `src/intercode/`: Gerador de IR (AST → IR).  
- `src/assembly/`: Gerador de Assembly (IR → Assembly).  

---

## Como Compilar e Executar

O projeto utiliza um `Makefile` para automatizar o processo de compilação e execução das fases.

### 1. Compilar o Compilador

```bash
make

2. Executar as Fases de Análise

O Makefile usa, por padrão, o arquivo teste.cmm como entrada.  
Você pode executar cada fase separadamente com os comandos abaixo:

# Análise Léxica: imprime tokens e salva em tokens.txt
make scan

# Análise Sintática: gera e salva a AST em ast.txt
make parse

# Análise Semântica: executa as três fases iniciais e valida tipos/escopos
make semantic

# Geração de Código Intermediário: imprime a IR e salva em ir.txt
make gen-ir

# Geração de Assembly: imprime o código Assembly e salva em teste.s
make gen-asm

3. Execução Completa (Compilar e Rodar)

Executa todas as fases do compilador e roda o programa final:

make run

Esse comando:
1. Gera o Assembly (teste.s) com make gen-asm.
2. Usa o gcc para montar e linkar o arquivo, criando o executável teste.
3. Executa o programa final (./teste).

4. Usando um Arquivo de Teste Diferente

Você pode especificar outro arquivo de entrada diretamente na linha de comando:

make scan INPUT=meu_teste.cmm
make gen-asm INPUT=meu_teste.cmm
make run INPUT=meu_teste.cmm

Os arquivos de saída (como .s e .txt) serão gerados com o nome correspondente.

5. Limpeza

Para remover todos os arquivos gerados (objetos, executáveis e temporários):

make clean

---

## Documentação

A documentação do código-fonte está configurada para uso com Doxygen.

### Gerar Documentação

Certifique-se de ter o Doxygen instalado e execute:

doxygen

(O comando pressupõe que o arquivo de configuração se chama Doxyfile.)

### Visualizar a Documentação

Após a geração, abra o seguinte arquivo no navegador:

docs/html/index.html

---

## Compilação Manual (sem Makefile)

Caso prefira compilar manualmente:

1. Compilar o Compilador

gcc -g -Wall \
-Isrc/tokens -Isrc/scanner -Isrc/parser -Isrc/ast -Isrc/symbol_table -Isrc/semantic -Isrc/ir -Isrc/intercode -Isrc/assembly \
src/main.c \
src/scanner/scanner.c \
src/parser/parser.c \
src/ast/ast.c \
src/symbol_table/symbol_table.c \
src/semantic/semantic.c \
src/ir/ir.c \
src/intercode/intercode.c \
src/assembly/assembly.c \
-o compilador

2. Executar as Fases (Exemplo com teste.cmm)

./compilador --scan teste.cmm       # Análise Léxica
./compilador --parse teste.cmm      # Análise Sintática
./compilador --semantic teste.cmm   # Análise Semântica
./compilador --gen-ir teste.cmm     # Geração de IR
./compilador --gen-asm teste.cmm    # Geração de Assembly

3. Rodar a Compilação Completa (Manual)

# 1. Gerar o Assembly
./compilador teste.cmm

# 2. Montar e linkar com o GCC
gcc teste.s -o teste

# 3. Executar o programa final
./teste
