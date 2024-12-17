#pragma once

#include <erebus/system/erebus.hxx>

#include <bitset>


namespace Er
{

/**
 * @brief A handy pack of flags
 *
 */


using Flag = unsigned;


template <std::size_t N, typename ValuesSetT>
class FlagsPack
    : public ValuesSetT
{
public:
    struct FromBitsT {};
    static constexpr FromBitsT FromBits = FromBitsT();
    
    static constexpr std::size_t Size = N;
    static_assert(Size > 0);

    constexpr FlagsPack() noexcept = default;

    explicit FlagsPack(std::uint32_t representation, FromBitsT) noexcept
        : m_bits(representation)
    {
        static_assert(Size >= 32);
    }

    explicit FlagsPack(std::uint64_t representation, FromBitsT) noexcept
        : m_bits(representation)
    {
        static_assert(Size >= 64);
    }

    FlagsPack(std::initializer_list<Flag> initializers) noexcept
        : m_bits()
    {
        for (auto& f : initializers)
            set(f);
    }

    template <typename CharT, typename TraitsT>
    explicit FlagsPack(std::basic_string_view<CharT, TraitsT> representation) noexcept
        : m_bits()
    {
        if (representation.empty())
            return;

        const auto SrcSize = representation.size();
        ErAssert(SrcSize <= Size);

        auto index = std::min(SrcSize, Size);
        do
        {
            --index;
            if (representation[SrcSize - index - 1] == static_cast<CharT>('1'))
            {
                set(index);
            }

        } while (index != 0);
    }

    FlagsPack& set(Flag f, bool value = true) noexcept
    {
        ErAssert(f < Size);
        m_bits.set(f, value);
        return *this;
    }

    FlagsPack& reset(Flag f) noexcept
    {
        ErAssert(f < Size);
        set(f, false);
        return *this;
    }

    FlagsPack& reset() noexcept
    {
        m_bits.reset();
        return *this;
    }

    [[nodiscard]] bool operator[](Flag f) const noexcept
    {
        ErAssert(f < Size);
        return m_bits[f];
    }

    [[nodiscard]] bool operator==(const FlagsPack& o) const noexcept
    {
        return m_bits == o.m_bits;
    }

    [[nodiscard]] bool operator!=(const FlagsPack& o) const noexcept
    {
        return m_bits != o.m_bits;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return Size;
    }

    [[nodiscard]] std::size_t count() const noexcept
    {
        return m_bits.count();
    }

    [[nodiscard]] bool any() const noexcept
    {
        return m_bits.any();
    }

    [[nodiscard]] bool none() const noexcept
    {
        return m_bits.none();
    }

    template <class CharT = char, class TraitsT = std::char_traits<CharT>, class AllocatorT = std::allocator<CharT>>
    [[nodiscard]] std::basic_string<CharT, TraitsT, AllocatorT> to_string() const
    {
        std::basic_string<CharT, TraitsT, AllocatorT> out;
        out.reserve(N);
        
        for (auto i = N; i > 0; --i)
        {
            auto bit = m_bits[i - 1];
            out.push_back(static_cast<CharT>(bit ? '1' : '0'));
        }

        return out;
    }

    template <typename T>
    [[nodiscard]] std::enable_if_t<(std::is_same_v<T, std::uint32_t> && (N <= 32)), std::uint32_t> pack() const noexcept
    {
        return static_cast<std::uint32_t>(m_bits.to_ulong());
    }

    template <typename T>
    [[nodiscard]] std::enable_if_t<(std::is_same_v<T, std::uint64_t> && (N <= 64)), std::uint64_t> pack() const noexcept
    {
        return static_cast<std::uint64_t>(m_bits.to_ullong());
    }

private:
    std::bitset<N> m_bits;
};


} // namespace Er{}