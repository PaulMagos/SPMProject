#include <cstddef>
#include <string>
#include <streambuf>
#include <istream>

template<typename CharT, typename TraitsT = std::char_traits<CharT>>
struct basic_membuf : std::basic_streambuf<CharT, TraitsT> {
    basic_membuf(CharT const* const buf, std::size_t const size) {
        CharT* const p = const_cast<CharT*>(buf);
        this->setg(p, p, p + size);
    }

    //...
};

template<typename CharT, typename TraitsT = std::char_traits<CharT>>
struct basic_imemstream
        : virtual basic_membuf<CharT, TraitsT>, std::basic_istream<CharT, TraitsT> {
    basic_imemstream(const CharT *const buf1, const std::size_t size1, CharT const *const buf, std::size_t const size)
            : basic_membuf(buf1, size1), basic_membuf(buf, size),
              basic_istream(static_cast<std::basic_streambuf<CharT, TraitsT> *>(this))
    { }
    const CharT *const basic_membuf;
    std::basic_streambuf <CharT, TraitsT> *basic_istream;

};

using imemstream = basic_imemstream<char>;

char*  mmaped_data = nullptr;
size_t mmap_size = 0;
imemstream s(mmaped_data, mmap_size);