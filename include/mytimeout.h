#ifndef TIMEOUT_H
#define TIMEOUT_H

#include "lexer.h"

int execute_with_timeout(tokenlist *tokens, int timeout_seconds);

#endif /* TIMEOUT_H */