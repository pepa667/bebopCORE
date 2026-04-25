#!/bin/zsh

# --- CONFIGURAÇÕES ---
ROOT_DIR=$(pwd)
FINAL_BUILD_DIR="$ROOT_DIR/build_final"
TMP_DIR="$ROOT_DIR/tmp_bins"
RELEASE_FILE="bebopCORE_Full.bin"

# Cores para o log
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

get-idf-5.0

echo "${BLUE}===> Iniciando processo de Merge do bebopCORE <===${NC}"

# 1. Limpeza e Preparação
rm -rf "$TMP_DIR" "$FINAL_BUILD_DIR"
mkdir -p "$TMP_DIR" "$FINAL_BUILD_DIR"

# 2. Coleta de Binários do bebopCORE (Switch Mode - v5.0)
echo "${YELLOW}[1/3] Coletando binários do bebopCORE (n64-switch)...${NC}"
cp "firmware/n64-switch/build/n64-switch.bin" "$TMP_DIR/n64-control-switch.bin" 2>/dev/null || cp "firmware/n64-switch/build/n64-control-switch.bin" "$TMP_DIR/n64-control-switch.bin"
cp "firmware/n64-switch/build/ota_data_initial.bin" "$TMP_DIR/"
cp "firmware/n64-switch/build/bootloader/bootloader.bin" "$TMP_DIR/"
cp "firmware/n64-switch/build/partition_table/partition-table.bin" "$TMP_DIR/"

# 3. Coleta de Binários do BlueRetro Mode (v5.2)
echo "${YELLOW}[2/3] Coletando binários do BlueRetro...${NC}"
cp "firmware/n64-blueretro/build/n64-control-blueretro.bin" "$TMP_DIR/" 2>/dev/null || echo "Aviso: Binário BlueRetro não encontrado."

# 4. Verificação de Integridade
if [ ! -f "$TMP_DIR/bootloader.bin" ] || [ ! -f "$TMP_DIR/n64-control-switch.bin" ]; then
    echo "\033[0;31mERRO: Binários essenciais não encontrados. Verifique se deu build em tudo.\033[0m"
    exit 1
fi

# 5. Merge via esptool.py
echo "${YELLOW}[3/3] Gerando binário unificado: $RELEASE_FILE...${NC}"

# Usamos o python do venv atual (certifique-se de estar com o ambiente carregado)
esptool.py --chip esp32 merge_bin -o "$FINAL_BUILD_DIR/$RELEASE_FILE" \
    0x1000   "$TMP_DIR/bootloader.bin" \
    0x8000   "$TMP_DIR/partition-table.bin" \
    0xd000   "$TMP_DIR/ota_data_initial.bin" \
    0x10000  "$TMP_DIR/n64-control-switch.bin" \
    0x110000 "$TMP_DIR/n64-control-blueretro.bin"

echo "${GREEN}===> SUCESSO! <===${NC}"
echo "Arquivo gerado em: ${BLUE}$FINAL_BUILD_DIR/$RELEASE_FILE${NC}"
echo "Comando para flash e monitorar (Tudo-em-um):"
echo "./flash_monitor.sh"