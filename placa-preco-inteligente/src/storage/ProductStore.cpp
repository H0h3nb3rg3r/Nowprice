#include "ProductStore.h"
#include <Preferences.h>
#include "../../include/config.h"

static Preferences prefs;

void ProductStore::begin(uint16_t defaultTagId) {
    prefs.begin(PREFS_NAMESPACE, false);

    _tagId = prefs.getUShort("tagId", 0);
    if (_tagId == 0) {
        if (defaultTagId != 0) {
            _tagId = defaultTagId;
        } else {
            // Deriva um ID curto e estável a partir do MAC do chip.
            uint64_t chipId = ESP.getEfuseMac();
            _tagId = (uint16_t)(chipId & 0xFFFF);
            if (_tagId == 0) _tagId = 1; // evita 0 (reservado)
        }
        saveTagId();
    }

    load();
    prefs.end();
}

void ProductStore::load() {
    prefs.begin(PREFS_NAMESPACE, true);
    _info.productId  = prefs.getUShort("prodId", 0);
    _info.priceCents = prefs.getUInt("price", 0);
    _info.promo      = prefs.getBool("promo", false);

    String n = prefs.getString("name", "Sem produto");
    String u = prefs.getString("unit", "un");
    String c = prefs.getString("code", "");
    n.toCharArray(_info.name, sizeof(_info.name));
    u.toCharArray(_info.unit, sizeof(_info.unit));
    c.toCharArray(_info.code, sizeof(_info.code));
    prefs.end();
}

void ProductStore::save() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUShort("prodId", _info.productId);
    prefs.putUInt("price", _info.priceCents);
    prefs.putBool("promo", _info.promo);
    prefs.putString("name", _info.name);
    prefs.putString("unit", _info.unit);
    prefs.putString("code", _info.code);
    prefs.end();
}

void ProductStore::saveTagId() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUShort("tagId", _tagId);
    prefs.end();
}

void ProductStore::setFull(const ProductInfo &info) {
    _info = info;
    save();
}

bool ProductStore::applyPriceUpdate(uint16_t productId, uint32_t priceCents, bool promo) {
    if (!matchesProduct(productId)) return false;
    if (_info.priceCents == priceCents && _info.promo == promo) return false; // nada mudou

    _info.priceCents = priceCents;
    _info.promo = promo;
    save();
    return true;
}
