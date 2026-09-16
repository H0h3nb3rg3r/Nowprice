#pragma once
#include <Arduino.h>

// Registro completo do produto exibido nesta placa.
// É gravado via provisionamento GATT (uma vez, ou quando o produto muda de
// posição na loja) e persistido em NVS. A mesh só atualiza `priceCents`/`promo`.
struct ProductInfo {
    uint16_t productId   = 0;
    char     name[24]    = "Sem produto";
    char     unit[8]     = "un";
    char     code[16]    = "";
    uint32_t priceCents  = 0;
    bool     promo       = false;
};

class ProductStore {
public:
    // Inicializa o NVS e carrega dados salvos. Se não houver tagId salvo,
    // gera um novo a partir do chip ID (ou usa defaultTagId, se != 0).
    void begin(uint16_t defaultTagId = 0);

    uint16_t tagId() const { return _tagId; }

    ProductInfo get() const { return _info; }

    // Provisionamento completo (via característica GATT), grava tudo.
    void setFull(const ProductInfo &info);

    // Atualização recebida pela mesh: só mexe em preço/promo, e só se o
    // productId bater com o produto atualmente atribuído a esta placa.
    bool applyPriceUpdate(uint16_t productId, uint32_t priceCents, bool promo);

    bool matchesProduct(uint16_t productId) const { return _info.productId == productId; }

private:
    uint16_t    _tagId = 0;
    ProductInfo _info;

    void load();
    void save();
    void saveTagId();
};
