#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <cstring>
#include <random>
#include <chrono>

namespace crypto {

// ── Epoch seconds (replaces std::time(nullptr)) ─────────────────────────

[[nodiscard]] inline int64_t epoch_seconds() noexcept {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// ── Constant-time comparison (prevents timing attacks) ──────────────────

[[nodiscard]] inline bool constant_time_eq(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    unsigned char result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        unsigned char diff = static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
        result = static_cast<unsigned char>(result | diff);
    }
    volatile unsigned char final_result = result;
    return final_result == 0;
}

// ── Secure random string (uses std::random_device = /dev/urandom) ───────

[[nodiscard]] inline std::string secure_random(size_t length, std::string_view charset) {
    std::random_device rd;
    std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i)
        result += charset[dist(rd)];
    return result;
}

// ── SHA-256 (FIPS 180-4, pure C++, no external deps) ────────────────────

namespace detail {

static constexpr uint32_t SHA256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline constexpr uint32_t rotr(uint32_t x, int n) noexcept { return (x >> n) | (x << (32 - n)); }
inline constexpr uint32_t ch(uint32_t x, uint32_t y, uint32_t z) noexcept { return (x & y) ^ (~x & z); }
inline constexpr uint32_t maj(uint32_t x, uint32_t y, uint32_t z) noexcept { return (x & y) ^ (x & z) ^ (y & z); }
inline constexpr uint32_t sig0(uint32_t x) noexcept { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline constexpr uint32_t sig1(uint32_t x) noexcept { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline constexpr uint32_t gam0(uint32_t x) noexcept { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline constexpr uint32_t gam1(uint32_t x) noexcept { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

struct SHA256State {
    uint32_t h[8]{};
    uint64_t total_len = 0;
    uint8_t buf[64]{};
    size_t buf_len = 0;

    SHA256State() noexcept {
        h[0] = 0x6a09e667; h[1] = 0xbb67ae85;
        h[2] = 0x3c6ef372; h[3] = 0xa54ff53a;
        h[4] = 0x510e527f; h[5] = 0x9b05688c;
        h[6] = 0x1f83d9ab; h[7] = 0x5be0cd19;
    }

    void transform(const uint8_t block[64]) noexcept {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t(block[4*i]) << 24) | (uint32_t(block[4*i+1]) << 16)
                  | (uint32_t(block[4*i+2]) << 8) | uint32_t(block[4*i+3]);
        for (int i = 16; i < 64; ++i)
            w[i] = gam1(w[i-2]) + w[i-7] + gam0(w[i-15]) + w[i-16];

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hv = h[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = hv + sig1(e) + ch(e, f, g) + SHA256_K[i] + w[i];
            uint32_t t2 = sig0(a) + maj(a, b, c);
            hv = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hv;
    }

    void update(const uint8_t* data, size_t len) noexcept {
        total_len += len;
        while (len > 0) {
            size_t space = 64 - buf_len;
            size_t n = (len < space) ? len : space;
            std::memcpy(buf + buf_len, data, n);
            buf_len += n;
            data += n;
            len -= n;
            if (buf_len == 64) {
                transform(buf);
                buf_len = 0;
            }
        }
    }

    void finalize(uint8_t hash[32]) noexcept {
        uint64_t bits = total_len * 8;
        uint8_t pad = 0x80;
        update(&pad, 1);
        pad = 0;
        while (buf_len != 56)
            update(&pad, 1);
        uint8_t len_bytes[8];
        for (int i = 7; i >= 0; --i) {
            len_bytes[i] = static_cast<uint8_t>(bits & 0xff);
            bits >>= 8;
        }
        update(len_bytes, 8);
        for (int i = 0; i < 8; ++i) {
            hash[4*i]   = static_cast<uint8_t>(h[i] >> 24);
            hash[4*i+1] = static_cast<uint8_t>(h[i] >> 16);
            hash[4*i+2] = static_cast<uint8_t>(h[i] >> 8);
            hash[4*i+3] = static_cast<uint8_t>(h[i]);
        }
    }
};

} // namespace detail

[[nodiscard]] inline std::string sha256_hex(std::string_view input) {
    detail::SHA256State ctx;
    ctx.update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    uint8_t hash[32];
    ctx.finalize(hash);
    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(64);
    for (int i = 0; i < 32; ++i) {
        result += hex[hash[i] >> 4];
        result += hex[hash[i] & 0x0f];
    }
    return result;
}

} // namespace crypto
