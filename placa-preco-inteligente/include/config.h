#pragma once
#include <Arduino.h>

// ============================================================
// Identificação
// ============================================================
#define DEVICE_NAME_PREFIX   "PlacaPreco"   // vira "PlacaPreco-<tagId>"

// ============================================================
// Áudio (acessibilidade)
// ============================================================
// Saída via DAC interno do ESP32 (GPIO25 = canal direito do DAC).
// Ligue aqui um amplificador de áudio pequeno (ex.: PAM8403) + alto-falante,
// ou um fone/mini speaker amplificado.
#define AUDIO_DAC_PIN         25
#define AUDIO_SAMPLE_RATE     16000   // deve bater com os PCM gerados em tools/generate_audio.py

// Buzzer passivo (fallback simples de "beep" quando não há clipe de áudio)
#define BUZZER_PIN            26

// Botão físico opcional de acessibilidade (além do botão na tela).
// Na CYD normalmente só existe o botão BOOT (GPIO0).
#define ACCESS_BUTTON_PIN     0

// ============================================================
// Rede Mesh (BLE flood com TTL)
// ============================================================
#define BLE_MESH_MAX_TTL           5      // nº máximo de saltos (repetições)
#define BLE_MESH_DEDUP_CACHE       32     // quantas msgIds recentes guardamos
#define BLE_MESH_ADV_DURATION_MS   350    // por quanto tempo anuncia cada repetição
#define BLE_MESH_SCAN_WINDOW_MS    60
#define BLE_MESH_SCAN_INTERVAL_MS  100
#define BLE_MANUFACTURER_ID   0xFFFF      // ID de teste/privado (trocar em produção)

// ============================================================
// Armazenamento (NVS / Preferences)
// ============================================================
#define PREFS_NAMESPACE "placapreco"
