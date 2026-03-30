# Makefile for HERMES — open-core model
# Targets:
#   make          → full build (core + enterprise plugins, statically linked)
#   make core     → OSS binary (core plugins only, no enterprise)
#   make edge     → minimal edge binary (ollama-only)
#   make enterprise-so → compile enterprise plugins as .so (for dynamic hot-reload)
CXX = g++
CXXFLAGS = -std=c++23 -O3 -Wall -Wextra -DNDEBUG -march=x86-64-v2 -flto -pipe -ffunction-sections -fdata-sections -Wno-deprecated-declarations -Wno-missing-field-initializers -I/usr/include/jsoncpp -Isrc -DCPPHTTPLIB_OPENSSL_SUPPORT
LDFLAGS = -flto -Wl,--gc-sections -s
LIBS = -ljsoncpp -lpthread -lssl -lcrypto -lcurl -lyaml-cpp -ldl

TARGET = hermes
SRCDIR = src

# ── Core plugins (OSS — compiled into binary, always available) ───────────────
CORE_PLUGIN_SOURCES = \
	$(SRCDIR)/plugins/core/auth_plugin.cpp \
	$(SRCDIR)/plugins/core/rate_limit_plugin.cpp \
	$(SRCDIR)/plugins/core/cache_plugin.cpp \
	$(SRCDIR)/plugins/core/logging_plugin.cpp \
	$(SRCDIR)/plugins/core/request_router_plugin.cpp \
	$(SRCDIR)/plugins/core/api_versioning_plugin.cpp \
	$(SRCDIR)/plugins/core/response_validator_plugin.cpp

# ── Enterprise plugins (loaded statically in full build; .so in enterprise-so) ─
ENTERPRISE_PLUGIN_SOURCES = \
	$(SRCDIR)/plugins/enterprise/guardrails_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/data_discovery_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/dlp_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/finops_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/semantic_cache_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/pii_redaction_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/content_moderation_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/prompt_injection_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/rag_injector_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/cost_controller_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/conversation_memory_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/adaptive_rate_limiter_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/audit_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/alerting_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/usage_tracking_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/prompt_manager_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/ab_testing_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/webhook_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/model_warmup_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/request_deduplication_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/request_queue_plugin.cpp \
	$(SRCDIR)/plugins/enterprise/streaming_transformer_plugin.cpp

# ── Gateway infrastructure sources (shared across all targets) ────────────────
GW_SOURCES = \
	$(SRCDIR)/main.cpp $(SRCDIR)/gateway.cpp $(SRCDIR)/plugin.cpp \
	$(SRCDIR)/update_checker.cpp $(SRCDIR)/core_services.cpp \
	$(SRCDIR)/request_queue.cpp \
	$(SRCDIR)/ollama_client.cpp $(SRCDIR)/api_keys.cpp \
	$(SRCDIR)/ollama_provider.cpp $(SRCDIR)/openai_custom_provider.cpp $(SRCDIR)/provider_router.cpp \
	$(SRCDIR)/benchmark.cpp \
	$(SRCDIR)/discovery/config_validator.cpp $(SRCDIR)/discovery/configuration_watcher.cpp \
	$(SRCDIR)/discovery/file_provider.cpp $(SRCDIR)/discovery/docker_provider.cpp \
	$(SRCDIR)/discovery/kubernetes_provider.cpp

# Full build: gateway + core + enterprise (statically linked)
SOURCES = $(GW_SOURCES) $(CORE_PLUGIN_SOURCES) $(ENTERPRISE_PLUGIN_SOURCES)
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = $(wildcard $(SRCDIR)/*.h) \
          $(wildcard $(SRCDIR)/plugins/core/*.h) \
          $(wildcard $(SRCDIR)/plugins/enterprise/*.h) \
          $(wildcard $(SRCDIR)/discovery/*.h)

# ── Edge build: minimal binary for edge computing ─────────────────────────────
EDGE_SOURCES = \
	$(SRCDIR)/main.cpp $(SRCDIR)/gateway.cpp $(SRCDIR)/plugin.cpp \
	$(SRCDIR)/core_services.cpp $(SRCDIR)/metrics_minimal.cpp \
	$(SRCDIR)/plugins/core/logging_plugin.cpp \
	$(SRCDIR)/plugins/core/cache_plugin.cpp \
	$(SRCDIR)/ollama_client.cpp $(SRCDIR)/api_keys.cpp
EDGE_OBJECTS = $(EDGE_SOURCES:.cpp=.o)
EDGE_CXXFLAGS = $(CXXFLAGS) -DBENCHMARK_ENABLED=0 -DDOCS_ENABLED=0 -DMULTI_PROVIDER=0 -DEDGE_CORE=1 -DDISCOVERY_ENABLED=0 -DENTERPRISE_ENABLED=0

# ── Core-only build (OSS binary, no enterprise) ───────────────────────────────
CORE_SOURCES = $(GW_SOURCES) $(CORE_PLUGIN_SOURCES)
CORE_OBJECTS = $(CORE_SOURCES:.cpp=.o)
CORE_CXXFLAGS = $(CXXFLAGS) -DENTERPRISE_ENABLED=0

# ── Enterprise .so build flags ────────────────────────────────────────────────
SO_CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -fPIC -shared -Isrc -I/usr/include/jsoncpp -DCPPHTTPLIB_OPENSSL_SUPPORT
SO_DIR = plugins/enterprise

all: $(SRCDIR)/httplib.h $(TARGET)
	@echo "Full build (core + enterprise, static): $(TARGET)"

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/httplib.h $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRCDIR)/plugins/core/%.o: $(SRCDIR)/plugins/core/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRCDIR)/plugins/enterprise/%.o: $(SRCDIR)/plugins/enterprise/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRCDIR)/discovery/%.o: $(SRCDIR)/discovery/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# OSS-only build: core plugins only, no enterprise features
core: CXXFLAGS := $(CORE_CXXFLAGS)
core: OBJECTS  := $(CORE_OBJECTS)
core: $(SRCDIR)/httplib.h $(TARGET)
	@echo "Core (OSS) build: $(TARGET) [ENTERPRISE_ENABLED=0]"

# Edge build: minimal binary for constrained environments
edge: CXXFLAGS := $(EDGE_CXXFLAGS)
edge: OBJECTS  := $(EDGE_OBJECTS)
edge: LIBS     := -ljsoncpp -lpthread -lssl -lcrypto -ldl
edge: $(SRCDIR)/httplib.h $(SRCDIR)/edge_config.h $(TARGET)
	@echo "Edge build: $(TARGET) [ENTERPRISE_ENABLED=0, EDGE_CORE=1]"

# Build enterprise plugins as individual .so files for dynamic hot-reload
enterprise-so: $(SRCDIR)/httplib.h
	@mkdir -p $(SO_DIR)
	@for src in $(ENTERPRISE_PLUGIN_SOURCES); do \
		name=$$(basename $$src .cpp); \
		echo "  [SO] $$name.so"; \
		$(CXX) $(SO_CXXFLAGS) $$src -o $(SO_DIR)/$$name.so -ljsoncpp -lssl -lcrypto; \
	done
	@echo "Enterprise plugins built in $(SO_DIR)/"

$(SRCDIR)/httplib.h:
	wget -q https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.34.0/httplib.h -O $(SRCDIR)/httplib.h

# E2E tests (Google Test) - gateway must be running separately
E2E_DIR = scripts/e2e-core-with-plugins
E2E_TEST_SRC = $(E2E_DIR)/e2e_core_plugins_test.cpp
E2E_TEST_OBJ = $(E2E_DIR)/e2e_core_plugins_test.o
E2E_TEST_BIN = $(E2E_DIR)/e2e_core_plugins_test
E2E_CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -Isrc -I/usr/include/jsoncpp

e2e-test: $(SRCDIR)/httplib.h $(E2E_TEST_BIN)

$(E2E_TEST_BIN): $(E2E_TEST_OBJ)
	$(CXX) $(E2E_TEST_OBJ) -o $@ -lgtest -lgtest_main -lpthread -ljsoncpp

$(E2E_TEST_OBJ): $(E2E_TEST_SRC)
	$(CXX) $(E2E_CXXFLAGS) -c $(E2E_TEST_SRC) -o $@

run-e2e: e2e-test
	@echo "Run gateway with config.e2e.json first, then:"
	./$(E2E_TEST_BIN)

# Benchmark models (gemma3:1b, 4b, 12b) - gateway + Ollama must be running
BENCH_DIR = scripts/benchmark-models
BENCH_SRCS = $(BENCH_DIR)/benchmark_models_test.cpp $(BENCH_DIR)/prompts.cpp
BENCH_OBJS = $(BENCH_DIR)/benchmark_models_test.o $(BENCH_DIR)/prompts.o
BENCH_BIN = $(BENCH_DIR)/benchmark_models_test
BENCH_CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -Isrc -I$(BENCH_DIR) -I/usr/include/jsoncpp

benchmark-models: $(SRCDIR)/httplib.h $(BENCH_BIN)

$(BENCH_BIN): $(BENCH_OBJS)
	$(CXX) $(BENCH_OBJS) -o $@ -lgtest -lgtest_main -lpthread -ljsoncpp

$(BENCH_DIR)/benchmark_models_test.o: $(BENCH_DIR)/benchmark_models_test.cpp $(BENCH_DIR)/prompts.h
	$(CXX) $(BENCH_CXXFLAGS) -c $(BENCH_DIR)/benchmark_models_test.cpp -o $@

$(BENCH_DIR)/prompts.o: $(BENCH_DIR)/prompts.cpp $(BENCH_DIR)/prompts.h
	$(CXX) $(BENCH_CXXFLAGS) -c $(BENCH_DIR)/prompts.cpp -o $@

# Full benchmark suite (reports + 12b analysis)
BENCH_SUITE_BIN = $(BENCH_DIR)/benchmark_suite_test
BENCH_SUITE_OBJS = $(BENCH_DIR)/benchmark_suite_test.o $(BENCH_DIR)/prompts.o \
	$(BENCH_DIR)/scoring/expected_answers.o $(BENCH_DIR)/scoring/math_validator.o \
	$(BENCH_DIR)/scoring/logic_validator.o $(BENCH_DIR)/scoring/score_utils.o
BENCH_SUITE_CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -Isrc -I$(BENCH_DIR) -I/usr/include/jsoncpp

benchmark-suite: $(SRCDIR)/httplib.h $(BENCH_SUITE_BIN)

$(BENCH_SUITE_BIN): $(BENCH_SUITE_OBJS)
	$(CXX) $(BENCH_SUITE_OBJS) -o $@ -lgtest -lgtest_main -lpthread -ljsoncpp

$(BENCH_DIR)/benchmark_suite_test.o: $(BENCH_DIR)/benchmark_suite_test.cpp
	$(CXX) $(BENCH_SUITE_CXXFLAGS) -c $(BENCH_DIR)/benchmark_suite_test.cpp -o $@

$(BENCH_DIR)/scoring/expected_answers.o: $(BENCH_DIR)/scoring/expected_answers.cpp $(BENCH_DIR)/scoring/expected_answers.h
	$(CXX) $(BENCH_SUITE_CXXFLAGS) -c $(BENCH_DIR)/scoring/expected_answers.cpp -o $@

$(BENCH_DIR)/scoring/math_validator.o: $(BENCH_DIR)/scoring/math_validator.cpp $(BENCH_DIR)/scoring/math_validator.h
	$(CXX) $(BENCH_SUITE_CXXFLAGS) -c $(BENCH_DIR)/scoring/math_validator.cpp -o $@

$(BENCH_DIR)/scoring/logic_validator.o: $(BENCH_DIR)/scoring/logic_validator.cpp $(BENCH_DIR)/scoring/logic_validator.h
	$(CXX) $(BENCH_SUITE_CXXFLAGS) -c $(BENCH_DIR)/scoring/logic_validator.cpp -o $@

$(BENCH_DIR)/scoring/score_utils.o: $(BENCH_DIR)/scoring/score_utils.cpp $(BENCH_DIR)/scoring/score_utils.h
	$(CXX) $(BENCH_SUITE_CXXFLAGS) -c $(BENCH_DIR)/scoring/score_utils.cpp -o $@

clean:
	rm -f $(OBJECTS) $(CORE_OBJECTS) $(EDGE_OBJECTS) $(TARGET) \
	      $(E2E_TEST_OBJ) $(E2E_TEST_BIN) \
	      $(BENCH_OBJS) $(BENCH_BIN) $(BENCH_SUITE_OBJS) $(BENCH_SUITE_BIN) \
	      $(SO_DIR)/*.so

install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential libjsoncpp-dev wget libcurl4-openssl-dev libyaml-cpp-dev

run: all
	./$(TARGET)

debug: CXXFLAGS = -std=c++23 -g -O0 -Wall -Wextra -I/usr/include/jsoncpp -Isrc
debug: LDFLAGS =
debug: $(SRCDIR)/httplib.h
debug: clean $(TARGET)

asan: CXXFLAGS = -std=c++23 -g -O1 -Wall -Wextra -I/usr/include/jsoncpp -Isrc -fsanitize=address,leak -fno-omit-frame-pointer
asan: LDFLAGS =
asan: LIBS += -fsanitize=address,leak
asan: $(SRCDIR)/httplib.h
asan: clean $(TARGET)

tsan: CXXFLAGS = -std=c++23 -g -O1 -Wall -Wextra -I/usr/include/jsoncpp -Isrc -fsanitize=thread -fno-omit-frame-pointer
tsan: LDFLAGS =
tsan: LIBS += -fsanitize=thread
tsan: $(SRCDIR)/httplib.h
tsan: clean $(TARGET)

.PHONY: all core edge enterprise-so clean install-deps run debug asan tsan e2e-test run-e2e benchmark-models benchmark-suite
