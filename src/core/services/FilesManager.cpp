#include "core/services/FilesManager.hpp"
#include "core/bd/DatabaseManager.hpp"
#include "core/bd/RepositoryFactory.hpp"
#include "core/entities/User.hpp"
#include "core/util/UnicodeHelper.hpp"

#include <iostream>
#include <ostream>

namespace core {
    FilesManager::FilesManager(ConfigManager& config,
                               std::shared_ptr<SongRepository> songRepo,
                               std::shared_ptr<ArtistRepository> artistRepo,
                               std::shared_ptr<AlbumRepository> albumRepo)
        : _config(config),
          _songRepo(songRepo),
          _artistRepo(artistRepo),
          _albumRepo(albumRepo),
          _usersManager(config) {
        DatabaseManager db_manager(_config.databasePath(),
                                   _config.databaseSchemaPath());

        RepositoryFactory repo_factory(db_manager.getDatabase());
        _songRepo = repo_factory.createSongRepository();
        _artistRepo = repo_factory.createArtistRepository();
        _albumRepo = repo_factory.createAlbumRepository();
        _userRepo = repo_factory.createUserRepository();
    }

    FilesManager::FilesManager(ConfigManager& config)
        : _config(config),
          _usersManager(config) {
        DatabaseManager db_manager(_config.databasePath(),
                                   _config.databaseSchemaPath());

        RepositoryFactory repo_factory(db_manager.getDatabase());
        _songRepo = repo_factory.createSongRepository();
        _artistRepo = repo_factory.createArtistRepository();
        _albumRepo = repo_factory.createAlbumRepository();
        _userRepo = repo_factory.createUserRepository();
    }

    FilesManager::FilesManager(ConfigManager& config, SQLite::Database& db)
        : _config(config),
          _usersManager(config, db) {
        RepositoryFactory repo_factory(
            std::shared_ptr<SQLite::Database>(&db, [](SQLite::Database*) {
            }));
        _songRepo = repo_factory.createSongRepository();
        _artistRepo = repo_factory.createArtistRepository();
        _albumRepo = repo_factory.createAlbumRepository();
        _userRepo = repo_factory.createUserRepository();
    }

    std::string FilesManager::cleanString(const std::string& str) {
        return UnicodeHelper::trim(str);
    }

    void FilesManager::move(std::string filePath, std::string newFilePath) {
        fs::path source = UnicodeHelper::toPath(filePath);
        fs::path destination = UnicodeHelper::toPath(newFilePath);

        try {
            fs::create_directories(destination.parent_path());

            fs::rename(source, destination);
        } catch (const std::exception& e) {
            std::cerr << "Erro ao mover arquivo de '" << filePath << "' para '"
                      << newFilePath << "': " << e.what() << std::endl;
            throw;
        }
    }

    std::shared_ptr<Song>
    FilesManager::readMetadata(TagLib::FileRef file,
                               User& user,
                               const std::string& sourceFilePath) {
        if (file.isNull() || !file.tag()) {
            throw std::invalid_argument("Arquivo sem metadados");
        }

        TagLib::Tag* tag = file.tag();

        std::shared_ptr<Song> song = std::make_shared<Song>();

        song->setTitle(tag->title().isEmpty() ? "Unknown Title"
                                              : tag->title().toCString(true));
        song->setGenre(tag->genre().isEmpty() ? "Unknown Genre"
                                              : tag->genre().toCString());
        song->setYear(tag->year() == 0 ? 1900 : tag->year());
        song->setTrackNumber(tag->track() == 0 ? 1 : tag->track());
        song->setUser(user);
        song->setDuration(file.audioProperties()
                              ? file.audioProperties()->lengthInSeconds()
                              : 0);

        std::string artistNames = tag->artist().isEmpty()
                                      ? "Unknown Artist"
                                      : tag->artist().toCString(true);

        const std::vector<std::string> separators = {" / ", "/", ";", ","};

        for (const auto& sep : separators) {
            size_t pos = 0;
            while ((pos = artistNames.find(sep, pos)) != std::string::npos) {
                artistNames.replace(pos, sep.length(), "/");
                pos += 1;
            }
        }

        // Processa lista de artistas
        std::stringstream ss(artistNames);
        std::string artistName;
        std::vector<std::shared_ptr<Artist>> featuring;
        bool mainArtistDefined = false;

        std::shared_ptr<Artist> mainArtist;
        while (std::getline(ss, artistName, '/')) {
            artistName = cleanString(artistName); // trim

            if (artistName.empty()) {
                continue;
            }

            std::vector<std::shared_ptr<Artist>> artists =
                _artistRepo->findByName(artistName);
            std::shared_ptr<Artist> artist;

            if (artists.empty()) {
                artist = std::make_shared<Artist>(artistName, song->getGenre());
                artist->setUser(user);
                _artistRepo->save(*artist);
            } else {
                artist = artists[0];
            }

            if (!mainArtistDefined) {
                mainArtist = artist;
                song->setArtist(mainArtist);
                mainArtistDefined = true;
            } else {
                featuring.push_back(artist);
            }
        }

        if (!mainArtist) {
            throw std::runtime_error("Nenhum artista encontrado nos metadados");
        }

        std::string albumTitle =
            tag->album().isEmpty() ? "Singles" : tag->album().toCString(true);
        std::vector<std::shared_ptr<Album>> albums =
            _albumRepo->findByArtist(mainArtist->getName());
        std::shared_ptr<Album> album;

        bool albumFound = false;
        for (const std::shared_ptr<Album>& existingAlbum : albums) {
            if (existingAlbum->getTitle() == albumTitle
                && existingAlbum->getUser()
                && existingAlbum->getUser()->getId() == user.getId()) {
                album = existingAlbum;
                albumFound = true;
                break;
            }
        }

        if (!albumFound) {
            album = std::make_shared<Album>(
                albumTitle, song->getGenre(), *mainArtist);
            album->setYear(song->getYear());
            album->setUser(*song->getUser());

            _albumRepo->save(*album);
            _albumRepo->setPrincipalArtist(
                *album, *mainArtist, *song->getUser());
        }

        song->setAlbum(album);
        _songRepo->save(*song);
        _songRepo->setPrincipalArtist(*song, *mainArtist, *song->getUser());

        for (auto feat : featuring) {
            _songRepo->addFeaturingArtist(*song, *feat, user);
        }

        std::string destinationPath = song->getAudioFilePath();
        if (!sourceFilePath.empty()) {
            move(sourceFilePath, destinationPath);
        }

        return song;
    }

    void FilesManager::verifyDir(std::string path) {
        fs::path dir = UnicodeHelper::toPath(path);

        if (!fs::exists(dir)) {
            try {
                fs::create_directories(dir);
            } catch (const std::exception& e) {
                std::cerr << "Erro ao criar diretório '" << path
                          << "': " << e.what() << std::endl;
                throw;
            }
        }
    }

    void FilesManager::update() {
        std::vector<std::shared_ptr<User>> allUsers;

        if (_userRepo) {
            allUsers = _userRepo->getAll();
        } else {
            std::cerr << "ERRO: _userRepo é NULL!" << std::endl;
            return;
        }

        if (allUsers.empty()) {
            std::shared_ptr<User> currentUser = _usersManager.getCurrentUser();
            std::shared_ptr<User> publicUser = _usersManager.getPublicUser();

            if (currentUser) {
                allUsers.push_back(currentUser);
            }
            if (publicUser
                && (!currentUser
                    || publicUser->getId() != currentUser->getId())) {
                allUsers.push_back(publicUser);
            }
        }

        for (const auto& user : allUsers) {
            if (!user || user->getId() == 0) {
                continue;
            }

            std::string inputDir = user->getInputPath();

            verifyDir(inputDir);

            fs::path inputDirPath = UnicodeHelper::toPath(inputDir);
            if (!fs::exists(inputDirPath)) {
                continue;
            }

            int fileCount = 0;
            for (const auto& entry : fs::directory_iterator(inputDirPath)) {
                if (!fs::is_regular_file(entry.status())) {
                    continue;
                }

                fileCount++;

                std::string filePath;
#ifdef _WIN32
                // No Windows, path::string() pode não funcionar com Unicode
                // Use path::u8string() ou converta via wstring
                filePath = UnicodeHelper::fromWide(entry.path().wstring());
#else
                // No Linux/Mac, string() já é UTF-8
                filePath = entry.path().string();
#endif

                TagLib::FileRef probe(filePath.c_str());
                if (probe.isNull() || !probe.audioProperties()) {
                    continue;
                }

                TagLib::FileRef f(filePath.c_str());
                if (!f.isNull()) {
                    std::shared_ptr<Song> song;
                    try {
                        song = readMetadata(f, *user, filePath);

                        if (!song) {
                            std::cerr
                                << "Metadados insuficientes para arquivo '"
                                << filePath << "', pulando." << std::endl;
                            continue;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Erro ao processar arquivo '" << filePath
                                  << "': " << e.what() << std::endl;
                        continue;
                    }
                }
            }
        }
    }

    bool FilesManager::isUpdated() {
        std::shared_ptr<User> currentUser = _usersManager.getCurrentUser();
        std::shared_ptr<User> publicUser = _usersManager.getPublicUser();

        if (!currentUser || !publicUser) {
            return true; // Sem usuários, considera atualizado
        }

        std::string userInput = currentUser->getInputPath();
        std::string publicInput = publicUser->getInputPath();

        verifyDir(userInput);
        verifyDir(publicInput);

        std::string inputDirs[] = {userInput, publicInput};

        for (const auto& dir : inputDirs) {
            fs::path dirPath = UnicodeHelper::toPath(dir);

            if (!fs::exists(dirPath)) {
                continue;
            }

            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (fs::is_regular_file(entry.status())) {
                    return false; // Encontrou arquivo novo
                }
            }
        }

        return true; // Nenhum arquivo novo
    }
} // namespace core
