# Placa de Preço Inteligente — ESP32 CYD 2.8"

Etiqueta eletrônica de prateleira (ESL) com display TFT 2.8" touch, rede
BLE em malha com repetição de sinal e leitura do preço em voz alta para
acessibilidade.

---

## Índice

1. [Decisões de arquitetura](#1-decisões-de-arquitetura)
2. [Estrutura de arquivos](#2-estrutura-de-arquivos)
3. [Hardware e ligações](#3-hardware-e-ligações)
4. [Configuração dos pinos do display](#4-configuração-dos-pinos-do-display)
5. [Configuração do áudio](#5-configuração-do-áudio)
6. [Compilando e gravando](#6-compilando-e-gravando)
7. [Como a mesh funciona](#7-como-a-mesh-funciona)
8. [Provisionando uma placa](#8-provisionando-uma-placa)
9. [Testando](#9-testando)
10. [Limitações conhecidas](#10-limitações-conhecidas)

---

## 1. Decisões de arquitetura

Duas escolhas importantes que fogem do pedido literal, e o porquê:

### Mesh por flood BLE, não Bluetooth Mesh (SIG)

O "Bluetooth Mesh" da especificação Bluetooth SIG exige uma pilha
completa de provisionamento, chaves de rede/aplicação, modelos e
endereços. No ESP32 isso só está bem suportado via **ESP-BLE-MESH
(ESP-IDF puro)** — a integração com o Arduino Framework é frágil, e a
pilha consome bastante RAM, que aqui já está disputada com LVGL e o
framebuffer.

O projeto usa uma **mesh de flood com TTL sobre advertising BLE**, que é
a mesma abordagem adotada por sistemas ESL comerciais:

- Cada placa fica escaneando continuamente.
- Ao receber um pacote novo, entrega à aplicação **e retransmite** com
  `ttl - 1` (é a função repetidora que você pediu).
- Um cache de `msgId` recentes impede loops e retransmissão duplicada.

Vantagens: sem pareamento, sem conexão, sem provisionamento de rede,
propagação em segundos, RAM mínima. Se você precisar mesmo da
especificação SIG (interoperabilidade com produtos de terceiros), o
caminho é migrar para ESP-IDF + ESP-BLE-MESH — é um projeto diferente.

### TTS por concatenação, não síntese em tempo real

Um TTS neural não roda no ESP32 com qualidade utilizável. O que roda bem
é **concatenar clipes pré-gravados**: o firmware converte o preço em uma
lista de palavras (`"vinte"`, `"e"`, `"quatro"`, `"reais"`, ...) e toca
os arquivos `.pcm` correspondentes do LittleFS via DAC interno.

O script `tools/generate_audio.py` gera esses clipes no seu computador
usando gTTS. Vocabulário total: ~45 arquivos, cobrindo qualquer preço de
R$ 0,01 a R$ 9.999,99.

Se nenhum clipe existir, o firmware cai para um padrão de bipes no
buzzer — que sinaliza atenção, mas **não** substitui a leitura por voz.

---

## 2. Estrutura de arquivos

```
placa-preco-inteligente/
├── platformio.ini              # Dependências e flags de build
├── include/
│   ├── config.h                # Pinos de áudio, parâmetros da mesh
│   ├── User_Setup.h            # Pinos do display TFT_eSPI  ← AJUSTE AQUI
│   └── lv_conf.h               # Configuração do LVGL v8
├── src/
│   ├── main.cpp                # Integra tudo: setup() e loop()
│   ├── mesh/
│   │   ├── BleMesh.h           # API da mesh
│   │   └── BleMesh.cpp         # Flood + TTL + repetição + GATT
│   ├── ui/
│   │   ├── display_driver.h/.cpp  # Ponte TFT_eSPI ↔ LVGL + touch
│   │   └── ui.h/.cpp           # Widgets: nome, preço, botão de áudio
│   ├── audio/
│   │   └── AudioAnnouncer.h/.cpp  # DAC/I2S + número por extenso pt-BR
│   └── storage/
│       └── ProductStore.h/.cpp # Persistência em NVS
├── data/audio/                 # Clipes .pcm (gerados, vão pro LittleFS)
└── tools/
    ├── generate_audio.py       # Gera os clipes de voz
    └── gateway_example.cpp     # ESP32 que origina as atualizações
```

---

## 3. Hardware e ligações

| Item | Detalhe |
|---|---|
| Placa | ESP32-2432S028R ("Cheap Yellow Display"), 2.8" 240×320, ILI9341 + XPT2046 |
| Áudio | GPIO25 (DAC1) → amplificador PAM8403 → alto-falante 8Ω |
| Buzzer | GPIO26 → buzzer **passivo** (fallback de bipes) |
| Botão | GPIO0 (BOOT, já na placa) — toque curto fala o preço, 3s entra em config |

**Sobre o áudio:** o DAC do ESP32 tem saída fraquíssima; ligar um
alto-falante direto no GPIO25 não funciona e pode danificar o pino. Use
sempre um amplificador. Um PAM8403 (alguns reais) resolve. Coloque um
capacitor de 10µF em série entre o GPIO25 e a entrada do amplificador
para bloquear o offset DC.

Alternativa melhor: um DAC I2S externo (MAX98357A). Nesse caso, troque a
configuração em `AudioAnnouncer::begin()` de `I2S_MODE_DAC_BUILT_IN`
para I2S padrão com pinos BCLK/LRC/DIN.

---

## 4. Configuração dos pinos do display

Os pinos ficam em **`include/User_Setup.h`**. O `platformio.ini` já força
a TFT_eSPI a usar esse arquivo em vez do padrão da biblioteca:

```ini
build_flags =
    -DUSER_SETUP_LOADED=1
    -include include/User_Setup.h
```

Isso significa que **você não precisa editar nada dentro da pasta
`.pio/libdeps/`** — um erro comum que se perde a cada reinstalação de
dependências.

Pinagem já configurada (CYD comum, USB-C):

```c
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21
#define TOUCH_CS 33
```

### Se a tela ficar branca / preta

- Confira o driver: algumas revisões usam **ST7789** em vez de ILI9341.
  Troque `#define ILI9341_DRIVER` por `#define ST7789_DRIVER`.
- Se as cores saírem invertidas, adicione `#define TFT_INVERSION_ON`.
- Se a imagem sair de cabeça para baixo, mude `tft.setRotation(0)` para
  `2` em `display_driver.cpp`.

### Se o toque não responder no lugar certo

O touch resistivo varia bastante de placa para placa. Rode o exemplo
`Touch_calibrate` da TFT_eSPI, anote os 5 valores, e substitua o array
`calData[]` em `src/ui/display_driver.cpp`.

Em algumas revisões da CYD o XPT2046 fica num **segundo barramento SPI**
(VSPI, pinos 25/32/39/33). Se for o seu caso, o `tft.getTouch()` não vai
funcionar — troque pela biblioteca `XPT2046_Touchscreen` com um
`SPIClass` dedicado. Note que nessa configuração o GPIO25 é usado pelo
touch e **não fica livre para o DAC** — mova o áudio para GPIO26 (DAC2).

---

## 5. Configuração do áudio

Pinos em `include/config.h`:

```c
#define AUDIO_DAC_PIN     25      // DAC1; use 26 (DAC2) se o 25 estiver ocupado
#define AUDIO_SAMPLE_RATE 16000   // deve bater com o generate_audio.py
#define BUZZER_PIN        26
#define ACCESS_BUTTON_PIN  0
```

Gerando os clipes de voz:

```bash
pip install gtts pydub
sudo apt install ffmpeg          # ou: brew install ffmpeg

# vocabulário de números + palavras fixas
python tools/generate_audio.py

# nome do produto desta placa específica
python tools/generate_audio.py --produto "Arroz Tipo 1 5 quilos" --apenas-produto

# grava a pasta data/ no LittleFS da placa
pio run --target uploadfs
```

O `AUDIO_SAMPLE_RATE` do firmware e o `SAMPLE_RATE` do script precisam
ser iguais — se divergirem, a voz sai acelerada ou arrastada.

---

## 6. Compilando e gravando

```bash
# clonar/extrair e entrar na pasta
cd placa-preco-inteligente

# compilar e gravar o firmware
pio run --target upload

# gravar os áudios (depois de rodar o generate_audio.py)
pio run --target uploadfs

# acompanhar o log
pio device monitor
```

---

## 7. Como a mesh funciona

O pacote propagado tem 15 bytes, dentro do campo *manufacturer data* de
um advertising BLE:

| Campo | Bytes | Função |
|---|---|---|
| `magic` | 1 | Assinatura `0xA5` — ignora BLE alheio |
| `msgId` | 4 | ID único da mensagem, usado no dedup |
| `ttl` | 1 | Saltos restantes (começa em 5) |
| `type` | 1 | Tipo da mensagem |
| `productId` | 2 | Produto alvo |
| `priceCents` | 4 | Preço novo, em centavos |
| `flags` | 1 | bit0 = promoção |
| `checksum` | 1 | XOR dos bytes anteriores |

Fluxo de uma atualização:

1. O gateway monta o pacote com `msgId` aleatório e `ttl = 5`, e anuncia.
2. Toda placa no alcance recebe. Se o `msgId` é novo:
   - registra no cache de dedup;
   - se `productId` for o produto dela, atualiza o display e o NVS;
   - se `ttl > 0`, decrementa e **reanuncia por 350ms** (a repetição).
3. As vizinhas dessas repetem de novo, até o `ttl` zerar.

Com `ttl = 5` e ~20m de alcance por salto, a cobertura chega a ~100m de
propagação em cadeia — suficiente para um supermercado de médio porte.
Aumente `BLE_MESH_MAX_TTL` em `config.h` se precisar de mais, mas note
que cada salto extra aumenta o tráfego de rádio exponencialmente.

O parâmetro `BLE_MESH_ADV_DURATION_MS` (350ms) é o tempo em que a placa
**para de escanear** para repetir. Valores muito altos fazem a placa
perder pacotes; muito baixos reduzem a chance de a vizinha ouvir. 300–500ms
é a faixa útil.

---

## 8. Provisionando uma placa

Cada placa guarda **qual produto ela exibe**. A mesh só carrega
atualizações de preço — o nome, unidade e código vêm do provisionamento.

1. Segure o botão BOOT por 3 segundos. Aparece "MODO CONFIG" na tela.
2. A placa fica conectável por 60s com o nome `PlacaPreco-<id>`.
3. Conecte por um app BLE (nRF Connect, LightBlue) ou pelo seu sistema,
   e escreva na característica `6e400002-...` o JSON:

```json
{"id":1042,"nome":"Arroz Tipo 1 5kg","preco":2490,"unidade":"pacote","codigo":"7891234567890"}
```

Os dados são gravados no NVS e sobrevivem a reinicializações.

---

## 9. Testando

Você precisa de no mínimo **duas placas** para ver a repetição
funcionando, mais o gateway (que pode ser um terceiro ESP32 qualquer,
sem display).

1. Grave o `tools/gateway_example.cpp` em um ESP32.
2. Provisione duas placas com o **mesmo** `productId` (1042, por exemplo).
3. Afaste uma delas até ficar fora do alcance do gateway, mas dentro do
   alcance da primeira placa.
4. No monitor serial do gateway, digite: `1042 2490`
5. Ambas devem atualizar. A segunda só recebeu porque a primeira repetiu
   — o log dela mostra `[Mesh] Preço atualizado`.

Para conferir a repetição de mensagens que não são da placa, provisione
uma delas com outro `productId`: ela vai logar
`[Mesh] Repetido update do produto X (não é desta placa)`.

---

## 10. Limitações conhecidas

Coisas que este código **não** resolve e que você vai precisar tratar
antes de um piloto real em loja:

- **Sem segurança.** O checksum XOR detecta corrupção, não falsificação.
  Qualquer pessoa com um celular consegue anunciar um pacote e mudar os
  preços da loja inteira. Antes de produção, adicione um HMAC truncado
  com chave pré-compartilhada e um contador anti-replay.
- **Sem confirmação de entrega.** Flood não garante que todas as placas
  receberam. Implemente um heartbeat periódico (cada placa anuncia seu
  `productId` + preço atual) para o gateway auditar divergências.
- **Sem economia de energia.** Escanear BLE continuamente com o display
  ligado consome ~150mA. Uma ESL real usa e-paper e deep sleep. Nesta
  placa, assuma alimentação USB permanente.
- **O nome do produto usa um clipe único** (`produto_atual.pcm`), ou
  seja, precisa ser regravado a cada mudança de produto. Um fluxo melhor
  seria o gateway enviar o áudio do nome junto no provisionamento.
- **Sem OTA.** Atualizar o firmware exige cabo. Considere adicionar
  `ArduinoOTA` via Wi-Fi para uma frota de dezenas de placas.

---

## Licença

MIT — use à vontade.
