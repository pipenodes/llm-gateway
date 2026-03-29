#pragma once
#include <string_view>

inline constexpr std::string_view OPENAPI_SPEC = R"JSON({
  "openapi": "3.0.3",
  "info": {
    "title": "HERMES API",
    "description": "OpenAI-compatible API gateway for Ollama backends. Supports chat completions, text completions, embeddings, streaming (SSE), model aliasing, tool calling, JSON mode, caching, rate limiting, and API key management.",
    "version": "2.0.0",
    "license": { "name": "MIT" }
  },
  "servers": [{ "url": "/", "description": "HERMES" }],
  "tags": [
    { "name": "Chat", "description": "Chat completions (OpenAI-compatible)" },
    { "name": "Completions", "description": "Text completions (legacy, OpenAI-compatible)" },
    { "name": "Embeddings", "description": "Text embeddings" },
    { "name": "Models", "description": "Model listing and retrieval" },
    { "name": "Admin", "description": "API key management (requires ADMIN_KEY)" },
    { "name": "System", "description": "Health, metrics, and documentation" }
  ],
  "paths": {
    "/v1/chat/completions": {
      "post": {
        "tags": ["Chat"],
        "summary": "Create chat completion",
        "description": "Generates a model response for the given chat conversation. Supports streaming via SSE, tool calling, and JSON mode.",
        "operationId": "createChatCompletion",
        "security": [{ "BearerAuth": [] }],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/ChatCompletionRequest" },
              "example": {
                "model": "gemma3:1b",
                "messages": [
                  { "role": "system", "content": "You are a helpful assistant." },
                  { "role": "user", "content": "Hello!" }
                ],
                "temperature": 0.7,
                "stream": false
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Successful response",
            "headers": {
              "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" },
              "X-Cache": { "schema": { "type": "string", "enum": ["HIT", "MISS", "BYPASS"] }, "description": "Cache status (only when cache is enabled)" },
              "x-ratelimit-limit-requests": { "$ref": "#/components/headers/x-ratelimit-limit-requests" },
              "x-ratelimit-remaining-requests": { "$ref": "#/components/headers/x-ratelimit-remaining-requests" },
              "x-ratelimit-reset-requests": { "$ref": "#/components/headers/x-ratelimit-reset-requests" }
            },
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/ChatCompletionResponse" }
              },
              "text/event-stream": {
                "schema": { "$ref": "#/components/schemas/ChatCompletionChunk" }
              }
            }
          },
          "400": { "$ref": "#/components/responses/BadRequest" },
          "401": { "$ref": "#/components/responses/Unauthorized" },
          "429": { "$ref": "#/components/responses/RateLimited" },
          "502": { "$ref": "#/components/responses/BadGateway" }
        }
      }
    },
    "/v1/completions": {
      "post": {
        "tags": ["Completions"],
        "summary": "Create text completion",
        "description": "Generates a completion for the given prompt. This is the legacy completions endpoint. Supports streaming via SSE.",
        "operationId": "createCompletion",
        "security": [{ "BearerAuth": [] }],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/CompletionRequest" },
              "example": {
                "model": "gemma3:1b",
                "prompt": "Once upon a time",
                "max_tokens": 100,
                "temperature": 0.7
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Successful response",
            "headers": {
              "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" },
              "x-ratelimit-limit-requests": { "$ref": "#/components/headers/x-ratelimit-limit-requests" },
              "x-ratelimit-remaining-requests": { "$ref": "#/components/headers/x-ratelimit-remaining-requests" },
              "x-ratelimit-reset-requests": { "$ref": "#/components/headers/x-ratelimit-reset-requests" }
            },
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/CompletionResponse" }
              },
              "text/event-stream": {
                "schema": { "$ref": "#/components/schemas/CompletionChunk" }
              }
            }
          },
          "400": { "$ref": "#/components/responses/BadRequest" },
          "401": { "$ref": "#/components/responses/Unauthorized" },
          "429": { "$ref": "#/components/responses/RateLimited" },
          "502": { "$ref": "#/components/responses/BadGateway" }
        }
      }
    },
    "/v1/embeddings": {
      "post": {
        "tags": ["Embeddings"],
        "summary": "Create embeddings",
        "operationId": "createEmbedding",
        "security": [{ "BearerAuth": [] }],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/EmbeddingRequest" },
              "example": {
                "model": "nomic-embed-text:latest",
                "input": "The quick brown fox jumps over the lazy dog"
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Successful response",
            "headers": {
              "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" },
              "X-Cache": { "schema": { "type": "string", "enum": ["HIT", "MISS"] }, "description": "Cache status" },
              "x-ratelimit-limit-requests": { "$ref": "#/components/headers/x-ratelimit-limit-requests" },
              "x-ratelimit-remaining-requests": { "$ref": "#/components/headers/x-ratelimit-remaining-requests" },
              "x-ratelimit-reset-requests": { "$ref": "#/components/headers/x-ratelimit-reset-requests" }
            },
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/EmbeddingResponse" }
              }
            }
          },
          "400": { "$ref": "#/components/responses/BadRequest" },
          "401": { "$ref": "#/components/responses/Unauthorized" },
          "502": { "$ref": "#/components/responses/BadGateway" }
        }
      }
    },
    "/v1/models": {
      "get": {
        "tags": ["Models"],
        "summary": "List available models",
        "operationId": "listModels",
        "security": [{ "BearerAuth": [] }],
        "responses": {
          "200": {
            "description": "List of models",
            "headers": {
              "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" }
            },
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/ModelList" }
              }
            }
          },
          "502": { "$ref": "#/components/responses/BadGateway" }
        }
      }
    },
    "/v1/models/{model}": {
      "get": {
        "tags": ["Models"],
        "summary": "Retrieve a model",
        "description": "Retrieves a model instance, providing basic information about the model.",
        "operationId": "retrieveModel",
        "security": [{ "BearerAuth": [] }],
        "parameters": [
          {
            "name": "model",
            "in": "path",
            "required": true,
            "schema": { "type": "string" },
            "description": "The ID of the model (or alias) to retrieve"
          }
        ],
        "responses": {
          "200": {
            "description": "Model details",
            "headers": {
              "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" }
            },
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Model" }
              }
            }
          },
          "404": {
            "description": "Model not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/ErrorResponse" }
              }
            }
          },
          "502": { "$ref": "#/components/responses/BadGateway" }
        }
      }
    },
    "/admin/keys": {
      "post": {
        "tags": ["Admin"],
        "summary": "Create API key",
        "description": "Generates a new API key with the given alias. Optionally set per-key rate limit and IP whitelist.",
        "operationId": "createApiKey",
        "security": [{ "AdminAuth": [] }],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/CreateKeyRequest" },
              "example": {
                "alias": "production-app",
                "rate_limit_rpm": 120,
                "ip_whitelist": ["10.0.0.0/8", "192.168.1.5"]
              }
            }
          }
        },
        "responses": {
          "201": {
            "description": "Key created",
            "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/CreateKeyResponse" }
              }
            }
          },
          "400": { "$ref": "#/components/responses/BadRequest" },
          "401": { "$ref": "#/components/responses/Unauthorized" },
          "403": { "$ref": "#/components/responses/Forbidden" }
        }
      },
      "get": {
        "tags": ["Admin"],
        "summary": "List API keys",
        "description": "Returns all API keys with masked key values.",
        "operationId": "listApiKeys",
        "security": [{ "AdminAuth": [] }],
        "responses": {
          "200": {
            "description": "List of keys",
            "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
            "content": {
              "application/json": {
                "schema": {
                  "type": "object",
                  "properties": {
                    "keys": {
                      "type": "array",
                      "items": { "$ref": "#/components/schemas/KeyInfo" }
                    }
                  }
                }
              }
            }
          },
          "401": { "$ref": "#/components/responses/Unauthorized" },
          "403": { "$ref": "#/components/responses/Forbidden" }
        }
      }
    },
    "/admin/keys/{alias}": {
      "delete": {
        "tags": ["Admin"],
        "summary": "Revoke API key",
        "operationId": "revokeApiKey",
        "security": [{ "AdminAuth": [] }],
        "parameters": [
          {
            "name": "alias",
            "in": "path",
            "required": true,
            "schema": { "type": "string" },
            "description": "The alias of the key to revoke"
          }
        ],
        "responses": {
          "200": {
            "description": "Key revoked",
            "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
            "content": {
              "application/json": {
                "schema": {
                  "type": "object",
                  "properties": {
                    "revoked": { "type": "boolean" },
                    "alias": { "type": "string" }
                  }
                }
              }
            }
          },
          "404": {
            "description": "Key not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/ErrorResponse" }
              }
            }
          },
          "401": { "$ref": "#/components/responses/Unauthorized" },
          "403": { "$ref": "#/components/responses/Forbidden" }
        }
      }
    },
    "/health": {
      "get": {
        "tags": ["System"],
        "summary": "Health check",
        "operationId": "healthCheck",
        "responses": {
          "200": {
            "description": "Gateway is healthy",
            "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
            "content": {
              "application/json": {
                "schema": {
                  "type": "object",
                  "properties": {
                    "status": { "type": "string", "example": "ok" }
                  }
                }
              }
            }
          }
        }
      }
    },
    "/metrics": {
      "get": {
        "tags": ["System"],
        "summary": "JSON metrics",
        "operationId": "getMetrics",
        "responses": {
          "200": {
            "description": "Runtime metrics",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/MetricsResponse" }
              }
            }
          }
        }
      }
    },
    "/metrics/prometheus": {
      "get": {
        "tags": ["System"],
        "summary": "Prometheus metrics",
        "operationId": "getPrometheusMetrics",
        "responses": {
          "200": {
            "description": "Metrics in Prometheus text format",
            "content": {
              "text/plain": {
                "schema": { "type": "string" }
              }
            }
          }
        }
      }
    }
  },
  "components": {
    "securitySchemes": {
      "BearerAuth": {
        "type": "http",
        "scheme": "bearer",
        "description": "API key (sk-...) required when at least one key has been created via /admin/keys"
      },
      "AdminAuth": {
        "type": "http",
        "scheme": "bearer",
        "description": "Admin key set via ADMIN_KEY environment variable"
      }
    },
    "headers": {
      "X-Request-Id": {
        "schema": { "type": "string", "format": "uuid" },
        "description": "Unique request identifier. Echoed from client X-Request-Id or X-Client-Request-Id header, or auto-generated UUID v4."
      },
      "x-ratelimit-limit-requests": {
        "schema": { "type": "integer" },
        "description": "Maximum number of requests per minute allowed for this key/IP."
      },
      "x-ratelimit-remaining-requests": {
        "schema": { "type": "integer" },
        "description": "Number of requests remaining in the current rate limit window."
      },
      "x-ratelimit-reset-requests": {
        "schema": { "type": "string", "example": "30s" },
        "description": "Time until the rate limit window resets (e.g. '30s')."
      }
    },
    "responses": {
      "BadRequest": {
        "description": "Invalid request",
        "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/ErrorResponse" } } }
      },
      "Unauthorized": {
        "description": "Missing or invalid authentication",
        "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/ErrorResponse" } } }
      },
      "Forbidden": {
        "description": "IP not in whitelist or admin not configured",
        "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/ErrorResponse" } } }
      },
      "RateLimited": {
        "description": "Rate limit exceeded",
        "headers": {
          "Retry-After": { "schema": { "type": "integer" }, "description": "Seconds until rate limit resets" },
          "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" }
        },
        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/ErrorResponse" } } }
      },
      "BadGateway": {
        "description": "Ollama backend unreachable or returned an error",
        "headers": { "X-Request-Id": { "$ref": "#/components/headers/X-Request-Id" } },
        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/ErrorResponse" } } }
      }
    },
    "schemas": {
      "ChatCompletionRequest": {
        "type": "object",
        "required": ["model", "messages"],
        "properties": {
          "model": { "type": "string", "description": "Model name or alias", "example": "gemma3:1b" },
          "messages": {
            "type": "array",
            "items": { "$ref": "#/components/schemas/Message" }
          },
          "temperature": { "type": "number", "minimum": 0, "maximum": 2, "description": "Sampling temperature. Set to 0 for deterministic (cacheable) responses." },
          "top_p": { "type": "number", "minimum": 0, "maximum": 1 },
          "max_tokens": { "type": "integer", "minimum": 1 },
          "stream": { "type": "boolean", "default": false, "description": "If true, returns SSE stream of ChatCompletionChunk" },
          "frequency_penalty": { "type": "number", "minimum": -2, "maximum": 2, "default": 0, "description": "Penalizes tokens based on existing frequency in text" },
          "presence_penalty": { "type": "number", "minimum": -2, "maximum": 2, "default": 0, "description": "Penalizes tokens based on whether they appear in text so far" },
          "stop": {
            "oneOf": [
              { "type": "string" },
              { "type": "array", "items": { "type": "string" }, "maxItems": 4 }
            ],
            "description": "Stop sequences"
          },
          "seed": { "type": "integer", "description": "Seed for deterministic sampling" },
          "n": { "type": "integer", "minimum": 1, "maximum": 1, "default": 1, "description": "Number of completions (only n=1 supported)" },
          "user": { "type": "string", "description": "Unique user identifier (logged but not forwarded)" },
          "logprobs": { "type": "boolean", "description": "Not supported, accepted for compatibility" },
          "top_logprobs": { "type": "integer", "description": "Not supported, accepted for compatibility" },
          "response_format": { "$ref": "#/components/schemas/ResponseFormat" },
          "tools": {
            "type": "array",
            "items": { "$ref": "#/components/schemas/Tool" },
            "description": "List of tools the model may call"
          },
          "tool_choice": {
            "description": "Controls which tool is called. 'auto' (default), 'none', 'required', or a specific tool.",
            "oneOf": [
              { "type": "string", "enum": ["auto", "none", "required"] },
              { "type": "object",
                "properties": {
                  "type": { "type": "string", "enum": ["function"] },
                  "function": { "type": "object", "properties": { "name": { "type": "string" } }, "required": ["name"] }
                },
                "required": ["type", "function"]
              }
            ]
          }
        }
      },
      "Message": {
        "type": "object",
        "required": ["role"],
        "properties": {
          "role": { "type": "string", "enum": ["system", "user", "assistant", "tool"] },
          "content": { "type": "string", "nullable": true },
          "name": { "type": "string", "description": "Name of the function (for tool messages)" },
          "tool_calls": {
            "type": "array",
            "items": { "$ref": "#/components/schemas/ToolCall" },
            "description": "Tool calls generated by the model (assistant messages only)"
          },
          "tool_call_id": { "type": "string", "description": "ID of the tool call this message responds to (tool messages only)" }
        }
      },
      "ToolCall": {
        "type": "object",
        "properties": {
          "id": { "type": "string", "example": "call_abc123" },
          "type": { "type": "string", "enum": ["function"] },
          "function": {
            "type": "object",
            "properties": {
              "name": { "type": "string" },
              "arguments": { "type": "string", "description": "JSON-encoded arguments" }
            }
          },
          "index": { "type": "integer" }
        }
      },
      "Tool": {
        "type": "object",
        "required": ["type", "function"],
        "properties": {
          "type": { "type": "string", "enum": ["function"] },
          "function": {
            "type": "object",
            "required": ["name"],
            "properties": {
              "name": { "type": "string" },
              "description": { "type": "string" },
              "parameters": { "type": "object", "description": "JSON Schema describing function parameters" }
            }
          }
        }
      },
      "ResponseFormat": {
        "type": "object",
        "properties": {
          "type": { "type": "string", "enum": ["text", "json_object"], "description": "'text' (default) or 'json_object' for JSON mode. 'json_schema' is not supported." }
        },
        "required": ["type"]
      },
      "ChatCompletionResponse": {
        "type": "object",
        "properties": {
          "id": { "type": "string", "example": "chatcmpl-abc123" },
          "object": { "type": "string", "enum": ["chat.completion"] },
          "created": { "type": "integer", "description": "Unix timestamp" },
          "model": { "type": "string" },
          "system_fingerprint": { "type": "string", "example": "fp_ollama" },
          "choices": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "index": { "type": "integer" },
                "message": { "$ref": "#/components/schemas/Message" },
                "finish_reason": { "type": "string", "enum": ["stop", "length", "tool_calls"] }
              }
            }
          },
          "usage": { "$ref": "#/components/schemas/Usage" }
        }
      },
      "ChatCompletionChunk": {
        "type": "object",
        "description": "SSE chunk (each line prefixed with 'data: '). Final message is 'data: [DONE]'.",
        "properties": {
          "id": { "type": "string" },
          "object": { "type": "string", "enum": ["chat.completion.chunk"] },
          "created": { "type": "integer" },
          "model": { "type": "string" },
          "system_fingerprint": { "type": "string" },
          "choices": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "index": { "type": "integer" },
                "delta": {
                  "type": "object",
                  "properties": {
                    "role": { "type": "string" },
                    "content": { "type": "string" },
                    "tool_calls": { "type": "array", "items": { "$ref": "#/components/schemas/ToolCall" } }
                  }
                },
                "finish_reason": { "type": "string", "nullable": true, "enum": ["stop", "length", "tool_calls"] }
              }
            }
          },
          "usage": { "$ref": "#/components/schemas/Usage" }
        }
      },
      "CompletionRequest": {
        "type": "object",
        "required": ["model", "prompt"],
        "properties": {
          "model": { "type": "string", "description": "Model name or alias" },
          "prompt": { "type": "string", "description": "The prompt to complete" },
          "suffix": { "type": "string", "description": "Text to append after the completion" },
          "max_tokens": { "type": "integer", "minimum": 1, "default": 16, "description": "Maximum number of tokens to generate" },
          "temperature": { "type": "number", "minimum": 0, "maximum": 2 },
          "top_p": { "type": "number", "minimum": 0, "maximum": 1 },
          "n": { "type": "integer", "minimum": 1, "maximum": 1, "default": 1 },
          "stream": { "type": "boolean", "default": false },
          "stop": {
            "oneOf": [
              { "type": "string" },
              { "type": "array", "items": { "type": "string" } }
            ]
          },
          "frequency_penalty": { "type": "number", "minimum": -2, "maximum": 2, "default": 0 },
          "presence_penalty": { "type": "number", "minimum": -2, "maximum": 2, "default": 0 },
          "seed": { "type": "integer" },
          "echo": { "type": "boolean", "default": false, "description": "Echo back the prompt in addition to the completion" },
          "user": { "type": "string" },
          "response_format": { "$ref": "#/components/schemas/ResponseFormat" }
        }
      },
      "CompletionResponse": {
        "type": "object",
        "properties": {
          "id": { "type": "string", "example": "cmpl-abc123" },
          "object": { "type": "string", "enum": ["text_completion"] },
          "created": { "type": "integer" },
          "model": { "type": "string" },
          "system_fingerprint": { "type": "string", "example": "fp_ollama" },
          "choices": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "text": { "type": "string" },
                "index": { "type": "integer" },
                "logprobs": { "type": "object", "nullable": true },
                "finish_reason": { "type": "string", "enum": ["stop", "length"] }
              }
            }
          },
          "usage": { "$ref": "#/components/schemas/Usage" }
        }
      },
      "CompletionChunk": {
        "type": "object",
        "description": "SSE chunk for text completions",
        "properties": {
          "id": { "type": "string" },
          "object": { "type": "string", "enum": ["text_completion"] },
          "created": { "type": "integer" },
          "model": { "type": "string" },
          "system_fingerprint": { "type": "string" },
          "choices": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "text": { "type": "string" },
                "index": { "type": "integer" },
                "logprobs": { "type": "object", "nullable": true },
                "finish_reason": { "type": "string", "nullable": true }
              }
            }
          },
          "usage": { "$ref": "#/components/schemas/Usage" }
        }
      },
      "Usage": {
        "type": "object",
        "properties": {
          "prompt_tokens": { "type": "integer" },
          "completion_tokens": { "type": "integer" },
          "total_tokens": { "type": "integer" }
        }
      },
      "EmbeddingRequest": {
        "type": "object",
        "required": ["model", "input"],
        "properties": {
          "model": { "type": "string", "example": "nomic-embed-text:latest" },
          "input": {
            "oneOf": [
              { "type": "string" },
              { "type": "array", "items": { "type": "string" } }
            ],
            "description": "Text or array of texts to embed"
          },
          "encoding_format": { "type": "string", "enum": ["float"], "default": "float", "description": "Only 'float' is supported" },
          "dimensions": { "type": "integer", "description": "Number of dimensions for the embedding (model-dependent)" },
          "user": { "type": "string" }
        }
      },
      "EmbeddingResponse": {
        "type": "object",
        "properties": {
          "object": { "type": "string", "enum": ["list"] },
          "model": { "type": "string" },
          "data": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "object": { "type": "string", "enum": ["embedding"] },
                "embedding": { "type": "array", "items": { "type": "number" } },
                "index": { "type": "integer" }
              }
            }
          },
          "usage": { "$ref": "#/components/schemas/Usage" }
        }
      },
      "ModelList": {
        "type": "object",
        "properties": {
          "object": { "type": "string", "enum": ["list"] },
          "data": {
            "type": "array",
            "items": { "$ref": "#/components/schemas/Model" }
          }
        }
      },
      "Model": {
        "type": "object",
        "properties": {
          "id": { "type": "string", "example": "gemma3:1b" },
          "object": { "type": "string", "enum": ["model"] },
          "created": { "type": "integer", "description": "Unix timestamp" },
          "owned_by": { "type": "string", "example": "ollama" },
          "permission": { "type": "array", "items": { "type": "object" } },
          "root": { "type": "string" },
          "parent": { "type": "string", "nullable": true }
        }
      },
      "CreateKeyRequest": {
        "type": "object",
        "required": ["alias"],
        "properties": {
          "alias": { "type": "string", "description": "Unique human-readable name", "example": "production-app" },
          "rate_limit_rpm": { "type": "integer", "default": 0, "description": "Per-key rate limit (requests/min). 0 = use global." },
          "ip_whitelist": {
            "type": "array",
            "items": { "type": "string" },
            "description": "Allowed IPs or CIDR ranges. Empty = allow all.",
            "example": ["10.0.0.0/8", "192.168.1.5"]
          }
        }
      },
      "CreateKeyResponse": {
        "type": "object",
        "properties": {
          "key": { "type": "string", "description": "Full API key (shown only once)", "example": "sk-aBcDeFgHiJkLmNoPqRsTuVwXyZ0123456789abcdefghijkl" },
          "alias": { "type": "string" },
          "created_at": { "type": "integer", "description": "Unix timestamp" }
        }
      },
      "KeyInfo": {
        "type": "object",
        "properties": {
          "alias": { "type": "string" },
          "key_prefix": { "type": "string", "description": "First 8 chars + ****", "example": "sk-aBcDe****" },
          "created_at": { "type": "integer" },
          "last_used_at": { "type": "integer" },
          "request_count": { "type": "integer" },
          "rate_limit_rpm": { "type": "integer" },
          "ip_whitelist": { "type": "array", "items": { "type": "string" } },
          "active": { "type": "boolean" }
        }
      },
      "MetricsResponse": {
        "type": "object",
        "properties": {
          "uptime_seconds": { "type": "integer" },
          "memory_rss_kb": { "type": "integer" },
          "memory_peak_kb": { "type": "integer" },
          "requests_total": { "type": "integer" },
          "requests_active": { "type": "integer" },
          "cache": {
            "type": "object",
            "properties": {
              "enabled": { "type": "boolean" },
              "hits": { "type": "integer" },
              "misses": { "type": "integer" },
              "evictions": { "type": "integer" },
              "entries": { "type": "integer" },
              "memory_bytes": { "type": "integer" },
              "hit_rate": { "type": "number" }
            }
          }
        }
      },
      "ErrorResponse": {
        "type": "object",
        "properties": {
          "error": {
            "type": "object",
            "properties": {
              "message": { "type": "string" },
              "type": { "type": "string", "enum": ["invalid_request_error", "auth_error", "forbidden_error", "rate_limit_error", "server_error"] }
            }
          }
        }
      }
    }
  }
})JSON";

inline constexpr std::string_view SWAGGER_HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>HERMES - Swagger UI</title>
  <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css">
  <style>body{margin:0;padding:0} .topbar{display:none}</style>
</head>
<body>
  <div id="swagger-ui"></div>
  <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js"></script>
  <script>
    SwaggerUIBundle({
      url: "/openapi.json",
      dom_id: '#swagger-ui',
      deepLinking: true,
      presets: [SwaggerUIBundle.presets.apis, SwaggerUIBundle.SwaggerUIStandalonePreset],
      layout: "BaseLayout"
    });
  </script>
</body>
</html>)HTML";

inline constexpr std::string_view REDOC_HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>HERMES - ReDoc</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>body{margin:0;padding:0}</style>
</head>
<body>
  <redoc spec-url="/openapi.json"></redoc>
  <script src="https://cdn.redoc.ly/redoc/latest/bundles/redoc.standalone.js"></script>
</body>
</html>)HTML";
