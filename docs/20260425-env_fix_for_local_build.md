### [GROUND-ZERO] - SAVESTATE: FIRMWARE CONTEXT MERGE

**PROJETO:** bebopCORE (n64-switch) / BlueRetro  
**AMBIENTE:** Nostromo (Intel Mac / Sequoia)  
**STATUS:** Build v5.0 COMPLETO. Ambientes isolados.

---

### [DEV] Configuração do Ambiente

# Context Prompt para Importação de Branch

# Repositório: ~/Desktop/bebopCORE/firmware/n64-switch

## 1. Stack Tecnológica

- ESP-IDF: v5.0 (bebopCORE) / v5.2 (BlueRetro)
- Python: 3.8.19 (v5.0) e 3.9.19 (v5.2) via pyenv
- OS: macOS Sequoia (Intel x86_64)

## 2. Decisões Arquiteturais (ADR)

- [PURGE]: Implementada função `_idf_purge` no .zshrc para limpar PATH e variáveis do IDF antes de qualquer `source export.sh`.
- [LIBS]: Downgrade manual de `cryptography==35.0.0` no venv v5.0 para resolver erro de arquitetura mach-o e satisfazer restrições do IDF.
- [PARTITION]: Tabela de partição customizada com `switch_mode` (ota_0) e `blueretro_mode` (ota_1), ambos de 1MB.

## 3. Comandos de Ativação (Sync)

- Alias `get-idf-5.0`: Seta PYENV_VERSION=3.8.19 e ativa export.sh.
- Alias `get-idf-5.2`: Seta PYENV_VERSION=3.9.19 e ativa export.sh com --prefer-system.

## 4. Estado do Build

- Firmwares buildados com sucesso após fix de criptografia.
- Ponto de falha anterior (626/1172 - mbedtls) RESOLVIDO.
- Binários gerados: build/n64-switch.bin e partições OTA.

---

### [NEXT] Próximos Passos

1. **Flash & Monitor:** Executar `idff idfm` na branch `restart` do bebopCORE.
2. **Shift Logic:** Validar comportamento momentary do pino Shift (boot/recovery mode no pino 12).
3. **Encryption Check:** Monitorar se o bootloader aceita as partições marcadas como `encrypted` na partition table sem o Flash Encryption ativo no hardware.

---

### [FIX] Troubleshooting Rápido

Se o ambiente "melar" na nova branch:

```sh
export IDF_SKIP_CHECK_PYTHON_PACKAGES=1
idf.py fullclean
idfb
```

**Merge concluído. Nostromo pronto para o flash.** 🍻
