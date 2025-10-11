/**
 * @file IRepository.hpp
 * @brief Interface para repositório de dados
 * @ingroup bd
 *
 * Interface que define operações essenciais para um repositório de dados, incluindo métodos para salvar, remover, atualizar e buscar entidades.
 *
 * @author Eloy Maciel
 * @date 2025-10-09
 */

#ifndef IREPOSITORY_HPP
#define IREPOSITORY_HPP

#include <vector>
#include <string>
#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>


namespace core {

    /**
     * @brief Interface para repositório de dados
     * Interface que define operações essenciais para um repositório de dados, incluindo métodos para salvar, remover, atualizar e buscar entidades.
     */
    template <typename T>
    class IRepository {
    protected:
        std::shared_ptr<SQLite::Database> db;
        std::string table_name;

        /**
         * @brief Obtém uma declaração preparada para a consulta SQL fornecida
         * @param sql Consulta SQL a ser preparada
         * @return Declaração preparada
         */
        SQLite::Statement prepare(const std::string& sql) const;

        /**
         * @brief Verifica se uma entidade existe no repositório pelo ID
         * @param id ID da entidade a ser verificada
         * @return true se a entidade existe, false caso contrário
         */
        bool exists(unsigned id) const;

        /**
         * @brief Obtém o ID da última inserção no banco de dados
         * @return ID da última inserção
         */
        unsigned getLastInsertId() const;

        /**
         * @brief Inserte uma entidade no repositório
         * @param entity Entidade a ser inserida
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
         virtual bool insert(const T& entity) = 0;

        /**
         * @brief Atualiza uma entidade no repositório
         * @param entity Entidade com os dados atualizados
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
        virtual bool update(const T& entity) = 0;
    public:
        IRepository(std::shared_ptr<SQLite::Database> db, const std::string& table_name);
        virtual ~IRepository() = default;

        const std::string& getTableName() const;

        /**
         * @brief Salva uma entidade no repositório
         * @param entity Entidade a ser salva
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
        virtual bool save(const T& entity) = 0;

        /**
         * @brief Remove uma entidade do repositório pelo ID
         * @param id ID da entidade a ser removida
         * @return true se a operação foi bem-sucedida, false caso contrário
         */
        virtual bool remove(unsigned id) = 0;


        /**
         * @brief Busca uma entidade pelo ID
         * @param id ID da entidade a ser buscada
         * @return Ponteiro para a entidade encontrada, ou nullptr se não encontrada
         */
        virtual std::shared_ptr<T> findById(unsigned id) const = 0;

        /**
         * @brief Obtem todas as entidades no repositório
         * @return Vetor contendo todas as entidades
         */
        virtual std::vector<std::shared_ptr<T>> getAll() const = 0;

        /**
         * @brief Busca entidades por um campo específico
         * @param field String com o campo a ser buscado
         * @param value Valor do campo a ser buscado
         * @return Vetor contendo as entidades que correspondem ao critério de busca
         */
         virtual std::vector<std::shared_ptr<T>> findBy(const std::string& field, const std::string& value) const;
    };

}


#endif
