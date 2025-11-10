/**
 * @file Cli.cpp
 * @ingroup cli
 * @author Bruno Vieira
 * @brief Implementação do servico de comunicação com o usuario
 * @version 0.1
 * @date 2025-11-08
 *
 */

#include "cli/Cli.hpp"

namespace cli
{

    bool Cli::doCommand(const std::string &command)
    {
        std::stringstream ss(command);

        std::string firstCommand;
        ss >> firstCommand;

        if (firstCommand.empty())
        {
            std::cout << "Comando vazio. Por favor, insira um comando válido." << std::endl;
            return false;
        }
        else if (firstCommand == "info" || firstCommand == "status")
        {
            showStatus();
            return true;
        }
        else if (firstCommand == "play" || firstCommand == "pause" || firstCommand == "resume")
        {
            if (firstCommand == "play")
            {
                std::string playable;
                std::getline(ss, playable);
                if (playable.empty())
                {
                    // no argument -> toggle
                    toggleResumePause();
                    return true;
                }

                auto optPlayable = resolvePlayable(playable);
                if (!optPlayable)
                {
                    std::cout << "Não encontrado: " << playable << std::endl;
                    return false;
                }
                auto sp = *optPlayable; // shared_ptr<Core::IPlayable>
                play(*sp);
                return true;
            }

            // pause/resume
            toggleResumePause();
            return true;
        }
        else if (firstCommand == "replay" || firstCommand == "restart")
        {
            restart();
            return true;
        }

        else if (firstCommand == "volume")
        {
            std::string volumeCommand;
            if (ss >> volumeCommand)
            {
                if (volumeCommand == "up" || volumeCommand == "down")
                {
                    setVolumeUpDown(volumeCommand);
                    return true;
                }
                else if (volumeCommand == "set")
                {
                    unsigned int num;
                    if (ss >> num)
                    {
                        setVolume(num);
                        return true;
                    }
                }
                std::cout << "Comando inválido para volume. Use 'volume up', 'volume down' ou 'volume set <value>'." << std::endl;
                return false;
            }

            std::string current_volume = getVolume();
            std::cout << "Volume atual: " << current_volume << std::endl;

            return true;
        }
        else if (firstCommand == "rewind")
        {
            unsigned int seconds;
            if (ss >> seconds)
            {
                rewind(seconds);
                return true;
            }
            else
            {
                std::cout << "Por favor, forneça o número de segundos para retroceder." << std::endl;
                return false;
            }
        }
        else if (firstCommand == "forward")
        {
            unsigned int seconds;
            if (ss >> seconds)
            {
                forward(seconds);
                return true;
            }

            std::cout << "Por favor, forneça o número de segundos para avançar." << std::endl;
            return false;
        }
        else if (firstCommand == "mute" || firstCommand == "unmute" || firstCommand == "toggle_mute")
        {
            toggleMute(firstCommand);
            return true;
        }
        else if (firstCommand == "next")
        {
            next();
            return true;
        }
        else if (firstCommand == "previous")
        {
            previous();
            return true;
        }
        else if (firstCommand == "like")
        {
            like();
            return true;
        }
        else if (firstCommand == "deslike")
        {
            deslike();
            return true;
        }
        else if (firstCommand == "shuffle")
        {
            shuffle();
            return true;
        }
        else if (firstCommand == "loop")
        {
            std::string loopCommand;
            if (ss >> loopCommand)
            {
                if (loopCommand == "on" || loopCommand == "off")
                {
                    loop(loopCommand);
                    return true;
                }
                else
                {
                    std::cout << "Comando inválido para loop. Use 'loop on' ou 'loop off'." << std::endl;

                    return false;
                }
            }
            showHelp("loop");
            return true;
        }
        else if (firstCommand == "queue")
        {
            std::string queueCommand;

            if (ss >> queueCommand)
            {
                if (queueCommand == "clear")
                {
                    clearQueue();
                    return true;
                }
                else if (queueCommand == "show")
                {
                    showQueue();
                    return true;
                }
                else if (queueCommand == "add")
                {
                    std::string playable;
                    std::getline(ss, playable);
                    playable = trim(playable);
                    if (!playable.empty())
                    {
                        auto opt = resolvePlayable(playable);
                        if (!opt)
                        {
                            std::cout << "Não encontrado: " << playable << std::endl;
                            return false;
                        }
                        auto sp = *opt;
                        addToQueue(*sp);
                        return true;
                    }

                    std::cout << "Por favor, forneça o nome da música para adicionar à fila." << std::endl;
                    return false;
                }
                else if (queueCommand == "remove")
                {
                    unsigned idx;
                    if (ss >> idx)
                    {
                        removeFromQueue(idx);
                        return true;
                    }
                    else
                    {
                        std::cout << "Por favor, forneça o índice da música para remover da fila." << std::endl;
                        return false;
                    }
                }
                else
                {
                    std::cout << "Comando inválido para queue. Use 'queue show', 'queue clear', 'queue add <song>' ou 'queue remove <index>'." << std::endl;
                    return false;
                }
            }

            showHelp("queue");
            return true;
        }
        else if (firstCommand == "playlist")
        {
            std::string playlistCommand;

            if (ss >> playlistCommand)
            {
                if (playlistCommand == "show")
                {
                    std::string playlist;
                    std::getline(ss, playlist);
                    playlist = trim(playlist);
                    if (!playlist.empty())
                    {
                        auto optPl = resolvePlayable(playlist);
                        if (!optPl)
                        {
                            std::cout << "Playlist não encontrada: " << playlist << std::endl;
                            return false;
                        }
                        auto spPl = *optPl;
                        showPlaylist(*spPl);
                        return true;
                    }
                    std::cout << "Por favor, forneça o nome da playlist que deseja ver." << std::endl;
                    showHelp("playlist");
                    return true;
                }
                else if (playlistCommand == "add")
                {
                    std::string playlist;
                    if (ss >> playlist)
                    {
                        std::string playable;
                        std::getline(ss, playable);
                        playable = trim(playable);
                        if (!playable.empty())
                        {
                            auto optPl = resolvePlayable(playlist);
                            if (!optPl)
                            {
                                std::cout << "Playlist não encontrada: " << playlist << std::endl;
                                return false;
                            }
                            auto optSong = resolvePlayable(playable);
                            if (!optSong)
                            {
                                std::cout << "Música não encontrada: " << playable << std::endl;
                                return false;
                            }
                            auto spPl = *optPl;
                            auto spSong = *optSong;
                            addToPlaylist(*spPl, *spSong);
                            return true;
                        }

                        std::cout << "Por favor, forneça o nome da música para adicionar à playlist." << std::endl;
                        showHelp("playlist");
                        return true;
                    }
                    std::cout << "Por favor, forneça o nome da da playlist e nome da musica a ser adicionada." << std::endl;
                    showHelp("playlist");
                    return true;
                }
                else if (playlistCommand == "remove")
                {
                    std::string playlist;
                    if (ss >> playlist)
                    {
                        std::string playable;
                        std::getline(ss, playable);
                        playable = trim(playable);
                        if (!playable.empty())
                        {
                            try
                            {
                                size_t pos;
                                int value = std::stoi(playable, &pos);

                                // Verifica se toda a string foi convertida
                                if (pos == playable.length())
                                {
                                    auto optPl = resolvePlayable(playlist);
                                    if (!optPl)
                                    {
                                        std::cout << "Playlist não encontrada: " << playlist << std::endl;
                                        return false;
                                    }
                                    auto spPl = *optPl;
                                    if (value < 0)
                                    {
                                        std::cout << "Índice inválido: " << value << std::endl;
                                        return false;
                                    }
                                    removeFromPlaylist(*spPl, static_cast<unsigned int>(value));
                                    return true;
                                }
                                std::cout << "Por favor, forneça o nome da música ou posicao dela para remover da playlist." << std::endl;
                                showHelp("playlist");
                                return true;
                            }
                            catch (const std::invalid_argument &)
                            {
                                // String não é um número válido -> trata como nome da música
                                {
                                    auto optPl = resolvePlayable(playlist);
                                    if (!optPl)
                                    {
                                        std::cout << "Playlist não encontrada: " << playlist << std::endl;
                                        return false;
                                    }
                                    auto optSong = resolvePlayable(playable);
                                    if (!optSong)
                                    {
                                        std::cout << "Música não encontrada: " << playable << std::endl;
                                        return false;
                                    }
                                    auto spPl = *optPl;
                                    auto spSong = *optSong;
                                    removeFromPlaylist(*spPl, *spSong);
                                    return true;
                                }
                            }
                            catch (const std::out_of_range &)
                            {
                                std::cout << "O valor fornecido está fora do intervalo." << std::endl;
                                return false;
                            }
                        }

                        std::cout << "Por favor, forneça o nome da música ou posicao dela para remover da playlist." << std::endl;
                        showHelp("playlist");
                        return true;
                    }

                    std::cout << "Por favor, forneça o nome da da playlist e nome da musica a ser removida." << std::endl;
                    showHelp("playlist");
                    return true;
                }

                std::cout << "Comando inválido para playlist." << std::endl;
            }

            showHelp("playlist");
            return true;
        }
        else if (firstCommand == "search")
        {
            std::string searchType;
            if (ss >> searchType)
            {
                std::string query;
                std::getline(ss, query);
                query.erase(0, query.find_first_not_of(" "));

                if (searchType == "music")
                {
                    searchMusic(query);
                    return true;
                }
                else if (searchType == "artist")
                {
                    searchArtist(query);
                    return true;
                }
                else if (searchType == "album")
                {
                    searchAlbum(query);
                    return true;
                }
                else if (searchType == "playlist")
                {
                    searchPlaylist(query);
                    return true;
                }
                else if (searchType == "podcast")
                {
                    searchPodcast(query);
                    return true;
                }
                else
                {
                    std::cout << "Tipo de busca inválido. Use 'music', 'artist', 'album', 'playlist' ou 'podcast'." << std::endl;
                    return false;
                }
            }

            showHelp("search");
            return true;
        }
        else if (firstCommand == "help")
        {
            showHelp();
            return true;
        }
        else
        {
            std ::cout << "Comando não reconhecido: " << firstCommand << std::endl;
            return false;
        }
    }
}