#!/bin/zsh

# --- CONFIGURAÇÕES ---
ROOT_DIR=$(pwd)
FINAL_BUILD_DIR="$ROOT_DIR/build_final"
RELEASE_FILE="bebopCORE_Full.bin"

# Cores
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "${BLUE}===> bebopCORE Flash Tool (Nostromo Edition) <===${NC}"

# 1. Verifica se o binário existe
if [ ! -f "$FINAL_BUILD_DIR/$RELEASE_FILE" ]; then
    echo "${RED}Erro: Binário não encontrado em $FINAL_BUILD_DIR/$RELEASE_FILE${NC}"
    exit 1
fi

# 2. Busca portas seriais usando FIND (mais seguro contra erros de match do Zsh)
echo "${BLUE}Buscando dispositivos serial...${NC}"

# Procuramos por cu.usbserial, cu.usbmodem ou cu.wchusbserial
# O grep remove erros de permissão ou caminhos inexistentes
ports=($(find /dev -name "cu.usbserial*" -o -name "cu.usbmodem*" -o -name "cu.wchusbserial*" 2>/dev/null))

if [ ${#ports[@]} -eq 0 ]; then
    echo "${YELLOW}Nenhuma porta encontrada automaticamente.${NC}"
    echo "Digite o caminho manual (ex: /dev/cu.usbserial-1420):"
    read "SELECTED_PORT"
else
    echo "Portas encontradas:"
    for i in {1..${#ports[@]}}; do
        echo "  $i) ${ports[$i]}"
    done
    
    default_choice=1
    read "choice?Escolha a porta [1-${#ports[@]}] (padrão $default_choice): "
    
    choice=${choice:-$default_choice}
    SELECTED_PORT=${ports[$choice]}
fi

# 3. Validação e Clipboard
if [ -z "$SELECTED_PORT" ]; then
    echo "${RED}Seleção inválida.${NC}"
    exit 1
fi

echo "${GREEN}Porta selecionada: $SELECTED_PORT${NC}"

# Entramos na pasta onde o projeto existe para o monitor funcionar com os símbolos de debug
FLASH_CMD="esptool.py --chip esp32 --port $SELECTED_PORT --baud 460800 write_flash 0x0 $FINAL_BUILD_DIR/$RELEASE_FILE && cd firmware/n64-switch && idf.py -p $SELECTED_PORT monitor"

echo "-------------------------------------------------------"
echo "${YELLOW}COPIE E COLE O COMANDO ABAIXO:${NC}"
echo "-------------------------------------------------------"
echo "\n$FLASH_CMD\n"
echo "-------------------------------------------------------"

echo -n "$FLASH_CMD" | pbcopy
echo "${BLUE}Comando copiado! Só dar Cmd+V.${NC}"