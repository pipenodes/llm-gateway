#include "score_utils.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace benchmark {

std::string normalizeForMatch(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) out += ' ';
        else if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-')
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    size_t start = 0;
    while (start < out.size() && out[start] == ' ') ++start;
    return out.substr(start);
}

bool exactMatch(const std::string& modelOutput, const ExpectedAnswer& expected) {
    if (!expected.has_expected || expected.exact.empty()) return false;
    std::string norm = normalizeForMatch(modelOutput);
    if (norm.empty()) return false;
    // Take first word or first 50 chars for comparison (model might add extra words).
    std::string firstPart = norm.substr(0, 50);
    std::istringstream is(firstPart);
    std::string firstWord;
    is >> firstWord;
    if (firstWord == expected.exact) return true;
    return norm.find(expected.exact) == 0 || firstPart.find(expected.exact) != std::string::npos;
}

void tokenize(const std::string& s, std::vector<std::string>& out) {
    out.clear();
    std::istringstream is(s);
    std::string t;
    while (is >> t) {
        std::string clean;
        for (char c : t)
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-')
                clean += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (!clean.empty()) out.push_back(clean);
    }
}

double f1Score(const std::string& modelOutput, const std::string& reference) {
    std::vector<std::string> pred, ref;
    tokenize(modelOutput, pred);
    tokenize(reference, ref);
    if (ref.empty()) return pred.empty() ? 1.0 : 0.0;
    std::set<std::string> refSet(ref.begin(), ref.end());
    int overlap = 0;
    for (const auto& t : pred)
        if (refSet.count(t)) ++overlap;
    double prec = pred.empty() ? 0 : static_cast<double>(overlap) / pred.size();
    double rec = static_cast<double>(overlap) / ref.size();
    if (prec + rec == 0) return 0.0;
    return 2.0 * prec * rec / (prec + rec);
}

static void ngrams(const std::vector<std::string>& tokens, int n, std::unordered_map<std::string, int>& out) {
    out.clear();
    for (size_t i = 0; i + n <= tokens.size(); ++i) {
        std::string ng;
        for (int k = 0; k < n; ++k) ng += (k ? " " : "") + tokens[k + i];
        out[ng]++;
    }
}

double simpleBleu(const std::string& modelOutput, const std::string& reference) {
    std::vector<std::string> pred, ref;
    tokenize(modelOutput, pred);
    tokenize(reference, ref);
    if (ref.empty()) return pred.empty() ? 1.0 : 0.0;
    double logProd = 0.0;
    int n = 0;
    for (int order = 1; order <= 4; ++order) {
        std::unordered_map<std::string, int> predNg, refNg;
        ngrams(pred, order, predNg);
        ngrams(ref, order, refNg);
        if (predNg.empty()) { logProd += -1e10; ++n; continue; }
        int match = 0, total = 0;
        for (const auto& p : predNg) {
            total += p.second;
            match += std::min(p.second, refNg[p.first]);
        }
        double prec = total ? static_cast<double>(match) / total : 0.0;
        logProd += std::log(prec + 1e-10);
        ++n;
    }
    return std::exp(logProd / n);
}

} // namespace benchmark
