#pragma once
#include <lvgl.h>
#include <functional>
#include "../storage/ProductStore.h"

using AudioButtonCallback = std::function<void()>;

// Inicializa os widgets da tela (chamar após lv_init() e registro do display).
void ui_init();

// Atualiza os textos exibidos com os dados atuais do produto.
void ui_update_product(const ProductInfo &info);

// Define o que acontece ao tocar no botão de acessibilidade/áudio.
void ui_set_audio_button_callback(AudioButtonCallback cb);

// Mostra um pequeno indicador (canto da tela) de atividade da mesh,
// útil para depuração em loja (pisca quando repete uma mensagem).
void ui_pulse_mesh_indicator();

// Mostra/esconde um selo de "modo provisionamento" ativo.
void ui_set_provisioning_indicator(bool active);
