# Plano de evolução arquitetural do mt

Este documento transforma a auditoria de modularidade do projeto em um backlog
executável. O objetivo é permitir que aparência, comportamento, modos e extensões
sejam alterados sem modificar partes não relacionadas do editor.

Ele complementa o [ROADMAP.md](ROADMAP.md): o roadmap descreve funcionalidades;
este plano descreve a fundação necessária para implementá-las com baixo
acoplamento.

## Regras de execução

- Executar uma etapa por vez, em branch própria.
- Não misturar refatoração estrutural com funcionalidade não necessária à etapa.
- Preservar o comportamento observável, salvo quando o critério de aceite disser o
  contrário.
- Manter cada commit compilável, sem warnings e com testes proporcionais ao risco.
- Preferir migrações incrementais a reescritas completas.
- Não adicionar dependências sem registrar a necessidade, o custo e as alternativas.
- Não expor estruturas internas novas pela API pública.
- Revisar o `README.md` em toda entrega e atualizá-lo quando o estado, uso,
  arquitetura, configuração ou limitações do projeto mudarem.
- Atualizar este documento somente depois que os critérios da etapa forem atendidos.

## Estado atual

### Pontos que devem ser preservados

- `Document` já concentra conteúdo, seleção, persistência e undo/redo.
- O renderer, o highlighter, o keymap e o minibuffer já possuem módulos próprios.
- Toda ação interativa relevante já tem um nome de comando.
- A configuração respeita `XDG_CONFIG_HOME`.
- Há suporte inicial a packages carregados dinamicamente.
- Existem testes para documentos, UTF-8, buffers, keymap e salvamento seguro.

### Limitações confirmadas

- `editor.c` concentra eventos, edição, busca, Dired, shell e coordenação da UI.
- O registro de comandos já é unificado, mas sua capacidade ainda é fixa.
- Packages recebem `Editor *` e dependem das estruturas internas do programa.
- Não existe uma interface formal de major mode.
- Cores, métricas e quase toda a configuração de fonte são constantes compiladas.
- Keymaps têm capacidade fixa, dependem de SDL e não suportam escopos ou sequências.
- Cursor, seleção, rolagem, buffer e janela ainda não formam modelos independentes.
- Buffers, comandos, packages e keybindings usam arrays de tamanho máximo fixo.
- O minibuffer exige um novo valor de enum e novas condições para cada fluxo.
- O núcleo ainda depende de tipos SDL em interfaces que deveriam ser agnósticas à UI.

## Ordem de implementação

### A1 — Registro unificado de comandos

Branch sugerida: `refactor/command-registry`

- [x] Definir `CommandSpec` com nome, função, descrição e flags.
- [x] Implementar um único `CommandRegistry` para comandos nativos e de packages.
- [x] Substituir o enum fechado de comandos por consulta ao registro.
- [x] Migrar todos os comandos nativos sem alterar seus nomes ou atalhos.
- [x] Fazer `M-x`, keymap e packages consultarem o mesmo registro.
- [x] Rejeitar nomes vazios, duplicados ou maiores que o limite documentado.
- [x] Testar registro, consulta, duplicidade, capacidade e execução.

Critérios de aceite:

- Adicionar um comando nativo não exige editar enum nem `switch` central.
- Comandos nativos e externos percorrem o mesmo caminho de resolução e execução.
- Os atalhos e nomes atuais continuam funcionando.

### A2 — Decomposição do controlador do editor

Branch sugerida: `refactor/editor-controller`

- [ ] Separar comandos de edição em módulo próprio.
- [ ] Separar busca e substituição.
- [ ] Separar operações de arquivo e confirmações.
- [ ] Separar Dired do loop geral de eventos.
- [ ] Separar execução e apresentação de processos externos.
- [ ] Manter `editor.c` responsável apenas por ciclo de vida e coordenação.
- [ ] Criar testes diretos para os controladores extraídos.

Critérios de aceite:

- O loop SDL não contém implementação de funcionalidades de domínio.
- Cada controlador possui dependências explícitas e uma responsabilidade principal.
- Nenhum comportamento existente é removido.

### A3 — Configuração tipada

Branch sugerida: `feat/settings`

- [ ] Definir uma estrutura de settings com defaults centralizados.
- [ ] Separar descoberta de caminhos, parsing e validação.
- [ ] Carregar a configuração inteira antes de aplicá-la.
- [ ] Reportar todos os erros encontrados, com arquivo e linha.
- [ ] Permitir recarga segura sem reiniciar o editor.
- [ ] Adicionar settings para Tab, busca, processos e preferências visuais.
- [ ] Testar defaults, valores válidos, erros e recarga transacional.

Critérios de aceite:

- Uma configuração inválida não deixa o editor parcialmente configurado.
- Novas opções podem ser adicionadas sem espalhar parsing pelo código.
- O editor sempre possui valores válidos, mesmo sem arquivos de configuração.

### A4 — Sistema de temas

Branch sugerida: `feat/theme-system`

- [ ] Definir cores sem depender de `SDL_Color` no modelo público.
- [ ] Criar papéis semânticos: fundo, painel, texto, texto secundário, seleção,
      cursor, números de linha e diagnósticos.
- [ ] Criar papéis semânticos para syntax highlighting.
- [ ] Mover todas as cores fixas para um tema padrão.
- [ ] Carregar `~/.config/mt/theme.conf` com validação completa.
- [ ] Permitir recarregar o tema em tempo de execução.
- [ ] Testar parsing, fallback e temas incompletos ou inválidos.

Critérios de aceite:

- O renderer e os highlighters não contêm cores literais de apresentação.
- Trocar o tema não exige recompilar o editor.
- Um tema inválido não destrói o tema ativo.

### A5 — Fonte e métricas configuráveis

Branch sugerida: `feat/font-settings`

- [ ] Configurar família ou caminho da fonte.
- [ ] Configurar tamanho, line height, padding e largura do gutter.
- [ ] Manter `MT_FONT` como override compatível ou documentar sua substituição.
- [ ] Reabrir a fonte de forma transacional durante uma recarga.
- [ ] Recalcular métricas, viewport e posição do cursor após mudanças.
- [ ] Preparar escala HiDPI sem assumir pixels físicos fixos.
- [ ] Testar validação e fallback de settings, isolando o máximo possível da SDL.

Critérios de aceite:

- Nenhuma métrica visual configurável permanece como macro em `editor.h`.
- Falha ao abrir uma fonte nova mantém a fonte anterior ativa.
- Alterações visuais não afetam o modelo de documento.

### A6 — Interface formal de major mode

Branch sugerida: `refactor/mode-interface`

- [ ] Definir `MajorMode` com nome, detecção, highlighting e indentação.
- [ ] Permitir detecção por extensão, nome de arquivo e conteúdo.
- [ ] Associar cada buffer a uma instância de modo.
- [ ] Mover o modo C para um módulo registrado.
- [ ] Criar modo fundamental/texto como fallback.
- [ ] Suportar comandos e keymap locais do modo.
- [ ] Remover detecção de `.c` e `.h` do renderer.
- [ ] Remover testes diretos de tipo de buffer do caminho geral de edição.

Critérios de aceite:

- Adicionar um modo não exige modificar o renderer nem o loop SDL.
- Um package pode futuramente registrar um modo pela API estável.
- C e texto simples continuam com o comportamento atual.

### A7 — Separação de Document, Buffer, View e Window

Branch sugerida: `refactor/view-model`

- [ ] Manter texto, arquivo e undo/redo em `Document`.
- [ ] Manter nome, modo e variáveis locais em `Buffer`.
- [ ] Mover cursor, seleção, scroll e coluna desejada para `View`.
- [ ] Introduzir `Window` como proprietário da geometria e da View exibida.
- [ ] Permitir duas Views do mesmo Buffer com posições independentes.
- [ ] Preparar divisão de janelas sem implementá-la nesta refatoração.
- [ ] Testar independência de cursor, seleção e scroll entre Views.

Critérios de aceite:

- Alternar ou duplicar Views não altera a posição das demais.
- `Document` não contém estado específico de apresentação.
- O renderer recebe uma View explícita.

### A8 — API pública opaca e ABI versionada

Branch sugerida: `refactor/public-api`

- [ ] Impedir que packages acessem os campos de `Editor` diretamente.
- [ ] Expor handles opacos para editor, buffer, documento e registro.
- [ ] Oferecer funções pequenas para mensagens, buffers, texto e comandos.
- [ ] Adicionar versão da ABI e negociação durante `mt_package_init`.
- [ ] Definir regras de propriedade e duração de ponteiros.
- [ ] Adicionar metadados mínimos de package.
- [ ] Migrar o package de exemplo para usar somente a API pública.
- [ ] Testar compatibilidade e rejeição de ABI incompatível.

Critérios de aceite:

- Alterar campos internos de `Editor` não exige recompilar packages compatíveis.
- O SDK não inclui headers internos.
- Falhas de carregamento informam package, causa e versão esperada.

### A9 — Keymaps extensíveis e independentes da SDL

Branch sugerida: `refactor/keymap-system`

- [ ] Criar representação própria para teclas e modificadores.
- [ ] Isolar a conversão SDL na camada de plataforma.
- [ ] Substituir o array fixo por armazenamento dinâmico.
- [ ] Permitir remover bindings explicitamente.
- [ ] Aplicar arquivos de keymap de forma transacional.
- [ ] Adicionar keymap global, local de modo e transitório.
- [ ] Adicionar sequências de teclas.
- [ ] Implementar recarga por comando.
- [ ] Testar precedência, remoção, sequências, erros e recarga.

Critérios de aceite:

- Keymaps podem ser testados sem inicializar SDL.
- Um binding local não precisa alterar o keymap global.
- Arquivo inválido não modifica o keymap ativo.

### A10 — Minibuffer orientado a sessões

Branch sugerida: `refactor/minibuffer-session`

- [ ] Substituir fluxos codificados em enum por uma sessão com callbacks.
- [ ] Definir callbacks de atualização, confirmação e cancelamento.
- [ ] Adicionar uma fonte opcional de completion.
- [ ] Separar histórico por categoria.
- [ ] Permitir validação antes de fechar o prompt.
- [ ] Migrar busca, `M-x`, abertura de arquivos e confirmações.
- [ ] Testar sessões sem loop SDL.

Critérios de aceite:

- Criar um novo prompt não exige modificar `submit_minibuffer`.
- Cancelamento e confirmação possuem comportamento uniforme.
- Completion e histórico não dependem de um modo específico do minibuffer.

### A11 — Coleções dinâmicas e limites configuráveis

Branch sugerida: `refactor/dynamic-collections`

- [ ] Substituir limites fixos de buffers por vetor dinâmico.
- [ ] Substituir limites fixos de comandos e packages.
- [ ] Definir limites de segurança configuráveis onde forem necessários.
- [ ] Tratar falhas de alocação sem perder o estado anterior.
- [ ] Testar crescimento, limites e falhas previsíveis.

Critérios de aceite:

- O número de buffers e comandos não depende de macros de capacidade.
- Crescimento preserva ponteiros ou documenta claramente sua invalidação.
- Limites de segurança produzem mensagens acionáveis.

### A12 — Núcleo testável sem vídeo

Branch sugerida: `refactor/core-library`

- [ ] Separar biblioteca principal do executável SDL.
- [ ] Remover tipos SDL das interfaces de documento, comandos, modos e keymaps.
- [ ] Criar uma camada de plataforma para clipboard, eventos, relógio e processos.
- [ ] Testar todos os comandos de edição sem inicializar vídeo.
- [ ] Adicionar targets oficiais para ASan e UBSan.
- [ ] Preparar CI para build, testes, formatação e sanitizers.

Critérios de aceite:

- A maior parte da suíte roda sem SDL vídeo ou fonte instalada.
- O executável é uma composição do core com o frontend SDL.
- Dependências de plataforma ficam em módulos explicitamente identificados.

## Dependências entre etapas

```text
A1 Command Registry
 ├── A2 Editor Controller
 ├── A6 Major Modes
 └── A8 Public API

A3 Settings
 ├── A4 Themes
 ├── A5 Font Settings
 └── A9 Keymaps

A6 Major Modes ── A9 Keymaps
A7 View Model ─── futuras divisões de janela
A8 Public API ─── registro externo de modos, hooks e renderizadores
A10 Minibuffer ── completion, históricos e descoberta de comandos
A11 Collections ─ escalabilidade de buffers, comandos e packages
A12 Core Library ─ testes amplos, CI e novos frontends
```

## Definição geral de pronto

Uma etapa só pode ser marcada como concluída quando:

- [ ] Todos os critérios de aceite específicos foram atendidos.
- [ ] `make`, `make test` e `make format-check` passaram sem warnings.
- [ ] `git diff --check` passou.
- [ ] Sanitizers relevantes passaram ou uma limitação do ambiente foi registrada.
- [ ] Testes cobrem o comportamento migrado e pelo menos um caso de erro.
- [ ] README, ROADMAP e documentação pública foram atualizados quando necessário.
- [ ] O `README.md` foi explicitamente revisado, mesmo quando não exigiu alteração.
- [ ] A branch foi publicada e integrada à `main` sem misturar outra etapa.
