# ==============================================================================
# CONFIGURAZIONE COMPILATORE E FLAG
# ==============================================================================
CC       := gcc
CFLAGS   := -Wall -Wextra -Iinclude
LDFLAGS  := -Llib
LDLIBS   :=
LUA      := lua

# DIRECTORY
SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
OUT_DIR   := out
LIB_DIR   := lib

# Nome dell'eseguibile finale nella cartella out/
TARGET    := $(OUT_DIR)/out

# ==============================================================================
# SORGENTI E OGGETTI
# ==============================================================================
# Trova tutti i file .c nella cartella src/
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Trasforma src/nome.c in build/nome.o
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# File di dipendenza (.d) autogenerati
DEPS := $(OBJS:.o=.d)

# ==============================================================================
# REGOLE / TARGETS
# ==============================================================================
.PHONY: all clean run json test

# Target predefinito
all: $(TARGET)

# Link dell'eseguibile finale nella cartella out/
$(TARGET): $(OBJS) | $(OUT_DIR)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Compilazione dei singoli file .o con generazione automatica delle dipendenze .d
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Creazione delle cartelle di output se non esistono
$(BUILD_DIR) $(OUT_DIR):
	mkdir -p $@

# Esegue l'applicazione
run: $(TARGET)
	./$(TARGET)

# Esegue la suite di test automatizzata in Lua
test: $(TARGET)
	$(LUA) test_runner.lua

# Pulizia dei file generati sia in build/ che in out/
clean:
	rm -rf $(BUILD_DIR) $(OUT_DIR)

# Genera compile_commands.json per autocompletamento/LSP (es. clangd)
json: clean
	bear -- $(MAKE) all

# Include i file di dipendenza autogenerati
-include $(DEPS)
