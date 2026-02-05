#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <string>

namespace Fig::StringClass::SizeFixed
{
    class String
    {
    private:
        static constexpr uint8_t SSO_SIZE = 32;
        static constexpr uint8_t SSO_MAX_ASCII_LEN = SSO_SIZE;
        static constexpr uint8_t SSO_MAX_UTF32_LEN = SSO_SIZE / 4;

        bool is_heap;
        bool is_ascii;
        uint64_t _length;

        union
        {
            struct
            {
                union
                {
                    unsigned char ascii[SSO_MAX_ASCII_LEN]; // 1 * 32 byte
                    char32_t utf32[SSO_MAX_UTF32_LEN];      // 4 * 8 byte
                };
            } sso;

            struct
            {
                union
                {
                    unsigned char *ascii;
                    char32_t *utf32;
                };
            } heap;
        };

        void destroy()
        {
            _length = 0;

            if (is_heap)
            {
                if (is_ascii) { delete[] heap.ascii; }
                else
                {
                    delete[] heap.utf32;
                }
            }
            // sso, pass
        }

        void alloc_heap(bool ascii, uint64_t size)
        {
            if (ascii) { heap.ascii = new unsigned char[size]; }
            else
            {
                heap.utf32 = new char32_t[size];
            }
        }

        void realloc_heap(bool ascii, uint64_t size)
        {
            destroy();

            if (ascii) { heap.ascii = new unsigned char[size]; }
            else
            {
                heap.utf32 = new char32_t[size];
            }
        }

        void copy(const String &other)
        {
            if (other.is_heap)
            {
                if (!is_heap)
                {
                    destroy();
                    is_heap = other.is_heap;
                }
                if (_length != other._length)
                {
                    realloc_heap(other.is_ascii, other._length);
                    _length = other._length;
                }
                is_ascii = other.is_ascii;
                if (is_ascii)
                {
                    for (uint64_t i = 0; i < _length; ++i) { heap.ascii[i] = other.heap.ascii[i]; }
                }
                else
                {
                    for (uint64_t i = 0; i < _length; ++i) { heap.utf32[i] = other.heap.utf32[i]; }
                }
            }
            else
            {
                // other is sso
                if (is_heap)
                {
                    destroy();
                    is_heap = other.is_heap;
                }
                is_ascii = other.is_ascii;
                _length = other._length;

                if (is_ascii)
                {
                    for (uint64_t i = 0; i < _length; ++i) { sso.ascii[i] = other.sso.ascii[i]; }
                }
                else
                {
                    for (uint64_t i = 0; i < _length; ++i) { sso.utf32[i] = other.sso.utf32[i]; }
                }
            }
        }

        void init(const char *cstr)
        {
            is_ascii = isPureASCII(cstr);

            if (is_ascii)
            {
                uint64_t len = strlen(cstr);
                _length = len;
                if (len <= SSO_MAX_ASCII_LEN)
                {
                    memcpy(sso.ascii, cstr, len);
                    is_heap = false;
                }
                else
                {
                    realloc_heap(true, len);
                    memcpy(heap.ascii, cstr, len);
                    is_heap = true;
                }
            }
            else
            {
                uint64_t len = fixed_strlen(cstr);
                _length = 0;
                if (len <= SSO_MAX_UTF32_LEN)
                {
                    for (size_t i = 0; cstr[i] != '\0';)
                    {
                        unsigned char c = static_cast<unsigned char>(cstr[i]);
                        if (c <= 0x7F)
                        {
                            sso.utf32[_length++] = static_cast<char32_t>(c);
                            ++i;
                        }
                        else if ((c & 0xE0) == 0xC0)
                        {
                            char32_t result = ((c & 0x1F) << 6) | (cstr[i + 1] & 0x3F);
                            sso.utf32[_length++] = result;
                            i += 2;
                        }
                        else if ((c & 0xF0) == 0xE0)
                        {
                            char32_t result = ((c & 0x1F) << 12) | ((cstr[i + 1] & 0x3F) << 6) | ((cstr[i + 2] & 0x3F));
                            sso.utf32[_length++] = result;
                            i += 3;
                        }
                        else if ((c & 0xF8) == 0xF0)
                        {
                            char32_t result = ((c & 0x07) << 18) | ((cstr[i + 1] & 0x3F) << 12)
                                              | ((cstr[i + 2] & 0x3F) << 6) | (cstr[i + 3] & 0x3F);
                            sso.utf32[_length++] = result;
                            i += 4;
                        }
                        else
                        {
                            assert(false && "bad utf8 string");
                        }
                    }
                    is_heap = false;
                }
                else
                {
                    realloc_heap(false, len);
                    _length = 0;
                    for (size_t i = 0; cstr[i] != '\0';)
                    {
                        unsigned char c = static_cast<unsigned char>(cstr[i]);
                        if (c <= 0x7F)
                        {
                            heap.utf32[_length++] = static_cast<char32_t>(c);
                            ++i;
                        }
                        else if ((c & 0xE0) == 0xC0)
                        {
                            char32_t result = ((c & 0x1F) << 6) | (cstr[i + 1] & 0x3F);
                            heap.utf32[_length++] = result;
                            i += 2;
                        }
                        else if ((c & 0xF0) == 0xE0)
                        {
                            char32_t result = ((c & 0x1F) << 12) | ((cstr[i + 1] & 0x3F) << 6) | ((cstr[i + 2] & 0x3F));
                            heap.utf32[_length++] = result;
                            i += 3;
                        }
                        else if ((c & 0xF8) == 0xF0)
                        {
                            char32_t result = ((c & 0x07) << 18) | ((cstr[i + 1] & 0x3F) << 12)
                                              | ((cstr[i + 2] & 0x3F) << 6) | (cstr[i + 3] & 0x3F);
                            heap.utf32[_length++] = result;
                            i += 4;
                        }
                        else
                        {
                            assert(false && "bad utf8 string");
                        }
                    }
                    is_heap = true;
                }
            }
        }

        void init(const char32_t *str)
        {
            assert(str);

            uint64_t len = 0;
            bool ascii = true;

            for (const char32_t *p = str; *p != U'\0'; ++p)
            {
                if (*p > 0x7F) ascii = false;
                ++len;
            }

            _length = len;
            is_ascii = ascii;

            if (ascii)
            {
                if (len <= SSO_MAX_ASCII_LEN)
                {
                    is_heap = false;
                    for (uint64_t i = 0; i < len; ++i) sso.ascii[i] = static_cast<unsigned char>(str[i]);
                }
                else
                {
                    is_heap = true;
                    realloc_heap(true, len);
                    for (uint64_t i = 0; i < len; ++i) heap.ascii[i] = static_cast<unsigned char>(str[i]);
                }
            }
            else
            {
                if (len <= SSO_MAX_UTF32_LEN)
                {
                    is_heap = false;
                    for (uint64_t i = 0; i < len; ++i) sso.utf32[i] = str[i];
                }
                else
                {
                    is_heap = true;
                    realloc_heap(false, len);
                    for (uint64_t i = 0; i < len; ++i) heap.utf32[i] = str[i];
                }
            }
        }

        void init(const std::string &str) { init(str.c_str()); }

        void promote_sso_to_heap(uint64_t len)
        {
            if (is_heap) return;

            is_heap = true;

            if (is_ascii)
            {
                heap.ascii = new unsigned char[len];
                memcpy(heap.ascii, sso.ascii, _length);
            }
            else
            {
                heap.utf32 = new char32_t[len];
                memcpy(heap.utf32, sso.utf32, sizeof(char32_t) * _length);
            }
            _length = len;
        }

        void move_sso_ascii_to_sso_utf()
        {
            // current: sso, ascii
            // after: sso, utf32

            assert(is_ascii);
            assert(!is_heap);

            is_ascii = false;

            for (uint8_t i = 0; i < _length; ++i) { sso.utf32[i] = static_cast<char32_t>(sso.ascii[i]); }
        }

        void move_heap_ascii_to_heap_utf()
        {
            // current: heap, ascii
            // after: heap, utf32

            assert(is_heap);
            assert(is_ascii);

            is_ascii = false;

            unsigned char *buf = new unsigned char[_length];
            memcpy(buf, heap.ascii, _length);

            realloc_heap(false, _length); // destroy + alloc

            for (uint64_t i = 0; i < _length; ++i) { heap.utf32[i] = static_cast<char32_t>(buf[i]); }
        }

        void move_sso_ascii_to_heap_utf()
        {
            // current: sso, ascii
            // after: heap, utf32

            assert(!is_heap);
            assert(is_ascii);

            is_heap = true;
            is_ascii = false;

            heap.utf32 = new char32_t[_length];

            for (uint64_t i = 0; i < _length; ++i) { heap.utf32[i] = static_cast<char32_t>(sso.ascii[i]); }
        }

    public:
        static bool isPureASCII(const char *cstr)
        {
            for (size_t i = 0; cstr[i] != '\0'; ++i)
            {
                // 最高位是否为0
                //                                     0b10000000
                if (static_cast<unsigned char>(cstr[i]) & 0x80) { return false; }
            }
            return true;
        }

        static uint64_t fixed_strlen(const char *cstr) // must pass a correct utf format str
        {
            // 0b1000 0000 --> 1
            // 0b1100 0000 --> 2
            // 0b1110 0000 --> 3
            // 0b1111 0000 --> 4
            uint64_t cnt = 0;
            for (size_t i = 0; cstr[i] != '\0'; ++i)
            {
                unsigned char c = static_cast<unsigned char>(cstr[i]);
                if ((c & 0xC0) != 0x80) { ++cnt; }
            }
            return cnt;
        }

        static char *decodeUTF32String(const char32_t *str, uint64_t len)
        {
            if (!str) return nullptr;

            size_t utf8_len = 0;
            for (uint64_t i = 0; i < len; ++i)
            {
                char32_t c = str[i];
                if (c <= 0x7F)
                    utf8_len += 1;
                else if (c <= 0x7FF)
                    utf8_len += 2;
                else if (c <= 0xFFFF)
                    utf8_len += 3;
                else if (c <= 0x10FFFF)
                    utf8_len += 4;
                else
                    assert(false && "invalid UTF-32 code point");
            }

            char *result = new char[utf8_len + 1];
            char *dest = result;

            for (uint64_t i = 0; i < len; ++i)
            {
                char32_t c = str[i];
                if (c <= 0x7F) { *dest++ = static_cast<char>(c); }
                else if (c <= 0x7FF)
                {
                    *dest++ = static_cast<char>(0xC0 | ((c >> 6) & 0x1F));
                    *dest++ = static_cast<char>(0x80 | (c & 0x3F));
                }
                else if (c <= 0xFFFF)
                {
                    *dest++ = static_cast<char>(0xE0 | ((c >> 12) & 0x0F));
                    *dest++ = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                    *dest++ = static_cast<char>(0x80 | (c & 0x3F));
                }
                else
                {
                    *dest++ = static_cast<char>(0xF0 | ((c >> 18) & 0x07));
                    *dest++ = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
                    *dest++ = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                    *dest++ = static_cast<char>(0x80 | (c & 0x3F));
                }
            }

            *dest = '\0';
            return result;
        }

        static char32_t UTF8BytesToUTF32CodePoint(const char *str, uint64_t len)
        {
            // unsafe, str must be valid, otherwise Access Violation Memory
            if (!str) return char32_t();

            assert(len <= 4);

            unsigned char c = str[0];
            if (len == 1) { return static_cast<char32_t>(c); }

            if (len == 2) { return ((c & 0x1F) << 6) | (str[1] & 0x3F); }
            if (len == 3) { return ((c & 0x1F) << 12) | ((str[1] & 0x3F) << 6) | ((str[2] & 0x3F)); }
            if (len == 4)
            {
                return ((c & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
            }
            assert(false && "invalid len");
        }

        const char *toCString() const
        {
            char *buf = new char[_length + 1];

            if (is_ascii)
            {
                if (is_heap)
                    memcpy(buf, heap.ascii, _length);
                else
                    memcpy(buf, sso.ascii, _length);
            }
            else
            {
                char *tmp = is_heap ? decodeUTF32String(heap.utf32, _length) : decodeUTF32String(sso.utf32, _length);

                memcpy(buf, tmp, strlen(tmp));
                delete[] tmp;
            }

            buf[_length] = '\0';
            return buf;
        }

        char *toCString()
        {
            char *n_c = new char[_length + 1];

            if (is_ascii)
            {
                if (is_heap)
                    memcpy(n_c, heap.ascii, _length);
                else
                    memcpy(n_c, sso.ascii, _length);

                n_c[_length] = '\0';
                return n_c;
            }

            else
            {
                if (is_heap) { return decodeUTF32String(heap.utf32, _length); }
                else
                {
                    return decodeUTF32String(sso.utf32, _length);
                }
            }
        }

        std::string toBasicString() const
        {
            if (is_ascii) return std::string(toCString());

            char *tmp = is_heap ? decodeUTF32String(heap.utf32, _length) : decodeUTF32String(sso.utf32, _length);

            std::string s(tmp);
            delete[] tmp;
            return s;
        }

        bool empty() const noexcept { return _length == 0; }
        uint64_t size() const noexcept { return _length; } // c++ api
        uint64_t length() const noexcept { return _length; }
        bool isAscii() const noexcept { return is_ascii; }
        bool isOnHeap() const noexcept { return is_heap; }

        void clear() noexcept { _length = 0; }

        bool operator==(const String &other) const
        {
            if (this == &other) return true;
            if (_length != other._length) return false;

            // ASCII == ASCII
            if (is_ascii && other.is_ascii)
            {
                if (is_heap && other.is_heap) return std::memcmp(heap.ascii, other.heap.ascii, _length) == 0;
                if (!is_heap && !other.is_heap) return std::memcmp(sso.ascii, other.sso.ascii, _length) == 0;

                const unsigned char *l = is_heap ? heap.ascii : sso.ascii;
                const unsigned char *r = other.is_heap ? other.heap.ascii : other.sso.ascii;
                return std::memcmp(l, r, _length) == 0;
            }

            // codepoint <--> codepoint
            for (uint64_t i = 0; i < _length; ++i)
            {
                if ((*this)[i] != other[i]) return false;
            }

            return true;
        }

        char32_t operator[](uint64_t idx) const
        {
            if (is_ascii)
            {
                if (is_heap) { return static_cast<char32_t>(heap.ascii[idx]); }
                else
                {
                    return static_cast<char32_t>(sso.ascii[idx]);
                }
            }
            else
            {
                if (is_heap) { return heap.utf32[idx]; }
                else
                {
                    return sso.utf32[idx];
                }
            }
        }

        char32_t at(uint64_t idx) const
        {
            if (idx >= _length) { throw std::runtime_error("index out of string range"); }
            return operator[](idx);
        }

        void set(uint64_t idx, char32_t codepoint)
        {
            if (idx >= _length) throw std::runtime_error("index out of string range");

            if (is_ascii)
            {
                if (codepoint <= 0x7F)
                {
                    unsigned char *dest = is_heap ? heap.ascii : sso.ascii;
                    dest[idx] = static_cast<unsigned char>(codepoint);
                    return;
                }

                // ASCII -> UTF32 (Heap)
                if (is_heap) { move_heap_ascii_to_heap_utf(); }
                else
                {
                    move_sso_ascii_to_heap_utf();
                }

                heap.utf32[idx] = codepoint;
            }
            else
            {
                char32_t *dest = is_heap ? heap.utf32 : sso.utf32;
                dest[idx] = codepoint;
            }
        }

        String operator+(const String &other) const
        {
            /*
            ASCII + ASCII --> ASCII (SSO/HEAP)
            UTF32 + UTF32 --> UTF32 (HEAP)

            ASCII + UTF32 --> UTF32 (HEAP)
            UTF32 + ASCII        ⬆
            */
            uint64_t nlen = _length + other._length;
            if (nlen == _length) return *this; // other._length = 0

            if (is_ascii && other.is_ascii)
            {
                const unsigned char *ld = is_heap ? heap.ascii : sso.ascii;
                const unsigned char *rd = other.is_heap ? other.heap.ascii : other.sso.ascii;

                String result;
                if (nlen <= SSO_MAX_ASCII_LEN)
                {
                    memcpy(result.sso.ascii, ld, _length);
                    memcpy(result.sso.ascii + _length, rd, other._length);

                    result.is_ascii = true;
                    result.is_heap = false;
                    result._length = nlen;
                }
                else
                {
                    result.heap.ascii = new unsigned char[nlen];
                    memcpy(result.heap.ascii, ld, _length);
                    memcpy(result.heap.ascii + _length, rd, other._length);

                    result.is_ascii = true;
                    result.is_heap = true;
                    result._length = nlen;
                }
                return result;
            }
            if (!is_ascii && !other.is_ascii)
            {
                // both utf32 mode
                const char32_t *ld = (is_heap ? heap.utf32 : sso.utf32);
                const char32_t *rd = (other.is_heap ? other.heap.utf32 : other.sso.utf32);

                String result; // UTF32 concatenation always uses heap

                result.heap.utf32 = new char32_t[nlen];

                result.is_heap = true;
                result.is_ascii = false;
                result._length = nlen;

                memcpy(result.heap.utf32, ld, sizeof(char32_t) * _length);
                memcpy(result.heap.utf32 + _length, rd, sizeof(char32_t) * other._length);
                return result;
            }

            String result;
            result.heap.utf32 = new char32_t[nlen]; // UTF32 concatenation always uses heap

            result.is_ascii = false;
            result.is_heap = true;
            result._length = nlen;

            if (is_ascii)
            {
                // other is utf32 mode
                const unsigned char *ld = (is_heap ? heap.ascii : sso.ascii);

                for (uint64_t i = 0; i < _length; ++i) { result.heap.utf32[i] = static_cast<char32_t>(ld[i]); }

                const char32_t *rd = (other.is_heap ? other.heap.utf32 : other.sso.utf32);
                memcpy(result.heap.utf32 + _length, rd, sizeof(char32_t) * other._length);
            }
            else
            {
                // other is ascii mode
                // (this) is utf32 mode

                const char32_t *ld = (is_heap ? heap.utf32 : sso.utf32);

                memcpy(result.heap.utf32, ld, sizeof(char32_t) * _length);
                const unsigned char *rd = (other.is_heap ? other.heap.ascii : other.sso.ascii);
                for (uint64_t i = 0; i < other._length; ++i)
                {
                    result.heap.utf32[_length + i] = static_cast<char32_t>(rd[i]);
                }
            }
            return result;
        }

        String &operator+=(const String &other)
        {
            if (other._length == 0) return *this;

            if (is_ascii && other.is_ascii)
            {
                uint64_t nlen = _length + other._length;

                const unsigned char *rd = other.is_heap ? other.heap.ascii : other.sso.ascii;

                if (!is_heap && nlen <= SSO_MAX_ASCII_LEN)
                {
                    // SSO
                    memcpy(sso.ascii + _length, rd, other._length);
                }
                else
                {
                    // heap
                    unsigned char *old = is_heap ? heap.ascii : nullptr;

                    unsigned char *buf = new unsigned char[nlen];

                    if (is_heap)
                        memcpy(buf, old, _length);
                    else
                        memcpy(buf, sso.ascii, _length);

                    memcpy(buf + _length, rd, other._length);

                    if (is_heap) delete[] old;

                    heap.ascii = buf;
                    is_heap = true;
                }

                _length = nlen;
                return *this;
            }

            // UTF32 + UTF32 / ASCII + UTF32 / UTF32 + ASCII
            uint64_t nlen = _length + other._length;

            char32_t *buf = new char32_t[nlen];

            // copy lhs
            if (is_ascii)
            {
                const unsigned char *ld = is_heap ? heap.ascii : sso.ascii;
                for (uint64_t i = 0; i < _length; ++i) buf[i] = static_cast<char32_t>(ld[i]);
            }
            else
            {
                const char32_t *ld = is_heap ? heap.utf32 : sso.utf32;
                memcpy(buf, ld, sizeof(char32_t) * _length);
            }

            // copy rhs
            if (other.is_ascii)
            {
                const unsigned char *rd = other.is_heap ? other.heap.ascii : other.sso.ascii;
                for (uint64_t i = 0; i < other._length; ++i) buf[_length + i] = static_cast<char32_t>(rd[i]);
            }
            else
            {
                const char32_t *rd = other.is_heap ? other.heap.utf32 : other.sso.utf32;
                memcpy(buf + _length, rd, sizeof(char32_t) * other._length);
            }

            // destroy old storage
            destroy();

            heap.utf32 = buf;
            is_ascii = false;
            is_heap = true;
            _length = nlen;

            return *this;
        }

        String(String &&other) noexcept
        {
            is_ascii = other.is_ascii;
            is_heap = other.is_heap;
            _length = other._length;

            if (other.is_heap)
            {
                if (other.is_ascii)
                {
                    heap.ascii = other.heap.ascii;
                    other.heap.ascii = nullptr;
                }
                else
                {
                    heap.utf32 = other.heap.utf32;
                    other.heap.utf32 = nullptr;
                }
            }
            else
            {
                if (other.is_ascii)
                {
                    // SSO
                    memcpy(sso.ascii, other.sso.ascii, _length);
                }
                else
                {
                    memcpy(sso.utf32, other.sso.utf32, sizeof(char32_t) * _length);
                }
            }

            other._length = 0;
            other.is_heap = false;
            other.is_ascii = true; // set to default mode
        }

        String &operator=(const String &other)
        {
            copy(other);
            return *this;
        }

        String &operator=(String &&other) noexcept
        {
            if (this == &other) return *this;

            destroy(); // cleanup current

            is_ascii = other.is_ascii;
            is_heap = other.is_heap;
            _length = other._length;

            if (other.is_heap)
            {
                if (other.is_ascii)
                {
                    heap.ascii = other.heap.ascii;
                    other.heap.ascii = nullptr;
                }
                else
                {
                    heap.utf32 = other.heap.utf32;
                    other.heap.utf32 = nullptr;
                }
            }
            else
            {
                if (other.is_ascii)
                {
                    // SSO
                    memcpy(sso.ascii, other.sso.ascii, _length);
                }
                else
                {
                    memcpy(sso.utf32, other.sso.utf32, sizeof(char32_t) * _length);
                }
            }

            other._length = 0;
            other.is_heap = false;
            other.is_ascii = true; // set to default mode

            return *this;
        }

        String()
        {
            is_heap = false;
            is_ascii = true;
        }

        String(const String &str) { copy(str); }

        String(const char *cstr) { init(cstr); }

        String(const std::string &str) { init(str); }

        String(const char32_t *u32str) { init(u32str); }

        String(char32_t codepoint)
        {
            char32_t *str = new char32_t[2];
            str[0] = codepoint;
            str[1] = U'\0';
            init(str);
            delete[] str;
        }

        ~String() { destroy(); }
    };
}; // namespace Fig::StringClass::SizeFixed

namespace Fig::StringClass::DynamicCapacity
{
    class String
    {
    private:
        static constexpr uint8_t SSO_SIZE = 32;
        static constexpr uint8_t SSO_MAX_ASCII_LEN = SSO_SIZE;
        static constexpr uint8_t SSO_MAX_UTF32_LEN = SSO_SIZE / 4;
        static constexpr uint8_t DEFAULT_CAPACITY = SSO_MAX_ASCII_LEN;
        static constexpr uint64_t HEAP_GROW_FACTOR = 2;

        bool is_heap;
        bool is_ascii;
        uint64_t _length;
        uint64_t _capacity; // Only used in heap mode

        struct HeapBuffer
        {
            unsigned char *ascii = nullptr;
            char32_t *utf32 = nullptr;

            HeapBuffer() = default;

            // no copy!
            HeapBuffer(const HeapBuffer &) = delete;
            HeapBuffer &operator=(const HeapBuffer &) = delete;

            // move is ok
            HeapBuffer(HeapBuffer &&other) noexcept : ascii(other.ascii), utf32(other.utf32)
            {
                other.ascii = nullptr;
                other.utf32 = nullptr;
            }

            HeapBuffer &operator=(HeapBuffer &&other) noexcept
            {
                if (this != &other)
                {
                    delete[] ascii;
                    delete[] utf32;
                    ascii = other.ascii;
                    utf32 = other.utf32;
                    other.ascii = nullptr;
                    other.utf32 = nullptr;
                }
                return *this;
            }

            ~HeapBuffer()
            {
                delete[] ascii;
                delete[] utf32;
            }
        };

        union
        {
            struct
            {
                union
                {
                    unsigned char ascii[SSO_MAX_ASCII_LEN];
                    char32_t utf32[SSO_MAX_UTF32_LEN];
                };
            } sso;

            HeapBuffer heap;
        };

        void destroy_heap_memory()
        {
            if (is_heap)
            {
                if (is_ascii)
                    delete[] heap.ascii;
                else
                    delete[] heap.utf32;
            }
        }

        void destroy()
        {
            destroy_heap_memory();
            _length = 0;
            _capacity = 0;
            is_heap = false;
            is_ascii = true;
        }

        uint64_t calculate_growth_capacity(uint64_t min_capacity) const
        {
            uint64_t new_capacity = _capacity > 0 ? _capacity : DEFAULT_CAPACITY;
            while (new_capacity < min_capacity) new_capacity = (new_capacity + 1) * HEAP_GROW_FACTOR;
            return new_capacity;
        }

        void ensure_capacity(uint64_t required_capacity)
        {
            if (is_heap)
            {
                if (required_capacity <= _capacity) return;

                uint64_t new_capacity = calculate_growth_capacity(required_capacity);
                if (is_ascii)
                {
                    unsigned char *new_buffer = new unsigned char[new_capacity];
                    std::memcpy(new_buffer, heap.ascii, _length);
                    delete[] heap.ascii;
                    heap.ascii = new_buffer;
                }
                else
                {
                    char32_t *new_buffer = new char32_t[new_capacity];
                    std::memcpy(new_buffer, heap.utf32, _length * sizeof(char32_t));
                    delete[] heap.utf32;
                    heap.utf32 = new_buffer;
                }
                _capacity = new_capacity;
            }
            else
            {
                uint64_t max_sso_capacity = is_ascii ? SSO_MAX_ASCII_LEN : SSO_MAX_UTF32_LEN;
                if (required_capacity > max_sso_capacity)
                {
                    uint64_t new_capacity = calculate_growth_capacity(required_capacity);
                    if (is_ascii)
                    {
                        unsigned char *new_buffer = new unsigned char[new_capacity];
                        std::memcpy(new_buffer, sso.ascii, _length);
                        heap.ascii = new_buffer;
                    }
                    else
                    {
                        char32_t *new_buffer = new char32_t[new_capacity];
                        std::memcpy(new_buffer, sso.utf32, _length * sizeof(char32_t));
                        heap.utf32 = new_buffer;
                    }
                    is_heap = true;
                    _capacity = new_capacity;
                }
            }
        }

        void copy_from(const String &other)
        {
            destroy();
            is_ascii = other.is_ascii;
            _length = other._length;
            is_heap = other.is_heap;
            _capacity = other._capacity;

            if (is_heap)
            {
                if (is_ascii)
                {
                    heap.ascii = new unsigned char[_capacity];
                    std::memcpy(heap.ascii, other.is_heap ? other.heap.ascii : other.sso.ascii, _length);
                }
                else
                {
                    heap.utf32 = new char32_t[_capacity];
                    std::memcpy(
                        heap.utf32, other.is_heap ? other.heap.utf32 : other.sso.utf32, _length * sizeof(char32_t));
                }
            }
            else
            {
                if (is_ascii)
                    std::memcpy(sso.ascii, other.sso.ascii, _length);
                else
                    std::memcpy(sso.utf32, other.sso.utf32, _length * sizeof(char32_t));
            }
        }

        void move_from(String &&other) noexcept
        {
            destroy();
            is_ascii = other.is_ascii;
            _length = other._length;
            is_heap = other.is_heap;
            _capacity = other._capacity;

            if (is_heap)
            {
                heap = std::move(other.heap);
            }
            else
            {
                if (!is_ascii)
                    std::memcpy(sso.utf32, other.sso.utf32, _length * sizeof(char32_t));
                else
                    std::memcpy(sso.ascii, other.sso.ascii, _length);
            }

            other.destroy();
        }

        void convert_to_utf32_mode()
        {
            if (!is_ascii) return;
            uint64_t len = _length;

            if (is_heap)
            {
                unsigned char *old = heap.ascii;
                heap.utf32 = new char32_t[_capacity];
                for (uint64_t i = 0; i < len; ++i) heap.utf32[i] = old[i];
                delete[] old;
            }
            else
            {
                char32_t tmp[SSO_MAX_UTF32_LEN];
                for (uint64_t i = 0; i < len; ++i) tmp[i] = sso.ascii[i];
                std::memcpy(sso.utf32, tmp, len * sizeof(char32_t));
            }
            is_ascii = false;
        }

        void init_from_cstr(const char *cstr)
        {
            assert(cstr);
            is_ascii = isPureASCII(cstr);
            uint64_t len = std::strlen(cstr);
            _length = len;

            if (is_ascii)
            {
                if (len <= SSO_MAX_ASCII_LEN)
                {
                    is_heap = false;
                    _capacity = 0;
                    std::memcpy(sso.ascii, cstr, len);
                }
                else
                {
                    is_heap = true;
                    _capacity = len;
                    heap.ascii = new unsigned char[_capacity];
                    std::memcpy(heap.ascii, cstr, len);
                }
            }
            else
            {
                uint64_t utf32_len = fixed_strlen(cstr);
                _length = utf32_len;

                if (utf32_len <= SSO_MAX_UTF32_LEN)
                {
                    is_heap = false;
                    _capacity = 0;
                    uint64_t idx = 0;
                    for (size_t i = 0; cstr[i];)
                    {
                        unsigned char c = cstr[i];
                        if (c <= 0x7F)
                            sso.utf32[idx++] = c, ++i;
                        else if ((c & 0xE0) == 0xC0)
                            sso.utf32[idx++] = ((c & 0x1F) << 6) | (cstr[i + 1] & 0x3F), i += 2;
                        else if ((c & 0xF0) == 0xE0)
                            sso.utf32[idx++] = ((c & 0x0F) << 12) | ((cstr[i + 1] & 0x3F) << 6) | (cstr[i + 2] & 0x3F),
                            i += 3;
                        else if ((c & 0xF8) == 0xF0)
                            sso.utf32[idx++] = ((c & 0x07) << 18) | ((cstr[i + 1] & 0x3F) << 12)
                                               | ((cstr[i + 2] & 0x3F) << 6) | (cstr[i + 3] & 0x3F),
                            i += 4;
                        else
                            assert(false && "Invalid UTF-8");
                    }
                }
                else
                {
                    is_heap = true;
                    _capacity = utf32_len;
                    heap.utf32 = new char32_t[_capacity];
                    uint64_t idx = 0;
                    for (size_t i = 0; cstr[i];)
                    {
                        unsigned char c = cstr[i];
                        if (c <= 0x7F)
                            heap.utf32[idx++] = c, ++i;
                        else if ((c & 0xE0) == 0xC0)
                            heap.utf32[idx++] = ((c & 0x1F) << 6) | (cstr[i + 1] & 0x3F), i += 2;
                        else if ((c & 0xF0) == 0xE0)
                            heap.utf32[idx++] = ((c & 0x0F) << 12) | ((cstr[i + 1] & 0x3F) << 6) | (cstr[i + 2] & 0x3F),
                            i += 3;
                        else if ((c & 0xF8) == 0xF0)
                            heap.utf32[idx++] = ((c & 0x07) << 18) | ((cstr[i + 1] & 0x3F) << 12)
                                                | ((cstr[i + 2] & 0x3F) << 6) | (cstr[i + 3] & 0x3F),
                            i += 4;
                        else
                            assert(false && "Invalid UTF-8");
                    }
                }
            }
        }

        void init_from_u32_str(const char32_t *utf32str)
        {
            assert(utf32str);
            is_ascii = false;

            // 计算长度
            _length = 0;
            while (utf32str[_length] != U'\0') ++_length;

            if (_length <= SSO_MAX_UTF32_LEN)
            {
                is_heap = false;
                _capacity = 0;
                std::memcpy(sso.utf32, utf32str, _length * sizeof(char32_t));
            }
            else
            {
                is_heap = true;
                _capacity = _length;
                heap.utf32 = new char32_t[_capacity];
                std::memcpy(heap.utf32, utf32str, _length * sizeof(char32_t));
            }
        }

    public:
        // Utilities
        static bool isPureASCII(const char *cstr)
        {
            for (size_t i = 0; cstr[i]; ++i)
                if (static_cast<unsigned char>(cstr[i]) & 0x80) return false;
            return true;
        }

        static uint64_t fixed_strlen(const char *cstr)
        {
            uint64_t cnt = 0;
            for (size_t i = 0; cstr[i]; ++i)
                if ((cstr[i] & 0xC0) != 0x80) ++cnt;
            return cnt;
        }

        // Constructors / destructors
        String() : is_heap(false), is_ascii(true), _length(0), _capacity(0) {}
        String(const std::string &str) { init_from_cstr(str.c_str()); }
        String(const char *cstr) { init_from_cstr(cstr); }
        String(const String &other) { copy_from(other); }
        String(String &&other) noexcept { move_from(std::move(other)); }
        String(const char32_t *utf32str) { init_from_u32_str(utf32str); }
        String(char32_t codepoint)
        {
            char32_t u32_str[] = {codepoint, U'\0'};
            init_from_u32_str(u32_str);
        }

        ~String() { destroy(); }

        // Assignment
        String &operator=(const String &other)
        {
            if (this != &other) copy_from(other);
            return *this;
        }
        String &operator=(String &&other) noexcept
        {
            if (this != &other) move_from(std::move(other));
            return *this;
        }

        // Access
        char32_t operator[](uint64_t idx) const
        {
            assert(idx < _length);
            if (is_ascii) return is_heap ? heap.ascii[idx] : sso.ascii[idx];
            return is_heap ? heap.utf32[idx] : sso.utf32[idx];
        }

        char32_t at(uint64_t idx) const
        {
            if (idx >= _length) throw std::out_of_range("String index out of range");

            if (is_ascii) return is_heap ? heap.ascii[idx] : sso.ascii[idx];
            return is_heap ? heap.utf32[idx] : sso.utf32[idx];
        }

        void set(uint64_t idx, char32_t c)
        {
            assert(idx < _length && "Index out of bounds");

            if (is_ascii)
            {
                if (c <= 0x7F)
                {
                    // ASCII
                    if (is_heap)
                        heap.ascii[idx] = static_cast<unsigned char>(c);
                    else
                        sso.ascii[idx] = static_cast<unsigned char>(c);
                }
                else
                {
                    // conv to UTF-32
                    convert_to_utf32_mode();
                    if (is_heap)
                        heap.utf32[idx] = c;
                    else
                        sso.utf32[idx] = c;
                }
            }
            else
            {
                // UTF-32
                if (is_heap)
                    heap.utf32[idx] = c;
                else
                    sso.utf32[idx] = c;
            }
        }

        uint64_t capacity() const noexcept { return _capacity; }
        uint64_t length() const noexcept { return _length; }
        bool empty() const noexcept { return _length == 0; }
        bool isAscii() const noexcept { return is_ascii; }
        bool isOnHeap() const noexcept { return is_heap; }

        // Append
        String &operator+=(const String &other)
        {
            if (other._length == 0) return *this;
            uint64_t new_length = _length + other._length;

            if (is_ascii && other.is_ascii)
            {
                ensure_capacity(new_length);
                const unsigned char *src = other.is_heap ? other.heap.ascii : other.sso.ascii;
                if (is_heap)
                    std::memcpy(heap.ascii + _length, src, other._length);
                else
                    std::memcpy(sso.ascii + _length, src, other._length);
                _length = new_length;
                return *this;
            }

            // Mixed or UTF-32
            if (is_ascii) convert_to_utf32_mode();
            if (other.is_ascii)
            {
                ensure_capacity(new_length);
                const unsigned char *src = other.is_heap ? other.heap.ascii : other.sso.ascii;
                for (uint64_t i = 0; i < other._length; ++i)
                {
                    if (is_heap)
                        heap.utf32[_length + i] = src[i];
                    else
                        sso.utf32[_length + i] = src[i];
                }
            }
            else
            {
                ensure_capacity(new_length);
                const char32_t *src = other.is_heap ? other.heap.utf32 : other.sso.utf32;
                if (is_heap)
                    std::memcpy(heap.utf32 + _length, src, other._length * sizeof(char32_t));
                else
                    std::memcpy(sso.utf32 + _length, src, other._length * sizeof(char32_t));
            }

            _length = new_length;
            return *this;
        }

        String operator+(const String &other) const
        {
            String res = *this;
            res += other;
            return res;
        }

        // Conversion
        char *toCString() const
        {
            if (is_ascii)
            {
                char *buf = new char[_length + 1];
                if (is_heap)
                    std::memcpy(buf, heap.ascii, _length);
                else
                    std::memcpy(buf, sso.ascii, _length);
                buf[_length] = '\0';
                return buf;
            }
            else
            {
                return is_heap ? decodeUTF32String(heap.utf32, _length) : decodeUTF32String(sso.utf32, _length);
            }
        }

        std::string toBasicString() const
        {
            if (is_ascii) return std::string(reinterpret_cast<const char *>(is_heap ? heap.ascii : sso.ascii), _length);
            char *tmp = is_heap ? decodeUTF32String(heap.utf32, _length) : decodeUTF32String(sso.utf32, _length);
            std::string res(tmp);
            delete[] tmp;
            return res;
        }

        static char *decodeUTF32String(const char32_t *str, uint64_t len)
        {
            if (!str) return nullptr;
            size_t utf8_len = 0;
            for (uint64_t i = 0; i < len; ++i)
            {
                char32_t c = str[i];
                if (c <= 0x7F)
                    utf8_len += 1;
                else if (c <= 0x7FF)
                    utf8_len += 2;
                else if (c <= 0xFFFF)
                    utf8_len += 3;
                else
                    utf8_len += 4;
            }
            char *buf = new char[utf8_len + 1];
            char *dest = buf;
            for (uint64_t i = 0; i < len; ++i)
            {
                char32_t c = str[i];
                if (c <= 0x7F)
                    *dest++ = c;
                else if (c <= 0x7FF)
                {
                    *dest++ = 0xC0 | (c >> 6);
                    *dest++ = 0x80 | (c & 0x3F);
                }
                else if (c <= 0xFFFF)
                {
                    *dest++ = 0xE0 | (c >> 12);
                    *dest++ = 0x80 | ((c >> 6) & 0x3F);
                    *dest++ = 0x80 | (c & 0x3F);
                }
                else
                {
                    *dest++ = 0xF0 | (c >> 18);
                    *dest++ = 0x80 | ((c >> 12) & 0x3F);
                    *dest++ = 0x80 | ((c >> 6) & 0x3F);
                    *dest++ = 0x80 | (c & 0x3F);
                }
            }
            *dest = '\0';
            return buf;
        }

        // Comparision

        bool operator==(const String &other) const
        {
            if (_length != other._length) return false;
            if (is_ascii && other.is_ascii)
            {
                const unsigned char *a = is_heap ? heap.ascii : sso.ascii;
                const unsigned char *b = other.is_heap ? other.heap.ascii : other.sso.ascii;
                return std::memcmp(a, b, _length) == 0;
            }

            for (uint64_t i = 0; i < _length; ++i)
            {
                char32_t c1 =
                    is_ascii ? (is_heap ? heap.ascii[i] : sso.ascii[i]) : (is_heap ? heap.utf32[i] : sso.utf32[i]);
                char32_t c2 = other.is_ascii ? (other.is_heap ? other.heap.ascii[i] : other.sso.ascii[i]) :
                                               (other.is_heap ? other.heap.utf32[i] : other.sso.utf32[i]);
                if (c1 != c2) return false;
            }
            return true;
        }

        bool operator!=(const String &other) const { return !(*this == other); }

        void clear()
        {
            destroy();
            is_ascii = true;
            _length = 0;
            _capacity = 0;
            is_heap = false;
        }

        void reverse()
        {
            if (_length <= 1) return;

            if (is_ascii)
            {
                if (is_heap)
                {
                    for (uint64_t i = 0, j = _length - 1; i < j; ++i, --j) std::swap(heap.ascii[i], heap.ascii[j]);
                }
                else
                {
                    for (uint64_t i = 0, j = _length - 1; i < j; ++i, --j) std::swap(sso.ascii[i], sso.ascii[j]);
                }
            }
            else
            {
                if (is_heap)
                {
                    for (uint64_t i = 0, j = _length - 1; i < j; ++i, --j) std::swap(heap.utf32[i], heap.utf32[j]);
                }
                else
                {
                    for (uint64_t i = 0, j = _length - 1; i < j; ++i, --j) std::swap(sso.utf32[i], sso.utf32[j]);
                }
            }
        }

        void reserve(uint64_t new_capacity)
        {
            if (new_capacity <= (_capacity > 0 ? _capacity : (is_ascii ? SSO_MAX_ASCII_LEN : SSO_MAX_UTF32_LEN)))
                return; // enough

            ensure_capacity(new_capacity); // 复用你已有的 ensure_capacity
        }


        void shrink_to_fit()
        {
            if (!is_heap) return;

            if (is_ascii)
            {
                if (_length <= SSO_MAX_ASCII_LEN)
                {
                    // back to SSO
                    unsigned char tmp[SSO_MAX_ASCII_LEN];
                    std::memcpy(tmp, heap.ascii, _length);
                    delete[] heap.ascii;
                    std::memcpy(sso.ascii, tmp, _length);
                    is_heap = false;
                    _capacity = 0;
                }
                else
                {
                    unsigned char *new_buffer = new unsigned char[_length];
                    std::memcpy(new_buffer, heap.ascii, _length);
                    delete[] heap.ascii;
                    heap.ascii = new_buffer;
                    _capacity = _length;
                }
            }
            else
            {
                if (_length <= SSO_MAX_UTF32_LEN)
                {
                    char32_t tmp[SSO_MAX_UTF32_LEN];
                    std::memcpy(tmp, heap.utf32, _length * sizeof(char32_t));
                    delete[] heap.utf32;
                    std::memcpy(sso.utf32, tmp, _length * sizeof(char32_t));
                    is_heap = false;
                    _capacity = 0;
                }
                else
                {
                    char32_t *new_buffer = new char32_t[_length];
                    std::memcpy(new_buffer, heap.utf32, _length * sizeof(char32_t));
                    delete[] heap.utf32;
                    heap.utf32 = new_buffer;
                    _capacity = _length;
                }
            }
        }

        friend struct std::hash<String>;
    };
}; // namespace Fig::StringClass::DynamicCapacity

namespace Fig
{
    using String = StringClass::DynamicCapacity::String;
};

inline std::ostream &operator<<(std::ostream &os, const Fig::String &str)
{
    os << str.toBasicString();
    return os;
}

namespace std
{
    template <>
    struct hash<Fig::StringClass::DynamicCapacity::String>
    {
        size_t operator()(const Fig::StringClass::DynamicCapacity::String &s) const noexcept
        {
            // FNV-1a 64-bit hash
            const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ull;
            const uint64_t FNV_PRIME = 1099511628211ull;

            uint64_t hash = FNV_OFFSET_BASIS;
            if (s.isAscii())
            {
                const unsigned char *data = s.isOnHeap() ? s.heap.ascii : s.sso.ascii;
                for (uint64_t i = 0; i < s.length(); ++i)
                {
                    hash ^= static_cast<uint64_t>(data[i]);
                    hash *= FNV_PRIME;
                }
            }
            else
            {
                const char32_t *data = s.isOnHeap() ? s.heap.utf32 : s.sso.utf32;
                for (uint64_t i = 0; i < s.length(); ++i)
                {
                    uint32_t c = data[i];
                    // 将 char32_t -> 4 bytes
                    for (int shift = 0; shift < 32; shift += 8)
                    {
                        hash ^= static_cast<uint64_t>((c >> shift) & 0xFF);
                        hash *= FNV_PRIME;
                    }
                }
            }
            return static_cast<size_t>(hash);
        }
    };
} // namespace std