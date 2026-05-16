# bebopCORE

Projeto novo baseado nas decisoes consolidadas para o controle no shell do GBA.

## Requisitos de ambiente

- ESP-IDF `5.5.x`
- Python `3.11.x`

## Objetivos desta fase

- firmware unico (sem troca por particao OTA)
- pronto para expansao futura de protocolos
- botao `SHIFT` para modificador de input
- botao `PROTOCOL` para funcoes de sistema (troca de protocolo/pairing)
- LED de power unico vermelho com frequencia de pisca conforme bateria
- arquitetura preparada para migrar de GPIO direto para matriz de botoes no futuro

## Estrutura

- `main/input_*`: camada de leitura de input (GPIO agora, matriz depois)
- `main/protocol_manager.*`: camada de protocolo em firmware unico
- `main/status_leds.*`: camada logica de LEDs
- `main/power_led.*`: politica do LED de power por estado de bateria
- `main/Kconfig.projbuild`: opcoes de `idf.py menuconfig` para backend, pinos, bateria, LED e tempos de hold

## Configuracao

Use `idf.py menuconfig` para ajustar o projeto sem editar o codigo.

## Status

Este `bebopCORE` e um esqueleto funcional para iniciar implementacao incremental.
