//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//
// Invocation of code elements
// ---------------------------
// ARIB STD-B24, part 2, chapter 7, table 7-1
//
//            Codes           Code  => Invocation  Invocation
//   Acronym  representation  element  area        effect
//
//   LS0      0F              G0       GL          Locking shift
//   LS1      0E              G1       GL          Locking shift
//   LS2      1B 6E           G2       GL          Locking shift
//   LS3      1B 6F           G3       GL          Locking shift
//   LS1R     1B 7E           G1       GR          Locking shift
//   LS2R     1B 7D           G2       GR          Locking shift
//   LS3R     1B 7C           G3       GR          Locking shift
//   SS2      19              G2       GL          Single shift
//   SS3      1D              G3       GL          Single shift
//
//
// Designation of graphic sets
// ---------------------------
// ARIB STD-B24, part 2, chapter 7, table 7-2
//
//   Codes           Classification   Designated
//   representation  of graphic sets  element
//
//   1B 28 F         1-byte G set     G0
//   1B 29 F         -                G1
//   1B 2A F         -                G2
//   1B 2B F         -                G3
//   1B 24 F         2-byte G set     G0
//   1B 24 29 F      -                G1
//   1B 24 2A F      -                G2
//   1B 24 2B F      -                G3
//   1B 28 20 F      1-byte DRCS      G0
//   1B 29 20 F      -                G1
//   1B 2A 20 F      -                G2
//   1B 2B 20 F      -                G3
//   1B 24 28 20 F   2-byte DRCS      G0
//   1B 24 29 20 F   -                G1
//   1B 24 2A 20 F   -                G2
//   1B 24 2B 20 F   -                G3
//
//
// Classification of code set and final byte (F)
// ---------------------------------------------
// ARIB STD-B24, part 2, chapter 7, table 7-3
//
//   Classification                             Final
//   of graphic sets  Graphic sets              (F)    Remarks
//
//   G set            Kanji                     42     2-byte code
//   -                Alphanumeric              4A     1-byte code
//   -                Hiragana                  30     1-byte code
//   -                Katakana                  31     1-byte code
//   -                Mosaic A                  32     1-byte code
//   -                Mosaic B                  33     1-byte code
//   -                Mosaic C                  34     1-byte code, non-spacing
//   -                Mosaic D                  35     1-byte code, non-spacing
//   -                Proportional alphanumeric 36     1-byte code
//   -                Proportional hiragana     37     1-byte code
//   -                Proportional katakana     38     1-byte code
//   -                JIS X 0201 katakana       49     1-byte code
//   -                JIS comp. Kanji Plane 1   39     2-byte code
//   -                JIS comp. Kanji Plane 2   3A     2-byte code
//   -                Additional symbols        3B     2-byte code
//   DRCS             DRCS-0                    40     2-byte code
//   -                DRCS-1                    41     1-byte code
//   -                DRCS-2                    42     1-byte code
//   -                DRCS-3                    43     1-byte code
//   -                DRCS-4                    44     1-byte code
//   -                DRCS-5                    45     1-byte code
//   -                DRCS-6                    46     1-byte code
//   -                DRCS-7                    47     1-byte code
//   -                DRCS-8                    48     1-byte code
//   -                DRCS-9                    49     1-byte code
//   -                DRCS-10                   4A     1-byte code
//   -                DRCS-11                   4B     1-byte code
//   -                DRCS-12                   4C     1-byte code
//   -                DRCS-13                   4D     1-byte code
//   -                DRCS-14                   4E     1-byte code
//   -                DRCS-15                   4F     1-byte code
//   -                Macro                     70     1-byte code
//
//
//----------------------------------------------------------------------------

#include "tsARIBCharset.h"
#include "tsUString.h"
#include "tsMemory.h"

// Define single instance
const ts::ARIBCharset2 ts::ARIBCharset2::B24({u"ARIB-STD-B24-2", u"ARIB-2"});


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ARIBCharset2::ARIBCharset2(std::initializer_list<const UChar*> names) :
    Charset(names)
{
}

//----------------------------------------------------------------------------
// Find the encoding entry for a Unicode point.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset2::FindEncoderEntry(char32_t code_point, size_t hint)
{
    // If a hint is specified, tried this slice, its next and its previous.
    if (hint < ENCODING_COUNT) {
        if (ENCODING_TABLE[hint].contains(code_point)) {
            // Found in same slice.
            return hint;
        }
        else if (hint + 1 < ENCODING_COUNT && ENCODING_TABLE[hint + 1].contains(code_point)) {
            // Found in next slice.
            return hint + 1;
        }
        else if (hint > 0 && ENCODING_TABLE[hint - 1].contains(code_point)) {
            // Found in previous slice.
            return hint - 1;
        }
        // Code point is too far, hint was useless, try standard method.
    }

    // Dichotomic search.
    size_t begin = 0;
    size_t end = ENCODING_COUNT;

    while (begin < end) {
        const size_t mid = begin + (end - begin) / 2;
        if (ENCODING_TABLE[mid].contains(code_point)) {
            return mid;
        }
        else if (code_point < ENCODING_TABLE[mid].code_point) {
            end = mid;
        }
        else {
            begin = mid + 1;
        }
    }

    return NPOS;
}


//----------------------------------------------------------------------------
// Check if a string can be encoded using the charset.
//----------------------------------------------------------------------------

bool ts::ARIBCharset2::canEncode(const UString& str, size_t start, size_t count) const
{
    const size_t len = str.length();
    const size_t end = count > len ? len : std::min(len, start + count);

    // Look for an encoding entry for each character.
    size_t index = 0;
    for (size_t i = start; i < end; ++i) {
        const UChar c = str[i];

        // Space is not in the encoding table but is always valid.
        if (c != SPACE && c != IDEOGRAPHIC_SPACE) {
            if (!IsLeadingSurrogate(c)) {
                // 16-bit code point
                index = FindEncoderEntry(c, index);
            }
            else if (++i >= len) {
                // Truncated surrogate pair.
                return false;
            }
            else {
                // Rebuilt 32-bit code point from surrogate pair.
                index = FindEncoderEntry(FromSurrogatePair(c, str[i]), index);
            }
            // Stop when a character cannot be encoded.
            if (index == NPOS) {
                return false;
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string into an ARIB string.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset2::encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    const size_t len = str.length();
    if (buffer == nullptr || size == 0 || start >= len) {
        return 0;
    }
    else {
        const UChar* const initial = str.data() + start;
        const UChar* in = initial;
        size_t in_count = count >= len || start + count >= len ? (len - start) : count;
        Encoder enc(buffer, size, in, in_count);

        return in - initial;
    }
}


//----------------------------------------------------------------------------
// An internal encoder class.
//----------------------------------------------------------------------------

ts::ARIBCharset2::Encoder::Encoder(uint8_t*& out, size_t& out_size, const UChar*& in, size_t& in_count) :
    _G{KANJI_STANDARD_MAP.selector1,   // Same initial state as decoding engine
       ALPHANUMERIC_MAP.selector1,
       HIRAGANA_MAP.selector1,
       JIS_X0201_KATAKANA_MAP.selector1},
    _byte2{KANJI_STANDARD_MAP.byte2,   // Same order as _G
           ALPHANUMERIC_MAP.byte2,
           HIRAGANA_MAP.byte2,
           JIS_X0201_KATAKANA_MAP.byte2},
    _GL(0),             // G0 -> GL
    _GR(2),             // G2 -> GR
    _GL_last(false),    // First charset switch will use GL
    _Gn_history(0x3210), // G3=oldest, G0=last-used
    first(true),
    character_size(NSZ)
{
    // Previous index in encoding table.
    size_t prev_index = NPOS;

    // Loop on input UTF-16 characters.
    while (in_count > 0 && out_size > 0) {

        // Get unicode code point (1 or 2 UChar from input).
        char32_t cp = *in;
        size_t cp_size = 1;
        if (IsLeadingSurrogate(*in)) {
            // Need a second UChar.
            if (in_count < 2) {
                // End of string, truncated surrogate pair. Consume the first char so that
                // the caller does not infinitely try to decode the rest of the string.
                ++in;
                in_count = 0;
                return;
            }
            else {
                // Rebuild the Unicode point from the pair.
                cp = FromSurrogatePair(in[0], in[1]);
                cp_size = 2;
            }
        }

        if (cp == '\0') {
            *out++ = '\0';
            --out_size;
        }
        else {
            // Find the entry for this code point in the encoding table.
            const size_t index = FindEncoderEntry(cp, prev_index);
            if (index != NPOS) {
                // This character is encodable.
                assert(index < ENCODING_COUNT);
                const EncoderEntry& enc(ENCODING_TABLE[index]);
                prev_index = index;

                // Make sure the right character set is selected.
                // Insert the corresponding escape sequence if necessary.
                // Also make sure that the encoded sequence will fit in output buffer.
                if (!selectCharSet(out, out_size, enc.selectorF(), enc.byte2())) {
                    // Cannot insert the right sequence. Do not attempt to encode the code point.
                    return;
                }

                // Insert the encoded code point (1 or 2 bytes).
                assert(cp >= enc.code_point);
                assert(cp < enc.code_point + enc.count());
                assert(cp - enc.code_point + enc.index() <= GL_LAST);
                const uint8_t mask = enc.selectorF() == _G[_GR] ? 0x80 : 0x00;
                if (enc.byte2()) {
                    // 2-byte character set, insert row first.
                    assert(out_size >= 2);
                    *out++ = enc.row() | mask;
                    --out_size;
                }
                assert(out_size >= 1);
                *out++ = uint8_t(cp - enc.code_point + enc.index()) | mask;
                --out_size;
            }
            else if ((cp == SPACE || cp == IDEOGRAPHIC_SPACE) && !encodeSpace(out, out_size, cp == IDEOGRAPHIC_SPACE)) {
                // Tried to encode a space but failed.
                return;
            }
        }

        // Now, the character has been successfully encoded (or ignored if not encodable).
        // Remove it from the input buffer.
        in += cp_size;
        in_count -= cp_size;
    }
}


//----------------------------------------------------------------------------
// Check if Gn (n=0-3) is alphanumeric.
//----------------------------------------------------------------------------

bool ts::ARIBCharset2::Encoder::isAlphaNumeric(uint8_t index) const
{
    return _G[index] == ALPHANUMERIC_MAP.selector1 || _G[index] == ALPHANUMERIC_MAP.selector2;
}


//----------------------------------------------------------------------------
// Encode a space, alphanumeric or ideographic.
//----------------------------------------------------------------------------

bool ts::ARIBCharset2::Encoder::encodeSpace(uint8_t*& out, size_t& out_size, bool ideographic)
{
    uint8_t code = 0;
    size_t count = 0;

    if (ideographic) {
        // Insert a space SP (0x20) in any ideographic (non-alphanumeric) character set.
        // We assume that at least GL or GR is not alphanumeric, so there is not need to switch character set.
        // If the two are ideographic, try to find one which uses 1-byte encoding.
        if (!_byte2[_GL] && !isAlphaNumeric(_GL)) {
            code = SP;
            count = 1;
        }
        else if (!_byte2[_GR] && !isAlphaNumeric(_GR)) {
            code = SP | 0x80;
            count = 1;
        }
        else if (!isAlphaNumeric(_GL)) {
            assert(_byte2[_GL]);
            code = SP;
            count = 2;
        }
        else {
            assert(_byte2[_GR] && !isAlphaNumeric(_GR));
            code = SP | 0x80;
            count = 2;
        }
    }
    else {
        // Insert a space SP (0x20) in alphanumeric character set.
        if (isAlphaNumeric(_GL)) {
            code = SP;
            count = 1;
        }
        else if (isAlphaNumeric(_GR)) {
            code = SP | 0x80;
            count = 1;
        }
        else if (selectCharSet(out, out_size, ALPHANUMERIC_MAP.selector1, false)) {
            code = ALPHANUMERIC_MAP.selector1 == _G[_GR] ? (SP | 0x80) : SP;
            count = 1;
        }
        else {
            // Cannot insert the sequence to switch to alphanumeric character set.
            return false;
        }
    }

    // Insert the encoded space.
    if (count > out_size) {
        return false;
    }
    else {
        while (count-- > 0) {
            *out++ = code;
            --out_size;
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Switch to a given character set (from selector F).
// Not really optimized. We always keep the same character set in G0.
//----------------------------------------------------------------------------

bool ts::ARIBCharset2::Encoder::selectCharSet(uint8_t*& out, size_t& out_size, uint8_t selectorF, bool byte2)
{
    // Required space for one character after escape sequence.
    const size_t char_size = byte2 ? 2 : 1;

    // An escape sequence is up to 7 bytes.
    uint8_t seq[7];
    size_t seq_size = 0;

    if(selectorF == 0x4A /* ascii */) {
        if(character_size != MSZ) {
            seq[0] = MSZ;
            character_size = MSZ;
            seq_size++;
        }
    }
    else {
        if(character_size != NSZ) {
            seq[0] = NSZ;
            character_size = NSZ;
            seq_size++;
        }
    }

    // There is some switching sequence to add only if the charset is neither in GL nor GR.
    if (selectorF != _G[_GL] && selectorF != _G[_GR]) {
        // If the charset is not in G0-G3, we need to load it in one of them.
        if (selectorF != _G[0] && selectorF != _G[1] && selectorF != _G[2] && selectorF != _G[3]) {
            seq_size += selectG0123(seq + seq_size, selectorF, byte2);
        }
        // Route the right Gx in either GL or GR.
        seq_size += selectGLR(seq + seq_size, selectorF);
        first = false;
    }

    // Finally, insert the escape sequence if there is enough room for it plus one character.
    if (seq_size + char_size > out_size) {
        return false;
    }
    if (seq_size > 0) {
        assert(seq_size < sizeof(seq));
        MemCopy(out, seq, seq_size);
        out += seq_size;
        out_size -= seq_size;
    }

    // Keep track of last GL/GR used.
    _GL_last = _G[_GL] == selectorF;
    return true;
}


//----------------------------------------------------------------------------
// Select GL/GR from G0-3 for a given selector F. Return escape sequence size.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset2::Encoder::selectGLR(uint8_t* seq, uint8_t F)
{
    int i = 0;

    // If GL was last used, use GR and vice versa.
    if (F == _G[0]) {
        // G0 can be routed to GL only.
        _GL = 0;
        seq[i++] = LS0;
        return i;
    }
    else if (F == _G[1]) {
        if (_GL_last) {
            _GR = 1;
            seq[i++] = ESC; seq[i++] = 0x7E;
            return i;

        }
        else {
            _GL = 1;
             seq[i++] = LS1;
            return i;
        }
    }
    else if (F == _G[2]) {
        if (_GL_last) {
            _GR = 2;
            seq[i++] = ESC; seq[i++] = 0x7D;
            return i;

        }
        else {
            _GL = 2;
            seq[i++] = ESC; seq[i++] = 0x6E;
            return i;
        }
    }
    else {
        assert(F == _G[3]);
        if (_GL_last) {
            _GR = 3;
            seq[i++] = ESC; seq[i++] = 0x7C;
            return i;
        }
        else {
            _GL = 3;
            seq[i++] = ESC; seq[i++] = 0x6F;
            return i;
        }
    }
}


//----------------------------------------------------------------------------
// Set G0-3 to a given selector F. Return escape sequence size.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset2::Encoder::selectG0123(uint8_t* seq, uint8_t F, bool byte2)
{
    const uint8_t index = 0;

    // Assign the new character set.
    _G[index] = F;
    _byte2[index] = byte2;

    // Generate the escape sequence.
    // ARIB STD-B24, part 2, chapter 7, table 7-2
    if (!byte2) {
        // 1-byte G set in G0-3
        seq[0] = ESC;
        seq[1] = 0x28 + index;
        seq[2] = F;
        return 3;
    }
    else if (index == 0) {
        // 2-byte G set in G0
        seq[0] = ESC;
        seq[1] = 0x24;
        seq[2] = F;
        return 3;
    }
    else {
        // 2-byte G set in G1-3
        seq[0] = ESC;
        seq[1] = 0x24;
        seq[2] = 0x28 + index;
        seq[3] = F;
        return 4;
    }
}