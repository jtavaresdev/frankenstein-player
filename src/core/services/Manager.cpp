#include "core/services/Manager.hpp"
#include "core/bd/DatabaseManager.hpp"
#include "core/bd/RepositoryFactory.hpp"
#include "core/entities/User.hpp"

#include <iostream>

namespace core
{
    Manager::Manager(ConfigManager &config,
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

    Manager::Manager(ConfigManager &config) : _config(config), _usersManager(config) {
            DatabaseManager db_manager(_config.databasePath(),
                                            _config.databaseSchemaPath());

            RepositoryFactory repo_factory(db_manager.getDatabase());
            _songRepo = repo_factory.createSongRepository();
            _artistRepo = repo_factory.createArtistRepository();
            _albumRepo = repo_factory.createAlbumRepository();
        }

    void Manager::move(std::string filePath, std::string newFilePath)
    {
        fs::path source(filePath);
        fs::path destination(newFilePath);

        fs::create_directories(destination.parent_path());

        fs::rename(source, destination);
    }

    std::shared_ptr<Song> Manager::readMetadata(TagLib::FileRef file, User &user)
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
        // song->setDuration(file.audioProperties() ? file.audioProperties()->length() : 0);
        song->setTrackNumber(tag->track() == 0 ? 1 : tag->track());
        song->setUser(user);

        std::string artistName = tag->artist().isEmpty() ? "Unknown Artist" : tag->artist().toCString();
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
        song->setArtist(artist);

        std::string albumTitle = tag->album().isEmpty() ? "Singles" : tag->album().toCString();
        std::vector<std::shared_ptr<Album>> albums = _albumRepo->findByArtist(artistName);
        std::shared_ptr<Album> album;

        bool albumFound = false;
        for (const std::shared_ptr<Album> &existingAlbum : albums)
        {
            if (existingAlbum->getName() == albumTitle)
            {
                album = existingAlbum;
                albumFound = true;
                break;
            }
        }

        if (!albumFound)
        {
            album = std::make_shared<Album>(albumTitle, artist, song->getGenre());
            album->setYear(song->getYear());
            album->setUser(*song->getUser());
            _albumRepo->save(*album);
            _albumRepo->setPrincipalArtist(*album, *artist, *song->getUser());
        }
        song->setAlbum(*album);
        _songRepo->save(*song);
        _songRepo->setPrincipalArtist(*song, *artist, *song->getUser());

        return song;
    }

    void Manager::verifyDir(std::string path)
    {
        fs::path dir(path);
        if (!fs::exists(dir))
        {
            fs::create_directories(dir);
        }
    }

    void Manager::update()
    {
        std::shared_ptr<User> currentUser = _usersManager.getCurrentUser();
        std::shared_ptr<User> publicUser = _usersManager.getPublicUser();

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

            if (!fs::exists(inputDir))
            {
                continue;
            }

            for (const auto &entry : fs::directory_iterator(inputDir))
            {
                if (!boost::filesystem::is_regular_file(entry.status()))
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
                    std::shared_ptr<Song> song = readMetadata(f, user);

                    move(filePath, song->getAudioFilePath());
                }
            }
        }
    }

    bool Manager::isUpdated()
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
                if (boost::filesystem::is_regular_file(entry.status()))
                {
                    return false;
                }
            }
        }

        return true;
    }

    void Manager::defineArtist(int songId, int artistId)
    {
        std::cout << "Método fora do escopo do MVP" << std::endl;
    }

}
