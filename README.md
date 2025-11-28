# Sistema de Gerenciamento de Música

Um sistema completo para gerenciamento local de músicas, álbuns, artistas e reprodução de áudio desenvolvido em C++.

##  Documentação

A documentação completa do projeto está disponível em:  
**[Documentação Doxygen](docs/documentation.pdf)**


##  Especificações
### Pré-requisitos
- Compilador C++11 ou superior
- CMake
- Doxygen (para documentação)

#### Guia de Configuração e Uso do Sistema de Música

## Configuração Inicial

Para configurar o ambiente, execute o script de setup:

```bash
#!/bin/bash

# Criar a pasta config se não existir
mkdir -p config

# Criar o arquivo frankenstein.config.json na pasta config
cat > config/frankenstein.config.json << 'EOF'
{
  "environment": "development",
  "database": {
    "filename": "dev.db",
    "schema_path": "../config/frankenstein_schema.sql"
  },
  "paths": {
    "public_user": "../user_data/",
    "user_home": "../user_data/user",
    "input_user": "../user_data/input/user/",
    "input_public": "../user_data/input/public/",
    "logs": "/var/log/frankenstein/",
    "cache": "~/.cache/frankenstein/"
  },
  "features": {
    "auto_scan_library": false
  }
}
EOF

echo "Arquivo config/frankenstein.config.json criado com sucesso!"

# Executar os comandos do git submodule
git submodule init
git submodule update

# Criar diretório build e executar cmake
mkdir build
cd build
cmake ..
cd ..

# Criar a estrutura de diretórios do user_data
mkdir user_data
mkdir user_data/input
mkdir user_data/input/public
mkdir user_data/input/user
mkdir user_data/user

echo "Todos os comandos foram executados com sucesso!"
```

### Como executar o script:
```bash
# 1. Salve o código acima em um arquivo setup.sh
# 2. Torne executável
chmod +x setup.sh
# 3. Execute
./setup.sh
```

## Adicionar Músicas

### 🎵 Músicas Pessoais (Apenas para seu usuário)
Coloque seus arquivos de música na pasta:
```
user_data/input/user/
```

###  Músicas Públicas (Para todos os usuários)
Coloque arquivos de música compartilhados na pasta:
```
user_data/input/public/
```

## Estrutura de Pastas Criada

Após executar o script, a seguinte estrutura será criada:
```
├── config/
│   └── frankenstein.config.json
├── user_data/
│   ├── input/
│   │   ├── user/       ← Coloque músicas pessoais aqui
│   │   └── public/     ← Coloque músicas públicas aqui
│   └── user/
├── build/
└── (outros arquivos do projeto)
```

## Formatos Suportados

Você pode adicionar arquivos de áudio nos formatos:
- MP3, WAV, FLAC, AAC, OGG
- E outros formatos comuns de áudio

# Guia de Uso do Frankenstein Music Player

## 🎵 Primeiros Passos

### Iniciar o Player
```bash
cd build
./frankenstein_player
```
### Comandos Essenciais para Começar

1. **Atualizar a biblioteca de músicas** (faça isso após adicionar novas músicas):
```
frankenstein> update_songs
```

2. **Reproduzir música**:
```
frankenstein> play
```

3. **Ver status do player**:
```
frankenstein> status
```

## Como Adicionar Músicas

### Músicas Pessoais (Apenas para seu usuário)
Coloque seus arquivos de música na pasta:
```
user_data/input/user/
```

### Músicas Públicas (Para todos os usuários)
Coloque arquivos de música compartilhados na pasta:
```
user_data/input/public/
```

**Importante**: Execute `update_songs` após adicionar novas músicas!

## 🎮 Comandos Principais

### Controle de Reprodução
- `play` - Inicia/pausa a reprodução
- `next` - Próxima música
- `previous` - Música anterior
- `forward X` - Avança X segundos
- `rewind X` - Retrocede X segundos
- `restart` - Reinicia a música atual

### Controle de Volume
- `volume set 50` - Define volume para 50%
- `volume up` - Aumenta volume
- `volume down` - Diminui volume
- `mute` - Silencia/reativa

### Gerenciamento de Fila
- `queue add "Nome da Música"` - Adiciona música à fila
- `queue show` - Mostra fila atual
- `queue clear` - Limpa a fila
- `shuffle` - Embaralha a fila

### Busca e Playlists
- `search "termo"` - Busca músicas, artistas ou álbuns
- `like` - Adiciona música atual aos favoritos
- `deslike` - Remove dos favoritos
- `playlist` - Gerencia playlists

## 💡 Dicas de Uso

### Exemplo de Sessão Típica:
```bash
# Atualizar biblioteca após adicionar músicas
frankenstein> update_songs

# Adicionar músicas à fila
frankenstein> queue add "Nome da Música"
frankenstein> queue add "Outra Música"

# Iniciar reprodução
frankenstein> play

# Ajustar volume
frankenstein> volume set 80

# Ver status
frankenstein> status
```

### Formas de Buscar Músicas:
- `queue add "Artista"` - Adiciona por artista
- `queue add "Nome da Música"` - Adiciona por título
- `queue add "Álbum"` - Adiciona por álbum


### Comando não reconhecido
Use `help` para ver todos os comandos disponíveis:
```
frankenstein> help
```

### Sair do Player
```
frankenstein> exit
```
### Geração da Documentação
```bash
# Gere a documentação
cd docs/
doxygen Doxyfile

# A documentação estará em:
# docs/latex/refman.pdf
```
##  Estrutura do Projeto

### Entidades Principais

####  Song (Música)
- Representa uma música com metadados completos
- Suporte para título, artista, álbum, duração, gênero e ano
- Futura integração com TagLib para extração automática de metadados

####  Artist (Artista)
- Gerencia informações de artistas/bandas
- Mantém coleções de músicas e álbuns
- Fornece estatísticas e buscas no catálogo

####  Album (Álbum)
- Representa coleções de músicas de um artista
- Mantém ordem original das faixas
- Calcula durações totais e fornece navegação sequencial

####  Playlist (Playlist)
- Agrupa músicas de forma organizada e personalizada
- Suporte para adicionar, remover e reordenar faixas
- Mantém metadados como duração total

####  Player (Reprodutor)
- Controle de reprodução de áudio
- Playlist management
- Controles de play, pause, next, previous e volume

####  Entity (Interface)
- Interface base para todas as entidades do sistema
- Fornece ID único e operações de comparação

##  Funcionalidades

### Gerenciamento de Conteúdo
- ✅ Cadastro e edição de músicas, álbuns e artistas
- ✅ Busca por nome, gênero e outros metadados
- ✅ Cálculo automático de durações
- ✅ Formatação de informações para exibição

### Reprodução de Áudio
- ✅ Controles básicos de reprodução
- ✅ Gerenciamento de playlists
- ✅ Controle de volume e mute
- ✅ Navegação sequencial em álbuns

### Estatísticas
- ✅ Contagem de músicas e álbuns
- ✅ Duração total de artistas e álbuns
- ✅ Formatação de tempos (HH:MM:SS)

#### Windows

```bash
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM="3.5"
cmake --build build
cd build
.\frankenstein_player.exe
```

## 👥 Autores

- **Bruno Vieira**
- **Eloy Maciel**
- **João Tavares**
- **Julia Reis**
- **Pedro Barruetavena**

** Documentação Detalhada:** [docs/output/latex/refman.pdf](docs/documentation.pdf)
