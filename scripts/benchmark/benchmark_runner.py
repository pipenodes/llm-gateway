#!/usr/bin/env python3
"""Helper for benchmark_sync.sh: resolve categories, detect agent models, build bodies."""
import json
import sys
import re

def main():
    if len(sys.argv) < 2:
        sys.exit(1)
    cmd = sys.argv[1]

    if cmd == "categories":
        mode = sys.argv[2].lower()
        with open(sys.argv[3]) as f:
            data = json.load(f)
        if mode == "full":
            out = [f"{g}_{s}" for g in ["general", "python", "agent"] for s in ["basic", "medium", "advanced"]]
        elif mode == "basic":
            out = ["general_basic", "python_basic", "agent_basic"]
        elif mode in ["general", "python", "agent"]:
            out = [f"{mode}_basic", f"{mode}_medium", f"{mode}_advanced"]
        elif mode in ["general_basic", "general_medium", "general_advanced",
                      "python_basic", "python_medium", "python_advanced",
                      "agent_basic", "agent_medium", "agent_advanced"]:
            out = [mode]
        else:
            sys.exit(1)
        print(" ".join(out))

    elif cmd == "agent_models":
        models_json = sys.stdin.read()
        keywords = re.compile(r"function|tool-use|tool_use|agent|agentcoder|groq-tool", re.I)
        fallback = ["functiongemma:latest", "llama3-groq-tool-use:8b", "qwen3-coder:latest"]
        try:
            data = json.loads(models_json)
            items = data.get("data", data.get("models", []))
            if isinstance(items, dict):
                items = list(items.values())
            names = []
            for m in items:
                if isinstance(m, dict):
                    names.append(m.get("id") or m.get("name") or m.get("model") or "")
                elif isinstance(m, str):
                    names.append(m)
            matched = [n for n in names if n and keywords.search(n)][:3]
            for f in fallback:
                if f not in matched and len(matched) < 3:
                    matched.append(f)
            result = [{"model": m, "provider": "ollama"} for m in matched[:3]]
        except Exception:
            result = [{"model": m, "provider": "ollama"} for m in fallback]
        print(json.dumps(result))

    elif cmd == "build_body":
        group, sub, prompts_file, tools_file, use_tools = sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6] == "1"
        with open(prompts_file) as f:
            prompts = json.load(f)[group][sub]
        models = json.loads(sys.argv[7])
        params = json.loads(sys.argv[8])
        evaluator = json.loads(sys.argv[9])
        body = {"models": models, "prompts": prompts, "params": params, "evaluator": evaluator}
        if use_tools:
            try:
                with open(tools_file) as f:
                    tools_data = json.load(f)
                if tools_data.get("tools"):
                    body["tools"] = tools_data["tools"]
                if tools_data.get("tool_choice") is not None:
                    body["tool_choice"] = tools_data["tool_choice"]
            except Exception:
                pass
        print(json.dumps(body))

    elif cmd == "merge":
        with open(sys.argv[2]) as f:
            all_results = json.load(f)
        resp_json = sys.stdin.read()
        cat = sys.argv[3]
        resp = json.loads(resp_json)
        for r in resp.get("results", []):
            r["category"] = cat
            all_results.append(r)
        print(json.dumps(all_results))

    elif cmd == "final":
        results_path, summary_json = sys.argv[2], sys.argv[3]
        with open(results_path) as f:
            results = json.load(f)
        summary = json.loads(summary_json)
        print(json.dumps({"status": "completed", "results": results, "summary": summary}))

    elif cmd == "save_md":
        data = json.loads(sys.stdin.read())
        out_path = sys.argv[2]
        ts, mode = sys.argv[3], sys.argv[4]
        with open(out_path, "w") as f:
            f.write("# Benchmark HERMES\n\n")
            f.write(f"**Data:** {ts}\n**Mode:** {mode}\n\n")
            s = data.get("summary", {})
            if s:
                f.write("## Resumo\n\n")
                f.write(f"- **Melhor modelo:** {s.get('best_model','N/A')}\n")
                f.write(f"- **Mais rapido:** {s.get('fastest_model','N/A')}\n")
                f.write(f"- **Maior qualidade:** {s.get('highest_quality_model','N/A')}\n")
                f.write(f"- **Maior throughput:** {s.get('highest_throughput_model','N/A')}\n\n")
            r = data.get("results", [])
            if r:
                f.write("## Resultados\n\n")
                f.write("| Modelo | Category | Prompt | Latencia (ms) | Tokens | Tokens/s | Score | Relevance | Coherence | Conciseness |\n")
                f.write("|--------|----------|--------|---------------|--------|----------|-------|----------|-----------|-------------|\n")
                for x in r:
                    tps = x.get("tokens_per_second", 0)
                    tps_s = f"{tps:.1f}" if isinstance(tps, (int, float)) else str(tps)
                    f.write(f"| {x.get('model','')} | {x.get('category','')} | {x.get('prompt_index',0)} | {x.get('latency_ms','')} | {x.get('tokens',0)} | {tps_s} | {x.get('score',0)} | {x.get('relevance',0)} | {x.get('coherence',0)} | {x.get('conciseness',0)} |\n")

    elif cmd == "summary":
        data = json.loads(sys.stdin.read())
        s = data.get("summary", {})
        if s:
            print(" Melhor modelo (geral):", s.get("best_model", "N/A"))
            print(" Mais rapido:", s.get("fastest_model", "N/A"))
            print(" Maior qualidade:", s.get("highest_quality_model", "N/A"))
            print(" Maior throughput (tokens/s):", s.get("highest_throughput_model", "N/A"))

    elif cmd == "save_csv":
        data = json.loads(sys.stdin.read())
        out_path = sys.argv[2]
        r = data.get("results", [])
        if r:
            fn = ["model", "provider", "category", "prompt_index", "latency_ms", "tokens", "tokens_per_second", "score", "relevance", "coherence", "conciseness", "reason"]
            import csv
            with open(out_path, "w", newline="") as f:
                w = csv.DictWriter(f, fieldnames=fn, extrasaction="ignore")
                w.writeheader()
                for x in r:
                    w.writerow({k: x.get(k, "") for k in fn})

if __name__ == "__main__":
    main()
