#include "file_statistics.h"

file_statistics::file_statistics() : commented(0), empty(0), code(0), total(0), non_space_chars(0) {
}

void file_statistics::operator+=(const file_statistics& to_add) {
    commented += to_add.commented;
    empty += to_add.empty;
    code += to_add.code;
    total += to_add.total;
    non_space_chars += to_add.non_space_chars;
}

