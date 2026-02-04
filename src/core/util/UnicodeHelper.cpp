#include "core/util/UnicodeHelper.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace core {

#ifdef _WIN32
    bool UnicodeHelper::s_consoleInitialized = false;

    std::wstring UnicodeHelper::toWide(const std::string& utf8) {
        if (utf8.empty()) {
            return L"";
        }

        int size = MultiByteToWideChar(CP_UTF8,
                                       0,
                                       utf8.c_str(),
                                       static_cast<int>(utf8.size()),
                                       nullptr,
                                       0);
        if (size <= 0) {
            throw std::runtime_error(
                "Falha ao converter UTF-8 para UTF-16: tamanho inválido");
        }

        std::wstring result(size, 0);
        int converted = MultiByteToWideChar(CP_UTF8,
                                            0,
                                            utf8.c_str(),
                                            static_cast<int>(utf8.size()),
                                            &result[0],
                                            size);
        if (converted <= 0) {
            throw std::runtime_error("Falha ao converter UTF-8 para UTF-16");
        }

        return result;
    }

    std::string UnicodeHelper::fromWide(const std::wstring& wide) {
        if (wide.empty()) {
            return "";
        }

        int size = WideCharToMultiByte(CP_UTF8,
                                       0,
                                       wide.c_str(),
                                       static_cast<int>(wide.size()),
                                       nullptr,
                                       0,
                                       nullptr,
                                       nullptr);
        if (size <= 0) {
            throw std::runtime_error(
                "Falha ao converter UTF-16 para UTF-8: tamanho inválido");
        }

        std::string result(size, 0);
        int converted = WideCharToMultiByte(CP_UTF8,
                                            0,
                                            wide.c_str(),
                                            static_cast<int>(wide.size()),
                                            &result[0],
                                            size,
                                            nullptr,
                                            nullptr);
        if (converted <= 0) {
            throw std::runtime_error("Falha ao converter UTF-16 para UTF-8");
        }

        return result;
    }

    std::filesystem::path UnicodeHelper::toPath(const std::string& utf8Path) {
        return std::filesystem::path(toWide(utf8Path));
    }

    void UnicodeHelper::setupConsole() {
        if (!s_consoleInitialized) {
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);

            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD mode = 0;
                if (GetConsoleMode(hOut, &mode)) {
                    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(hOut, mode);
                }
            }

            s_consoleInitialized = true;
        }
    }

#else // Linux/Mac

    void UnicodeHelper::setupConsole() {
        // Não precisa fazer nada no Linux/Mac
        // Terminais modernos já suportam UTF-8 por padrão
    }

#endif


    bool UnicodeHelper::isValidUtf8(const std::string& str) {
        const unsigned char* bytes =
            reinterpret_cast<const unsigned char*>(str.data());
        size_t i = 0;

        while (i < str.length()) {
            if (bytes[i] <= 0x7F) {
                // ASCII de 1 byte (0xxxxxxx)
                i++;
            } else if ((bytes[i] & 0xE0) == 0xC0) {
                // 2 bytes (110xxxxx 10xxxxxx)
                if (i + 1 >= str.length() || (bytes[i + 1] & 0xC0) != 0x80) {
                    return false;
                }
                // Verifica overlong encoding
                if ((bytes[i] & 0xFE) == 0xC0) {
                    return false;
                }
                i += 2;
            } else if ((bytes[i] & 0xF0) == 0xE0) {
                // 3 bytes (1110xxxx 10xxxxxx 10xxxxxx)
                if (i + 2 >= str.length() || (bytes[i + 1] & 0xC0) != 0x80
                    || (bytes[i + 2] & 0xC0) != 0x80) {
                    return false;
                }
                // Verifica overlong encoding
                if (bytes[i] == 0xE0 && (bytes[i + 1] & 0xE0) == 0x80) {
                    return false;
                }
                // Verifica surrogates UTF-16 (inválidos em UTF-8)
                if (bytes[i] == 0xED && (bytes[i + 1] & 0xE0) == 0xA0) {
                    return false;
                }
                i += 3;
            } else if ((bytes[i] & 0xF8) == 0xF0) {
                // 4 bytes (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
                if (i + 3 >= str.length() || (bytes[i + 1] & 0xC0) != 0x80
                    || (bytes[i + 2] & 0xC0) != 0x80
                    || (bytes[i + 3] & 0xC0) != 0x80) {
                    return false;
                }
                // Verifica overlong encoding
                if (bytes[i] == 0xF0 && (bytes[i + 1] & 0xF0) == 0x80) {
                    return false;
                }
                // Verifica se está fora do range Unicode válido (> U+10FFFF)
                if (bytes[i] >= 0xF5) {
                    return false;
                }
                i += 4;
            } else {
                // Byte inválido
                return false;
            }
        }

        return true;
    }

    std::string UnicodeHelper::sanitizeFilename(const std::string& name) {
        // Caracteres inválidos em Windows: < > : " / \ | ? *
        // Caracteres de controle: 0-31
        const std::string invalidChars = "<>:\"/\\|?*";
        std::string result;
        result.reserve(name.length());

        for (unsigned char c : name) {
            if (c < 32) {
                // Caractere de controle
                result += '_';
            } else if (invalidChars.find(c) != std::string::npos) {
                // Caractere inválido para filesystem
                result += '_';
            } else {
                result += c;
            }
        }

        // Remove espaços no final (problemático em Windows)
        while (!result.empty() && result.back() == ' ') {
            result.pop_back();
        }

        // Remove pontos no final (problemático em Windows)
        while (!result.empty() && result.back() == '.') {
            result.pop_back();
        }

        // Se ficou vazio, retorna nome padrão
        if (result.empty()) {
            return "unnamed";
        }

        return result;
    }

    std::string UnicodeHelper::trim(const std::string& str) {
        auto isSpace = [](unsigned char c) {
            return std::isspace(c);
        };

        auto begin = std::find_if_not(str.begin(), str.end(), isSpace);
        auto end = std::find_if_not(str.rbegin(), str.rend(), isSpace).base();

        if (begin >= end) {
            return "";
        }

        return std::string(begin, end);
    }

    std::string UnicodeHelper::normalize(const std::string& str) {
        std::string trimmed = trim(str);
        if (trimmed.empty()) {
            return "";
        }

        std::string result;
        result.reserve(trimmed.length());

        bool lastWasSpace = false;
        for (unsigned char c : trimmed) {
            if (std::isspace(c)) {
                if (!lastWasSpace) {
                    result += ' ';
                    lastWasSpace = true;
                }
            } else {
                result += c;
                lastWasSpace = false;
            }
        }

        return result;
    }

    bool UnicodeHelper::equalsIgnoreCaseAscii(const std::string& a,
                                              const std::string& b) {
        if (a.length() != b.length()) {
            return false;
        }

        return std::equal(a.begin(), a.end(), b.begin(), [](char ca, char cb) {
            return std::tolower(static_cast<unsigned char>(ca))
                   == std::tolower(static_cast<unsigned char>(cb));
        });
    }

    bool UnicodeHelper::contains(const std::string& str,
                                 const std::string& substring) {
        return str.find(substring) != std::string::npos;
    }

} // namespace core
