#!/usr/bin/env python3
"""
generate_audio.py - Gera os clipes de voz usados pela placa.

A placa NÃO sintetiza voz em tempo real (o ESP32 não tem recursos para
isso com qualidade aceitável). Em vez disso, ela concatena pequenos
clipes pré-gravados. Este script gera esses clipes UMA VEZ, no seu
computador, usando um TTS de verdade.

Saída: data/audio/<token>.pcm
       PCM bruto, 16 bits com sinal, mono, 16000 Hz, sem cabeçalho.
       (É o formato que AudioAnnouncer::playToken espera.)

Uso:
    pip install gtts pydub
    sudo apt install ffmpeg          # o pydub precisa do ffmpeg
    python tools/generate_audio.py

    # gerar também o nome de um produto específico:
    python tools/generate_audio.py --produto "Arroz Tipo 1 5 quilos"

Depois:
    pio run --target uploadfs        # grava a pasta data/ no LittleFS
"""

import argparse
import os
import sys

SAMPLE_RATE = 16000
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "data", "audio")

# Vocabulário fixo. Os nomes de arquivo (chaves) precisam bater exatamente
# com os tokens usados em AudioAnnouncer.cpp.
VOCAB = {
    "zero": "zero", "um": "um", "dois": "dois", "tres": "três",
    "quatro": "quatro", "cinco": "cinco", "seis": "seis", "sete": "sete",
    "oito": "oito", "nove": "nove", "dez": "dez", "onze": "onze",
    "doze": "doze", "treze": "treze", "catorze": "catorze", "quinze": "quinze",
    "dezesseis": "dezesseis", "dezessete": "dezessete", "dezoito": "dezoito",
    "dezenove": "dezenove",

    "vinte": "vinte", "trinta": "trinta", "quarenta": "quarenta",
    "cinquenta": "cinquenta", "sessenta": "sessenta", "setenta": "setenta",
    "oitenta": "oitenta", "noventa": "noventa",

    "cem": "cem", "cento": "cento", "duzentos": "duzentos",
    "trezentos": "trezentos", "quatrocentos": "quatrocentos",
    "quinhentos": "quinhentos", "seiscentos": "seiscentos",
    "setecentos": "setecentos", "oitocentos": "oitocentos",
    "novecentos": "novecentos",

    "mil": "mil", "e": "e",
    "real": "real", "reais": "reais",
    "centavo": "centavo", "centavos": "centavos",

    # Clipe genérico usado quando não há gravação do nome do produto
    "produto_atual": "produto",
}


def synth(text: str, out_path: str):
    """Sintetiza `text` e salva como PCM 16k mono s16le."""
    from gtts import gTTS
    from pydub import AudioSegment
    import io

    mp3_buf = io.BytesIO()
    gTTS(text=text, lang="pt", tld="com.br").write_to_fp(mp3_buf)
    mp3_buf.seek(0)

    seg = AudioSegment.from_file(mp3_buf, format="mp3")
    seg = seg.set_frame_rate(SAMPLE_RATE).set_channels(1).set_sample_width(2)

    # Normaliza o volume e corta silêncio nas pontas para a fala
    # concatenada não ficar com pausas longas entre as palavras.
    seg = seg.normalize()
    seg = seg.strip_silence(silence_len=80, silence_thresh=-45, padding=30)

    # O DAC do ESP32 é unsigned 8 bits internamente, mas o driver I2S em
    # modo DAC_BUILT_IN espera amostras de 16 bits com sinal e converte
    # sozinho. Então gravamos s16le puro.
    with open(out_path, "wb") as f:
        f.write(seg.raw_data)

    print(f"  ✓ {os.path.basename(out_path)}  ({len(seg.raw_data)} bytes, {len(seg)}ms)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--produto", help="Gera o clipe do nome de um produto (produto_atual.pcm)")
    ap.add_argument("--apenas-produto", action="store_true",
                    help="Não regerar o vocabulário de números")
    args = ap.parse_args()

    os.makedirs(OUT_DIR, exist_ok=True)

    try:
        import gtts, pydub  # noqa: F401
    except ImportError:
        print("Faltam dependências. Rode: pip install gtts pydub", file=sys.stderr)
        sys.exit(1)

    if not args.apenas_produto:
        print(f"Gerando vocabulário em {os.path.abspath(OUT_DIR)}:")
        for token, text in VOCAB.items():
            if token == "produto_atual" and args.produto:
                continue  # será gerado abaixo com o nome real
            synth(text, os.path.join(OUT_DIR, f"{token}.pcm"))

    if args.produto:
        print(f"Gerando nome do produto: {args.produto!r}")
        synth(args.produto, os.path.join(OUT_DIR, "produto_atual.pcm"))

    print("\nPronto. Agora rode:  pio run --target uploadfs")


if __name__ == "__main__":
    main()
