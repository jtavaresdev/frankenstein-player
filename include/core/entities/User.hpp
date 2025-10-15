/**
 * @file User.hpp
 * @author Bruno Vieira
 * @brief entidade  para usuários do sistema
 * @version 0.1
 * @date 2025-10-15
 *
 */

#pragma once
#include <string>
#include <memory>
#include <vector>
#include "../entities/Entity.hpp"
#include "../entities/Playlist.hpp"
namespace core
{
    class User : public core::Entity
    {
    private:
        std::string _username;
        std::string file_path;
        std::vector<std::shared_ptr<core::Playlist>> _playlists;
        std::unique_ptr<core::Playlist> _likedSongs;

    public:
        /**
         * @brief Constroi um novo usuário
         *
         * @param _username Nome do usuário
         */
        User(const std::string &_username);

        /**
         * @brief Destrutor de um usuário
         *
         */
        ~User();

        /**
         * @brief Adiciona uma playlist do usuário
         *
         * @param playlist PLaylist a ser adicionada
         */
        void addPlaylist(const std::shared_ptr<core::Playlist> &playlist);

        /**
         * @brief Remove uma playlist do usuário
         *
         * @param playlist_id ID da playlist a ser removida
         */
        void removePlaylist(const unsigned playlist_id);

        /**
         * @brief Obtém as playlists do usuário
         *
         * @return Vector com as playlists do usuário
         */
        std::vector<std::shared_ptr<core::Playlist>> getPlaylists() const;

        /**
         * @brief Obtém a playlist de músicas curtidas do usuário
         *
         * @return Playlist de músicas curtidas do usuário
         */
        std::shared_ptr<core::Playlist> getLiked() const;

        /**
         * @brief Obtém o nome do usuário
         *
         * @return String com o nome do usuário
         */
        std::string getUsername() const;

        /**
         * @brief Seta o nome do usuário
         *
         * @param username Novo nome do usuário
         */
        void setUsername(const std::string &username);
    };

}