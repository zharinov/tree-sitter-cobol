#include <tree_sitter/parser.h>
#include <wctype.h>

enum TokenType {
    WHITE_SPACES,
    LINE_PREFIX_COMMENT,
    LINE_SUFFIX_COMMENT,
    LINE_COMMENT,
    COMMENT_ENTRY,
    multiline_string,
    EXEC_BLOCK,
};

void *tree_sitter_COBOL_external_scanner_create() {
    return NULL;
}

static bool is_white_space(int c) {
    return iswspace(c) || c == ';' || c == ',';
}

const int number_of_comment_entry_keywords = 9;
char* any_content_keyword[] = {
    "author",
    "installation",
    "date-written",
    "date-compiled",
    "security",
    "identification division",
    "environment division",
    "data division",
    "procedure division",
};

static bool start_with_word( TSLexer *lexer, char *words[], int number_of_words) {
    while(lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        lexer->advance(lexer, true);
    }

    char *keyword_pointer[number_of_words];
    bool continue_check[number_of_words];
    for(int i=0; i<number_of_words; ++i) {
        keyword_pointer[i] = words[i];
        continue_check[i] = true;
    }

    while(true) {
        // At the end of the line
        if(lexer->get_column(lexer) > 71 || lexer->lookahead == '\n' || lexer->lookahead == 0) {
            return false;
        }

        // If all keyword matching fails, move to the end of the line
        bool all_match_failed = true;
        for(int i=0; i<number_of_words; ++i) {
            if(continue_check[i]) {
                all_match_failed = false;
            }
        }

        if(all_match_failed) {
            for(; lexer->get_column(lexer) < 71 && lexer->lookahead != '\n' && lexer->lookahead != 0;
            lexer->advance(lexer, true)) {
            }
            return false;
        }

        // If the head of the line matches any of specified keywords, return true;
        char c = lexer->lookahead;
        for(int i=0; i<number_of_words; ++i) {
            if(*(keyword_pointer[i]) == 0 && continue_check[i]) {
                return true;
            }
        }

        // matching keywords
        for(int i=0; i<number_of_words; ++i) {
            char k = *(keyword_pointer[i]);
            if(continue_check[i]) {
                continue_check[i] = c == towupper(k) || c == towlower(k);
            }
            (keyword_pointer[i])++;
        }

        // next character
        lexer->advance(lexer, true);
    }

    return false;
}

// Case-insensitive character match
static bool char_eq_i(int c, char expected) {
    return towlower(c) == towlower(expected);
}

// Try to match a keyword case-insensitively, advancing lexer if matched
static bool match_keyword(TSLexer *lexer, const char *keyword) {
    for (const char *p = keyword; *p; p++) {
        if (!char_eq_i(lexer->lookahead, *p)) {
            return false;
        }
        lexer->advance(lexer, false);
    }
    return true;
}

// Scan EXEC ... END-EXEC block
static bool scan_exec_block(TSLexer *lexer) {
    // Must start with EXEC
    if (!char_eq_i(lexer->lookahead, 'E')) {
        return false;
    }

    // Try to match EXEC
    if (!match_keyword(lexer, "EXEC")) {
        return false;
    }

    // Must be followed by whitespace
    if (!iswspace(lexer->lookahead)) {
        return false;
    }

    // Skip whitespace
    while (iswspace(lexer->lookahead)) {
        lexer->advance(lexer, false);
    }

    // Check for known EXEC types: SQL, CICS, DLI
    // We accept any word after EXEC, but these are the common ones
    bool found_type = false;
    if (char_eq_i(lexer->lookahead, 'S') ||
        char_eq_i(lexer->lookahead, 'C') ||
        char_eq_i(lexer->lookahead, 'D')) {
        found_type = true;
    }

    if (!found_type) {
        return false;
    }

    // Now scan until END-EXEC
    // We need to find END-EXEC (case insensitive)
    int end_exec_state = 0;  // Tracks matching "END-EXEC"
    const char *end_exec = "END-EXEC";

    while (lexer->lookahead != 0) {
        char c = lexer->lookahead;

        // Try to match END-EXEC
        if (char_eq_i(c, end_exec[end_exec_state])) {
            end_exec_state++;
            if (end_exec[end_exec_state] == 0) {
                // Matched END-EXEC
                lexer->advance(lexer, false);
                lexer->mark_end(lexer);
                lexer->result_symbol = EXEC_BLOCK;
                return true;
            }
        } else if (char_eq_i(c, 'E')) {
            // Could be start of END-EXEC
            end_exec_state = 1;
        } else {
            end_exec_state = 0;
        }

        lexer->advance(lexer, false);
    }

    // Hit EOF without finding END-EXEC
    return false;
}

bool tree_sitter_COBOL_external_scanner_scan(void *payload, TSLexer *lexer,
                                            const bool *valid_symbols) {
    if(lexer->lookahead == 0) {
        return false;
    }

    if(valid_symbols[WHITE_SPACES]) {
        if(is_white_space(lexer->lookahead)) {
            while(is_white_space(lexer->lookahead)) {
                lexer->advance(lexer, true);
            }
            lexer->result_symbol = WHITE_SPACES;
            lexer->mark_end(lexer);
            return true;
        }
    }

    if(valid_symbols[LINE_PREFIX_COMMENT] && lexer->get_column(lexer) <= 5) {
        while(lexer->get_column(lexer) <= 5) {
            lexer->advance(lexer, true);
        }
        lexer->result_symbol = LINE_PREFIX_COMMENT;
        lexer->mark_end(lexer);
        return true;
    }

    if(valid_symbols[LINE_COMMENT]) {
        if(lexer->get_column(lexer) == 6) {
            if(lexer->lookahead == '*' || lexer->lookahead == '/') {
                while(lexer->lookahead != '\n' && lexer->lookahead != 0) {
                    lexer->advance(lexer, true);
                }
                lexer->result_symbol = LINE_COMMENT;
                lexer->mark_end(lexer);
                return true;
            } else {
                lexer->advance(lexer, true);
                lexer->mark_end(lexer);
                return false;
            }
        }
    }

    if(valid_symbols[LINE_SUFFIX_COMMENT]) {
        if(lexer->get_column(lexer) >= 72) {
            while(lexer->lookahead != '\n' && lexer->lookahead != 0) {
                lexer->advance(lexer, true);
            }
            lexer->result_symbol = LINE_SUFFIX_COMMENT;
            lexer->mark_end(lexer);
            return true;
        }
    }

    if(valid_symbols[COMMENT_ENTRY]) {
        if(!start_with_word(lexer, any_content_keyword, number_of_comment_entry_keywords)) {
            lexer->mark_end(lexer);
            lexer->result_symbol = COMMENT_ENTRY;
            return true;
        } else {
            return false;
        }
    }

    if(valid_symbols[multiline_string]) {
        while(true) {
            if(lexer->lookahead != '"') {
                return false;
            }
            lexer->advance(lexer, false);
            while(lexer->lookahead != '"' && lexer->lookahead != 0 && lexer->get_column(lexer) < 72) {
                lexer->advance(lexer, false);
            }
            if(lexer->lookahead == '"') {
                lexer->result_symbol = multiline_string;
                lexer->advance(lexer, false);
                lexer->mark_end(lexer);
                return true;
            }
            while(lexer->lookahead != 0 && lexer->lookahead != '\n') {
                lexer->advance(lexer, true);
            }
            if(lexer->lookahead == 0) {
                return false;
            }
            lexer->advance(lexer, true);
            int i;
            for(i=0; i<=5; ++i) {
                if(lexer->lookahead == 0 || lexer->lookahead == '\n') {
                    return false;
                }
                lexer->advance(lexer, true);
            }

            if(lexer->lookahead != '-') {
                return false;
            }

            lexer->advance(lexer, true);
            while(lexer->lookahead == ' ' && lexer->get_column(lexer) < 72) {
                lexer->advance(lexer, true);
            }
        }
    }

    if(valid_symbols[EXEC_BLOCK]) {
        if(scan_exec_block(lexer)) {
            return true;
        }
    }

    return false;
}

unsigned tree_sitter_COBOL_external_scanner_serialize(void *payload, char *buffer) {
    return 0;
}

void tree_sitter_COBOL_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
}

void tree_sitter_COBOL_external_scanner_destroy(void *payload) {
}
