#include "core/services/Manager.hpp"
#include <stdexcept>
#include <filesystem>

namespace core {
    Manager::Manager(const ConfigManager& config,
                    std::shared_ptr<SongRepository> songRepo,
                    std::shared_ptr<ArtistRepository> artistRepo,
                    std::shared_ptr<AlbumRepository> albumRepo)
        : _config(config)
        , _songRepo(songRepo)
        , _artistRepo(artistRepo)
        , _albumRepo(albumRepo) {}

    void Manager::move(std::string filePath, std::string newFilePath) {
        fs::path source(filePath);
        fs::path destination(newFilePath);
        
        fs::create_directories(destination.parent_path());
        
        fs::rename(source, destination);
    }

    std::shared_ptr<Song> Manager::readMetadata(TagLib::FileRef file) {
        if(file.isNull() || !file.tag()) {
            std::cout << "Nenhum metadado encontrado" << std::endl;
        }

        TagLib::Tag *tag = file.tag();
        
        std::shared_ptr<Song> song = std::make_shared<Song>();
        song->setTitle(tag->title().isEmpty() ? "Unknown Title" : tag->title().toCString());
        
        std::string artistName = tag->artist().isEmpty() ? "Unknown" : tag->artist().toCString();
        std::vector<std::shared_ptr<Artist>> artists = _artistRepo->findByName(artistName);
        std::shared_ptr<Artist> artist;
        
        if (artists.empty()) {
            artist = std::make_shared<Artist>(artistName);
            _artistRepo->save(*artist);
        } else {
            artist = artists[0];
        }
        song->setArtist(artist);
        
        std::string albumTitle = tag->album().isEmpty() ? "Unknown" : tag->album().toCString();
        std::vector<std::shared_ptr<Album>> albums = _albumRepo->findByArtist(artistName);
        std::shared_ptr<Album> album;
        
        bool albumFound = false;
        for (const std::shared_ptr<Album>& existingAlbum : albums) {
            if (existingAlbum->getName() == albumTitle) {
                album = existingAlbum;
                albumFound = true;
                break;
            }
        }
        
        if (!albumFound) {
            album = std::make_shared<Album>(albumTitle, artist);
            _albumRepo->save(*album);
        }
        song->setAlbum(*album);

        fs::path originalFile(file.file()->name());
        std::string baseDir;
        
        fs::path publicInputPath(_config.inputPublicPath());
        fs::path userInputPath(_config.inputUserPath());
        fs::path originalPath(originalFile);
        
        if (fs::equivalent(originalPath.parent_path(), publicInputPath)) {
            baseDir = _config.publicMusicDirectory();
        } else if (fs::equivalent(originalPath.parent_path(), userInputPath)) {
            baseDir = _config.userMusicDirectory();
        } else {
            baseDir = _config.publicMusicDirectory();
        }

        fs::path destPath = fs::path(baseDir) / 
                           fs::path(artist->getName()) / 
                           fs::path(album->getName()) / 
                           originalPath.filename();

        //song->setPath(destPath.string());
        
        _songRepo->save(*song);
        
        return song;
    }

    void Manager::verifyDir(std::string path) {
        fs::path dir(path);
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
        }
    }

    void Manager::update() {
        std::vector<fs::path> inputDirs = {
            fs::path(_config.inputPublicPath()),
            fs::path(_config.inputUserPath())
        };
        
        verifyDir(_config.publicMusicDirectory());
        verifyDir(_config.userMusicDirectory());
        
        for (const auto& inputDir : inputDirs) {
            if (!fs::exists(inputDir)) {
                continue;
            }

            for (const auto& entry : fs::directory_iterator(inputDir)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                std::string filePath = entry.path().string();
                
                TagLib::FileRef f(filePath.c_str());
                if (!f.isNull()) {

                    std::shared_ptr<Song> song = readMetadata(f);
                    
                    std::string baseDestDir = (inputDir == fs::path(_config.inputPublicPath())) 
                        ? _config.publicMusicDirectory() 
                        : _config.userMusicDirectory();
                    
                    fs::path destPath = fs::path(baseDestDir) / 
                           fs::path(song->getArtist()->getName()) / 
                           fs::path(song->getArtist()->getName()) / 
                           song->getTitle();
                    
                    move(filePath, destPath.string());
                        
                }
            }
        }
    }

    bool Manager::isUpdated() {
        std::vector<fs::path> inputDirs = {
            fs::path(_config.inputPublicPath()),
            fs::path(_config.inputUserPath())
        };
        
        for (const auto& dir : inputDirs) {
            if (!fs::exists(dir)) {
                continue;
            }
            
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    return false;
                }
            }
        }
        
        return true;
    }

    void Manager::defineArtist(int songId, int artistId) {
        std::cout << "Método fora do escopo do MVP" << std::endl; 
    }

}