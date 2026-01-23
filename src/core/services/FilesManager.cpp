#include "core/services/FilesManager.hpp"
#include "core/bd/DatabaseManager.hpp"
#include "core/bd/RepositoryFactory.hpp"
#include "core/entities/User.hpp"

#include <iostream>
#include <ostream>

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
            _userRepo = repo_factory.createUserRepository();
        }

    FilesManager::FilesManager(ConfigManager &config) : _config(config), _usersManager(config) {
            DatabaseManager db_manager(_config.databasePath(),
                                            _config.databaseSchemaPath());

            RepositoryFactory repo_factory(db_manager.getDatabase());
            _songRepo = repo_factory.createSongRepository();
            _artistRepo = repo_factory.createArtistRepository();
            _albumRepo = repo_factory.createAlbumRepository();
            _userRepo = repo_factory.createUserRepository();
        }

    FilesManager::FilesManager(ConfigManager &config, SQLite::Database &db)
        : _config(config), _usersManager(config, db) {
            RepositoryFactory repo_factory(std::shared_ptr<SQLite::Database>(&db, [](SQLite::Database*){}));
            _songRepo = repo_factory.createSongRepository();
            _artistRepo = repo_factory.createArtistRepository();
            _albumRepo = repo_factory.createAlbumRepository();
            _userRepo = repo_factory.createUserRepository();
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

    std::shared_ptr<Song> FilesManager::readMetadata(TagLib::FileRef file, User &user, const std::string& sourceFilePath)
    {
        if (file.isNull() || !file.tag())
        {
            // std::cout << "Nenhum metadado encontrado" << std::endl;
            throw std::invalid_argument("Arquivo sem metadados");
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
        bool mainArtistDefined = false;

        std::shared_ptr<Artist> mainArtist;
        while(std::getline(ss, artistName, '/')) {
            artistName = cleanString(artistName);
            std::vector<std::shared_ptr<Artist>> artists = _artistRepo->findByName(artistName);
            std::shared_ptr<Artist> artist;
            if (artists.empty())
            {
                artist = std::make_shared<Artist>(artistName, song->getGenre());
                artist->setUser(user);
                _artistRepo->save(*artist);
            }
            else
            {
                artist = artists[0];
            }

            if(!mainArtistDefined) {
                mainArtist = artist;
                song->setArtist(mainArtist);
                mainArtistDefined = true;
            } else {
                featuring.push_back(artist);
            }

        }

        if (!mainArtist) {
            throw std::runtime_error("No artist found in metadata");
        }

        std::string albumTitle = tag->album().isEmpty() ? "Singles" : tag->album().toCString();
        std::vector<std::shared_ptr<Album>> albums = _albumRepo->findByArtist(mainArtist->getName());
        std::shared_ptr<Album> album;

        bool albumFound = false;
        for (const std::shared_ptr<Album> &existingAlbum : albums) {

            if (existingAlbum->getTitle() == albumTitle && 
                existingAlbum->getUser() && 
                existingAlbum->getUser()->getId() == user.getId())
            {
                album = existingAlbum;
                albumFound = true;
                break;
            }
        }

        if (!albumFound)
        {
            album = std::make_shared<Album>(albumTitle, song->getGenre(), *mainArtist);
            album->setYear(song->getYear());
            album->setUser(*song->getUser());

            _albumRepo->save(*album);
            _albumRepo->setPrincipalArtist(*album, *mainArtist, *song->getUser());
        }

        song->setAlbum(album);  

        _songRepo->save(*song);
        
        _songRepo->setPrincipalArtist(*song, *mainArtist, *song->getUser());
        
        for(auto feat : featuring) {
            _songRepo->addFeaturingArtist(*song, *feat, user);
        }

        std::string destinationPath = song->getAudioFilePath();
        if (!sourceFilePath.empty()) {
            move(sourceFilePath, destinationPath);
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
        std::vector<std::shared_ptr<User>> allUsers;
        
        if (_userRepo) {
            allUsers = _userRepo->getAll();
        } else {
            std::cerr << "DEBUG update(): _userRepo é NULL!" << std::endl;
        }
        
        if (allUsers.empty()) {
            std::shared_ptr<User> currentUser = _usersManager.getCurrentUser();
            std::shared_ptr<User> publicUser = _usersManager.getPublicUser();
            
            if (currentUser) {
                allUsers.push_back(currentUser);
            }
            if (publicUser && (!currentUser || publicUser->getId() != currentUser->getId())) {
                allUsers.push_back(publicUser);
            }
        }

        for (const auto &user : allUsers)
        {
            if (!user || user->getId() == 0) {
                continue;
            }

            std::string inputDir = user->getInputPath();
            
            verifyDir(inputDir);

            if (!fs::exists(inputDir))
            {
                continue;
            }

            int fileCount = 0;
            for (const auto &entry : fs::directory_iterator(inputDir))
            {
                if (!fs::is_regular_file(entry.status()))
                {
                    continue;
                }

                fileCount++;

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
                        song = readMetadata(f, *user, filePath);

                        if (!song) {
                            std::cerr << "Metadados insuficientes para arquivo '" << filePath << "', pulando." << std::endl;
                            continue;
                        }
                        // File already moved by readMetadata
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
