# mt

Editor extensível em C com SDL3, inspirado na arquitetura do Emacs: tudo acontece
em buffers e as ações são comandos nomeados vindos do núcleo ou de packages.

O planejamento de evolução, TODOs e dívidas técnicas está em
[ROADMAP.md](ROADMAP.md).

## Compilar

```sh
make
make test
./mt arquivo.c
```

Requer compilador C17, `pkg-config`, SDL3 e SDL3_ttf.

## Uso

- `Alt+X`: abre o minibuffer `M-x` e executa um comando pelo nome
- `Alt+T`: abre diretamente o prompt do comando `cmd`
- `Ctrl+O`: abre arquivo ou diretório
- `Ctrl+D`: abre um diretório no dired
- `Ctrl+B`: alterna para o próximo buffer
- `Ctrl+S`: salva o buffer atual
- `Ctrl+A/C/X/V`: selecionar tudo, copiar, recortar e colar
- `Ctrl+Z` / `Ctrl+Shift+Z`: desfazer e refazer alterações do buffer atual
- `Ctrl+←/→`: pula uma palavra
- `Ctrl+Shift+←/→`: seleciona por palavra
- `Home/End`: início e fim da linha (`Fn+←/→` em muitos notebooks)
- `Ctrl+Home/End`: início e fim do buffer
- `Ctrl+K`: apaga do cursor até o fim da linha; no fim, remove a quebra
- `Ctrl+F`: busca incremental
- Setas, Home, End, Page Up/Down e Shift para selecionar

Comandos importantes via `M-x`:

- `cmd`: solicita um comando de terminal e mostra stdout/stderr em `*cmd*`
- `find-file`: abre um caminho em outro buffer
- `dired`: abre o navegador de diretórios
- `next-buffer`: alterna entre os buffers
- `save`, `copy`, `paste`, `select-all` e `quit`
- `isearch` e `query-replace`

O minibuffer usa `Enter` para confirmar e `Esc` para cancelar.
Pressionar `Enter` em um `M-x` vazio abre o buffer `*commands*` com todos os
comandos nativos e os comandos fornecidos pelos packages.

Ao sair com alterações não salvas, ou ao abrir um arquivo que substituiria um
buffer modificado de mesmo nome, o editor exige a confirmação textual `yes`.

## Busca e substituição

`Ctrl+F` ou `M-x isearch` abre a busca incremental. O resultado é atualizado
enquanto você digita. `Ctrl+F` novamente procura a próxima ocorrência, `Enter`
aceita o resultado e `Esc` cancela e restaura a posição inicial.

`M-x query-replace` pede primeiro o texto procurado e depois a substituição. Em
cada ocorrência:

- `y`: substitui esta ocorrência;
- `n`: pula esta ocorrência;
- `!`: substitui todas as ocorrências restantes;
- `q` ou `Esc`: encerra.

Em teclados nos quais `Fn` é tratado pelo firmware, SDL recebe diretamente
`Home`, `End`, `Page Up` ou `Page Down`; esses eventos já estão mapeados.

## Dired

Execute `M-x dired`, informe um diretório e pressione Enter. Dentro do buffer:

- setas movem o cursor;
- `Enter` abre o arquivo ou entra no diretório;
- `g` atualiza a listagem;
- `Ctrl+B` volta para outro buffer.

Operações de gerenciamento disponíveis via `M-x` dentro do Dired:

- `dired-create-file`
- `dired-create-directory`
- `dired-rename` — atua sobre a entrada sob o cursor
- `dired-delete` — exige digitar `yes` antes de excluir

## Highlighting de C

Buffers `.c` e `.h` destacam palavras-chave, strings, caracteres, números,
comentários de linha e diretivas do preprocessador. O highlighter é independente
do renderer para facilitar a inclusão de outras linguagens.

## Configuração

No primeiro início o mt cria automaticamente:

```text
~/.config/mt/
├── keymap.conf
└── packages/
```

Se `XDG_CONFIG_HOME` estiver definido, ele é usado no lugar de `~/.config`. Não é
mais necessário definir `MT_KEYMAP`.

O formato de `keymap.conf` é:

```text
# tecla = comando
alt+x = execute-command
ctrl+o = find-file
ctrl+d = dired
ctrl+b = next-buffer
alt+t = cmd
```

Modificadores aceitos: `ctrl`, `shift`, `alt` e `super`. O nome pode apontar para
um comando nativo ou para um comando registrado por package.

## Packages

Packages são bibliotecas compartilhadas `.so` colocadas em
`~/.config/mt/packages`. Cada package exporta:

```c
bool mt_package_init(MtAPI *api);
```

O package usa `api->register_command` para expor comandos ao `M-x` e ao keymap.
Há um exemplo em `examples/hello-package.c`:

```sh
make package-example
cp build/hello-package.so ~/.config/mt/packages/
./mt
```

Então execute `M-x hello`. O executável usa `-rdynamic`, permitindo que packages
usem a API pública declarada em `include/`.

## Arquitetura

- `buffer`: ciclo de vida e troca de buffers, arquivos e diretórios
- `document`: armazenamento mutável, seleção e persistência
- `text`: navegação UTF-8 e coordenadas de texto
- `editor`: eventos e execução dos comandos nativos
- `minibuffer`: entrada de comandos e argumentos
- `keymap`: tradução configurável de teclas para nomes de comandos
- `package`: registro de comandos e carregamento dinâmico com `dlopen`
- `highlight`: análise léxica independente da interface
- `renderer`: apresentação SDL dos buffers e do minibuffer
- `config`: criação e descoberta da configuração XDG
