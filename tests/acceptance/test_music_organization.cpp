#include <filesystem>
#include <filesystem>
#include <doctest/doctest.h>
#include <memory>
#include <string>

#include "SQLiteCpp/Database.h"
#include "core/bd/AlbumRepository.hpp"
#include "core/bd/ArtistRepository.hpp"
#include "core/bd/RepositoryFactory.hpp"
#include "core/bd/SongRepository.hpp"
#include "core/bd/UserRepository.hpp"
#include "core/entities/User.hpp"
#include "core/services/FilesManager.hpp"

#include "fixtures/ConfigFixture.hpp"
#include "fixtures/DatabaseFixture.hpp"
#include "fixtures/MediaFixture.hpp"

namespace fs = std::filesystem;

#ifdef _WIN32
std::string uid1001 = "1001";
std::string uid1002 = "1002";
std::string uid1003 = "1003";
#else
uid_t uid1001 = 1000;
uid_t uid1002 = 1002;
uid_t uid1003 = 1003;
#endif

TEST_SUITE("HISTÓRIA DE USUÁRIO: Organização de Músicas") {
    ConfigFixture config = ConfigFixture();
    MediaFixture media = MediaFixture();
    std::shared_ptr<SQLite::Database> db;

    std::shared_ptr<core::SongRepository> song_repo;
    std::shared_ptr<core::ArtistRepository> artist_repo;
    std::shared_ptr<core::AlbumRepository> album_repo;
    std::shared_ptr<core::UserRepository> user_repo;

    class DirectoryFixture {
    public:
        DirectoryFixture() {
            // Remove all contents of tests/fixtures/data using boost
            // fs::path data_dir("../tests/fixtures/data");
            // if (fs::exists(data_dir) &&
            // boost::filesystem::is_directory(data_dir)) {
            //     for (boost::filesystem::directory_iterator end, it(data_dir);
            //     it != end; ++it)
            //         fs::remove_all(it->path());
            // }
        }
    };

    void checkSongInDatabase(const MediaFixture::SongTestMock& song,
                             const core::User& user) {
        auto songs = song_repo->findByTitleAndUser(song.title, user);

        CHECK(songs.size() > 0);
        CHECK_EQ(songs[0]->getTitle(), song.title);
        CHECK_EQ(songs[0]->getDuration(), song.duracao);
        if (!song.artist.empty())
            CHECK_EQ(songs[0]->getArtist()->getName(), song.artist);
        // CHECK_EQ(songs[0]->getArtist()->getName(), song.artist);
        if (!song.album.empty())
            CHECK_EQ(songs[0]->getAlbum()->getTitle(), song.album);
        CHECK_EQ(songs[0]->getYear(), std::stoi(song.year));
        // CHECK_EQ(songs[0]->getGenre(), song.genre);
    }

    void clearTestEnvironment(core::User user = core::User()) {
        song_repo->removeAll();
        artist_repo->removeAll();
        album_repo->removeAll();
        user_repo->removeAll();

        fs::remove_all(config.userMusicDirectory());
        fs::remove_all(config.publicMusicDirectory());
        fs::remove_all(config.inputUserPath());
        fs::remove_all(config.inputPublicPath());
        fs::remove_all(user.getInputPath());
        fs::remove_all(user.getHomePath());
        // fs::path input_dir(config.inputPublicPath());
        // if (fs::exists(input_dir) &&
        // boost::filesystem::is_directory(input_dir)) {
        //     for (boost::filesystem::directory_iterator end, it(input_dir); it
        //     != end; ++it)
        //         fs::remove_all(it->path());
        // }
    }

    // void createDirectoriesForUser(const core::User& user) {
    //     fs::create_directories(user.getHomePath());
    //     fs::create_directories(user.getInputPath());
    // }
    std::shared_ptr<core::FilesManager> manager;

    void setup() {
        db = DatabaseFixture().getDatabase();

        core::RepositoryFactory repo_factory(db);

        song_repo = repo_factory.createSongRepository();
        artist_repo = repo_factory.createArtistRepository();
        album_repo = repo_factory.createAlbumRepository();
        user_repo = repo_factory.createUserRepository();
        manager = std::make_shared<core::FilesManager>(config, *db);
    }

    TEST_CASE_FIXTURE(DirectoryFixture,
                      "CT-AC-01: Adicionar e organizar um album") {
        SUBCASE("Organizar um álbum") {
            setup();
            core::User user("usertest1",
                            config.userMusicDirectory(),
                            config.inputUserPath(),
                            uid1001);
            CHECK(user_repo->save(user));

            auto song_mock1 =
                media.getSongTestMock("Short_Song_Test_The_Testers");

            auto song_mock2 =
                media.getSongTestMock("Medium_Song_Test_The_Testers");

            fs::path input_dir(user.getInputPath());
            if (!fs::exists(input_dir))
                fs::create_directories(input_dir);

            fs::copy(song_mock1.path,
                     user.getInputPath() + "/tmp1.mp3",
                     fs::copy_options::overwrite_existing);
            fs::copy(song_mock2.path,
                     user.getInputPath() + "/tmp2.mp3",
                     fs::copy_options::overwrite_existing);
            CHECK(fs::exists(user.getInputPath() + "/tmp1.mp3"));
            CHECK(fs::exists(user.getInputPath() + "/tmp2.mp3"));
            manager->update();

            std::string song1_path_registre =
                user.getHomePath() + song_mock1.artist + "/" + song_mock1.album
                + "/" + song_mock1.title + ".mp3";
            std::string song2_path_registre =
                user.getHomePath() + song_mock2.artist + "/" + song_mock2.album
                + "/" + song_mock2.title + ".mp3";

            CHECK(fs::exists(song1_path_registre));
            CHECK(fs::exists(song2_path_registre));
            CHECK_EQ(song_mock1.album, song_mock2.album);
            CHECK_EQ(song_mock1.artist, song_mock2.artist);
            checkSongInDatabase(song_mock1, user);
            checkSongInDatabase(song_mock2, user);

            clearTestEnvironment(user);
        }

        SUBCASE("Atualizar um álbum existente") {
            setup();
            core::User user("usertest1",
                            config.userMusicDirectory(),
                            config.inputUserPath(),
                            uid1001);
            user_repo->save(user);

            auto song_mock1 =
                media.getSongTestMock("Short_Song_Test_The_Testers");
            auto song_mock2 =
                media.getSongTestMock("Medium_Song_Test_The_Testers");

            fs::path input_dir(user.getInputPath());
            if (!fs::exists(input_dir))
                fs::create_directories(input_dir);

            fs::copy(song_mock1.path,
                     user.getInputPath() + "/tmp1.mp3",
                     fs::copy_options::overwrite_existing);
            CHECK(fs::exists(user.getInputPath() + "/tmp1.mp3"));
            manager->update();

            std::string song1_path_registre =
                user.getHomePath() + song_mock1.artist + "/" + song_mock1.album
                + "/" + song_mock1.title + ".mp3";

            CHECK(fs::exists(song1_path_registre));
            checkSongInDatabase(song_mock1, user);

            fs::copy(song_mock2.path,
                     user.getInputPath() + "/tmp2.mp3",
                     fs::copy_options::overwrite_existing);
            manager->update();

            std::string song2_path_registre =
                user.getHomePath() + song_mock2.artist + "/" + song_mock2.album
                + "/" + song_mock2.title + ".mp3";

            CHECK(fs::exists(song2_path_registre));
            checkSongInDatabase(song_mock2, user);

            CHECK_EQ(song_mock1.album, song_mock2.album);
            CHECK_EQ(song_mock1.artist, song_mock2.artist);

            auto albuns = album_repo->getAll();
            CHECK_EQ(albuns.size(), 1);
            CHECK_EQ(albuns[0]->getSongsCount(), 2);

            clearTestEnvironment();
        }

        SUBCASE("Organizar musicas de mais de um usuario") {
            setup();

            std::string user1_home = "../tests/fixtures/data/user/user1";
            std::string user1_input = "../tests/fixtures/data/input/user/user1";
            std::string user2_home = "../tests/fixtures/data/user/user2";
            std::string user2_input = "../tests/fixtures/data/input/user/user2";

            core::User user1("usertest1", user1_home, user1_input, uid1001);
            core::User user2("usertest2", user2_home, user2_input, uid1002);

            user_repo->save(user1);
            user_repo->save(user2);

            const MediaFixture::SongTestMock song_mock1 =
                media.getSongTestMock("Short_Song_Test_The_Testers");
            const MediaFixture::SongTestMock song_mock2 =
                media.getSongTestMock("Medium_Song_Test_The_Testers");

            fs::create_directories(user1_input);
            fs::create_directories(user2_input);

            fs::copy(song_mock1.path,
                     user1_input + "/tmp1.mp3",
                     fs::copy_options::overwrite_existing);
            fs::copy(song_mock2.path,
                     user2_input + "/tmp2.mp3",
                     fs::copy_options::overwrite_existing);

            CHECK(fs::exists(user1_input + "/tmp1.mp3"));
            CHECK(fs::exists(user2_input + "/tmp2.mp3"));

            manager->update();

            std::string song1_path_registre =
                user1_home + "/" + song_mock1.artist + "/" + song_mock1.album
                + "/" + song_mock1.title + ".mp3";
            std::string song2_path_registre =
                user2_home + "/" + song_mock2.artist + "/" + song_mock2.album
                + "/" + song_mock2.title + ".mp3";

            CHECK(fs::exists(song1_path_registre));
            CHECK(fs::exists(song2_path_registre));

            checkSongInDatabase(song_mock1, user1);
            checkSongInDatabase(song_mock2, user2);

            auto albuns = album_repo->getAll();
            CHECK_EQ(albuns.size(), 2);

            auto artists = artist_repo->getAll();
            CHECK_EQ(artists.size(), 1); 

            clearTestEnvironment(user1);
            clearTestEnvironment(user2);
        }

        SUBCASE("Organizar musicas duplicadas") {
            setup();
            core::User user("usertest1",
                            config.userMusicDirectory(),
                            config.inputUserPath(),
                            uid1001);
            user_repo->save(user);

            const MediaFixture::SongTestMock song_mock =
                media.getSongTestMock("Short_Song_Test_The_Testers");

            fs::path input_dir(user.getInputPath());
            if (!fs::exists(input_dir))
                fs::create_directories(input_dir);

            fs::copy(song_mock.path,
                     user.getInputPath() + "/tmp.mp3",
                     fs::copy_options::overwrite_existing);
            CHECK(fs::exists(user.getInputPath() + "/tmp.mp3"));

            manager->update();

            std::string song_path_registre =
                user.getHomePath() + song_mock.artist + "/" + song_mock.album
                + "/" + song_mock.title + ".mp3";
            CHECK(fs::exists(song_path_registre));
            checkSongInDatabase(song_mock, user);

            fs::copy(song_mock.path,
                     user.getInputPath() + "/tmp2.mp3",
                     fs::copy_options::overwrite_existing);
            manager->update();

            auto songs = song_repo->findByTitleAndUser(song_mock.title, user);
            // TODO ?
            CHECK_EQ(songs.size(), 2);

            clearTestEnvironment(user);
        }
    }

    TEST_CASE("CT-AC-02: Adicionar e organizar uma musica") {
        SUBCASE("Organizar uma música") {
            setup();
            core::User user("usertest1",
                            config.userMusicDirectory(),
                            config.inputUserPath(),
                            uid1001);
            user_repo->save(user);

            const auto song_mock1 =
                media.getSongTestMock("Short_Song_Test_The_Testers");
            const auto song_mock2 =
                media.getSongTestMock("Medium_Song_Test_The_Testers");

            fs::path input_dir(user.getInputPath());
            if (!fs::exists(input_dir))
                fs::create_directories(input_dir);

            fs::copy(song_mock1.path,
                     user.getInputPath() + "/tmp1.mp3",
                     fs::copy_options::overwrite_existing);
            fs::copy(song_mock2.path,
                     user.getInputPath() + "/tmp2.mp3",
                     fs::copy_options::overwrite_existing);
            CHECK(fs::exists(user.getInputPath() + "/tmp1.mp3"));
            CHECK(fs::exists(user.getInputPath() + "/tmp2.mp3"));

            manager->update();

            std::string song1_path_registre =
                user.getHomePath() + song_mock1.artist + "/" + song_mock1.album
                + "/" + song_mock1.title + ".mp3";
            std::string song2_path_registre =
                user.getHomePath() + song_mock2.artist + "/" + song_mock2.album
                + "/" + song_mock2.title + ".mp3";

            CHECK(fs::exists(song1_path_registre));
            CHECK(fs::exists(song2_path_registre));
            checkSongInDatabase(song_mock1, user);
            checkSongInDatabase(song_mock2, user);

            clearTestEnvironment(user);
        }

        SUBCASE("Organizar música sem album") {
            setup();
            core::User user("usertest1",
                            config.userMusicDirectory(),
                            config.inputUserPath(),
                            uid1001);
            user_repo->save(user);

            const MediaFixture::SongTestMock song_mock =
                media.getSongTestMock("Song_Test_No_Album");

            fs::path input_dir(user.getInputPath());
            if (!fs::exists(input_dir))
                fs::create_directories(input_dir);

            fs::copy(song_mock.path,
                     user.getInputPath() + "/tmp.mp3",
                     fs::copy_options::overwrite_existing);
            CHECK(fs::exists(user.getInputPath() + "/tmp.mp3"));

            manager->update();

            std::string song_path_registre = user.getHomePath()
                                             + song_mock.artist + "/Singles/"
                                             + song_mock.title + ".mp3";
            CHECK(fs::exists(song_path_registre));
            checkSongInDatabase(song_mock, user);

            clearTestEnvironment(user);
        }

        // SUBCASE("Organizar música sem artista") {
        //     /*
        //      copia um arquivo de música para o diretório de input
        //      confere se foi registrado no banco de dados
        //      confere se o arquivo foi movido para o diretório correto
        //      */
        // }

        // SUBCASE("Reorganizar após remoção de musicas") {
        //     core::User user("usertest1",
        //                     config.userMusicDirectory(),
        //                     config.inputPublicPath(),
        //                     uid1001);
        //     user_repo->save(user);

        //     const MediaFixture::SongTestMock song_mock =
        //         media.getSongTestMock("Short_Song_Test_The_Testers");

        //     fs::copy(song_mock.path, user.getInputPath(),
        //     fs::copy_options::overwrite_existing); manager->update();

        //     std::string song_path_registre =
        //         user.getInputPath() + song_mock.artist + "/" +
        //         song_mock.album
        //         + "/" + song_mock.title + ".mp3";
        //     CHECK(fs::exists(song_path_registre));

        //     boost::filesystem::remove(song_path_registre);

        //     manager->update();
        //     CHECK_FALSE(fs::exists(song_path_registre));
        //     CHECK(song_repo->findByTitleAndUser(song_mock.title, user).size()
        //           == 0);
        // }
    }

    TEST_CASE("CT-AC-03: Adicionar e organizar musicas conforme autores") {
        SUBCASE("Organizar musicas de artistas diferentes") {
            setup();
            core::User user("usertest1",
                            config.userMusicDirectory(),
                            config.inputUserPath(),
                            uid1001);
            user_repo->save(user);

            const MediaFixture::SongTestMock song_mock1 =
                media.getSongTestMock("Short_Song_Test_The_Testers");
            const MediaFixture::SongTestMock song_mock2 =
                media.getSongTestMock("Short_Song_Examples_Example_Band");

            fs::path input_dir(user.getInputPath());
            if (!fs::exists(input_dir))
                fs::create_directories(input_dir);

            fs::copy(song_mock1.path,
                     user.getInputPath() + "/tmp1.mp3",
                     fs::copy_options::overwrite_existing);
            fs::copy(song_mock2.path,
                     user.getInputPath() + "/tmp2.mp3",
                     fs::copy_options::overwrite_existing);
            CHECK(fs::exists(user.getInputPath() + "/tmp1.mp3"));
            CHECK(fs::exists(user.getInputPath() + "/tmp2.mp3"));

            manager->update();

            std::string song_path_registre =
                user.getHomePath() + song_mock1.artist + "/" + song_mock1.album
                + "/" + song_mock1.title + ".mp3";
            std::string song2_path_registre =
                user.getHomePath() + song_mock2.artist + "/" + song_mock2.album
                + "/" + song_mock2.title + ".mp3";

            CHECK(fs::exists(song_path_registre));
            CHECK(fs::exists(song2_path_registre));

            auto songs = song_repo->getAll();
            CHECK_EQ(songs.size(), 2);

            auto artists = artist_repo->getAll();

            bool found_artist1 = false;
            bool found_artist2 = false;

            for (const auto& artist : artists) {
                if (artist->getName() == song_mock1.artist)
                    found_artist1 = true;
                if (artist->getName() == song_mock2.artist)
                    found_artist2 = true;
            }

            CHECK_EQ(found_artist1, true);
            CHECK_EQ(found_artist2, true);

            clearTestEnvironment();
        }

        SUBCASE("Organizar musicas de mesmo autor de usuários diferentes") {
            setup();

            std::string user1_home = "../tests/fixtures/data/user/user1";
            std::string user1_input = "../tests/fixtures/data/input/user/user1";
            std::string user2_home = "../tests/fixtures/data/user/user2";
            std::string user2_input = "../tests/fixtures/data/input/user/user2";

            core::User user1("usertest1", user1_home, user1_input, uid1001);
            core::User user2("usertest2", user2_home, user2_input, uid1002);

            user_repo->save(user1);
            user_repo->save(user2);

            const MediaFixture::SongTestMock song_mock1 =
                media.getSongTestMock("Short_Song_Test_The_Testers");
            const MediaFixture::SongTestMock song_mock2 =
                media.getSongTestMock("Medium_Song_Test_The_Testers");

            fs::create_directories(user1_input);
            fs::create_directories(user2_input);

            fs::copy(song_mock1.path,
                     user1_input + "/tmp1.mp3",
                     fs::copy_options::overwrite_existing);
            fs::copy(song_mock2.path,
                     user2_input + "/tmp2.mp3",
                     fs::copy_options::overwrite_existing);
            CHECK(fs::exists(user1_input + "/tmp1.mp3"));
            CHECK(fs::exists(user2_input + "/tmp2.mp3"));

            manager->update();

            std::string song1_path_registre =
                user1_home + "/" + song_mock1.artist + "/" + song_mock1.album
                + "/" + song_mock1.title + ".mp3";
            std::string song2_path_registre =
                user2_home + "/" + song_mock2.artist + "/" + song_mock2.album
                + "/" + song_mock2.title + ".mp3";

            CHECK(fs::exists(song1_path_registre));
            CHECK(fs::exists(song2_path_registre));
            checkSongInDatabase(song_mock1, user1);
            checkSongInDatabase(song_mock2, user2);

            auto artists = artist_repo->getAll();
            CHECK_EQ(artist_repo->count(), 1); 

            clearTestEnvironment(user1);
            clearTestEnvironment(user2);
        }
    }
}
