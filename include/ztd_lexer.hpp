#pragma once

namespace parsing {

    enum token_type {
        // ASCII

        TOKEN_IDENTIFIER = 256,
        TOKEN_STRING     = 257,
        TOKEN_NUMBER     = 258,

        TOKEN_EOF        = 500,
        TOKEN_ERROR      = 511,
    };

    struct token_t {
        token_type type {TOKEN_ERROR};

        // start
        i32 l0;
        i32 c0;

        // end
        i32 l1 = -1;
        i32 c1 = -1;

        union {
            struct { std::string_view name; u64 hash; } identifier;

            f64               float_value;
            u64               integer_value;
            std::string_view  string_value;
        };
    };

    #define MAX_TOKEN_COUNT 16

    struct lexer_t {
        i32 line_number{0};
        i32 character_index{0};

        token_t tokens[MAX_TOKEN_COUNT];
        u32     token_cursor{0};
        u32     incoming_tokens{0};

        std::string_view    input;
        u64                 input_cursor{0};

        token_t* peek_token() {
            if (incoming_tokens) {
                return &tokens[token_cursor];
            }
            return nullptr;
        }

        void eat_token() {
            assert(incoming_tokens > 0);
            assert(((MAX_TOKEN_COUNT - 1) & MAX_TOKEN_COUNT) == 0);

            incoming_tokens--;
            token_cursor = (token_cursor + 1) & (MAX_TOKEN_COUNT - 1);
        }

        void eat_character() {
            if (input[input_cursor] == '\n') {
                line_number++;
                character_index = 0;
            }

            input_cursor++;
            character_index++;
        }

        i32 peek_next_character() {
            if (input_cursor >= input.size()) {
                return -1;
            }
            return input[input_cursor];
        }

        token_t* make_token() {
            // token_cursor
        }

    }

}