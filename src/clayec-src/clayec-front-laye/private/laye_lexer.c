#include "kos/utf8.h"

#include "layec/front/laye/front.h"

#include "layec/compiler.h"

#include "ast.h"
#include "token.h"

typedef struct laye_lexer
{
    layec_context* context;
    layec_fileid fileId;
    arena_allocator* tokenArena;
    string sourceText;
    rune currentRune;
    usize currentPosition;
} laye_lexer;

static laye_token* laye_token_alloc(laye_lexer* l)
{
    laye_token* result = arena_push(l->tokenArena, sizeof(laye_token));
    return result;
}

typedef struct keyword_info
{
    laye_token_kind kind;
    const char* image;
} layec_keyword_info;

static layec_keyword_info keywords[] = {
#define T(E, S)
#define K(E, S) { LAYE_TOKEN_ ## E, S },
LAYE_TOKEN_KINDS
#undef K
#undef T

    { 0 },
};

#define LAYE_KEYWORD_COUNT (cast(size_t) (sizeof(keywords) / sizeof(keyword_info)))

static bool lexer_is_eof(laye_lexer* l)
{
    return l->currentRune == 0 || l->currentPosition >= l->sourceText.count;
}

static char lexer_current(laye_lexer* l)
{
    return l->currentRune;
}

static char lexer_peek(laye_lexer* l, int ahead)
{
    if (lexer_is_eof(l) || ahead <= 0)
        return 0;
    
    usize peekPosition = l->currentPosition;
    while (ahead > 0 && peekPosition < l->sourceText.count)
    {
        ahead--;
        peekPosition += utf8_calc_encoded_byte_count(l->sourceText.memory[peekPosition]);
    }

    if (peekPosition >= l->sourceText.count)
        return 0;

    utf8_decode_result decodeResult = utf8_decode_rune_at_string_position(l->sourceText, peekPosition);

    if (decodeResult.kind != UTF8_DECODE_OK)
        return 0;
    
    return decodeResult.value;
}

static layec_location lexer_location(laye_lexer* l, usize startPosition)
{
    return (layec_location){
        .fileId = l->fileId,
        .offset = startPosition,
        .length = l->currentPosition - startPosition,
    };
}

static void lexer_advance(laye_lexer* l)
{
    if (lexer_is_eof(l))
        return;

    rune lastCurrent = l->currentRune;

    int currentByteCount = utf8_calc_encoded_byte_count(l->sourceText.memory[l->currentPosition]);
    l->currentPosition += currentByteCount;

    utf8_decode_result decodeResult = utf8_decode_rune_at_string_position(l->sourceText, l->currentPosition);

    l->currentRune = decodeResult.value;
    
    // the source text should include a nul terminator, so check for at least 1 before that.
    if (l->currentRune == 0 && l->currentPosition < l->sourceText.count - 1)
    {
        // if we hit this case, prove that the previous assumption is true
        assert(l->sourceText.memory[l->sourceText.count - 1] == 0);

        layec_issue_diagnostic(l->context, SEV_ERROR, lexer_location(l, l->currentPosition - decodeResult.nBytesRead), "Unexpected NUL character in source.");

        lexer_advance(l);
    }
}

static bool lexer_is_space(rune value)
{
    return value == ' '
        || value == '\r'
        || value == '\n'
        || value == '\t';
}

static void lexer_skip_whitespace(laye_lexer* l)
{
    rune current = lexer_current(l);
    while (lexer_is_space(current))
    {
        lexer_advance(l);
        current = lexer_current(l);
    }
}

static laye_token* lexer_get_token(laye_lexer* l)
{
    lexer_skip_whitespace(l);
    if (lexer_is_eof(l))
        return NULL;

    laye_token* token = laye_token_alloc(l);
    assert(token->location.fileId == 0, "after allocating token");

    usize startPosition = l->currentPosition;
    
    rune c = lexer_current(l);
    rune firstChar = c;

    switch (c)
    {
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z':
        
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z':

        lex_identifier:;
        case '_':
        {
            int possibleSizedTypeParamterValue = 0;
            bool isSizedTypeParamterOutOfRange = false;
            bool areAllSubsequentCharactersDigits = true;

            lexer_advance(l);
            while ((c = lexer_current(l)),
                (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' ||
                (c >= '0' && c <= '9'))
            {
                if (areAllSubsequentCharactersDigits)
                {
                    if (c >= '0' && c <= '9')
                    {
                        if (!isSizedTypeParamterOutOfRange)
                        {
                            possibleSizedTypeParamterValue *= 10;
                            possibleSizedTypeParamterValue += (c - '0');
                        }

                        if (possibleSizedTypeParamterValue > 65535)
                        {
                            isSizedTypeParamterOutOfRange = true;
                            possibleSizedTypeParamterValue = 65535;
                        }
                    }
                    else areAllSubsequentCharactersDigits = false;
                }

                lexer_advance(l);
            }
            
            token->kind = LAYE_TOKEN_IDENTIFIER;
            token->location = lexer_location(l, startPosition);

            if (areAllSubsequentCharactersDigits && token->location.length > 1)
            {
                if (isSizedTypeParamterOutOfRange)
                {
                    layec_issue_diagnostic(l->context, SEV_ERROR, token->location, "Sized primitives types cannot have a bit width larger than 65535.");
                }

                token->sizeParameter = possibleSizedTypeParamterValue;
                switch (firstChar)
                {
                    case 'i': token->kind = LAYE_TOKEN_IX; break;
                    case 'u': token->kind = LAYE_TOKEN_UX; break;
                    case 'b': token->kind = LAYE_TOKEN_BX; break;
                    case 'f': token->kind = LAYE_TOKEN_FX; break;
                    default: break;
                }
            }
            else
            {
                for (size_t i = 0; i < keywords[i].kind != 0; i++)
                {
                    layec_keyword_info kw = keywords[i];
                    if (strlen(kw.image) != token->location.length)
                        continue;

                    if (0 == strncmp(cast(const char*) (l->sourceText.memory + startPosition), kw.image, token->location.length))
                    {
                        token->kind = kw.kind;
                        break;
                    }
                }
            }
        } break;

        case '=':
        {
            lexer_advance(l);
            c = lexer_current(l);
            if (c == '=')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_EQUAL_EQUAL;
            }
            else if (c == '>')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_EQUAL_GREATER;
            }
            else
            {
                token->kind = cast(laye_token_kind) '=';
            }
        } break;

        case '!':
        {
            lexer_advance(l);
            c = lexer_current(l);
            if (c == '=')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_BANG_EQUAL;
            }
            else
            {
                token->kind = cast(laye_token_kind) '!';
            }
        } break;

        case '+':
        {
            lexer_advance(l);
            token->kind = cast(laye_token_kind) '+';
        } break;

        case '-':
        {
            lexer_advance(l);
            token->kind = cast(laye_token_kind) '-';
        } break;

        case '*':
        {
            lexer_advance(l);
            token->kind = cast(laye_token_kind) '*';
        } break;

        case '#':
        {
            lexer_advance(l);
            c = lexer_current(l);
            if (c == '!')
            {
                lexer_advance(l);
                goto continue_line_comment;
            }
            else
            {
                token->kind = LAYE_TOKEN_UNKNOWN;
                token->location = lexer_location(l, startPosition);

                layec_issue_diagnostic(l->context, SEV_ERROR, token->location, "invalid character '%lc' in Laye source file (did you mean `#!` to start a line comment?)", c);
            }
        } break;

        case '/':
        {
            lexer_advance(l);
            c = lexer_current(l);
            if (c == '/')
            {
                lexer_advance(l);
            continue_line_comment:
                while (!lexer_is_eof(l))
                {
                    c = lexer_current(l);
                    lexer_advance(l);

                    if (c == '\n')
                        break;
                }

                return lexer_get_token(l);
            }
            else if (c == '*')
            {
                lexer_advance(l);
                int delimiterCount = 1;

                char lastChar = lexer_current(l);
                lexer_advance(l);

                while (!lexer_is_eof(l) && delimiterCount > 0)
                {
                    c = lexer_current(l);
                    lexer_advance(l);

                    if (c == '/' && lastChar == '*')
                        delimiterCount--;
                    
                    lastChar = c;
                }

                if (delimiterCount > 0)
                {
                    layec_location location = lexer_location(l, startPosition);
                    layec_issue_diagnostic(l->context, SEV_ERROR, location, "unfinished delimited comment in Laye source file (%d open delimiter(s) went unclosed.)", delimiterCount);
                }

                return lexer_get_token(l);
            }

            token->kind = cast(laye_token_kind) '/';
        } break;

        case '>':
        {
            lexer_advance(l);
            c = lexer_current(l);
            if (c == '=')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_GREATER_EQUAL;
            }
            else if (c == '>')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_GREATER_GREATER;
            }
            else
            {
                token->kind = cast(laye_token_kind) '>';
            }
        } break;

        case '<':
        {
            lexer_advance(l);
            c = lexer_current(l);
            if (c == '=')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_LESS_EQUAL;
            }
            else if (c == '<')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_LESS_LESS;
            }
            else
            {
                token->kind = cast(laye_token_kind) '<';
            }
        } break;

        case '?':
        {
            lexer_advance(l);
            token->kind = cast(laye_token_kind) '?';
        } break;

        case '(': case ')':
        case '[': case ']':
        case '{': case '}':
        case ',': case '.':
        case ';':
        {
            lexer_advance(l);
            token->kind = cast(laye_token_kind) c;
        } break;

        case ':':
        {
            lexer_advance(l);
            c = lexer_current(l);
            if (c == ':')
            {
                lexer_advance(l);
                token->kind = LAYE_TOKEN_COLON_COLON;
            }
            else
            {
                token->kind = cast(laye_token_kind) ':';
            }
        } break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            u64 integerValue = cast(u64) (c - '0');
            bool isIntTooLarge = false;

            lexer_advance(l);
            while ((c = lexer_current(l)), (c >= '0' && c <= '9'))
            {
                u64 digitValue = cast(u64) (c - '0');
                if ((U64_MAX - digitValue) / 10 < integerValue)
                    isIntTooLarge = true;
                
                if (!isIntTooLarge)
                    integerValue = integerValue * 10 + digitValue;
                
                lexer_advance(l);
            }

            if ((c = lexer_current(l)),
                (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' ||
                (c >= '0' && c <= '9'))
            {
                l->currentPosition = startPosition;
                goto lex_identifier;
            }
            
            // TODO(local): lex floats, eventually, when we give a shit about floats
            token->kind = LAYE_TOKEN_LITERAL_INTEGER;

            if (lexer_current(l) == '#')
            {
                layec_location radixLocation = lexer_location(l, startPosition);
                u64 radix = integerValue;

                if (radix > 36 || isIntTooLarge)
                {
                    radix = 36;
                    layec_issue_diagnostic(l->context, SEV_ERROR, radixLocation, "Number radix (digit base) cannot be greater than 36.");
                }

                integerValue = 0;
                isIntTooLarge = false;

                lexer_advance(l);

                bool startsWithUnderscore = lexer_current(l) == '_';
                bool endsWithUnderscore = false;

                while ((c = lexer_current(l)),
                    (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     c == '_' ||
                    (c >= '0' && c <= '9'))
                {
                    if (c == '_')
                    {
                        endsWithUnderscore = true;
                        continue;
                    }
                    
                    endsWithUnderscore = false;

                    u64 digitValue = cast(u64) (
                        (c >= '0' && c <= '9') ? c - '0' :
                        (c >= 'a' && c <= 'z') ? c - 'a' + 10 :
                                                 c - 'A' + 10);

                    if ((U64_MAX - digitValue) / radix < integerValue)
                        isIntTooLarge = true;
                    
                    if (!isIntTooLarge)
                        integerValue = integerValue * radix + digitValue;
                    
                    lexer_advance(l);
                }
            }

            token->integerValue = integerValue;
            token->location = lexer_location(l, startPosition);

            if (isIntTooLarge)
            {
                layec_issue_diagnostic(l->context, SEV_ERROR, token->location, "Number '%.*s' does not fit within the maximum constant size of 64 bits.", cast(int) token->location.length, cast(const char*) (l->sourceText.memory + startPosition));
            }
        } break;

        case '"':
        {
            assert(token->location.fileId == 0, "at start of string literal");
            lexer_advance(l);

            string_builder stringBuilder = { 0 };
            string_builder_init(&stringBuilder, default_allocator);

            while (!lexer_is_eof(l))
            {
                c = lexer_current(l);
                if (c == '"') break;

                lexer_advance(l);
                switch (c)
                {
                    #if 0
                    case '\\':
                    {
                        fprintf(stderr, "%s:%d: string escape sequences are not supported\n", __FILE__, __LINE__);
                        abort();
                    } break;
                    #endif

                    default:
                    {
                        string_builder_append_rune(&stringBuilder, c);
                    } break;
                }
            }

            c = lexer_current(l);
            if (c != '"')
            {
                layec_location location = lexer_location(l, startPosition);
                layec_issue_diagnostic(l->context, SEV_ERROR, location, "unfinished string literal in Laye source file");
            }
            else lexer_advance(l);

            token->kind = LAYE_TOKEN_LITERAL_STRING;
            token->stringValue = string_builder_to_string_arena(&stringBuilder, l->context->constantArena);
            string_builder_deallocate(&stringBuilder);

            assert(token->location.fileId == 0, "at end of string literal");
        } break;

        default:
        {
            lexer_advance(l);

            token->kind = LAYE_TOKEN_UNKNOWN;
            token->location = lexer_location(l, startPosition);

            layec_issue_diagnostic(l->context, SEV_ERROR, token->location, "invalid character '%lc' in Laye source file", c);
        } break;
    }
    
    if (token->location.fileId == 0)
        token->location = lexer_location(l, startPosition);
    assert(token->location.length > 0, "at start position %zu", startPosition);
    assert(token->location.offset < l->sourceText.count, "at start position %zu", startPosition);
    assert(token->location.length <= l->sourceText.count, "at start position %zu", startPosition);
    assert(token->location.offset + token->location.length <= l->sourceText.count, "at start position %zu", startPosition);
    
    lexer_skip_whitespace(l);

    assert(token->kind > 0);
    assert(token->location.fileId != 0);
    assert(token->location.length != 0);

    return token;
}

list(laye_token*) laye_lex(layec_context* context, layec_fileid fileId, arena_allocator* tokenArena)
{
    laye_lexer lexer = { 0 };
    lexer.context = context;
    lexer.fileId = fileId;
    lexer.tokenArena = tokenArena;
    lexer.sourceText = layec_context_get_file_source(context, fileId);
    
    utf8_decode_result firstRuneResult = utf8_decode_rune_at_string_position(lexer.sourceText, 0);
    if (firstRuneResult.kind != UTF8_DECODE_OK)
        return nullptr;
    
    lexer.currentRune = firstRuneResult.value;
    
    list(laye_token*) tokens = nullptr;
    laye_token* token = nullptr;

    while ((token = lexer_get_token(&lexer)) != nullptr)
        arrput(tokens, token);

    return tokens;
}
