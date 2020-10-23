////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/21/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        stdio.c
//      Environment: Tiny OS
//      Description: stdio defines the vsnprintf, snprintf, printf and the internal
//                   __printf and __vprintf methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <lib/stdlib.h>
#include <lib/string.h>
#include <lib/stdint.h>
#include <lib/stdbool.h>
#include <lib/stdarg.h>
#include <lib/ctype.h>

#include <debug.h>

#include <lib/stdio.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A format specifier follows this prototype: %[flags][width][.precision][length]type.
//
// Supported Types:
// - d (or i): signed decimal integer
// - u: unsigned decimal integer
// - b: unsigned binary
// - o: unsigned octal
// - x: unsigned hexadecimal integer (lowercase)
// - X: unsigned hexadecimal integer (uppercase)
// - f: decimal floating point        (TODO)
// - e (or E): decimal floating point (TODO)
// - g (or G): decimal floating point (TODO)
// - c: single character
// - s: string
// - p: pointer
// - %: %% is printed as %
//
// Supported Flags:
// - -: Left-justify within the given field width
// - 0: Field is padded with 0's instead of blanks.
// - +: Forces to precede the result with a plus or minus sign (even for positive numbers)
// - (blank): If no sign is going to be written, a blank is inserted before the value
// - #: various uses:
//      %#o (Octal)                         0 prefix inserted.
//      %#x (Lowercase Hexadecimal)         0x prefix is added.
//      %#X (Uppercase Hexadecimal)         0X prefix is added
//      %#e, %#E, %#f, %#g, %#G             Always show the decimal point
//
// Supported Width:
// - (number): Minimum number of chars to be printed. Spaces are used as padding. Value is not truncated if larger
// - *: Width is specified using an additional variable argument
//
// Supported Precision:
// - .(number): various uses
//              (d, i, o, u, x X):    specifies the minimum number of digits. Result is padded with leading zeros.
//              (f, F):               specifies number of digits after decimal point. Default is 6, maximum is 12
//              (s):                  specifies maximum number of chars to be printed.
// - .*: Precision is specified using an additional variable argument
//
// Supported Length:
//      Length          d/i             u,o,x,X
//      ------          -------         --------
//      (none)          int32_t         uint32_t
//      hh              int8_t          uint8_t
//      h               int16_t         uint16_t
//      l               int64_t         uint64_t
//      ll              int128_t        uint128_t
//      z               size_t          size_t
//      j               intmax_t        uintmax_t
//      z               ptrdiff_t       ptrdiff_t
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PARAMETERS                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NTOA (number to ascii) conversion buffer size.
// The buffer size must be sufficiently large to hold a rendered integer with padding.
#define NTOA_BUFFER_SIZE 128

// TODO: #define PRINTK_FTOA_BUFFER_SIZE 32

// The floating point precision.
// TODO: #define PRINTK_FLOAT_PRECISION 12

// The maximum float suitable to print with %f.
// TODO: #define PRINTK_MAX_FLOAT 1e9

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL BUFFER STRUCT                                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    char* buffer;
    size_t length;
    size_t max_length;
} buf_t;

/*
 * Procedure:   buf_putc
 * ---------------------
 * This inline procedure appends character [c] to the buffer with pointer [p]
 * provided length < max_length.
 *
 * @char  c:        The character to be appended.
 * @void* p:        The pointer to the buffer.
 *
 */
static inline void buf_putc(char c, void* p) {
    buf_t* buf = (buf_t*)p;

    if (buf->length++ < buf->max_length) {
        *(buf->buffer++) = c;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// STANDARD METHODS: vsnprintf, snprintf, printf. (Note that vprintf is implemented in the console dev)            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Function:    vsnprintf
 * ----------------------
 * Produces a string stored in [buffer] that would be printed if [format] was used on printf
 * with a maximum buffer capacity of [n].
 *
 * @char* buffer:       Pointer to a buffer where the resulting string is stored.
 *                      The buffer should have a size of at least [n] characters.
 * @size_t n:           Maximum number of bytes to be used in the buffer.
 * @char* format:       String that contains a string that contains a list of characters or format prototypes
 *                      according to the specification above.
 * @va_list va:         A value identifying a variable argument list initialized with va_start.
 *                      (See <stdarg.h>)
 *
 * @returns:            The number of characters that have been written to the buffer
 *                      if [n] is sufficiently large. (doesn't include the null terminating
 *                      character).
 *
 */
int vsnprintf(char* buffer, size_t n, const char* format, va_list va) {

    buf_t buf;

    buf.buffer = buffer;
    buf.length = 0;
    buf.max_length = n > 0 ? n - 1 : 0;

    __vprintf(format, va, buf_putc, &buf);

    if (n > 0) *buf.buffer = '\0';

    return buf.length;
}

/*
 * Function:    snprintf
 * ---------------------
 * Identical to vsnprintf, however uses additional arguments instead of a variable argument list
 * (va_list).
 *
 * @char* buffer:       Pointer to a buffer where the resulting string is stored.
 *                      The buffer should have a size of at least [n] characters.
 * @size_t n:           Maximum number of bytes to be used in the buffer.
 * @char* format:       String that contains a string that contains a list of characters or format prototypes
 *                      according to the specification above.
 * @...:                Additional arguments for the format string [format].
 *
 * @returns:            The number of characters that have been written to the buffer
 *                      if [n] is sufficiently large. (doesn't include the null terminating
 *                      character).
 *
 */
int snprintf(char *buffer, size_t n, const char* format, ...) {
    va_list va;
    int x;

    va_start(va, format);
    x = vsnprintf(buffer, n, format, va);
    va_end(va);

    return x;
}

/*
 * Function:    printf
 * -------------------
 * Writes the string [format] to the standard output. If format includes a format specifier, then the additional
 * arguments following [format] (i.e. ...) are formatted and inserted into the resulting string (replacing their
 * format specifier).
 *
 * @char* format:       String that contains a string that contains a list of characters or format prototypes
 *                      according to the specification above.
 * @...:                Additional arguments for the format string [format].
 *
 * @returns:            The number of characters that have been written to the buffer
 *                      if [n] is sufficiently large. (doesn't include the null terminating
 *                      character).
 *
 */
int printf(const char* format, ...) {
    va_list va;
    int x;

    va_start(va, format);
    x = vprintf(format, va);
    va_end(va);

    return x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL BUFFER AND NUMBER BASE STRUCT                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    enum {
        LEFT    = 1 << 0,       /* '-' */
        ZERO    = 1 << 1,       /* '0' */
        PLUS    = 1 << 2,       /* '+' */
        BLANK   = 1 << 3,       /* ' ' */
        HASH    = 1 << 4        /* '#' */
    } flags;

    // Minimum field width
    int width;

    // Numeric precision. -1 indicates no precision was specified.
    int precision;

    // Type of argument to format.
    enum {
        INT8    = 1,            /* hh */
        INT16   = 2,            /* h  */
        INT32   = 3,            /* (none) */
        INT64   = 4,            /* l  */
        INT128  = 5,            /* ll */
        INTMAX  = 6,            /* j  */
        SIZE    = 7,            /* z  */
        PTRDIFF = 8             /* t  */
    } type;
} format_t;

typedef struct {
    int base;                   /* Base */
    const char* digits;         /* Collection of digits */
    int x;                      /* `x` character to use, for base 16 only. */
} base_t;

static const base_t BASE_D = {10,   "0123456789",       0};
static const base_t BASE_O = {8,    "01234567",         0};
static const base_t BASE_x = {16,   "0123456789abcdef", 'x'};
static const base_t BASE_X = {16,   "0123456789ABCDEF", 'X'};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL format, ntoa AND string METHODS                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Function:    parse_format
 * -------------------------
 * Parses format prototype characters starting at [format] and stores the parsed arguments
 * into [f].
 *
 * @char* format:       String that contains a string that contains a list of characters or format prototypes
 *                      according to the specification above.
 * @format_t* f:        Pointer to the format_t that contains the parsed arguments.
 * @va_list va:         A value identifying a variable argument list initialized with va_start.
 *                      (See <stdarg.h>). Used to parse '*' field widths and precisions.
 *
 * @returns:            The character in format that indicates the conversion (e.g. the `i` in `%i`).
 *
 */
static const char* parse_format(const char* format, format_t* f, va_list* va) {
    f->flags = 0;

    // Parse flags
    bool cont = false;
    do {
        switch (*format) {
            case '0': f->flags |= ZERO;         format++; cont = true;  break;
            case '-': f->flags |= LEFT;         format++; cont = true;  break;
            case '+': f->flags |= PLUS;         format++; cont = true;  break;
            case ' ': f->flags |= BLANK;        format++; cont = true;  break;
            case '#': f->flags |= HASH;         format++; cont = true;  break;
            default:                                      cont = false; break;
        }
    } while (cont);

    if (f->flags & LEFT)    f->flags &= ~ZERO;
    if (f->flags & PLUS)    f->flags &= ~BLANK;

    f->width = 0;
    // Parse width
    if (*format == '*') {
        // .* option. The optional argument is prior to the type argument. So just pop it off the va list.
        f->width = va_arg(*va, int);
        format++;
    } else {
        // .(number) option.
        for (; isdigit(*format); format++) f->width = f->width * 10 + (*format - '0');
    }

    if (f->width < 0) {
        f->flags |= LEFT;
        f->width = -(f->width);
    }

    f->precision = -1;
    // Parse precision
    if (*format == '.') {
        format++;

        if (*format == '*') {
            f->precision = va_arg(*va, int);
            format++;
        } else {
            f->precision = 0;
            for (; isdigit(*format); format++) f->precision = f->precision * 10 + (*format - '0');
        }

        // If precision < 0, then set precision to unset (-1)
        if (f->precision < 0) f->precision = -1;
    }

    // Parse type
    f->type = INT32;
    switch (*format++) {
        case 'h':
            if (*format == 'h') {
                f->type = INT8;
                format++;
            } else {
                f->type = INT16;
            }
            break;
        case 'l':
            if (*format == 'l') {
                f->type = INT128;
                format++;
            } else {
                f->type = INT64;
            }
            break;
        case 'j':
            f->type = INTMAX;
            break;
        case 't':
            f->type = PTRDIFF;
            break;
        case 'z':
            f->type = SIZE;
            break;
        default:
            format--;
            break;
    }

    return format;
}

/*
 * Procedure:   nputc
 * ------------------
 * Calls putc on [c] and [buf], [n] times. Used for padding.
 *
 * @char c:                     The character that is written to the buffer [buf].
 * @int  n:                     The number of times the character is written to the buffer [buf].
 * @void (*putc)(char, void*):  The pointer to a putc method.
 * @void* buf:                  The pointer to a buffer.
 *
 */
static void inline nputc(char c, int n, void (*putc)(char, void*), void* buf) {
    while (n-- > 0) {
        putc(c, buf);
    }
}

/*
 * Procedure:   ntoa
 * -----------------
 * Performs an integer conversion, writing to [buf] using the [putc] method.
 * The integer converted has an absolute value [value]. If [is_signed] is true,
 * then a signed conversion with [negative] indiciating whether the value was
 * negative is performed. Otherwise the procedure does an unsigned conversion
 * and ignored [negative].
 * The conversion is done according to the base [b] provided.
 *
 * @uintmax_t value:            The absolute value of the integer to be converted.
 * @bool is_signed:             Indicates whether the value is unsigned or signed.
 * @bool negative:              Indicates whether the value is negative.
 * @base_t* b:                  Pointer to the base representation of the integer.
 *                              See constants BASE_D, BASE_O, BASE_x, and  BASE_X.
 * @void (*putc)(char, void*):  The pointer to a putc method.
 * @void* buf:                  The pointer to a buffer.
 *
 */
static void ntoa(
        uintmax_t value, bool is_signed, bool negative,
        const base_t* b, const format_t* f,
        void (*putc)(char, void*), void* buf
        ) {
    // Buffer and current position
    char buffer[NTOA_BUFFER_SIZE], *pos;

    // The so-called 'x' character to use (or 0 if none)
    char x;

    // The sign character (or 0 if none)
    char sign;
    int precision;

    // The # of pad and digit characters
    int pad_len;
    int digit_len;

    // Determine the sign character to use (or 0 if none).
    sign = 0;
    if (is_signed) {
        if (f->flags & PLUS)            sign = negative ? '-' : '+';
        else if (f->flags & BLANK)      sign = negative ? '-' : ' ';
        else if (negative)              sign = '-';
    }

    // Determine whether to include '0x' or '0X'.
    // Only included with a hexadecimal conversion of non-zero value with the hash flag
    x = (f->flags & HASH) && value ? b->x : 0;

    // We push digits into a buffer. Note that these digits will be in reverse order,
    // so we must later output the buffer's content in reverse :)
    pos = &buffer[0]; digit_len = 0;

    while ((value > 0) && (pos - buffer < NTOA_BUFFER_SIZE)) {
        *pos++ = b->digits[value % b->base];
        value /= b->base;
        digit_len++;
    }

    // We now append zeros to match precision.
    // If the precision is 0, then a value of zero is an empty string, otherwise as 0.
    precision = f->precision < 0 ? 1 : f->precision;
    while ((pos - buffer < precision) && (pos - buffer < NTOA_BUFFER_SIZE)) *pos++ = '0';

    // If the # flag is used with octal, the result must always begin with a zero
    if ((f->flags & HASH) && (b->base == 8) && (pos == buffer || pos[-1] != '0')) *pos++ = '0';

    // Calculate the number of pad characters required
    pad_len = MAX(f->width - (pos - buffer) - (x ? 2 : 0) - (sign != 0), 0);


    // Now we begin outputting the integer :)

    // If we don't have left-justify or zero pad, then pad with whitespace :)
    if ((f->flags & (LEFT | ZERO)) == 0)    nputc(' ', pad_len, putc, buf);

    // If sign != 0, then output the sign
    if (sign)                               putc(sign, buf);

    // If x != 0 => contains 'x' character (either x or X). So we must output 0x or 0X.
    if (x) {
        putc('0', buf);
        putc(x, buf);
    }

    // If we have a zero flag, then we pad :)
    if (f->flags & ZERO)                    nputc('0', pad_len, putc, buf);

    // Reverse output the buffer (from current pos to &buffer[0])
    while (pos > buffer)                    putc(*--pos, buf);

    // If we have a left-justify, then pad the right side
    if (f->flags & LEFT)                    nputc(' ', pad_len, putc, buf);
}

/*
 * Procedure:   string
 * -------------------
 * Formats [n] characters of the string with pointer [str]
 * according to the format [f]. The converted output is written to [buf]
 * using the [putc] method.
 *
 * @char* str:                  Pointer to the start of the string to be converted.
 * @int n:                      The length of the string [str].
 * @void (*putc)(char, void*):  The pointer to a putc method.
 * @void* buf:                  The pointer to a buffer.
 *
 */
static void string(const char* str, int n, format_t* f, void (*putc)(char, void*), void* buf) {

    if ((f->width > n) && (f->flags & LEFT) == 0)          nputc(' ', f->width - n, putc, buf);

    for (int i = 0; i < n; i++)                            putc(str[i], buf);

    if ((f->width > n) && (f->flags & LEFT) != 0)          nputc(' ', f->width - n, putc, buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal __printf and __vprintf implementations                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void __printf(const char* format, void (*putc)(char, void*), void* buf, ...) {
    va_list va;

    va_start(va, buf);
    __vprintf(format, va, putc, buf);
    va_end(va);

}

void __vprintf(const char* format, va_list va, void (*putc)(char, void*), void* buf) {

    for (; *format != '\0'; format++) {
        format_t f;

        // Is [*format] the beginning of a format prototype?
        if (*format != '%') {
            // no
            putc(*format, buf);
            continue;
        }
        // yes :)
        format++;

        format = parse_format(format, &f, &va);

        switch (*format) {
            case 'd':
            case 'i': {
                intmax_t x;
                switch (f.type) {
                    case INT8:
                        x = (int8_t)va_arg(va, int32_t);
                        break;
                    case INT16:
                        x = (int16_t)va_arg(va, int32_t);
                        break;
                    case INT32:
                        x = va_arg(va, int32_t);
                        break;
                    case INT64:
                        x = va_arg(va, int64_t);
                        break;
                    case INT128:
                        x = va_arg(va, int128_t);
                        break;
                    case INTMAX:
                        x = va_arg(va, intmax_t);
                        break;
                    case PTRDIFF:
                        x = va_arg(va, ptrdiff_t);
                        break;
                    case SIZE:
                        x = va_arg(va, size_t);
                        break;
                    default:
                        NOT_REACHABLE;
                }

                uintmax_t abs_x = abs(x);
                ntoa(abs_x, true, x < 0, &BASE_D, &f, putc, buf);

                break;
            }
            case 'o':
            case 'u':
            case 'x':
            case 'X': {
                uintmax_t x;
                const base_t *b;

                switch (f.type) {
                    case INT8:
                        x = (uint8_t)va_arg(va, uint32_t);
                        break;
                    case INT16:
                        x = (uint16_t)va_arg(va, uint32_t);
                        break;
                    case INT32:
                        x = va_arg(va, uint32_t);
                        break;
                    case INT64:
                        x = va_arg(va, uint64_t);
                        break;
                    case INT128:
                        x = va_arg(va, uint128_t);
                        break;
                    case INTMAX:
                        x = va_arg(va, uintmax_t);
                        break;
                    case PTRDIFF:
                        x = va_arg(va, ptrdiff_t);
                        break;
                    case SIZE:
                        x = va_arg(va, size_t);
                        break;
                    default:
                        NOT_REACHABLE;
                }

                switch (*format) {
                    case 'o': b = &BASE_O; break;
                    case 'u': b = &BASE_D; break;
                    case 'x': b = &BASE_x; break;
                    case 'X': b = &BASE_X; break;
                    default: NOT_REACHABLE;
                }

                ntoa(x, false, false, b, &f, putc, buf);
                break;
            }
            case 'c': {
                char c = (char)va_arg(va, int);
                string(&c, 1, &f, putc, buf);
                break;
            }
            case 's': {
                const char* s = va_arg(va, char*);
                if (s == null) s = "(null)";

                // Some bit magic here. If f.precision = -1,
                // then when we cast to size_t (an uint64_t)
                // then we get a MAXSIZE (0xff..ff).
                string(s, strnlen(s, f.precision), &f, putc, buf);
                break;
            }
            case 'p': {
                void* p = va_arg(va, void*);
                f.flags = HASH;
                ntoa((uintptr_t)p, false, false, &BASE_x, &f, putc, buf);
                break;
            }
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            case 'n':
                // We currently don't support floating-point formatting and %n
                __printf("Unsupported formatting option %%%c.", putc, buf, *format);
                break;
            default:
                __printf("Unknown formatting option %%%c.", putc, buf, *format);
                break;
        }
    }
}