# Smart Price Tag - Placa de Preço Inteligente

Sistema de placa de preço inteligente usando ESP32 com display TFT touch, Bluetooth e acessibilidade por áudio.

## Características

- Display TFT 2.8" com interface LVGL
- Comunicação Bluetooth entre dispositivos
- Função de repetidor para estender alcance
- Acessibilidade por áudio para pessoas com deficiência visual
- Touch screen para interação
- Atualização em tempo real de preços

## Hardware Necessário

- ESP32 2.8" TFT Touch Display (CYD - Cheap Yellow Display)
- Buzzer passivo ou módulo de áudio
- Fonte de alimentação 5V

## Instalação

1. Instale o PlatformIO no VSCode
2. Clone o repositório
3. Compile e faça upload:

```bash
pio run -t upload