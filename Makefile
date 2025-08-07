# Compilador a ser usado
CC = gcc

# Flags de compilação: -g para debug, -Wall para avisos, -I para diretórios de include
CFLAGS = -g -Wall -Isrc/tokens -Isrc/scanner -Isrc/parser -Isrc/ast -Isrc/symbol_table -Isrc/semantic

# Nome do executável final
TARGET = compilador

# Arquivo de entrada padrão para os testes
INPUT = teste.cmm

# Lista de todos os arquivos fonte (.c) do projeto
SOURCES = src/main.c \
          src/scanner/scanner.c \
          src/parser/parser.c \
          src/ast/ast.c \
          src/symbol_table/symbol_table.c \
          src/semantic/semantic.c

# Gera a lista de arquivos objeto (.o) a partir dos fontes
OBJECTS = $(SOURCES:.c=.o)

# Lista de todos os arquivos de cabeçalho para checagem de dependências
DEPS = src/scanner/scanner.h \
       src/tokens/tokens.h \
       src/parser/parser.h \
       src/ast/ast.h \
       src/symbol_table/symbol_table.h \
       src/semantic/semantic.h

# --- ALVOS PRINCIPAIS ---

# 'make' ou 'make all' irá apenas compilar o programa.
all: $(TARGET)
	@echo "------------------------------------------------------------------"
	@echo "✅ Compilacao concluida. Executavel '$(TARGET)' criado."
	@echo "Para rodar as fases no arquivo '$(INPUT)', use:"
	@echo "   make scan      (para ver os tokens)"
	@echo "   make parse     (para ver a AST)"
	@echo "   make semantic  (para rodar a analise semantica)"
	@echo "   make run       (para rodar a compilacao completa)"
	@echo "------------------------------------------------------------------"

# 'make run' roda o compilador no modo completo.
run: $(TARGET)
	@echo "--- Executando Compilacao Completa em $(INPUT) ---"
	./$(TARGET) $(INPUT)

# 'make scan' roda apenas a análise léxica.
scan: $(TARGET)
	@echo "--- Executando Analise Lexica (scan) em $(INPUT) ---"
	./$(TARGET) --scan $(INPUT)

# 'make parse' roda as análises léxica e sintática.
parse: $(TARGET)
	@echo "--- Executando Analise Sintatica (parse) em $(INPUT) ---"
	./$(TARGET) --parse $(INPUT)

# 'make semantic' roda as análises léxica, sintática e semântica.
semantic: $(TARGET)
	@echo "--- Executando Analise Semantica em $(INPUT) ---"
	./$(TARGET) --semantic $(INPUT)

# --- REGRAS DE COMPILAÇÃO E LIMPEZA ---

# Regra para linkar os arquivos objeto e criar o executável final
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Regra genérica para compilar um arquivo .c em um .o
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# Comando para limpar os arquivos gerados
clean:
	-del /Q src\*.o src\scanner\*.o src\parser\*.o src\ast\*.o src\symbol_table\*.o src\semantic\*.o compilador.exe

# Declara alvos que não são arquivos
.PHONY: all clean run scan parse semantic