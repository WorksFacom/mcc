# bc

Academic Basic Compiler

Como Compilar e Executar:

make

Para remover os arquivos objeto (.o) e o executável, use:

make clean

A documentação do código é feita com Doxygen. Para gerá-la, basta executar:

doxygen

Para compilar manualmente (sem o make), caso queira:

gcc -g -Wall -Isrc/tokens -Isrc/scanner src/main.c src/scanner/scanner.c -o compilador

./compilador teste.cmm
