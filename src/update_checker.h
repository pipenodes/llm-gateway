#pragma once
#include <string>
#include <json/json.h>
#include "version.h"

struct UpdateChecker {
    /** Versão atual do core (ex: "0.0.4"). */
    static const char* core_version() { return HERMES_VERSION; }

    /**
     * Verifica atualizações consultando a URL do manifest (JSON).
     * Retorna um Json::Value pronto para resposta da API:
     *   - core: { current, latest?, available, release_notes?, download_url? }
     *   - plugins: [ { name, current, latest?, available, download_url? } ]
     *   - error?: string (se check_url vazio ou falha de rede/parse)
     * Se check_url estiver vazio, retorna error "update_check_disabled".
     */
    static Json::Value check(const std::string& check_url,
                             const Json::Value& current_plugins);

    /**
     * Baixa o binário de download_url e grava em staged_path (escrita atômica:
     * grava em staged_path + ".tmp" e renomeia). Retorna string vazia em
     * sucesso, ou mensagem de erro.
     */
    static std::string apply_download(const std::string& download_url,
                                      const std::string& staged_path);
};
