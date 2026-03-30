# Changelog

Todas as alterações notáveis a este projeto são documentadas aqui, no formato inspirado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/) e em [Semantic Versioning](https://semver.org/lang/pt-BR/).

Cada tag `v*` que dispara o workflow de release deve ter uma secção `[vX.Y.Z] - YYYY-MM-DD` com bullets alinhados a `TAG.md`. Antes do push da tag, move o conteúdo relevante de `[Unreleased]` para essa secção.

## [Unreleased]

### Added

### Changed

### Fixed

### Removed

## [v2.0.7] - 2026-03-30

### Fixed

- Imagem Docker: compilação com `make all` (fontes em `src/plugins/core` e `src/plugins/enterprise`), em vez do comando `g++` monolítico com caminhos antigos ([`Dockerfile`](Dockerfile)).
- Build Docker em contentor Debian 12: exclusão recursiva de artefactos `**/*.o`, `**/*.a`, `**/*.so` e binário `hermes` no [`.dockerignore`](.dockerignore), mais `make clean` antes de `make all`, para evitar erro de LTO (bytecode do GCC do host vs do container).

## [v2.0.6] - 2026-03-30

### Changed

- CI separado em [`.github/workflows/ci.yml`](.github/workflows/ci.yml): testes em PR e push a `main`/`master` (sem Docker/K8s).
- Release/deploy em [`.github/workflows/deploy.yml`](.github/workflows/deploy.yml): dispara em tag `v*` ou `workflow_dispatch`; verifica que o commit da tag é ancestral de `origin/master` ou `origin/main`; imagem no GHCR com digest SHA, `latest` e etiqueta com o nome da tag quando o evento é push de tag.
- Introdução de [`TAG.md`](TAG.md) e deste ficheiro como índice e registo de releases.
