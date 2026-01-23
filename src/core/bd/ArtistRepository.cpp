#include "core/bd/ArtistRepository.hpp"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include "core/bd/AlbumRepository.hpp"
#include "core/bd/SQLiteRepositoryBase.hpp"
#include "core/bd/SongRepository.hpp"
#include "core/bd/UserRepository.hpp"
#include <memory>

/*
 * CREATE TABLE IF NOT EXISTS artists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    user_id INTEGER NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
*/

namespace core {

    ArtistRepository::ArtistRepository(std::shared_ptr<SQLite::Database> db)
        : core::SQLiteRepositoryBase<Artist>(db, "artists") {};

    bool ArtistRepository::insert(Artist& entity) {
        if (!entity.getUser())
            throw std::invalid_argument(
                "Artist must be associated with a User.");
        std::string sql = "INSERT INTO " + _table_name + " (name, user_id) "
                          + "VALUES(?, ?);";
        SQLite::Statement query = prepare(sql);
        query.bind(1, entity.getName());
        query.bind(2, entity.getUser()->getId());

        bool success = query.exec() > 0;

        if (success)
            entity.setId(static_cast<unsigned>(getLastInsertId()));

        return success;
    }

    bool ArtistRepository::update(const Artist& entity) {
        std::string sql =
            "UPDATE " + _table_name + " SET name = ?, user_id = ? WHERE id = ?";
        SQLite::Statement query = prepare(sql);

        query.bind(1, entity.getName());
        query.bind(2, entity.getUser()->getId());
        query.bind(3, entity.getId());

        return query.exec() > 0;
    };

    std::shared_ptr<Artist>
    ArtistRepository::mapRowToEntity(SQLite::Statement& query) const {
        unsigned id = query.getColumn("id").getInt();
        std::string name = query.getColumn("name").getString();
        unsigned user_id = query.getColumn("user_id").getInt();

        auto user_repo = UserRepository(_db);
        auto user_ptr = user_repo.findById(user_id);

        if (!user_ptr) {
            throw std::runtime_error("Usuário não encontrado para artista id "
                                     + std::to_string(id));
        }

        auto artist = std::make_shared<Artist>(id, name, *user_ptr);

        auto songs_loader = [this, id]() -> std::vector<std::shared_ptr<Song>> {
            auto artist_ptr = this->findById(id);
            if (artist_ptr) {
                return this->getSongs(*artist_ptr);
            }
            return std::vector<std::shared_ptr<Song>>();
        };

        auto albums_loader = [this,
                              id]() -> std::vector<std::shared_ptr<Album>> {
            auto artist_ptr = this->findById(id);
            if (artist_ptr) {
                return this->getAlbums(*artist_ptr);
            }
            return std::vector<std::shared_ptr<Album>>();
        };

        artist->setSongsLoader(songs_loader);
        artist->setAlbumsLoader(albums_loader);

        return artist;
    }

    bool ArtistRepository::save(Artist& entity) {
        if (entity.getId() == 0) {
            return insert(entity);
        } else {
            return update(entity);
        }
    };

    bool ArtistRepository::remove(unsigned id) {
        std::string sql = "DELETE FROM " + _table_name + " WHERE id = ?;";

        SQLite::Statement query = prepare(sql);
        query.bind(1, id);

        return query.exec() > 0;
    }

    std::vector<std::shared_ptr<Artist>>
    ArtistRepository::findByNameAndUser(const std::string& name,
                                        const User& user) const {
        std::string sql = "SELECT * FROM " + _table_name
                          + " WHERE name LIKE ? AND user_id = ?;";

        SQLite::Statement query = prepare(sql);
        query.bind(1, "%" + name + "%");
        query.bind(2, user.getId());

        std::vector<std::shared_ptr<Artist>> artists;
        while (query.executeStep()) {
            artists.push_back(mapRowToEntity(query));
        }

        return artists;
    }

    std::vector<std::shared_ptr<Artist>>
    ArtistRepository::findByName(const std::string& name) const {
        std::string sql = "SELECT * FROM " + _table_name + " WHERE name = ?;";
        SQLite::Statement query = prepare(sql);
        query.bind(1, name);
        std::vector<std::shared_ptr<Artist>> artists;
        while (query.executeStep()) {
            artists.push_back(mapRowToEntity(query));
        }

        return artists;
    };

    std::vector<std::shared_ptr<Album>>
    ArtistRepository::getAlbums(const Artist& artist) const {
        AlbumRepository album_repository(_db);
        return album_repository.findByArtist(artist.getName());
    };

    std::vector<std::shared_ptr<Song>>
    ArtistRepository::getSongs(const Artist& artist) const {
        SongRepository song_repository(_db);
        return song_repository.findByArtist(artist);
    };

}; // namespace core
