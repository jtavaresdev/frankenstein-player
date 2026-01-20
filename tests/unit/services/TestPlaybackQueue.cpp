#include "core/services/FilesManager.hpp"
#include "core/bd/DatabaseManager.hpp"
#include "core/bd/RepositoryFactory.hpp"
#include "core/entities/User.hpp"

#include <iostream>

namespace core
{
    FilesManager::FilesManager(ConfigManager &config,
                     std::shared_ptr<SongRepository> songRepo,
                     std::shared_ptr<ArtistRepository> artistRepo,
                     std::shared_ptr<AlbumRepository> albumRepo)
        : _config(config), _songRepo(songRepo), _artistRepo(artistRepo), _albumRepo(albumRepo), _usersManager(config) {
            DatabaseManager db_manager(_config.databasePath(),
                                           _config.databaseSchemaPath());

            RepositoryFactory repo_factory(db_manager.getDatabase());
            _songRepo = repo_factory.createSongRepository();
            _artistRepo = repo_factory.createArtistRepository();
            _albumRepo = repo_factory.createAlbumRepository();
        }

    FilesManager::FilesManager(ConfigManager &config) : _config(config), _usersManager(config) {
            DatabaseManager db_manager(_config.databasePath(),
                                            _config.databaseSchemaPath());

            RepositoryFactory repo_factory(db_manager.getDatabase());
            _songRepo = repo_factory.createSongRepository();
            _artistRepo = repo_factory.createArtistRepository();
            _albumRepo = repo_factory.createAlbumRepository();
        }

    FilesManager::FilesManager(ConfigManager &config, SQLite::Database &db)
        : _config(config), _usersManager(config, db) {
            RepositoryFactory repo_factory(std::shared_ptr<SQLite::Database>(&db, [](SQLite::Database*){}));
            _songRepo = repo_factory.createSongRepository();
            _artistRepo = repo_factory.createArtistRepository();
            _albumRepo = repo_factory.createAlbumRepository();
        }

    std::string FilesManager::cleanString(const std::string& str)
    {
        auto begin = std::find_if_not(str.begin(), str.end(),
                                        [](unsigned char ch){ return std::isspace(ch); });

        auto end = std::find_if_not(str.rbegin(), str.rend(),
                                    [](unsigned char ch){ return std::isspace(ch); }).base();

        if (begin >= end)
            return "";

        return std::string(begin, end);
    }

    void FilesManager::move(std::string filePath, std::string newFilePath)
    {
        fs::path source(filePath);
        fs::path destination(newFilePath);
        try {
            fs::create_directories(destination.parent_path());

            fs::rename(source, destination);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Erro ao mover arquivo de '" << source.string() << "' para '" << destination.string() << "': " << e.what() << std::endl;
        }
    }

    std::shared_ptr<Song> FilesManager::readMetadata(TagLib::FileRef file, User &user)
    {
        if (file.isNull() || !file.tag())
        {
            // std::cout << "Nenhum metadado encontrado" << std::endl;
            throw std::invalid_argument("Arquivo sem metadados");
        }

        // FIX 1: Verify that user has a valid ID before proceeding
        if (user.getId() == 0) {
            throw std::invalid_argument("User must have a valid ID (must be saved to database first)");
        }

        TagLib::Tag *tag = file.tag();

        std::shared_ptr<Song> song = std::make_shared<Song>();
        song->setTitle(tag->title().isEmpty() ? "Unknown Title" : tag->title().toCString());
        song->setGenre(tag->genre().isEmpty() ? "Unknown Genre" : tag->genre().toCString());
        song->setYear(tag->year() == 0 ? 1900 : tag->year());
        song->setTrackNumber(tag->track() == 0 ? 1 : tag->track());
        song->setUser(user);
        song->setDuration(file.audioProperties() ? file.audioProperties()->length() : 0);


        std::string artistNames = tag->artist().isEmpty() ? "Unknown Artist" : tag->artist().toCString();

        const std::vector<std::string> separators = {
            " / ", "/", ";", ","
        };

        for (const auto& sep : separators) {
            size_t pos = 0;
            while ((pos = artistNames.find(sep, pos)) != std::string::npos) {
                artistNames.replace(pos, sep.length(), "/");
                pos += 1;
            }
        }

        std::stringstream ss(artistNames);
        std::string artistName;
        std::vector<std::shared_ptr<Artist>> featuring;
        bool mainArtistNotDefined = false;  // FIX 2: Changed from false to track state

        std::shared_ptr<Artist> artist;
        while(std::getline(ss, artistName, '/')) {
            artistName = cleanString(artistName);
            std::vector<std::shared_ptr<Artist>> artists = _artistRepo->findByName(artistName);

            if (artists.empty())
            {
                artist = std::make_shared<Artist>(artistName, song->getGenre());
                artist->setUser(user);
                
                // FIX 3: Check if save was successful
                if (!_artistRepo->save(*artist)) {
                    throw std::runtime_error("Failed to save artist: " + artistName);
                }
            }
            else
            {
                artist = artists[0];
            }

            if(!mainArtistNotDefined) {
                song->setArtist(artist);
                mainArtistNotDefined = true;
            } else {
                featuring.push_back(artist);
            }

        }

        // FIX 4: Ensure we have a valid main artist before proceeding
        if (!artist || artist->getId() == 0) {
            throw std::runtime_error("Failed to create or retrieve main artist");
        }

        std::string albumTitle = tag->album().isEmpty() ? "Singles" : tag->album().toCString();
        
        // FIX 5: Use the actual artist object, not artistName which is from the last iteration
        std::vector<std::shared_ptr<Album>> albums = _albumRepo->findByArtist(artistName);
        std::shared_ptr<Album> album;

        bool albumFound = false;
        for (const std::shared_ptr<Album> &existingAlbum : albums) {
            if (existingAlbum->getTitle() == albumTitle)
            {
                album = existingAlbum;
                albumFound = true;
                break;
            }
        }

        if (!albumFound)
        {
            album = std::make_shared<Album>(albumTitle, song->getGenre(), *artist);
            album->setYear(song->getYear());
            album->setUser(*song->getUser());
            
            // FIX 6: Check if album save was successful
            if (!_albumRepo->save(*album)) {
                throw std::runtime_error("Failed to save album: " + albumTitle);
            }
            
            if (!_albumRepo->setPrincipalArtist(*album, *artist, *song->getUser())) {
                throw std::runtime_error("Failed to set principal artist for album");
            }
        }
        
        song->setAlbum(*album);
        
        // FIX 7: Check if song save was successful
        if (!_songRepo->save(*song)) {
            throw std::runtime_error("Failed to save song: " + song->getTitle());
        }
        
        if (!_songRepo->setPrincipalArtist(*song, *artist, *song->getUser())) {
            throw std::runtime_error("Failed to set principal artist for song");
        }

        for(auto feat : featuring) {
            _songRepo->addFeaturingArtist(*song, *feat, user);
        }

        return song;
    }

    void FilesManager::verifyDir(std::string path)
    {
        fs::path dir(path);
        if (!fs::exists(dir))
        {
            try {
                fs::create_directories(dir);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Erro ao criar diretório '" << dir.string() << "': " << e.what() << std::endl;
            }
        }
    }

    void FilesManager::update()
    {
        std::shared_ptr<User> currentUser = _usersManager.getCurrentUser();
        std::shared_ptr<User> publicUser = _usersManager.getPublicUser();

        // FIX 8: Validate that users exist and have valid IDs
        if (!currentUser || currentUser->getId() == 0) {
            throw std::runtime_error("Current user is not valid or not saved to database");
        }
        if (!publicUser || publicUser->getId() == 0) {
            throw std::runtime_error("Public user is not valid or not saved to database");
        }

        std::string userInput = currentUser.get()->getInputPath();
        std::string publicInput = publicUser.get()->getInputPath();

        verifyDir(userInput);
        verifyDir(publicInput);

        std::string inputDirs[] = {userInput, publicInput};
        std::map<User, std::string> userInputMap = {
            {*currentUser, userInput},
            {*publicUser, publicInput}
        };

        // for (const auto &inputDir : inputDirs)
        for (const auto &par : userInputMap)
        {
            std::string inputDir = par.second;
            User user = par.first;

            // std::cout << "Processando diretório de input: " << inputDir << " para o usuário: " << user.getUsername() << std::endl;

            if (!fs::exists(inputDir))
            {
                continue;
            }

            for (const auto &entry : fs::directory_iterator(inputDir))
            {
                if (!fs::is_regular_file(entry.status()))
                {
                    continue;
                }

                TagLib::FileRef probe(entry.path().string().c_str());
                if (probe.isNull() || !probe.audioProperties())
                {
                    continue;
                }

                std::string filePath = entry.path().string();

                TagLib::FileRef f(filePath.c_str());
                if (!f.isNull())
                {

                    std::shared_ptr<Song> song;
                    try {
                        song = readMetadata(f, user);
                        // FIX 9: Remove redundant setUser and save calls
                        // song->setUser(user);  // Already done in readMetadata
                        // _songRepo->save(*song);  // Already done in readMetadata

                        if (!song) {
                            std::cerr << "Metadados insuficientes para arquivo '" << filePath << "', pulando." << std::endl;
                            continue;
                        }
                        move(filePath, song->getAudioFilePath());
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Erro ao processar o arquivo '" << filePath << "': " << e.what() << std::endl;
                        continue;
                    }
                }
            }
        }
    }

    bool FilesManager::isUpdated()
    {
        std::shared_ptr<User> currentUser = _usersManager.getCurrentUser();
        std::shared_ptr<User> publicUser = _usersManager.getPublicUser();

        std::string userInput = currentUser.get()->getInputPath();
        std::string publicInput = publicUser.get()->getInputPath();

        verifyDir(userInput);
        verifyDir(publicInput);

        std::string inputDirs[] = {userInput, publicInput};


        for (const auto &dir : inputDirs)
        {
            if (!fs::exists(dir))
            {
                continue;
            }

            for (const auto &entry : fs::directory_iterator(dir))
            {
                if (fs::is_regular_file(entry.status()))
                {
                    return false;
                }
            }
        }

        return true;
    }
}
