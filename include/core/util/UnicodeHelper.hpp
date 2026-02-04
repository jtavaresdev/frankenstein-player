/**
 * @brief Helper para Unicode seguindo princípio "UTF-8 Everywhere"
 *
 * - Internamente: SEMPRE std::string com UTF-8
 * - Windows APIs: Converte para wstring APENAS quando necessário
 * - Filesystem: Use toPath() para garantir compatibilidade
 * QUANDO USAR:
 * - toWide()/fromWide(): Apenas para chamar APIs do Windows (CreateFileW, etc.)
 * - toPath(): Para todas as operações de filesystem
 * - sanitizeFilename(): Para limpar nomes antes de criar arquivos
 * - NÃO use toUtf8(string) - seu string JÁ É UTF-8!
 */

#pragma once

#include <string>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace core {

    class UnicodeHelper {
    private:
#ifdef _WIN32
        static bool s_consoleInitialized;
#endif

    public:
        using string_type = std::string;

#ifdef _WIN32
        /**
         * @brief Converte UTF-8 para UTF-16 (wstring do Windows)
         * @param utf8 String em UTF-8
         * @return wstring (UTF-16) para APIs do Windows
         */
        static std::wstring toWide(const std::string& utf8);

        /**
         * @brief Converte UTF-16 (wstring) para UTF-8
         * @param wide wstring do Windows
         * @return String em UTF-8
         */
        static std::string fromWide(const std::wstring& wide);

        /**
         * @brief Converte string UTF-8 para std::filesystem::path
         * @param utf8Path Caminho em UTF-8
         * @return path configurado para Windows (UTF-16 interno)
         */
        static std::filesystem::path toPath(const std::string& utf8Path);
#else
        /**
         * @brief Converte string UTF-8 para std::filesystem::path
         * No Linux/Mac, path aceita UTF-8 diretamente
         * @param utf8Path Caminho em UTF-8
         * @return path com UTF-8
         */
        static inline std::filesystem::path
        toPath(const std::string& utf8Path) {
            return std::filesystem::path(utf8Path);
        }
#endif

        /**
         * @brief Valida se string é UTF-8 bem formado
         * @param str String a validar
         * @return true se for UTF-8 válido
         */
        static bool isValidUtf8(const std::string& str);

        /**
         * @brief Remove caracteres inválidos para nomes de arquivo
         * Remove/substitui: < > : " / \ | ? * e caracteres de controle (0-31)
         * @param name Nome a sanitizar
         * @return Nome seguro para usar como arquivo
         */
        static std::string sanitizeFilename(const std::string& name);

        /**
         * @brief Remove espaços no início e fim
         * @param str String a processar
         * @return String sem espaços nas extremidades
         */
        static std::string trim(const std::string& str);

        /**
         * @brief Normaliza: trim + colapsa múltiplos espaços em um
         * @param str String a normalizar
         * @return String normalizada
         */
        static std::string normalize(const std::string& str);

        /**
         * @brief Comparação case-insensitive (APENAS para ASCII)
         * @param a Primeira string
         * @param b Segunda string
         * @return true se forem iguais (ignorando case ASCII)
         */
        static bool equalsIgnoreCaseAscii(const std::string& a,
                                          const std::string& b);

        /**
         * @brief Verifica se string contém substring
         * @param str String onde buscar
         * @param substring Substring a procurar
         * @return true se contém
         */
        static bool contains(const std::string& str,
                             const std::string& substring);

        /**
         * @brief Configura console para UTF-8 (Windows) ou não faz nada
         * (Linux/Mac) Chame UMA VEZ no início do programa
         */
        static void setupConsole();
    };

} // namespace core
