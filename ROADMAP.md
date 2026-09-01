# Roadmap do mt

Este documento registra os próximos passos para transformar o `mt` em um editor
pequeno, estável e genuinamente extensível. O objetivo não é copiar todo o Emacs,
mas preservar suas melhores ideias: buffers como unidade central, comandos
nomeados, configuração transparente e extensões desacopladas do núcleo.

As refatorações necessárias para manter esses recursos modulares estão detalhadas
em [ARCHITECTURE.md](ARCHITECTURE.md).

## Princípios

- O núcleo deve permanecer pequeno e independente da interface gráfica.
- Toda ação interativa relevante deve ser um comando nomeado.
- Comandos nativos e comandos de packages devem funcionar da mesma forma.
- Buffers especiais não devem precisar de exceções espalhadas pelo editor.
- Operações que podem perder dados precisam de confirmação ou recuperação.
- Recursos de texto devem funcionar corretamente com UTF-8.
- Cada mudança deve compilar sem warnings e incluir testes proporcionais ao risco.

## Prioridade 0 — Confiabilidade e segurança

- [x] Avisar sobre buffers modificados antes de sair.
- [x] Avisar antes de fechar ou substituir um buffer modificado.
- [x] Implementar salvamento atômico usando arquivo temporário e `rename`.
- [x] Preservar permissões do arquivo durante o salvamento.
- [ ] Detectar alterações externas no arquivo aberto.
- [ ] Exibir erros completos de abertura, leitura e escrita no buffer `*messages*`.
- [ ] Impedir que o Dired apague diretórios não vazios sem confirmação explícita.
- [ ] Tornar a execução de `cmd` assíncrona para não congelar a interface.
- [ ] Adicionar limite configurável para a saída de processos muito grandes.
- [ ] Tratar arquivos binários e bytes UTF-8 inválidos sem corromper o buffer.

Critério de conclusão: nenhum fluxo normal pode perder alterações silenciosamente
e processos externos não podem bloquear o loop principal da SDL.

## Prioridade 1 — Fundamentos de edição

- [x] Implementar histórico de undo/redo por buffer.
- [ ] Criar kill ring no estilo Emacs, com `kill-region`, `kill-line` e `yank-pop`.
- [ ] Adicionar apagar palavra anterior/próxima.
- [ ] Adicionar transposição de caracteres, palavras e linhas.
- [ ] Implementar duplicar, mover e selecionar linha.
- [ ] Implementar indentação e desindentação de região.
- [ ] Detectar e preservar LF e CRLF.
- [ ] Permitir configurar largura e comportamento de Tab.
- [ ] Adicionar autoindentação básica ao pressionar Enter.
- [ ] Implementar busca reversa e histórico de buscas.
- [ ] Tornar `query-replace` sensível ou insensível a maiúsculas sob configuração.
- [ ] Adicionar expressões regulares para busca e substituição.
- [ ] Manter coluna visual corretamente com tabs, Unicode largo e combining marks.

Critério de conclusão: editar código e texto por longos períodos deve ser seguro,
previsível e reversível.

## Prioridade 2 — Buffers e janelas

- [ ] Implementar `list-buffers` com seleção, fechamento e indicação de modificação.
- [ ] Implementar `kill-buffer` e `rename-buffer`.
- [ ] Permitir vários buffers com o mesmo nome usando nomes únicos automáticos.
- [ ] Separar `Buffer`, `View` e `Window` para permitir duas visualizações do mesmo
      buffer.
- [ ] Dividir a janela horizontal e verticalmente.
- [ ] Redimensionar, alternar e fechar divisões.
- [ ] Manter cursor e rolagem por visualização, não globalmente.
- [ ] Criar buffers especiais por meio de uma interface de modo, sem testes diretos
      de `BufferType` no loop de eventos.
- [ ] Implementar buffer `*scratch*` persistente e configurável.
- [ ] Persistir sessão: arquivos abertos, posições, divisões e buffer ativo.

Critério de conclusão: o usuário pode organizar vários arquivos simultaneamente
sem perder contexto de cursor ou rolagem.

## Prioridade 3 — Minibuffer e comandos

- [ ] Adicionar autocomplete de comandos no `M-x`.
- [ ] Exibir descrição e keybinding do comando selecionado.
- [ ] Manter histórico separado para comandos, caminhos, buscas e shell.
- [ ] Implementar navegação do histórico com setas.
- [ ] Completar caminhos e nomes de arquivos no minibuffer.
- [ ] Permitir argumentos prefixados, equivalente conceitual ao `C-u` do Emacs.
- [ ] Adicionar `describe-command`, `describe-key` e `where-is`.
- [ ] Criar comandos para recarregar keymap e packages sem reiniciar.
- [ ] Validar o arquivo de keymap inteiro e mostrar todos os erros encontrados.
- [ ] Permitir remover bindings explicitamente.
- [ ] Oferecer keymaps locais por modo e keymaps transitórios.
- [ ] Adicionar sequências de teclas como `Ctrl+X Ctrl+S`.

Critério de conclusão: todos os recursos podem ser descobertos e acionados sem
consultar o código-fonte.

## Prioridade 4 — Dired e arquivos

- [ ] Mostrar tamanho, permissões e data de modificação.
- [ ] Ordenar por nome, tamanho, data e tipo.
- [ ] Alternar exibição de arquivos ocultos.
- [ ] Marcar múltiplas entradas para operações em lote.
- [ ] Copiar e mover arquivos entre diretórios.
- [ ] Excluir usando lixeira quando disponível.
- [ ] Criar confirmação visual com a lista exata dos alvos.
- [ ] Renomear vários arquivos.
- [ ] Pesquisar e filtrar entradas.
- [ ] Atualizar a listagem preservando a seleção.
- [ ] Observar mudanças do sistema de arquivos.
- [ ] Abrir arquivos com aplicações externas por comando explícito.

Critério de conclusão: tarefas comuns de gerenciamento podem ser realizadas sem
sair do editor e sem operações destrutivas acidentais.

## Prioridade 5 — Modos e highlighting

- [ ] Definir uma interface formal de major mode.
- [ ] Associar modos por extensão, nome de arquivo e conteúdo.
- [ ] Transformar o modo C em um módulo separado.
- [ ] Manter estado léxico entre linhas para comentários e strings multilinha.
- [ ] Fazer highlighting incremental apenas nas regiões alteradas.
- [ ] Adicionar modos Markdown, JSON, Makefile e texto simples.
- [ ] Destacar pares de delimitadores.
- [ ] Implementar matching de parênteses.
- [ ] Adicionar números de linha relativos como opção.
- [ ] Adicionar whitespace mode e indicação de trailing whitespace.
- [ ] Criar uma interface opcional para Tree-sitter.
- [ ] Adicionar diagnósticos e integração futura com LSP.

Critério de conclusão: modos podem controlar highlighting, indentação, comandos e
keymaps locais sem modificar o núcleo.

## Prioridade 6 — Packages

- [ ] Versionar formalmente a ABI pública de packages.
- [ ] Ocultar estruturas internas e expor apenas funções estáveis.
- [ ] Informar erro detalhado quando `dlopen` ou `mt_package_init` falhar.
- [ ] Permitir descarregar e recarregar packages com segurança.
- [ ] Adicionar metadados: nome, versão, autor, descrição e versão mínima do mt.
- [ ] Resolver dependências entre packages.
- [ ] Criar hooks para abertura, salvamento, alteração e troca de buffer.
- [ ] Permitir que packages registrem modos, renderizadores e keymaps locais.
- [ ] Criar um SDK pequeno com exemplo, documentação e template.
- [ ] Adicionar testes de compatibilidade da ABI.
- [ ] Avaliar uma linguagem de configuração segura, como Lua, para extensões que
      não precisam de C nativo.

Critério de conclusão: atualizar o editor não deve quebrar silenciosamente os
packages compatíveis com a mesma versão de ABI.

## Prioridade 7 — Interface e desempenho

- [ ] Cachear texturas ou usar atlas de glifos; hoje cada trecho cria texturas.
- [ ] Renderizar somente linhas visíveis que mudaram.
- [ ] Trocar o buffer linear por piece table, gap buffer ou rope após benchmarks.
- [ ] Criar índice de linhas incremental para evitar varreduras completas.
- [ ] Suportar arquivos grandes sem carregar ou redesenhar tudo.
- [ ] Corrigir largura visual de Unicode com uma biblioteca apropriada.
- [ ] Adicionar rolagem horizontal.
- [ ] Criar tema configurável em `~/.config/mt/theme.conf`.
- [ ] Permitir configurar fonte, tamanho, line height e margens.
- [ ] Adicionar scrollbar, cursor configurável e indicador de progresso.
- [ ] Melhorar acessibilidade, contraste e suporte a escala HiDPI.

Critério de conclusão: arquivos grandes continuam responsivos e a renderização
não recria recursos gráficos desnecessariamente a cada frame.

## Prioridade 8 — Testes e ferramentas de desenvolvimento

- [ ] Separar biblioteca principal do executável para facilitar testes.
- [ ] Testar todos os comandos de edição sem inicializar SDL vídeo.
- [ ] Testar busca, substituição, undo e seleção com UTF-8.
- [ ] Testar Dired em uma árvore temporária controlada.
- [ ] Testar carregamento e falhas de packages.
- [ ] Adicionar AddressSanitizer e UndefinedBehaviorSanitizer.
- [ ] Rodar análise estática com `clang-tidy` ou equivalente.
- [ ] Adicionar fuzzing para documento, UTF-8, keymap e arquivos de configuração.
- [ ] Criar testes de eventos SDL para atalhos e minibuffer.
- [ ] Adicionar integração contínua para Linux e, depois, macOS e Windows.
- [ ] Medir cobertura sem transformar cobertura em objetivo isolado.

## Dívidas técnicas conhecidas

- O `Document` usa um array contíguo e move memória em inserções grandes.
- Cálculos de coluna ainda não representam perfeitamente largura visual Unicode.
- O highlighter de C é deliberadamente simples e não mantém estado entre linhas.
- `cmd` usa `popen` de forma síncrona.
- O estado de cursor e rolagem pertence ao editor, não a cada visualização.
- O registro unificado de comandos ainda usa capacidade máxima fixa.
- A API de packages expõe estruturas internas e ainda não possui versão de ABI.
- Buffers são armazenados em um array de tamanho máximo fixo.
- Mensagens usam um único campo curto em vez de um buffer de log.
- O Dired ainda trabalha com uma entrada por linha e parsing do texto exibido.

## Marcos sugeridos

### Marco 1 — Editor seguro

Undo/redo, confirmação de buffers modificados, salvamento atômico e `cmd`
assíncrono.

### Marco 2 — Ambiente de trabalho

Lista de buffers, fechamento de buffers, divisões de janela, histórico e
autocomplete do minibuffer.

### Marco 3 — Modos reais

Interface de major mode, modo C incremental, Markdown, JSON e keymaps locais.

### Marco 4 — Extensibilidade estável

ABI versionada, hooks, SDK de packages, recarga e testes de compatibilidade.

### Marco 5 — Escala

Nova estrutura de texto baseada em benchmarks, índice de linhas, cache de glifos
e suporte confiável a arquivos grandes.

## Próxima implementação recomendada

O próximo trabalho deve ser undo/redo. Ele afeta praticamente toda operação de
edição e deve existir antes que mais comandos mutáveis sejam adicionados. Uma
sequência adequada seria:

1. definir uma operação reversível de inserção ou remoção;
2. manter pilhas de undo e redo em cada buffer;
3. agrupar digitação contínua em uma única transação;
4. integrar Dired e comandos que substituem regiões;
5. adicionar testes de ida e volta para ASCII, UTF-8 e múltiplas linhas.

Depois disso, a prioridade deve ser salvamento seguro e proteção contra saída com
buffers modificados.
