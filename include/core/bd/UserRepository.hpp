/**
 * @file UserRepository.hpp
 * @brief Repositorio de usuarios
 * @ingroup bd
 *
 * Repositorio para gerenciar operacoes de CRUD para a entidade User.
 *
 * @author Eloy Maciel
 * @date 2025-10-09
 */

#ifndef USERREPOSITORY_HPP
#define USERREPOSITORY_HPP

#include <vector>

#include "core/bd/IRepository.hpp"
#include "core/entities/EntitiesFWD.hpp"

namespace core {

    /**
     * @brief Repositorio de usuarios
     * Repositorio para gerenciar operacoes de CRUD para a entidade User.
     */
    class UserRepository : public IRepository<User> {
    protected:
        bool insert(const User& entity) override;
        bool update(const User& entity) override;

    public:
        UserRepository(std::shared_ptr<SQLite::Database> db);
        ~UserRepository() override = default;

        /**
         * @brief Salva ou atualiza um usuario no repositório
         * @copydoc IRepository::save
         * @param entity Usuario a ser salvo ou atualizado
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
        bool save(const User& entity) override;

        /**
         * @brief Remove um usuario do repositório pelo ID
         * @copydoc IRepository::remove
         * @param id ID do usuario a ser removido
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
        bool remove(unsigned id) override;

        /**
         * @brief Obtém todos os usuarios do repositório
         * @copydoc IRepository::getAll
         * @return Vetor contendo todos os usuarios
         */
        std::vector<std::shared_ptr<User>> getAll() const override;

        /**
         * @brief Busca usuarios pelo nome de usuário
         * @param username Nome de usuário a ser buscado
         * @return Vetor contendo os usuarios que correspondem ao nome de usuário fornecido
         */
        std::vector<std::shared_ptr<User>> findByUsername(const std::string& username) const;

        /**
         * @brief Busca um usuario pelo ID
         * @copydoc IRepository::findById
         * @param id ID do usuario a ser buscado
         * @return Ponteiro compartilhado para o usuario encontrado, ou nullptr se não encontrado
         */
        std::shared_ptr<User> findById(unsigned id) const override;
    };

}


#endif
