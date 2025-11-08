# Compilador a ser usado
CC = gcc

# Flags de compilação: -g para debug, -Wall para avisos, -I para diretórios de include
CFLAGS = -g -Wall -Isrc/tokens -Isrc/scanner -Isrc/parser -Isrc/ast -Isrc/symbol_table -Isrc/semantic -Isrc/ir -Isrc/intercode -Isrc/assembly

# Nome do executável final
TARGET = compilador

# Arquivo de entrada padrão para os testes
# (Pode ser sobrescrito: make run INPUT=teste2.cmm)
INPUT = teste.cmm


# Pega o "nome base" do arquivo de entrada (ex: "teste")
BASENAME = $(basename $(INPUT))
# Define os nomes de saída a partir do nome base (ex: "teste.s", "teste")
ASM_OUTPUT = $(BASENAME).s
EXEC_NAME = $(BASENAME)


# Encontra todos os arquivos .cmm no diretório
CMM_FILES = $(wildcard *.cmm)
# Gera uma lista de todos os executáveis possíveis (ex: teste, codigo)
EXEC_OUTPUTS = $(patsubst %.cmm,%,$(CMM_FILES))


# Lista de todos os arquivos fonte (.c) do projeto
SOURCES = src/main.c \
          src/scanner/scanner.c \
          src/parser/parser.c \
          src/ast/ast.c \
          src/symbol_table/symbol_table.c \
          src/semantic/semantic.c \
          src/ir/ir.c \
          src/intercode/intercode.c \
          src/assembly/assembly.c

# Gera a lista de arquivos objeto (.o) a partir dos fontes
OBJECTS = $(SOURCES:.c=.o)

# Lista de todos os arquivos de cabeçalho para checagem de dependências
DEPS = src/scanner/scanner.h \
       src/tokens/tokens.h \
       src/parser/parser.h \
       src/ast/ast.h \
       src/symbol_table/symbol_table.h \
       src/semantic/semantic.h \
       src/ir/ir.h \
       src/intercode/intercode.h \
       src/assembly/assembly.h



# 'make' ou 'make all' irá apenas compilar o programa.
all: $(TARGET)
	@echo "------------------------------------------------------------------"
	@echo "✅ Compilacao concluida. Executavel '$(TARGET)' criado."
	@echo "Para rodar as fases no arquivo '$(INPUT)', use:"
	@echo "   make scan      (para ver os tokens)"
	@echo "   make parse     (para ver a AST)"
	@echo "   make semantic  (para rodar a analise semantica)"
	@echo "   make gen-ir    (para gerar o Codigo Intermediario)"
	@echo "   make gen-asm   (para gerar o Codigo Assembly)"
	@echo "   make run       (para compilar e rodar o '$(INPUT)')"
	@echo "------------------------------------------------------------------"

# 'make run' compila e executa o código fonte de teste

run: $(TARGET)
	@echo "=============================================="
	@echo "Iniciando compilação completa de: $(INPUT)"
	@echo "=============================================="
	@echo ""
	# 1. Compila o código-fonte com o compilador completo (sem flags)
	@./$(TARGET) $(INPUT)
	
	@echo ""
	@echo "Compilando o assembly '$(ASM_OUTPUT)' com GCC..."
	@$(CC) -Wl,-z,noexecstack $(ASM_OUTPUT) -o $(EXEC_NAME)
	
	@echo ""
	@echo "Executável '$(EXEC_NAME)' criado com sucesso!"
	@echo ""
	
	@echo "Executando o programa:"
	@./$(EXEC_NAME)
	
	@echo ""
	@echo "=============================================="
	@echo "Execução finalizada de: $(INPUT)"
	@echo "=============================================="


# 'make gen-asm' roda o compilador até a geração de assembly

gen-asm: $(ASM_OUTPUT)


# Alvo para criar o executável final a partir do assembly
# A regra $(EXEC_NAME) (ex: teste) depende de $(ASM_OUTPUT) (ex: teste.s)
$(EXEC_NAME): $(ASM_OUTPUT)
	@echo "--- Compilando o Assembly '$<' com o GCC ---"
	# $< é o pré-requisito (teste.s)
	# $@ é o alvo (teste)
	$(CC) -Wl,-z,noexecstack $< -o $@
	@echo "--- Executavel '$@' criado com sucesso ---"

# Esta regra ensina o 'make' a criar um arquivo .s (como teste.s)
# a partir de um .cmm (como teste.cmm)
$(ASM_OUTPUT): $(TARGET) $(INPUT)
	@echo "--- Gerando Codigo Assembly (gen-asm) de $(INPUT) ---"
	./$(TARGET) --gen-asm $(INPUT)
	@echo "--- [ CONTEUDO DE $(ASM_OUTPUT) ] ---"
	@cat $(ASM_OUTPUT)
	@echo "--- [ FIM DE $(ASM_OUTPUT) ] ---"


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

# 'make gen-ir' roda até a geração de código intermediário.
gen-ir: $(TARGET)
	@echo "--- Gerando Codigo Intermediario (gen-ir) de $(INPUT) ---"
	./$(TARGET) --gen-ir $(INPUT)

# --- REGRAS DE COMPILAÇÃO E LIMPEZA ---

# Regra para linkar os arquivos objeto e criar o executável final
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Regra genérica para compilar um arquivo .c em um .o
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@


# Comando para limpar os arquivos gerados
clean:
ifeq ($(OS),Windows_NT)
	-del /Q src\*.o src\scanner\*.o src\parser\*.o src\ast\*.o src\symbol_table\*.o src\semantic\*.o src\ir\*.o src\intercode\*.o src\assembly\*.o $(TARGET).exe *.s $(EXEC_OUTPUTS:%=%.exe)
else
	-rm -f src/*.o src/scanner/*.o src/parser/*.o src/ast/*.o src/symbol_table/*.o src/semantic/*.o src/ir/*.o src/intercode/*.o src/assembly/*.o $(TARGET) *.s $(EXEC_OUTPUTS)
endif


# Declara alvos que não são arquivos
.PHONY: all clean run scan parse semantic gen-ir gen-asm