# HERMES — Revisão de Posicionamento Técnico e de Produto

> Revisão realizada em: 2026-03-23  
> Escopo: análise crítica técnica e de produto do HERMES comparada ao mercado atual de LLM Gateways.

---

## 1. O que é o HERMES

**HERMES** (Hybrid Engine for Reasoning, Models & Execution Services) é um gateway HTTP escrito em **C++23** que expõe uma API compatível com OpenAI e roteia requisições para backends de LLM — principalmente Ollama, com suporte a OpenRouter e qualquer API OpenAI-compatível. Seu diferencial central é ser um **ponto único de entrada** para tráfego LLM com controle operacional rigoroso: autenticação, rate limiting, cache, pipeline de plugins para segurança (PII, prompt injection, moderação de conteúdo), auditoria e observabilidade.

O projeto está organizado em dois entregáveis:
- **Core C++** — o binário `hermes`, compilável em modo full (todas as features) ou edge (footprint mínimo para IoT/Raspberry Pi).
- **Dashboard (hermes-dashboard)** — app React + MUI para administração visual (em desenvolvimento).

---

## 2. Panorama do Mercado de LLM Gateways (2026)

### 2.1 Principais players

| Produto | Linguagem / Runtime | Modelo | Foco principal |
|---|---|---|---|
| **LiteLLM** | Python | OSS + SaaS managed | Cobertura máxima de providers (100+), simplicidade de adoção |
| **Portkey** | Node.js / SaaS | Freemium + Enterprise | Observabilidade rich, semantic caching, governança enterprise |
| **Bifrost** | Go | OSS | Performance extrema (11µs overhead, 50x LiteLLM P99) |
| **Kong AI Gateway** | Lua/Go (Kong) | Enterprise | Integração ao ecossistema Kong, throughput alto (4.200 RPS) |
| **Cloudflare AI Gateway** | Edge / SaaS | Managed | Edge computing, latência global mínima, zero infra |
| **OpenRouter** | SaaS | Marketplace | Catálogo de 600+ modelos, pay-per-token, zero setup |
| **Olla** | Go | OSS | Load balancer/proxy de alta performance focado em Ollama |
| **SmarterRouter** | Python | OSS | Roteamento VRAM-aware para Ollama/llama.cpp |
| **HERMES** | **C++23** | **OSS** | Gateway seguro, air-gap ready, pipeline de segurança/compliance |

### 2.2 Benchmarks de referência (dados de mercado, 2025-2026)

| Gateway | Overhead de latência (P99) | Throughput máximo | Memory footprint |
|---|---|---|---|
| Kong AI Gateway | ~150µs | 4.200+ RPS | Alto (Kong + plugins) |
| Bifrost (Go) | ~11µs | 5.000+ RPS | Baixo (Go runtime) |
| LiteLLM (Python) | ~200ms | ~1.200 RPS | Alto (CPython GIL) |
| Portkey (managed) | ~50ms | ~2.800 RPS | N/A (SaaS) |
| **HERMES (C++23)** | **Sub-milissegundo (estimado)** | **Alta (sem GIL, sem GC)** | **Muito baixo (edge build ~MB)** |

> **Observação:** O HERMES não possui benchmark publicado comparativo contra peers. Este é um gap prioritário a resolver para credibilidade técnica.

---

## 3. Matriz de Comparação de Features

### Legenda
- `●` — Implementado e estável
- `◐` — Parcialmente implementado / stub
- `○` — Não implementado / roadmap
- `—` — N/A para o produto

| Feature | HERMES | LiteLLM | Portkey | Bifrost | Kong AI |
|---|:---:|:---:|:---:|:---:|:---:|
| **Core Gateway** | | | | | |
| API OpenAI-compatível | ● | ● | ● | ● | ● |
| Streaming SSE | ● | ● | ● | ● | ● |
| Suporte a embeddings | ● | ● | ● | ● | ◐ |
| Tool calling / JSON mode | ● | ● | ● | ● | ○ |
| **Multi-Provider** | | | | | |
| Ollama (self-hosted) | ● | ● | ◐ | ● | ○ |
| OpenRouter | ● | ● | ● | ● | — |
| OpenAI / Azure / Anthropic | ◐* | ● (100+) | ● (1.600+) | ● (25+) | ● (20+) |
| Provider fallback chain | ● | ● | ● | ● | ● |
| Load balancing multi-backend | ● | ● | ● | ● | ● |
| Discovery dinâmico (Docker/K8s) | ● | ○ | ○ | ○ | ◐ |
| **Segurança e Controle de Acesso** | | | | | |
| API Keys (hash SHA-256) | ● | ● | ● | ● | ● |
| Rate limiting (global + por key) | ● | ◐ | ● | ● | ● |
| IP Whitelist / CIDR | ● | ○ | ○ | ○ | ● |
| Prompt injection detection | ● | ○ | ◐ | ◐ | ○ |
| PII redaction | ● | ○ | ◐ | ○ | ○ |
| Content moderation | ● | ○ | ● | ◐ | ○ |
| Response validation | ◐ | ○ | ◐ | ○ | ○ |
| **Cache** | | | | | |
| Cache LRU | ● | ● | ● | ● | ● |
| Semantic cache | ● | ◐ | ● | ● | ● |
| Request deduplication | ● | ○ | ○ | ○ | ○ |
| **Observabilidade** | | | | | |
| Métricas JSON | ● | ● | ● | ● | ● |
| Métricas Prometheus | ● | ● | ● | ● | ● |
| Request tracking (UUID) | ● | ● | ● | ● | ● |
| Audit log (JSONL consultável) | ● | ○ | ● | ○ | ○ |
| Alerting (webhook) | ● | ○ | ● | ○ | ● |
| OpenTelemetry | ○ | ◐ | ● | ● | ● |
| **Compliance e Governança** | | | | | |
| Posture ASPM | ● | ○ | ◐ | ○ | ○ |
| Compliance report | ● | ○ | ◐ | ○ | ○ |
| Security scan proativo | ● | ○ | ○ | ○ | ○ |
| Usage tracking + quotas | ◐ | ● | ● | ● | ● |
| Cost tracking | ◐ | ● | ● | ● | ● |
| **Arquitetura e Deploy** | | | | | |
| Air-gap / offline ready | ● | ● | ○ | ● | ● |
| Build edge (IoT/Raspberry Pi) | ● | ○ | ○ | ○ | ○ |
| Sem banco de dados (file-based) | ● | ○ | ○ | ○ | ○ |
| Docker + Kubernetes (Kustomize) | ● | ● | ● | ● | ● |
| Plugin system extensível | ● | ◐ | ○ | ● (WASM) | ● |
| Dashboard web de administração | ◐ | ● | ● | ◐ | ● |
| Benchmark integrado de modelos | ● | ○ | ○ | ○ | ○ |
| Open source | ● | ● | ◐ | ● | ○ |

> *HERMES implementa Ollama e qualquer provider OpenAI-compatível (OpenRouter, Azure, custom). Provedores diretos como Anthropic e Google Vertex via SDK nativo ainda são roadmap.

---

## 4. Análise Crítica: Pontos Fortes

### 4.1 Performance e eficiência de recursos
O HERMES é escrito em **C++23 com otimizações de compilação agressivas** (`-O3`, LTO, `-march=x86-64-v2`). Sem garbage collector, sem GIL, sem overhead de VM, o HERMES tem potencial para latência de gateway abaixo de 1ms e throughput comparable ao Bifrost (Go) — superando LiteLLM (Python) e Portkey (Node.js gerenciado) por pelo menos uma ordem de magnitude em overhead puro. O **build edge** leva isso ao extremo: um binário compilado sem cache/auth/rate/queue executável em hardware embarcado ou Raspberry Pi, sem paralelo direto no mercado.

### 4.2 Pipeline de segurança nativamente integrado
Nenhum dos concorrentes open source (LiteLLM, Bifrost, Olla) oferece um **pipeline de segurança completo e nativo** com: prompt injection, PII redaction, content moderation, response validation, posture ASPM e security scan — tudo construído como plugins de primeira classe, executando dentro do próprio processo, sem dependência de serviços externos. O Portkey se aproxima, mas é SaaS (sem controle do dado) e sua abordagem de compliance é mais focada em observabilidade do que em proteção ativa em runtime.

### 4.3 Compliance e auditoria como cidadãos de primeira classe
O HERMES trata compliance como feature de produto, não como addon: audit log consultável por período/key/modelo, compliance report, alerting via webhook, posture check. Para cenários regulatórios (LGPD, GDPR, EU AI Act), isso é um diferencial real — organizações com dados sensíveis ou obrigações regulatórias que precisam de um gateway self-hosted encontram poucos substitutos.

### 4.4 Discovery dinâmico de backends
O suporte a discovery automático via **Docker labels, Kubernetes service discovery e file watcher** é único entre os concorrentes avaliados para gateways OSS focados em Ollama. Isso permite escalar horizontalmente o pool de backends Ollama sem reconfiguração manual do gateway — relevante para on-prem com múltiplos servidores GPU.

### 4.5 Zero dependência de banco de dados
A persistência file-based (config.json, keys.json, audit JSONL) elimina uma dependência externa crítica. Em cenários air-gap, edge computing ou ambientes corporativos com restrições de infraestrutura, isso é uma vantagem operacional real. Nenhum concorrente direto OSS oferece isso com o mesmo grau de feature.

### 4.6 OpenAPI embutida
O gateway serve sua própria documentação interativa (`/docs`, `/redoc`, `/openapi.json`) sem dependência externa — benefício operacional em ambientes isolados onde acessar documentação online é inviável.

---

## 5. Análise Crítica: Lacunas e Riscos

### 5.1 Benchmark público inexistente
**Este é o risco de credibilidade número 1.** O HERMES tem vantagem teórica de performance significativa (C++ vs Python/Node.js), mas sem benchmark reproduzível e público comparando o HERMES contra LiteLLM, Portkey e Bifrost, essa vantagem é apenas uma afirmação. O Bifrost publicou benchmarks detalhados e isso se tornou o seu principal argumento de adoção. O HERMES precisa do mesmo.

**Recomendação:** Publicar benchmark reproduzível (Docker Compose + script) comparando latência P50/P95/P99, throughput (RPS), e consumo de memória contra LiteLLM e Bifrost em configuração equivalente.

### 5.2 Cobertura de providers limitada
LiteLLM suporta **100+ providers**, Portkey suporta **1.600+ modelos**. O HERMES cobre Ollama (self-hosted) e qualquer API OpenAI-compatível via `OpenAICustomProvider`. Provedores com SDK proprietário (Anthropic Claude via SDK nativo, Google Gemini via Vertex AI SDK, AWS Bedrock) não têm suporte direto — o usuário depende de um wrapper externo ou do OpenRouter como intermediário.

**Recomendação:** Para o segmento cloud, posicionar o OpenRouter como "ponte" (já suportado) e comunicar isso claramente. Para Anthropic e Gemini direto, avaliar a adição de providers nativos no roadmap como prioridade.

### 5.3 Usage tracking e cost controller incompletos
Três dos cinco principais concorrentes (LiteLLM, Portkey, Bifrost) oferecem cost tracking preciso por token, por key, por modelo, com histórico. No HERMES, o `cost_controller` retorna valor fixo (sem persistência), e usage tracking & quotas (RF-02) ainda não estão implementados. Isso é um gap relevante para o persona de "responsável financeiro de IA" em empresas.

**Recomendação:** Implementar RF-02 com precisão (uso por key + por modelo + por período) e persistência em JSONL. Pode ser o mesmo arquivo de audit log com campos de tokens/custo.

### 5.4 Conversation memory e A/B testing parciais
O `conversation_memory` armazena apenas mensagens do assistant (não o contexto completo de sessão). A/B testing e webhook notifications são listados como não implementados na spec. Esses três itens juntos representam features esperadas por equipes de produto que iterarão sobre prompts e modelos em produção.

### 5.5 OpenTelemetry ausente
Bifrost, Portkey e LiteLLM integram com OpenTelemetry (Grafana, New Relic, Honeycomb, Datadog). O HERMES oferece Prometheus e logging estruturado, mas sem OTLP trace export. Para organizações com plataformas de observabilidade baseadas em OTEL, isso é um obstáculo de adoção.

**Recomendação:** Adicionar suporte a OTLP trace export como plugin opcional seria suficiente para cobrir a maioria dos casos.

### 5.6 Dashboard web incompleto
O `/dashboard` embutido (HTML estático no binário) cobre o mínimo. O `hermes-dashboard` (React + MUI) está em desenvolvimento. Todos os concorrentes enterprise (Portkey, Kong, LiteLLM enterprise) oferecem dashboards ricos como ponto de entrada principal. A ausência de um dashboard funcional prejudica a experiência de adoção para personas não-técnicas.

### 5.7 Ausência de SDK cliente e documentação de quickstart
LiteLLM se instalava com `pip install litellm` e funcionava em dois minutos. Bifrost publica benchmarks e demo containers em segundos. O HERMES requer compilar C++ ou usar Docker — o que é perfeitamente aceitável, mas a documentação de quickstart precisa ser impecável. Não existe SDK Python/Node.js que wrapeie o HERMES facilitando a adoção por times de engenharia de produto.

**Recomendação:** Criar uma imagem Docker publicada no Docker Hub + um `docker run` de uma linha com configuração mínima para Ollama.

### 5.8 Plugins são builtin (sem .so dinâmico)
Os plugins do HERMES são compilados junto ao binário — não há carregamento dinâmico de .so em runtime. Isso garante performance e segurança, mas impede que usuários finais estendam o sistema sem recompilar. O Bifrost resolveu isso com plugins WASM. O HERMES documenta suporte a plugins dinâmicos como roadmap.

---

## 6. Posicionamento Estratégico Recomendado

### 6.1 Onde o HERMES vence claramente

| Cenário | Por que o HERMES ganha |
|---|---|
| **On-prem com Ollama e múltiplos backends GPU** | Discovery dinâmico, round-robin, health check nativo, air-gap ready |
| **Requisitos de compliance / regulatório** | PII redaction, audit log consultável, compliance report, prompt injection, posture ASPM — tudo em um binário |
| **Edge computing / IoT / recursos limitados** | Build edge em C++ sem dependências — sem paralelo no mercado |
| **Ambientes air-gap** | Zero dependência externa de runtime, sem banco de dados, documentação embutida |
| **Segurança em runtime como prioridade** | Pipeline nativo com prompt injection + PII + moderação + validação em um único hop |

### 6.2 Onde o HERMES perde hoje

| Cenário | Produto melhor posicionado |
|---|---|
| **Acesso rápido a 100+ providers cloud** | LiteLLM ou Portkey |
| **Observabilidade OTEL + dashboards ricos** | Portkey ou Bifrost |
| **Time zero de setup (SaaS, sem infra)** | OpenRouter ou Portkey managed |
| **Integração com ecossistema Kong existente** | Kong AI Gateway |
| **Cost tracking preciso e quotas por team** | LiteLLM ou Portkey |

### 6.3 Persona ideal para o HERMES

1. **Engenheiro de plataforma / MLOps** em empresa com restrições de compliance que roda Ollama on-prem e precisa de um gateway seguro, auditável e extensível — sem expor dados para SaaS externo.
2. **Equipe de segurança de IA** que precisa de proteção ativa em runtime (PII, prompt injection, moderação) na borda do tráfego LLM, não como add-on externo.
3. **Equipe de edge computing / embedded AI** que precisa de um gateway em hardware com recursos limitados (Raspberry Pi, dispositivos IoT).

---

## 7. Roadmap de Posicionamento — Prioridades

As lacunas identificadas na seção 5 foram priorizadas por impacto no posicionamento de mercado:

### Prioridade 1 — Crítico para credibilidade técnica

| Item | Justificativa |
|---|---|
| Publicar benchmark reproduzível (vs LiteLLM, Bifrost) | Sem números, a vantagem em C++ é apenas promessa |
| Docker Hub + quickstart `docker run` em uma linha | Reduz drasticamente o time-to-first-request |
| RF-02: Usage tracking + quotas com persistência | Bloqueador de adoção para personas de gestão de custo |

### Prioridade 2 — Importante para completude de produto

| Item | Justificativa |
|---|---|
| Dashboard web (hermes-dashboard) funcional | Personas não-técnicas precisam de UI para operar o gateway |
| Cost controller preciso (tokens reais por modelo) | Feature esperada em todo gateway de produção |
| Conversation memory completa | Requisito para casos de uso de chatbot com contexto |

### Prioridade 3 — Diferenciação avançada

| Item | Justificativa |
|---|---|
| OTLP trace export (plugin) | Integração com stacks de observabilidade enterprise |
| Plugin dinâmico (.so) ou WASM | Extensibilidade sem recompilação |
| Providers nativos: Anthropic, Google Vertex | Cobertura além do OpenAI-compatível |
| A/B testing de modelos (RF-08) | Feature de produto para teams que iteram sobre modelos |

---

## 8. Comparação Narrativa com Principais Concorrentes

### HERMES vs LiteLLM

LiteLLM é a referência de adoção rápida — `pip install` e suporte a 100+ providers. Mas sofre com performance (Python GIL, ~200ms overhead P99), sem segurança em runtime (sem PII nativo, sem prompt injection), e sem compliance built-in. O HERMES inverte esse trade-off: **menor cobertura de providers cloud, mas runtime de alta performance com segurança e compliance nativos**. Para o perfil de empresa que roda Ollama on-prem com dados sensíveis, o HERMES é superior em todas as dimensões que importam.

### HERMES vs Portkey

Portkey é o concorrente mais completo em observabilidade e feature set para SaaS. Mas é **SaaS-first** (os dados de auditoria passam pela infraestrutura deles), e o plano self-hosted tem limitações. Para qualquer organização com LGPD/GDPR rigoroso ou setor financeiro/saúde, o HERMES como **gateway 100% self-hosted com auditoria local** é o argumento definitivo. A lacuna do HERMES aqui é o dashboard e a riqueza de analytics.

### HERMES vs Bifrost

O concorrente técnico mais próximo. Bifrost (Go) é rápido, OSS, e tem plugin WASM. O HERMES em C++ tem potencial de ser ainda mais rápido, mas **Bifrost ganhou o mercado de performance** por publicar benchmarks primeiro. O HERMES se diferencia pelo **stack de segurança em runtime** (PII, prompt injection, compliance) e pelo **build edge** — o Bifrost não tem paralelo nessas frentes. A disputa de performance precisa ser resolvida com benchmark público.

### HERMES vs Kong AI Gateway

Kong é infraestrutura enterprise consolidada. O HERMES não compete com Kong como plataforma API geral — compete como **gateway especializado em LLM com segurança nativa**. Kong trata LLM calls como HTTP opaco; o HERMES entende o payload semântico (prompt injection, PII, moderação). Para equipes que já usam Kong como API gateway, o HERMES pode ser complementar — Kong gerencia o acesso externo, HERMES gerencia o tráfego LLM internamente.

---

## 9. Conclusão

O HERMES ocupa um **nicho técnico defensável e subatendido**: gateway LLM de alta performance, self-hosted, air-gap ready, com pipeline de segurança e compliance nativos. Nenhum concorrente avaliado combina as quatro dimensões simultaneamente: **C++ runtime**, **PII/prompt injection/moderação nativa**, **compliance report/audit/posture**, e **edge build para IoT**.

A vulnerabilidade estratégica atual é de **percepção, não de produto**: sem benchmark público, sem Docker Hub e sem quickstart de uma linha, o HERMES não converte o visitante técnico em usuário. As prioridades de roadmap descritas na seção 7 endereçam exatamente essa lacuna entre capacidade real e visibilidade de mercado.

Com essas melhorias, o posicionamento ideal é:

> **"O gateway LLM de performance extrema para times que não abrem mão de segurança, compliance e controle total dos dados — onde quer que a inferência aconteça."**

---

## Referências

- [Portkey vs LiteLLM vs OpenRouter — PkgPulse 2026](https://www.pkgpulse.com/blog/portkey-vs-litellm-vs-openrouter-llm-gateway-2026)
- [AI Gateway Benchmark: Kong AI Gateway, Portkey, LiteLLM — Kong Inc.](https://konghq.com/blog/engineering/ai-gateway-benchmark-kong-ai-gateway-portkey-litellm)
- [Top 5 LLM Gateways for Production in 2026 — DEV Community](http://www.dev.to/hadil/top-5-llm-gateways-for-production-in-2026-a-deep-practical-comparison-16p)
- [Bifrost AI Gateway Docs](https://docs.getbifrost.ai/overview)
- [A Definitive Guide to AI Gateways in 2026 — TrueFoundry](https://www.truefoundry.com/blog/a-definitive-guide-to-ai-gateways-in-2026-competitive-landscape-comparison)
- `docs/COMPARISON_HERMES_VS_ALLTRUE.md` — comparação interna com AllTrue.ai por pilares TRiSM
- `docs/briefing.md` — escopo e status de implementação do HERMES
