/**
 * @file ArtistRepository.hpp
 * @brief Repositorio de artistas
 * @ingroup bd
 *
 * Repositorio para gerenciar operacoes de CRUD para a entidade Artist.
 *
 * @author Eloy Maciel
 * @date 2025-10-09
 */

#ifndef ARTISTREPOSITORY_HPP
#define ARTISTREPOSITORY_HPP

#include <vector>
#include <string>
#include <memory>

#include "core/bd/IRepository.hpp"
#include "core/entities/EntitiesFWD.hpp"

namespace core {

    /**
     * @brief Repositorio de artistas
     * Repositorio para gerenciar operacoes de CRUD para a entidade Artist.
     */
    class ArtistRepository : public IRepository<Artist> {
    protected:
        bool insert(const Artist& entity) override;
        bool update(const Artist& entity) override;

    public:
        ArtistRepository(std::shared_ptr<SQLite::Database> db);
        ~ArtistRepository() override = default;

        /**
         * @brief Salva ou atualiza um artista no repositório
         * @copydoc IRepository::save
         * @param entity Artista a ser salvo ou atualizado
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
        bool save(const Artist& entity) override;

        /**
         * @brief Remove um artista do repositório pelo ID
         * @copydoc IRepository::remove
         * @param id ID do artista a ser removido
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
        bool remove(unsigned id) override;

        /**
         * @brief Obtém todos os artistas do repositório
         * @return Vetor contendo todos os artistas
         */
        std::vector<std::shared_ptr<Artist>> getAll() const override;

        /**
         * @brief Busca artistas pelo nome e usuário
         * @param name Nome do artista a ser buscado
         * @param user Usuário cujos artistas serão buscados
         * @return Vetor contendo os artistas que correspondem ao nome fornecido
         */
        std::vector<std::shared_ptr<Artist>> findByNameAndUser(const std::string& name, const User& user) const;

        /**
         * @brief Busca artistas pelo nome
         * @param name Nome do artista a ser buscado
         * @return Vetor contendo os artistas que correspondem ao nome fornecido
         */
        std::vector<std::shared_ptr<Artist>> findByName(const std::string& name) const;

        /**
         * @brief Busca artistas por ID
         * @param id ID do artista a ser buscado
         * @return Ponteiro para o artista encontrado, ou nullptr se não encontrado
         */
        std::shared_ptr<Artist> findById(unsigned id) const override;
    };

}

#endif // ARTISTREPOSITORY_HPP
