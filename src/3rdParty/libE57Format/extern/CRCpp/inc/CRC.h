/**
    @file CRC.h
    @author Daniel Bahr
    @version 0.2.0.6
    @copyright
    @parblock
        CRC++
        Copyright (c) 2016, Daniel Bahr
        All rights reserved.

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are met:

        * Redistributions of source code must retain the above copyright notice, this
          list of conditions and the following disclaimer.

        * Redistributions in binary form must reproduce the above copyright notice,
          this list of conditions and the following disclaimer in the documentation
          and/or other materials provided with the distribution.

        * Neither the name of CRC++ nor the names of its
          contributors may be used to endorse or promote products derived from
          this software without specific prior written permission.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
        AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
        DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
        FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
        DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
        SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
        CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
        OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
        OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    @endparblock
*/

/*
    CRC++ can be configured by setting various #defines before #including this header file:

        #define crcpp_uint8                             - Specifies the type used to store CRCs that have a width of 8 bits or less.
                                                          This type is not used in CRC calculations. Defaults to ::std::uint8_t.
        #define crcpp_uint16                            - Specifies the type used to store CRCs that have a width between 9 and 16 bits (inclusive).
                                                          This type is not used in CRC calculations. Defaults to ::std::uint16_t.
        #define crcpp_uint32                            - Specifies the type used to store CRCs that have a width between 17 and 32 bits (inclusive).
                                                          This type is not used in CRC calculations. Defaults to ::std::uint32_t.
        #define crcpp_uint64                            - Specifies the type used to store CRCs that have a width between 33 and 64 bits (inclusive).
                                                          This type is not used in CRC calculations. Defaults to ::std::uint64_t.
        #define crcpp_size                              - This type is used for loop iteration and function signatures only. Defaults to ::std::size_t.
        #define CRCPP_USE_NAMESPACE                     - Define to place all CRC++ code within the ::CRCPP namespace.
        #define CRCPP_BRANCHLESS                        - Define to enable a branchless CRC implementation. The branchless implementation uses a single integer
                                                          multiplication in the bit-by-bit calculation instead of a small conditional. The branchless implementation
                                                          may be faster on processor architectures which support single-instruction integer multiplication.
        #define CRCPP_USE_CPP11                         - Define to enables C++11 features (move semantics, constexpr, static_assert, etc.).
        #define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS  - Define to include definitions for little-used CRCs. 
*/

#ifndef CRCPP_CRC_H_
#define CRCPP_CRC_H_

#include <climits>  // Includes CHAR_BIT
#ifdef CRCPP_USE_CPP11
#include <cstddef>  // Includes ::std::size_t
#include <cstdint>  // Includes ::std::uint8_t, ::std::uint16_t, ::std::uint32_t, ::std::uint64_t
#else
#include <stddef.h> // Includes size_t
#include <stdint.h> // Includes uint8_t, uint16_t, uint32_t, uint64_t
#endif
#include <limits>   // Includes ::std::numeric_limits
#include <utility>  // Includes ::std::move

#ifndef crcpp_uint8
#   ifdef CRCPP_USE_CPP11
        /// @brief Unsigned 8-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint8 ::std::uint8_t
#   else
        /// @brief Unsigned 8-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint8 uint8_t
#   endif
#endif

#ifndef crcpp_uint16
#   ifdef CRCPP_USE_CPP11
        /// @brief Unsigned 16-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint16 ::std::uint16_t
#   else
        /// @brief Unsigned 16-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint16 uint16_t
#   endif
#endif

#ifndef crcpp_uint32
#   ifdef CRCPP_USE_CPP11
        /// @brief Unsigned 32-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint32 ::std::uint32_t
#   else
        /// @brief Unsigned 32-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint32 uint32_t
#   endif
#endif

#ifndef crcpp_uint64
#   ifdef CRCPP_USE_CPP11
        /// @brief Unsigned 64-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint64 ::std::uint64_t
#   else
        /// @brief Unsigned 64-bit integer definition, used primarily for parameter definitions.
#       define crcpp_uint64 uint64_t
#   endif
#endif

#ifndef crcpp_size
#   ifdef CRCPP_USE_CPP11
        /// @brief Unsigned size definition, used for specifying data sizes.
#       define crcpp_size ::std::size_t
#   else
        /// @brief Unsigned size definition, used for specifying data sizes.
#       define crcpp_size size_t
#   endif
#endif

#ifdef CRCPP_USE_CPP11
    /// @brief Compile-time expression definition.
#   define crcpp_constexpr constexpr
#else
    /// @brief Compile-time expression definition.
#   define crcpp_constexpr const
#endif

#ifdef CRCPP_USE_NAMESPACE
namespace CRCPP
{
#endif

/**
    @brief Static class for computing CRCs.
    @note This class supports computation of full and multi-part CRCs, using a bit-by-bit algorithm or a
        byte-by-byte lookup table. The CRCs are calculated using as many optimizations as is reasonable.
        If compiling with C++11, the constexpr keyword is used liberally so that many calculations are
        performed at compile-time instead of at runtime.
*/
class CRC
{
public:
    // Forward declaration
    template <typename CRCType, crcpp_uint16 CRCWidth>
    struct Table;

    /**
        @brief CRC parameters.
    */
    template <typename CRCType, crcpp_uint16 CRCWidth>
    struct Parameters
    {
        CRCType polynomial;   ///< CRC polynomial
        CRCType initialValue; ///< Initial CRC value
        CRCType finalXOR;     ///< Value to XOR with the final CRC
        bool reflectInput;    ///< true to reflect all input bytes
        bool reflectOutput;   ///< true to reflect the output CRC (reflection occurs before the final XOR)

        Table<CRCType, CRCWidth> MakeTable() const;
    };

    /**
        @brief CRC lookup table. After construction, the CRC parameters are fixed.
        @note A CRC table can be used for multiple CRC calculations.
    */
    template <typename CRCType, crcpp_uint16 CRCWidth>
    struct Table
    {
        // Constructors are intentionally NOT marked explicit.
        Table(const Parameters<CRCType, CRCWidth> & parameters);

#ifdef CRCPP_USE_CPP11
        Table(Parameters<CRCType, CRCWidth> && parameters);
#endif

        const Parameters<CRCType, CRCWidth> & GetParameters() const;

        const CRCType * GetTable() const;

        CRCType operator[](unsigned char index) const;

    private:
        void InitTable();

        Parameters<CRCType, CRCWidth> parameters; ///< CRC parameters used to construct the table
        CRCType table[1 << CHAR_BIT];             ///< CRC lookup table
    };

    // The number of bits in CRCType must be at least as large as CRCWidth.
    // CRCType must be an unsigned integer type or a custom type with operator overloads.
    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType Calculate(const void * data, crcpp_size size, const Parameters<CRCType, CRCWidth> & parameters);

    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType Calculate(const void * data, crcpp_size size, const Parameters<CRCType, CRCWidth> & parameters, CRCType crc);

    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType Calculate(const void * data, crcpp_size size, const Table<CRCType, CRCWidth> & lookupTable);

    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType Calculate(const void * data, crcpp_size size, const Table<CRCType, CRCWidth> & lookupTable, CRCType crc);

    // Common CRCs up to 64 bits.
    // Note: Check values are the computed CRCs when given an ASCII input of "123456789" (without null terminator)
#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
    static const Parameters< crcpp_uint8,  4> & CRC_4_ITU();
    static const Parameters< crcpp_uint8,  5> & CRC_5_EPC();
    static const Parameters< crcpp_uint8,  5> & CRC_5_ITU();
    static const Parameters< crcpp_uint8,  5> & CRC_5_USB();
    static const Parameters< crcpp_uint8,  6> & CRC_6_CDMA2000A();
    static const Parameters< crcpp_uint8,  6> & CRC_6_CDMA2000B();
    static const Parameters< crcpp_uint8,  6> & CRC_6_ITU();
    static const Parameters< crcpp_uint8,  7> & CRC_7();
#endif
    static const Parameters< crcpp_uint8,  8> & CRC_8();
#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
    static const Parameters< crcpp_uint8,  8> & CRC_8_EBU();
    static const Parameters< crcpp_uint8,  8> & CRC_8_MAXIM();
    static const Parameters< crcpp_uint8,  8> & CRC_8_WCDMA();
    static const Parameters<crcpp_uint16, 10> & CRC_10();
    static const Parameters<crcpp_uint16, 10> & CRC_10_CDMA2000();
    static const Parameters<crcpp_uint16, 11> & CRC_11();
    static const Parameters<crcpp_uint16, 12> & CRC_12_CDMA2000();
    static const Parameters<crcpp_uint16, 12> & CRC_12_DECT();
    static const Parameters<crcpp_uint16, 12> & CRC_12_UMTS();
    static const Parameters<crcpp_uint16, 13> & CRC_13_BBC();
    static const Parameters<crcpp_uint16, 15> & CRC_15();
    static const Parameters<crcpp_uint16, 15> & CRC_15_MPT1327();
#endif
    static const Parameters<crcpp_uint16, 16> & CRC_16_ARC();
    static const Parameters<crcpp_uint16, 16> & CRC_16_BUYPASS();
    static const Parameters<crcpp_uint16, 16> & CRC_16_CCITTFALSE();
#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
    static const Parameters<crcpp_uint16, 16> & CRC_16_CDMA2000();
    static const Parameters<crcpp_uint16, 16> & CRC_16_DECTR();
    static const Parameters<crcpp_uint16, 16> & CRC_16_DECTX();
    static const Parameters<crcpp_uint16, 16> & CRC_16_DNP();
#endif
    static const Parameters<crcpp_uint16, 16> & CRC_16_GENIBUS();
    static const Parameters<crcpp_uint16, 16> & CRC_16_KERMIT();
#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
    static const Parameters<crcpp_uint16, 16> & CRC_16_MAXIM();
    static const Parameters<crcpp_uint16, 16> & CRC_16_MODBUS();
    static const Parameters<crcpp_uint16, 16> & CRC_16_T10DIF();
    static const Parameters<crcpp_uint16, 16> & CRC_16_USB();
#endif
    static const Parameters<crcpp_uint16, 16> & CRC_16_X25();
    static const Parameters<crcpp_uint16, 16> & CRC_16_XMODEM();
#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
    static const Parameters<crcpp_uint32, 17> & CRC_17_CAN();
    static const Parameters<crcpp_uint32, 21> & CRC_21_CAN();
    static const Parameters<crcpp_uint32, 24> & CRC_24();
    static const Parameters<crcpp_uint32, 24> & CRC_24_FLEXRAYA();
    static const Parameters<crcpp_uint32, 24> & CRC_24_FLEXRAYB();
    static const Parameters<crcpp_uint32, 30> & CRC_30();
#endif
    static const Parameters<crcpp_uint32, 32> & CRC_32();
    static const Parameters<crcpp_uint32, 32> & CRC_32_BZIP2();
#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
    static const Parameters<crcpp_uint32, 32> & CRC_32_C();
#endif
    static const Parameters<crcpp_uint32, 32> & CRC_32_MPEG2();
    static const Parameters<crcpp_uint32, 32> & CRC_32_POSIX();
#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
    static const Parameters<crcpp_uint32, 32> & CRC_32_Q();
    static const Parameters<crcpp_uint64, 40> & CRC_40_GSM();
    static const Parameters<crcpp_uint64, 64> & CRC_64();
#endif

#ifdef CRCPP_USE_CPP11
    CRC() = delete;
    CRC(const CRC & other) = delete;
    CRC & operator=(const CRC & other) = delete;
    CRC(CRC && other) = delete;
    CRC & operator=(CRC && other) = delete;
#endif

private:
#ifndef CRCPP_USE_CPP11
    CRC();
    CRC(const CRC & other);
    CRC & operator=(const CRC & other);
#endif

    template <typename IntegerType>
    static IntegerType Reflect(IntegerType value, crcpp_uint16 numBits);

    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType Finalize(CRCType remainder, CRCType finalXOR, bool reflectOutput);

    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType UndoFinalize(CRCType remainder, CRCType finalXOR, bool reflectOutput);

    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType CalculateRemainder(const void * data, crcpp_size size, const Parameters<CRCType, CRCWidth> & parameters, CRCType remainder);

    template <typename CRCType, crcpp_uint16 CRCWidth>
    static CRCType CalculateRemainder(const void * data, crcpp_size size, const Table<CRCType, CRCWidth> & lookupTable, CRCType remainder);

    template <typename IntegerType>
    static crcpp_constexpr IntegerType BoundedConstexprValue(IntegerType x);
};

/**
    @brief Returns a CRC lookup table construct using these CRC parameters.
    @note This function primarily exists to allow use of the auto keyword instead of instantiating
        a table directly, since template parameters are not inferred in constructors.
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC lookup table
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRC::Table<CRCType, CRCWidth> CRC::Parameters<CRCType, CRCWidth>::MakeTable() const
{
    // This should take advantage of RVO and optimize out the copy.
    return CRC::Table<CRCType, CRCWidth>(*this);
}

/**
    @brief Constructs a CRC table from a set of CRC parameters
    @param[in] parameters CRC parameters
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRC::Table<CRCType, CRCWidth>::Table(const Parameters<CRCType, CRCWidth> & parameters) :
    parameters(parameters)
{
    InitTable();
}

#ifdef CRCPP_USE_CPP11
/**
    @brief Constructs a CRC table from a set of CRC parameters
    @param[in] parameters CRC parameters
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRC::Table<CRCType, CRCWidth>::Table(Parameters<CRCType, CRCWidth> && parameters) :
    parameters(::std::move(parameters))
{
    InitTable();
}
#endif

/**
    @brief Gets the CRC parameters used to construct the CRC table
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC parameters
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline const CRC::Parameters<CRCType, CRCWidth> & CRC::Table<CRCType, CRCWidth>::GetParameters() const
{
    return parameters;
}

/**
    @brief Gets the CRC table
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC table
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline const CRCType * CRC::Table<CRCType, CRCWidth>::GetTable() const
{
    return table;
}

/**
    @brief Gets an entry in the CRC table
    @param[in] index Index into the CRC table
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC table entry
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::Table<CRCType, CRCWidth>::operator[](unsigned char index) const
{
    return table[index];
}

/**
    @brief Initializes a CRC table.
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline void CRC::Table<CRCType, CRCWidth>::InitTable()
{
    // For masking off the bits for the CRC (in the event that the number of bits in CRCType is larger than CRCWidth)
    static crcpp_constexpr CRCType BIT_MASK((CRCType(1) << (CRCWidth - CRCType(1))) |
                                           ((CRCType(1) << (CRCWidth - CRCType(1))) - CRCType(1)));

    static crcpp_constexpr CRCType SHIFT(CRC::BoundedConstexprValue(CHAR_BIT - CRCWidth));

    CRCType crc;
    unsigned char byte = 0;

    // Loop over each dividend (each possible number storable in an unsigned char)
    do
    {
        crc = CRC::CalculateRemainder<CRCType, CRCWidth>(&byte, sizeof(byte), parameters, CRCType(0));

        // This mask might not be necessary; all unit tests pass with this line commented out,
        // but that might just be a coincidence based on the CRC parameters used for testing.
        // In any case, this is harmless to leave in and only adds a single machine instruction per loop iteration.
        crc &= BIT_MASK;

        if (!parameters.reflectInput && CRCWidth < CHAR_BIT)
        {
            // Undo the special operation at the end of the CalculateRemainder()
            // function for non-reflected CRCs < CHAR_BIT.
            crc <<= SHIFT;
        }

        table[byte] = crc;
    }
    while (++byte);
}

/**
    @brief Computes a CRC.
    @param[in] data Data over which CRC will be computed
    @param[in] size Size of the data
    @param[in] parameters CRC parameters
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::Calculate(const void * data, crcpp_size size, const Parameters<CRCType, CRCWidth> & parameters)
{
    CRCType remainder = CalculateRemainder(data, size, parameters, parameters.initialValue);

    // No need to mask the remainder here; the mask will be applied in the Finalize() function.

    return Finalize<CRCType, CRCWidth>(remainder, parameters.finalXOR, parameters.reflectInput != parameters.reflectOutput);
}
/**
    @brief Appends additional data to a previous CRC calculation.
    @note This function can be used to compute multi-part CRCs.
    @param[in] data Data over which CRC will be computed
    @param[in] size Size of the data
    @param[in] parameters CRC parameters
    @param[in] crc CRC from a previous calculation
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::Calculate(const void * data, crcpp_size size, const Parameters<CRCType, CRCWidth> & parameters, CRCType crc)
{
    CRCType remainder = UndoFinalize<CRCType, CRCWidth>(crc, parameters.finalXOR, parameters.reflectInput != parameters.reflectOutput);

    remainder = CalculateRemainder(data, size, parameters, remainder);

    // No need to mask the remainder here; the mask will be applied in the Finalize() function.

    return Finalize<CRCType, CRCWidth>(remainder, parameters.finalXOR, parameters.reflectInput != parameters.reflectOutput);
}

/**
    @brief Computes a CRC via a lookup table.
    @param[in] data Data over which CRC will be computed
    @param[in] size Size of the data
    @param[in] lookupTable CRC lookup table
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::Calculate(const void * data, crcpp_size size, const Table<CRCType, CRCWidth> & lookupTable)
{
    const Parameters<CRCType, CRCWidth> & parameters = lookupTable.GetParameters();

    CRCType remainder = CalculateRemainder(data, size, lookupTable, parameters.initialValue);

    // No need to mask the remainder here; the mask will be applied in the Finalize() function.

    return Finalize<CRCType, CRCWidth>(remainder, parameters.finalXOR, parameters.reflectInput != parameters.reflectOutput);
}

/**
    @brief Appends additional data to a previous CRC calculation using a lookup table.
    @note This function can be used to compute multi-part CRCs.
    @param[in] data Data over which CRC will be computed
    @param[in] size Size of the data
    @param[in] lookupTable CRC lookup table
    @param[in] crc CRC from a previous calculation
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::Calculate(const void * data, crcpp_size size, const Table<CRCType, CRCWidth> & lookupTable, CRCType crc)
{
    const Parameters<CRCType, CRCWidth> & parameters = lookupTable.GetParameters();

    CRCType remainder = UndoFinalize<CRCType, CRCWidth>(crc, parameters.finalXOR, parameters.reflectInput != parameters.reflectOutput);

    remainder = CalculateRemainder(data, size, lookupTable, remainder);

    // No need to mask the remainder here; the mask will be applied in the Finalize() function.

    return Finalize<CRCType, CRCWidth>(remainder, parameters.finalXOR, parameters.reflectInput != parameters.reflectOutput);
}

/**
    @brief Reflects (i.e. reverses the bits within) an integer value.
    @param[in] value Value to reflect
    @param[in] numBits Number of bits in the integer which will be reflected
    @tparam IntegerType Integer type of the value being reflected
    @return Reflected value
*/
template <typename IntegerType>
inline IntegerType CRC::Reflect(IntegerType value, crcpp_uint16 numBits)
{
    IntegerType reversedValue(0);

    for (crcpp_uint16 i = 0; i < numBits; ++i)
    {
        reversedValue = (reversedValue << 1) | (value & 1);
        value >>= 1;
    }

    return reversedValue;
}

/**
    @brief Computes the final reflection and XOR of a CRC remainder.
    @param[in] remainder CRC remainder to reflect and XOR
    @param[in] finalXOR Final value to XOR with the remainder
    @param[in] reflectOutput true to reflect each byte of the remainder before the XOR
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return Final CRC
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::Finalize(CRCType remainder, CRCType finalXOR, bool reflectOutput)
{
    // For masking off the bits for the CRC (in the event that the number of bits in CRCType is larger than CRCWidth)
    static crcpp_constexpr CRCType BIT_MASK = (CRCType(1) << (CRCWidth - CRCType(1))) |
                                             ((CRCType(1) << (CRCWidth - CRCType(1))) - CRCType(1));

    if (reflectOutput)
    {
        remainder = Reflect(remainder, CRCWidth);
    }

    return (remainder ^ finalXOR) & BIT_MASK;
}

/**
    @brief Undoes the process of computing the final reflection and XOR of a CRC remainder.
    @note This function allows for computation of multi-part CRCs
    @note Calling UndoFinalize() followed by Finalize() (or vice versa) will always return the original remainder value:

        CRCType x = ...;
        CRCType y = Finalize(x, finalXOR, reflectOutput);
        CRCType z = UndoFinalize(y, finalXOR, reflectOutput);
        assert(x == z);

    @param[in] crc Reflected and XORed CRC
    @param[in] finalXOR Final value XORed with the remainder
    @param[in] reflectOutput true if the remainder is to be reflected
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return Un-finalized CRC remainder
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::UndoFinalize(CRCType crc, CRCType finalXOR, bool reflectOutput)
{
    // For masking off the bits for the CRC (in the event that the number of bits in CRCType is larger than CRCWidth)
    static crcpp_constexpr CRCType BIT_MASK = (CRCType(1) << (CRCWidth - CRCType(1))) |
                                             ((CRCType(1) << (CRCWidth - CRCType(1))) - CRCType(1));

    crc = (crc & BIT_MASK) ^ finalXOR;

    if (reflectOutput)
    {
        crc = Reflect(crc, CRCWidth);
    }

    return crc;
}

/**
    @brief Computes a CRC remainder.
    @param[in] data Data over which the remainder will be computed
    @param[in] size Size of the data
    @param[in] parameters CRC parameters
    @param[in] remainder Running CRC remainder. Can be an initial value or the result of a previous CRC remainder calculation.
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC remainder
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::CalculateRemainder(const void * data, crcpp_size size, const Parameters<CRCType, CRCWidth> & parameters, CRCType remainder)
{
#ifdef CRCPP_USE_CPP11
    // This static_assert is put here because this function will always be compiled in no matter what
    // the template parameters are and whether or not a table lookup or bit-by-bit algorithm is used.
    static_assert(::std::numeric_limits<CRCType>::digits >= CRCWidth, "CRCType is too small to contain a CRC of width CRCWidth.");
#else
    // Catching this compile-time error is very important. Sadly, the compiler error will be very cryptic, but it's
    // better than nothing.
    enum { static_assert_failed_CRCType_is_too_small_to_contain_a_CRC_of_width_CRCWidth = 1 / (::std::numeric_limits<CRCType>::digits >= CRCWidth ? 1 : 0) };
#endif

    const unsigned char * current = reinterpret_cast<const unsigned char *>(data);

    // Slightly different implementations based on the parameters. The current implementations try to eliminate as much
    // computation from the inner loop (looping over each bit) as possible.
    if (parameters.reflectInput)
    {
        CRCType polynomial = CRC::Reflect(parameters.polynomial, CRCWidth);
        while (size--)
        {
            remainder ^= *current++;

            // An optimizing compiler might choose to unroll this loop.
            for (crcpp_size i = 0; i < CHAR_BIT; ++i)
            {
#ifdef CRCPP_BRANCHLESS
                // Clever way to avoid a branch at the expense of a multiplication. This code is equivalent to the following:
                // if (remainder & 1)
                //     remainder = (remainder >> 1) ^ polynomial;
                // else
                //     remainder >>= 1;
                remainder = (remainder >> 1) ^ ((remainder & 1) * polynomial);
#else
                remainder = (remainder & 1) ? ((remainder >> 1) ^ polynomial) : (remainder >> 1);
#endif
            }
        }
    }
    else if (CRCWidth >= CHAR_BIT)
    {
        static crcpp_constexpr CRCType CRC_WIDTH_MINUS_ONE(CRCWidth - CRCType(1));
#ifndef CRCPP_BRANCHLESS
        static crcpp_constexpr CRCType CRC_HIGHEST_BIT_MASK(CRCType(1) << CRC_WIDTH_MINUS_ONE);
#endif
        static crcpp_constexpr CRCType SHIFT(BoundedConstexprValue(CRCWidth - CHAR_BIT));

        while (size--)
        {
            remainder ^= (static_cast<CRCType>(*current++) << SHIFT);

            // An optimizing compiler might choose to unroll this loop.
            for (crcpp_size i = 0; i < CHAR_BIT; ++i)
            {
#ifdef CRCPP_BRANCHLESS
                // Clever way to avoid a branch at the expense of a multiplication. This code is equivalent to the following:
                // if (remainder & CRC_HIGHEST_BIT_MASK)
                //     remainder = (remainder << 1) ^ parameters.polynomial;
                // else
                //     remainder <<= 1;
                remainder = (remainder << 1) ^ (((remainder >> CRC_WIDTH_MINUS_ONE) & 1) * parameters.polynomial);
#else
                remainder = (remainder & CRC_HIGHEST_BIT_MASK) ? ((remainder << 1) ^ parameters.polynomial) : (remainder << 1);
#endif
            }
        }
    }
    else
    {
        static crcpp_constexpr CRCType CHAR_BIT_MINUS_ONE(CHAR_BIT - 1);
#ifndef CRCPP_BRANCHLESS
        static crcpp_constexpr CRCType CHAR_BIT_HIGHEST_BIT_MASK(CRCType(1) << CHAR_BIT_MINUS_ONE);
#endif
        static crcpp_constexpr CRCType SHIFT(BoundedConstexprValue(CHAR_BIT - CRCWidth));

        CRCType polynomial = parameters.polynomial << SHIFT;
        remainder <<= SHIFT;

        while (size--)
        {
            remainder ^= *current++;

            // An optimizing compiler might choose to unroll this loop.
            for (crcpp_size i = 0; i < CHAR_BIT; ++i)
            {
#ifdef CRCPP_BRANCHLESS
                // Clever way to avoid a branch at the expense of a multiplication. This code is equivalent to the following:
                // if (remainder & CHAR_BIT_HIGHEST_BIT_MASK)
                //     remainder = (remainder << 1) ^ polynomial;
                // else
                //     remainder <<= 1;
                remainder = (remainder << 1) ^ (((remainder >> CHAR_BIT_MINUS_ONE) & 1) * polynomial);
#else
                remainder = (remainder & CHAR_BIT_HIGHEST_BIT_MASK) ? ((remainder << 1) ^ polynomial) : (remainder << 1);
#endif
            }
        }

        remainder >>= SHIFT;
    }

    return remainder;
}

/**
    @brief Computes a CRC remainder using lookup table.
    @param[in] data Data over which the remainder will be computed
    @param[in] size Size of the data
    @param[in] lookupTable CRC lookup table
    @param[in] remainder Running CRC remainder. Can be an initial value or the result of a previous CRC remainder calculation.
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return CRC remainder
*/
template <typename CRCType, crcpp_uint16 CRCWidth>
inline CRCType CRC::CalculateRemainder(const void * data, crcpp_size size, const Table<CRCType, CRCWidth> & lookupTable, CRCType remainder)
{
    const unsigned char * current = reinterpret_cast<const unsigned char *>(data);

    if (lookupTable.GetParameters().reflectInput)
    {
        while (size--)
        {
#if defined(WIN32) || defined(_WIN32) || defined(WINCE)
    // Disable warning about data loss when doing (remainder >> CHAR_BIT) when
    // remainder is one byte long. The algorithm is still correct in this case,
    // though it's possible that one additional machine instruction will be executed.
#   if defined(_MSC_VER)
#   pragma warning (push)
#   pragma warning (disable : 4333)
#   endif
#endif
            remainder = (remainder >> CHAR_BIT) ^ lookupTable[static_cast<unsigned char>(remainder ^ *current++)];
#if defined(WIN32) || defined(_WIN32) || defined(WINCE)
#   if defined(_MSC_VER)
#   pragma warning (pop)
#   endif
#endif
        }
    }
    else if (CRCWidth >= CHAR_BIT)
    {
        static crcpp_constexpr CRCType SHIFT(BoundedConstexprValue(CRCWidth - CHAR_BIT));

        while (size--)
        {
            remainder = (remainder << CHAR_BIT) ^ lookupTable[static_cast<unsigned char>((remainder >> SHIFT) ^ *current++)];
        }
    }
    else
    {
        static crcpp_constexpr CRCType SHIFT(BoundedConstexprValue(CHAR_BIT - CRCWidth));

        remainder <<= SHIFT;

        while (size--)
        {
            // Note: no need to mask here since remainder is guaranteed to fit in a single byte.
            remainder = lookupTable[static_cast<unsigned char>(remainder ^ *current++)];
        }

        remainder >>= SHIFT;
    }

    return remainder;
}

/**
    @brief Function to force a compile-time expression to be >= 0.
    @note This function is used to avoid compiler warnings because all constexpr values are evaluated
        in a function even in a branch will never be executed. This also means we don't need pragmas
        to get rid of warnings, but it still can be computed at compile-time. Win-win!
    @param[in] x Compile-time expression to bound
    @tparam CRCType Integer type for storing the CRC result
    @tparam CRCWidth Number of bits in the CRC
    @return Non-negative compile-time expression
*/
template <typename IntegerType>
inline crcpp_constexpr IntegerType CRC::BoundedConstexprValue(IntegerType x)
{
    return (x < IntegerType(0)) ? IntegerType(0) : x;
}

#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
/**
    @brief Returns a set of parameters for CRC-4 ITU.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-4 ITU has the following parameters and check value:
        - polynomial     = 0x3
        - initial value  = 0x0
        - final XOR      = 0x0
        - reflect input  = true
        - reflect output = true
        - check value    = 0x7
    @return CRC-4 ITU parameters
*/
inline const CRC::Parameters<crcpp_uint8, 4> & CRC::CRC_4_ITU()
{
    static const Parameters<crcpp_uint8, 4> parameters = { 0x3, 0x0, 0x0, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-5 EPC.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-5 EPC has the following parameters and check value:
        - polynomial     = 0x09
        - initial value  = 0x09
        - final XOR      = 0x00
        - reflect input  = false
        - reflect output = false
        - check value    = 0x00
    @return CRC-5 EPC parameters
*/
inline const CRC::Parameters<crcpp_uint8, 5> & CRC::CRC_5_EPC()
{
    static const Parameters<crcpp_uint8, 5> parameters = { 0x09, 0x09, 0x00, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-5 ITU.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-5 ITU has the following parameters and check value:
        - polynomial     = 0x15
        - initial value  = 0x00
        - final XOR      = 0x00
        - reflect input  = true
        - reflect output = true
        - check value    = 0x07
    @return CRC-5 ITU parameters
*/
inline const CRC::Parameters<crcpp_uint8, 5> & CRC::CRC_5_ITU()
{
    static const Parameters<crcpp_uint8, 5> parameters = { 0x15, 0x00, 0x00, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-5 USB.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-5 USB has the following parameters and check value:
        - polynomial     = 0x05
        - initial value  = 0x1F
        - final XOR      = 0x1F
        - reflect input  = true
        - reflect output = true
        - check value    = 0x19
    @return CRC-5 USB parameters
*/
inline const CRC::Parameters<crcpp_uint8, 5> & CRC::CRC_5_USB()
{
    static const Parameters<crcpp_uint8, 5> parameters = { 0x05, 0x1F, 0x1F, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-6 CDMA2000-A.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-6 CDMA2000-A has the following parameters and check value:
        - polynomial     = 0x27
        - initial value  = 0x3F
        - final XOR      = 0x00
        - reflect input  = false
        - reflect output = false
        - check value    = 0x0D
    @return CRC-6 CDMA2000-A parameters
*/
inline const CRC::Parameters<crcpp_uint8, 6> & CRC::CRC_6_CDMA2000A()
{
    static const Parameters<crcpp_uint8, 6> parameters = { 0x27, 0x3F, 0x00, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-6 CDMA2000-B.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-6 CDMA2000-A has the following parameters and check value:
        - polynomial     = 0x07
        - initial value  = 0x3F
        - final XOR      = 0x00
        - reflect input  = false
        - reflect output = false
        - check value    = 0x3B
    @return CRC-6 CDMA2000-B parameters
*/
inline const CRC::Parameters<crcpp_uint8, 6> & CRC::CRC_6_CDMA2000B()
{
    static const Parameters<crcpp_uint8, 6> parameters = { 0x07, 0x3F, 0x00, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-6 ITU.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-6 ITU has the following parameters and check value:
        - polynomial     = 0x03
        - initial value  = 0x00
        - final XOR      = 0x00
        - reflect input  = true
        - reflect output = true
        - check value    = 0x06
    @return CRC-6 ITU parameters
*/
inline const CRC::Parameters<crcpp_uint8, 6> & CRC::CRC_6_ITU()
{
    static const Parameters<crcpp_uint8, 6> parameters = { 0x03, 0x00, 0x00, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-7 JEDEC.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-7 JEDEC has the following parameters and check value:
        - polynomial     = 0x09
        - initial value  = 0x00
        - final XOR      = 0x00
        - reflect input  = false
        - reflect output = false
        - check value    = 0x75
    @return CRC-7 JEDEC parameters
*/
inline const CRC::Parameters<crcpp_uint8, 7> & CRC::CRC_7()
{
    static const Parameters<crcpp_uint8, 7> parameters = { 0x09, 0x00, 0x00, false, false };
    return parameters;
}
#endif // CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

/**
    @brief Returns a set of parameters for CRC-8 SMBus.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-8 SMBus has the following parameters and check value:
        - polynomial     = 0x07
        - initial value  = 0x00
        - final XOR      = 0x00
        - reflect input  = false
        - reflect output = false
        - check value    = 0xF4
    @return CRC-8 SMBus parameters
*/
inline const CRC::Parameters<crcpp_uint8, 8> & CRC::CRC_8()
{
    static const Parameters<crcpp_uint8, 8> parameters = { 0x07, 0x00, 0x00, false, false };
    return parameters;
}

#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
/**
    @brief Returns a set of parameters for CRC-8 EBU (aka CRC-8 AES).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-8 EBU has the following parameters and check value:
        - polynomial     = 0x1D
        - initial value  = 0xFF
        - final XOR      = 0x00
        - reflect input  = true
        - reflect output = true
        - check value    = 0x97
    @return CRC-8 EBU parameters
*/
inline const CRC::Parameters<crcpp_uint8, 8> & CRC::CRC_8_EBU()
{
    static const Parameters<crcpp_uint8, 8> parameters = { 0x1D, 0xFF, 0x00, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-8 MAXIM (aka CRC-8 DOW-CRC).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-8 MAXIM has the following parameters and check value:
        - polynomial     = 0x31
        - initial value  = 0x00
        - final XOR      = 0x00
        - reflect input  = true
        - reflect output = true
        - check value    = 0xA1
    @return CRC-8 MAXIM parameters
*/
inline const CRC::Parameters<crcpp_uint8, 8> & CRC::CRC_8_MAXIM()
{
    static const Parameters<crcpp_uint8, 8> parameters = { 0x31, 0x00, 0x00, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-8 WCDMA.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-8 WCDMA has the following parameters and check value:
        - polynomial     = 0x9B
        - initial value  = 0x00
        - final XOR      = 0x00
        - reflect input  = true
        - reflect output = true
        - check value    = 0x25
    @return CRC-8 WCDMA parameters
*/
inline const CRC::Parameters<crcpp_uint8, 8> & CRC::CRC_8_WCDMA()
{
    static const Parameters<crcpp_uint8, 8> parameters = { 0x9B, 0x00, 0x00, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-10 ITU.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-10 ITU has the following parameters and check value:
        - polynomial     = 0x233
        - initial value  = 0x000
        - final XOR      = 0x000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x199
    @return CRC-10 ITU parameters
*/
inline const CRC::Parameters<crcpp_uint16, 10> & CRC::CRC_10()
{
    static const Parameters<crcpp_uint16, 10> parameters = { 0x233, 0x000, 0x000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-10 CDMA2000.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-10 CDMA2000 has the following parameters and check value:
        - polynomial     = 0x3D9
        - initial value  = 0x3FF
        - final XOR      = 0x000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x233
    @return CRC-10 CDMA2000 parameters
*/
inline const CRC::Parameters<crcpp_uint16, 10> & CRC::CRC_10_CDMA2000()
{
    static const Parameters<crcpp_uint16, 10> parameters = { 0x3D9, 0x3FF, 0x000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-11 FlexRay.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-11 FlexRay has the following parameters and check value:
        - polynomial     = 0x385
        - initial value  = 0x01A
        - final XOR      = 0x000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x5A3
    @return CRC-11 FlexRay parameters
*/
inline const CRC::Parameters<crcpp_uint16, 11> & CRC::CRC_11()
{
    static const Parameters<crcpp_uint16, 11> parameters = { 0x385, 0x01A, 0x000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-12 CDMA2000.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-12 CDMA2000 has the following parameters and check value:
        - polynomial     = 0xF13
        - initial value  = 0xFFF
        - final XOR      = 0x000
        - reflect input  = false
        - reflect output = false
        - check value    = 0xD4D
    @return CRC-12 CDMA2000 parameters
*/
inline const CRC::Parameters<crcpp_uint16, 12> & CRC::CRC_12_CDMA2000()
{
    static const Parameters<crcpp_uint16, 12> parameters = { 0xF13, 0xFFF, 0x000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-12 DECT (aka CRC-12 X-CRC).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-12 DECT has the following parameters and check value:
        - polynomial     = 0x80F
        - initial value  = 0x000
        - final XOR      = 0x000
        - reflect input  = false
        - reflect output = false
        - check value    = 0xF5B
    @return CRC-12 DECT parameters
*/
inline const CRC::Parameters<crcpp_uint16, 12> & CRC::CRC_12_DECT()
{
    static const Parameters<crcpp_uint16, 12> parameters = { 0x80F, 0x000, 0x000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-12 UMTS (aka CRC-12 3GPP).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-12 UMTS has the following parameters and check value:
        - polynomial     = 0x80F
        - initial value  = 0x000
        - final XOR      = 0x000
        - reflect input  = false
        - reflect output = true
        - check value    = 0xDAF
    @return CRC-12 UMTS parameters
*/
inline const CRC::Parameters<crcpp_uint16, 12> & CRC::CRC_12_UMTS()
{
    static const Parameters<crcpp_uint16, 12> parameters = { 0x80F, 0x000, 0x000, false, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-13 BBC.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-13 BBC has the following parameters and check value:
        - polynomial     = 0x1CF5
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x04FA
    @return CRC-13 BBC parameters
*/
inline const CRC::Parameters<crcpp_uint16, 13> & CRC::CRC_13_BBC()
{
    static const Parameters<crcpp_uint16, 13> parameters = { 0x1CF5, 0x0000, 0x0000, false, false };
   return parameters;
}

/**
    @brief Returns a set of parameters for CRC-15 CAN.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-15 CAN has the following parameters and check value:
        - polynomial     = 0x4599
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x059E
    @return CRC-15 CAN parameters
*/
inline const CRC::Parameters<crcpp_uint16, 15> & CRC::CRC_15()
{
    static const Parameters<crcpp_uint16, 15> parameters = { 0x4599, 0x0000, 0x0000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-15 MPT1327.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-15 MPT1327 has the following parameters and check value:
        - polynomial     = 0x6815
        - initial value  = 0x0000
        - final XOR      = 0x0001
        - reflect input  = false
        - reflect output = false
        - check value    = 0x2566
    @return CRC-15 MPT1327 parameters
*/
inline const CRC::Parameters<crcpp_uint16, 15> & CRC::CRC_15_MPT1327()
{
    static const Parameters<crcpp_uint16, 15> parameters = { 0x6815, 0x0000, 0x0001, false, false };
    return parameters;
}
#endif // CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

/**
    @brief Returns a set of parameters for CRC-16 ARC (aka CRC-16 IBM, CRC-16 LHA).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 ARC has the following parameters and check value:
        - polynomial     = 0x8005
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = true
        - reflect output = true
        - check value    = 0xBB3D
    @return CRC-16 ARC parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_ARC()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x8005, 0x0000, 0x0000, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 BUYPASS (aka CRC-16 VERIFONE, CRC-16 UMTS).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 BUYPASS has the following parameters and check value:
        - polynomial     = 0x8005
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0xFEE8
    @return CRC-16 BUYPASS parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_BUYPASS()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x8005, 0x0000, 0x0000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 CCITT FALSE.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 CCITT FALSE has the following parameters and check value:
        - polynomial     = 0x1021
        - initial value  = 0xFFFF
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x29B1
    @return CRC-16 CCITT FALSE parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_CCITTFALSE()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x1021, 0xFFFF, 0x0000, false, false };
    return parameters;
}

#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
/**
    @brief Returns a set of parameters for CRC-16 CDMA2000.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 CDMA2000 has the following parameters and check value:
        - polynomial     = 0xC867
        - initial value  = 0xFFFF
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x4C06
    @return CRC-16 CDMA2000 parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_CDMA2000()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0xC867, 0xFFFF, 0x0000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 DECT-R (aka CRC-16 R-CRC).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 DECT-R has the following parameters and check value:
        - polynomial     = 0x0589
        - initial value  = 0x0000
        - final XOR      = 0x0001
        - reflect input  = false
        - reflect output = false
        - check value    = 0x007E
    @return CRC-16 DECT-R parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_DECTR()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x0589, 0x0000, 0x0001, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 DECT-X (aka CRC-16 X-CRC).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 DECT-X has the following parameters and check value:
        - polynomial     = 0x0589
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x007F
    @return CRC-16 DECT-X parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_DECTX()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x0589, 0x0000, 0x0000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 DNP.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 DNP has the following parameters and check value:
        - polynomial     = 0x3D65
        - initial value  = 0x0000
        - final XOR      = 0xFFFF
        - reflect input  = true
        - reflect output = true
        - check value    = 0xEA82
    @return CRC-16 DNP parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_DNP()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x3D65, 0x0000, 0xFFFF, true, true };
    return parameters;
}
#endif // CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

/**
    @brief Returns a set of parameters for CRC-16 GENIBUS (aka CRC-16 EPC, CRC-16 I-CODE, CRC-16 DARC).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 GENIBUS has the following parameters and check value:
        - polynomial     = 0x1021
        - initial value  = 0xFFFF
        - final XOR      = 0xFFFF
        - reflect input  = false
        - reflect output = false
        - check value    = 0xD64E
    @return CRC-16 GENIBUS parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_GENIBUS()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x1021, 0xFFFF, 0xFFFF, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 KERMIT (aka CRC-16 CCITT, CRC-16 CCITT-TRUE).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 KERMIT has the following parameters and check value:
        - polynomial     = 0x1021
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = true
        - reflect output = true
        - check value    = 0x2189
    @return CRC-16 KERMIT parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_KERMIT()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x1021, 0x0000, 0x0000, true, true };
    return parameters;
}

#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
/**
    @brief Returns a set of parameters for CRC-16 MAXIM.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 MAXIM has the following parameters and check value:
        - polynomial     = 0x8005
        - initial value  = 0x0000
        - final XOR      = 0xFFFF
        - reflect input  = true
        - reflect output = true
        - check value    = 0x44C2
    @return CRC-16 MAXIM parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_MAXIM()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x8005, 0x0000, 0xFFFF, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 MODBUS.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 MODBUS has the following parameters and check value:
        - polynomial     = 0x8005
        - initial value  = 0xFFFF
        - final XOR      = 0x0000
        - reflect input  = true
        - reflect output = true
        - check value    = 0x4B37
    @return CRC-16 MODBUS parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_MODBUS()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x8005, 0xFFFF, 0x0000, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 T10-DIF.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 T10-DIF has the following parameters and check value:
        - polynomial     = 0x8BB7
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0xD0DB
    @return CRC-16 T10-DIF parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_T10DIF()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x8BB7, 0x0000, 0x0000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 USB.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 USB has the following parameters and check value:
        - polynomial     = 0x8005
        - initial value  = 0xFFFF
        - final XOR      = 0xFFFF
        - reflect input  = true
        - reflect output = true
        - check value    = 0xB4C8
    @return CRC-16 USB parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_USB()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x8005, 0xFFFF, 0xFFFF, true, true };
    return parameters;
}
#endif // CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

/**
    @brief Returns a set of parameters for CRC-16 X-25 (aka CRC-16 IBM-SDLC, CRC-16 ISO-HDLC, CRC-16 B).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 X-25 has the following parameters and check value:
        - polynomial     = 0x1021
        - initial value  = 0xFFFF
        - final XOR      = 0xFFFF
        - reflect input  = true
        - reflect output = true
        - check value    = 0x906E
    @return CRC-16 X-25 parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_X25()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x1021, 0xFFFF, 0xFFFF, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-16 XMODEM (aka CRC-16 ZMODEM, CRC-16 ACORN, CRC-16 LTE).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-16 XMODEM has the following parameters and check value:
        - polynomial     = 0x1021
        - initial value  = 0x0000
        - final XOR      = 0x0000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x31C3
    @return CRC-16 XMODEM parameters
*/
inline const CRC::Parameters<crcpp_uint16, 16> & CRC::CRC_16_XMODEM()
{
    static const Parameters<crcpp_uint16, 16> parameters = { 0x1021, 0x0000, 0x0000, false, false };
    return parameters;
}

#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
/**
    @brief Returns a set of parameters for CRC-17 CAN.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-17 CAN has the following parameters and check value:
        - polynomial     = 0x1685B
        - initial value  = 0x00000
        - final XOR      = 0x00000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x04F03
    @return CRC-17 CAN parameters
*/
inline const CRC::Parameters<crcpp_uint32, 17> & CRC::CRC_17_CAN()
{
    static const Parameters<crcpp_uint32, 17> parameters = { 0x1685B, 0x00000, 0x00000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-21 CAN.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-21 CAN has the following parameters and check value:
        - polynomial     = 0x102899
        - initial value  = 0x000000
        - final XOR      = 0x000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x0ED841
    @return CRC-21 CAN parameters
*/
inline const CRC::Parameters<crcpp_uint32, 21> & CRC::CRC_21_CAN()
{
    static const Parameters<crcpp_uint32, 21> parameters = { 0x102899, 0x000000, 0x000000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-24 OPENPGP.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-24 OPENPGP has the following parameters and check value:
        - polynomial     = 0x864CFB
        - initial value  = 0xB704CE
        - final XOR      = 0x000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x21CF02
    @return CRC-24 OPENPGP parameters
*/
inline const CRC::Parameters<crcpp_uint32, 24> & CRC::CRC_24()
{
    static const Parameters<crcpp_uint32, 24> parameters = { 0x864CFB, 0xB704CE, 0x000000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-24 FlexRay-A.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-24 FlexRay-A has the following parameters and check value:
        - polynomial     = 0x5D6DCB
        - initial value  = 0xFEDCBA
        - final XOR      = 0x000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x7979BD
    @return CRC-24 FlexRay-A parameters
*/
inline const CRC::Parameters<crcpp_uint32, 24> & CRC::CRC_24_FLEXRAYA()
{
    static const Parameters<crcpp_uint32, 24> parameters = { 0x5D6DCB, 0xFEDCBA, 0x000000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-24 FlexRay-B.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-24 FlexRay-B has the following parameters and check value:
        - polynomial     = 0x5D6DCB
        - initial value  = 0xABCDEF
        - final XOR      = 0x000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x1F23B8
    @return CRC-24 FlexRay-B parameters
*/
inline const CRC::Parameters<crcpp_uint32, 24> & CRC::CRC_24_FLEXRAYB()
{
    static const Parameters<crcpp_uint32, 24> parameters = { 0x5D6DCB, 0xABCDEF, 0x000000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-30 CDMA.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-30 CDMA has the following parameters and check value:
        - polynomial     = 0x2030B9C7
        - initial value  = 0x3FFFFFFF
        - final XOR      = 0x00000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x3B3CB540
    @return CRC-30 CDMA parameters
*/
inline const CRC::Parameters<crcpp_uint32, 30> & CRC::CRC_30()
{
    static const Parameters<crcpp_uint32, 30> parameters = { 0x2030B9C7, 0x3FFFFFFF, 0x00000000, false, false };
    return parameters;
}
#endif // CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

/**
    @brief Returns a set of parameters for CRC-32 (aka CRC-32 ADCCP, CRC-32 PKZip).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-32 has the following parameters and check value:
        - polynomial     = 0x04C11DB7
        - initial value  = 0xFFFFFFFF
        - final XOR      = 0xFFFFFFFF
        - reflect input  = true
        - reflect output = true
        - check value    = 0xCBF43926
    @return CRC-32 parameters
*/
inline const CRC::Parameters<crcpp_uint32, 32> & CRC::CRC_32()
{
    static const Parameters<crcpp_uint32, 32> parameters = { 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-32 BZIP2 (aka CRC-32 AAL5, CRC-32 DECT-B, CRC-32 B-CRC).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-32 BZIP2 has the following parameters and check value:
        - polynomial     = 0x04C11DB7
        - initial value  = 0xFFFFFFFF
        - final XOR      = 0xFFFFFFFF
        - reflect input  = false
        - reflect output = false
        - check value    = 0xFC891918
    @return CRC-32 BZIP2 parameters
*/
inline const CRC::Parameters<crcpp_uint32, 32> & CRC::CRC_32_BZIP2()
{
    static const Parameters<crcpp_uint32, 32> parameters = { 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, false, false };
    return parameters;
}

#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
/**
    @brief Returns a set of parameters for CRC-32 C (aka CRC-32 ISCSI, CRC-32 Castagnoli, CRC-32 Interlaken).
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-32 C has the following parameters and check value:
        - polynomial     = 0x1EDC6F41
        - initial value  = 0xFFFFFFFF
        - final XOR      = 0xFFFFFFFF
        - reflect input  = true
        - reflect output = true
        - check value    = 0xE3069283
    @return CRC-32 C parameters
*/
inline const CRC::Parameters<crcpp_uint32, 32> & CRC::CRC_32_C()
{
    static const Parameters<crcpp_uint32, 32> parameters = { 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true };
    return parameters;
}
#endif

/**
    @brief Returns a set of parameters for CRC-32 MPEG-2.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-32 MPEG-2 has the following parameters and check value:
        - polynomial     = 0x04C11DB7
        - initial value  = 0xFFFFFFFF
        - final XOR      = 0x00000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x0376E6E7
    @return CRC-32 MPEG-2 parameters
*/
inline const CRC::Parameters<crcpp_uint32, 32> & CRC::CRC_32_MPEG2()
{
    static const Parameters<crcpp_uint32, 32> parameters = { 0x04C11DB7, 0xFFFFFFFF, 0x00000000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-32 POSIX.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-32 POSIX has the following parameters and check value:
        - polynomial     = 0x04C11DB7
        - initial value  = 0x00000000
        - final XOR      = 0xFFFFFFFF
        - reflect input  = false
        - reflect output = false
        - check value    = 0x765E7680
    @return CRC-32 POSIX parameters
*/
inline const CRC::Parameters<crcpp_uint32, 32> & CRC::CRC_32_POSIX()
{
    static const Parameters<crcpp_uint32, 32> parameters = { 0x04C11DB7, 0x00000000, 0xFFFFFFFF, false, false };
    return parameters;
}

#ifdef CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
/**
    @brief Returns a set of parameters for CRC-32 Q.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-32 Q has the following parameters and check value:
        - polynomial     = 0x814141AB
        - initial value  = 0x00000000
        - final XOR      = 0x00000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x3010BF7F
    @return CRC-32 Q parameters
*/
inline const CRC::Parameters<crcpp_uint32, 32> & CRC::CRC_32_Q()
{
    static const Parameters<crcpp_uint32, 32> parameters = { 0x814141AB, 0x00000000, 0x00000000, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-40 GSM.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-40 GSM has the following parameters and check value:
        - polynomial     = 0x0004820009
        - initial value  = 0x0000000000
        - final XOR      = 0xFFFFFFFFFF
        - reflect input  = false
        - reflect output = false
        - check value    = 0xD4164FC646
    @return CRC-40 GSM parameters
*/
inline const CRC::Parameters<crcpp_uint64, 40> & CRC::CRC_40_GSM()
{
    static const Parameters<crcpp_uint64, 40> parameters = { 0x0004820009, 0x0000000000, 0xFFFFFFFFFF, false, false };
    return parameters;
}

/**
    @brief Returns a set of parameters for CRC-64 ECMA.
    @note The parameters are static and are delayed-constructed to reduce memory footprint.
    @note CRC-64 ECMA has the following parameters and check value:
        - polynomial     = 0x42F0E1EBA9EA3693
        - initial value  = 0x0000000000000000
        - final XOR      = 0x0000000000000000
        - reflect input  = false
        - reflect output = false
        - check value    = 0x6C40DF5F0B497347
    @return CRC-64 ECMA parameters
*/
inline const CRC::Parameters<crcpp_uint64, 64> & CRC::CRC_64()
{
    static const Parameters<crcpp_uint64, 64> parameters = { 0x42F0E1EBA9EA3693, 0x0000000000000000, 0x0000000000000000, false, false };
    return parameters;
}
#endif // CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

#ifdef CRCPP_USE_NAMESPACE
}
#endif

#endif // CRCPP_CRC_H_
