#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>

#include "core/bd/DatabaseManager.hpp"
#include "core/bd/SongRepository.hpp"
#include "core/bd/UserRepository.hpp"
#include "core/bd/ArtistRepository.hpp"
#include "core/entities/Artist.hpp"
#include "core/entities/Song.hpp"
#include "core/entities/User.hpp"
#include "fixtures/ConfigFixture.hpp"

TEST_SUITE("Unit Tests - core::SongRepository") {
    
    // Fixture para compartilhar o banco de dados entre os testes
    struct SongRepositoryFixture {
        ConfigFixture config;
        std::unique_ptr<core::DatabaseManager> db_manager;
        std::shared_ptr<SQLite::Database> db;
        
        SongRepositoryFixture() {
            std::string db_path = config.databasePath();
            std::string schema_path = config.databaseSchemaPath();
            
            db_manager = std::make_unique<core::DatabaseManager>(db_path, schema_path);
            db = db_manager->getDatabase();
        }
        
        // Helper para criar usuário e artista necessários
        void setupUserAndArtist(core::User& user, core::Artist& artist) {
            // Inserir usuário no banco
            core::UserRepository user_repo(db);
            user.setId(0); // Força inserção
            user_repo.save(user);
            
            // Inserir artista no banco
            core::ArtistRepository artist_repo(db);
            artist.setUser(user);
            artist.setId(0); // Força inserção
            artist_repo.save(artist);
        }
    };
    
    TEST_CASE_FIXTURE(SongRepositoryFixture, "SongRepository: Inserir e remover") {
        core::SongRepository repo(db);
        
        core::User user;
        user.setUsername("test_user");
        
        core::Artist artist(0, "The Void", user);
        
        setupUserAndArtist(user, artist);
        
        core::Song song1(0, "Música teste 1", artist.getId());
        core::Song song2(0, "Minha Música", artist.getId());
        
        song1.setDuration(100);
        song1.setUser(user);
        song2.setDuration(100);
        song2.setUser(user);

        CHECK(repo.count() == 0);
        CHECK(repo.save(song1) == true);
        CHECK(repo.count() == 1);
        
        CHECK(repo.save(song2) == true);
        CHECK(repo.count() == 2);
        
        CHECK(repo.remove(9999) == false);
        CHECK(repo.count() == 2);
        
        CHECK(repo.remove(song2.getId()) == true);
        CHECK(repo.count() == 1);
    }
    
    TEST_CASE_FIXTURE(SongRepositoryFixture, "SongRepository: Buscar") {
        core::SongRepository repo(db);
        
        core::User user;
        user.setUsername("test_user");
        
        core::Artist artist1(0, "Artist 1", user);
        core::Artist artist2(0, "Artist 2", user);
        
        setupUserAndArtist(user, artist1);
        
        core::ArtistRepository artist_repo(db);
        artist2.setUser(user);
        artist2.setId(0);
        artist_repo.save(artist2);
        
        core::Song song1(0, "Música teste 1", artist1.getId());
        core::Song song2(0, "Minha Música", artist1.getId());
        core::Song song3(0, "Música teste 2", artist2.getId());
        
        song1.setDuration(100);
        song1.setUser(user);

        song2.setDuration(200);
        song2.setUser(user);

        song3.setDuration(300);
        song3.setUser(user);

        CHECK(repo.count() == 0);
        CHECK(repo.save(song1) == true);
        CHECK(repo.save(song2) == true);
        
        std::shared_ptr<core::Song> ptr = repo.findById(song2.getId());
        CHECK(ptr != nullptr);
        CHECK(ptr->getTitle() == "Minha Música");
        CHECK(ptr->getArtistId() == artist1.getId());
        CHECK(ptr->getDuration() == 200);
        
        CHECK(repo.findById(9999) == nullptr);
        
        CHECK(repo.save(song3) == true);
        std::shared_ptr<core::Song> ptr3 = repo.findById(song3.getId());
        CHECK(ptr3 != nullptr);
        CHECK(ptr3->getTitle() == "Música teste 2");
        CHECK(ptr3->getArtistId() == artist2.getId());
    }
    
    TEST_CASE_FIXTURE(SongRepositoryFixture, "SongRepository: Get All") {
        core::SongRepository repo(db);
        
        core::User user;
        user.setUsername("test_user");
        
        core::Artist artist1(0, "Artist 1", user);
        core::Artist artist2(0, "Artist 2", user);
        
        setupUserAndArtist(user, artist1);
        
        core::ArtistRepository artist_repo(db);
        artist2.setUser(user);
        artist2.setId(0);
        artist_repo.save(artist2);
        
        core::Song song1(0, "Música teste 1", artist1.getId());
        core::Song song2(0, "Minha Música", artist1.getId());
        core::Song song3(0, "Música teste 2", artist2.getId());
        
        song1.setDuration(100);
        song1.setUser(user);
        
        song2.setDuration(200);
        song2.setUser(user);

        song3.setDuration(300);
        song3.setUser(user);
        
        CHECK(repo.save(song1) == true);
        CHECK(repo.save(song2) == true);
        
        std::vector<std::shared_ptr<core::Song>> vector = repo.getAll();
        CHECK(vector.size() == 2);
        
        bool found_song1 = false;
        bool found_song2 = false;
        for (const auto& song : vector) {
            if (song->getTitle() == "Música teste 1") found_song1 = true;
            if (song->getTitle() == "Minha Música") found_song2 = true;
        }
        CHECK(found_song1);
        CHECK(found_song2);
        
        CHECK(repo.save(song3) == true);
        vector = repo.getAll();
        CHECK(vector.size() == 3);
        
        bool found_song3 = false;
        for (const auto& song : vector) {
            if (song->getTitle() == "Música teste 2") found_song3 = true;
        }
        CHECK(found_song3);
    }
}
