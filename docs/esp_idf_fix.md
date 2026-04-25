# NOSTROMO - ESP-IDF MULTI-VERSION SETUP LOG (INTEL MAC / SEQUOIA)

# DATA: ABRIL 2026

# STATUS: OPERACIONAL

---

## 1. CONTEXTO DO DESASTRE

* **Hardware:** Mac Intel (Nostromo Workstation).
* **SO:** macOS Sequoia.
* **Problema:** Erro de dlopen() no cryptography/cffi devido a mismatch de arquitetura e versões de bibliotecas.
* **Causa:** Mesmo sendo Intel, o Pip tentou injetar binários incompatíveis ou destinados a slices de arquitetura que o Python 3.8 legado não suporta no Sequoia, gerando conflito no carregamento das extensões em C.

---

## 2. MATRIZ DE AMBIENTE (PYENV)

Cada versão do ESP-IDF isolada em seu próprio Python para garantir que o sistema global não quebre os firmwares legados:

| Versão IDF | Versão Python | Projeto Alvo |
| :--- | :--- | :--- |
| **v5.0** | 3.8.19 | bebopCORE (n64-switch) |
| **v5.2** | 3.9.19 | BlueRetro |
| **v5.5** | 3.10.20 | Testes / Futuros |

---

## 3. FIX DA CRYPTOGRAPHY (INTEL x86_64)

O componente mbedtls (gen_crt_bundle.py) falhava por erro de arquitetura. O fix definitivo foi forçar o downgrade para a versão compatível com a "Constraint File" da v5.0:

```bash

# Executado dentro do venv da v5.0

/Users/pepa/.espressif/python_env/idf5.0_py3.8_env/bin/python -m pip install \
--force-reinstall --no-cache-dir cryptography==35.0.0
```

---

## 4. BLINDAGEM DO .ZSHRC (THE PURGE)

Configuração obrigatória para garantir que a troca entre versões limpe o ambiente anterior e evite "cross-contamination" no PATH.

```bash

# --- ESP-IDF MULTI-VERSION (NOSTROMO EDITION) ---

_idf_purge() {
    unset IDF_PATH
    unset IDF_PYTHON_ENV_PATH
    unset VIRTUAL_ENV
    unset PYENV_VERSION
}

# ESP-IDF v5.0

get-idf-5.0() {
    _idf_purge
    export PYENV_VERSION=3.8.19
    export IDF_PATH="$HOME/esp/v5.0/esp-idf"
    . $IDF_PATH/export.sh
}

# ESP-IDF v5.2

get-idf-5.2() {
    _idf_purge
    export PYENV_VERSION=3.9.19
    export IDF_PATH="$HOME/esp/v5.2.3/esp-idf"
    . $IDF_PATH/export.sh --prefer-system
}

# Atalhos Universais (Context-Aware)

alias idfb='idf.py build'
alias idff='idf.py flash'
alias idfm='idf.py monitor'
```

---

## 5. COMANDOS DE MANUTENÇÃO (RESCUE)

Se o diagnóstico de pacotes falhar mesmo com o build funcionando:

* **Ignorar checagem de pacotes:** export IDF_SKIP_CHECK_PYTHON_PACKAGES=1
* **Limpeza de Artefatos:** idf.py fullclean (Obrigatório após mexer no Pip ou trocar de versão).

---

## 6. STATUS FINAL

Builds de firmware concluídos com sucesso. O ambiente Intel está operando com binários nativos x86_64 em todas as instâncias do ESP-IDF.

---
