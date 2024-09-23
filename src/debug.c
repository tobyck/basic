#include <stdio.h>

#include "debug.h"

void print_token(Token token) {
	printf("%s ", stringify_token_type(token.type));
	switch (token.literal_type) {
		case LITERAL_STRING: printf("\"%s\" ", token.string_literal); break;
		case LITERAL_CHAR: printf("'%c' ", token.char_literal); break;
		default: break;
	}
	printf("at %zu:%zu", token.line, token.column);
}

void print_token_buffer_range(TokenBuffer buffer, size_t start, size_t end) {
	for (size_t i = start; i < end; i++) {
		printf("  ");
		print_token(buffer.tokens[i]);
		if (i < end - 1) printf(",");
		printf("\n");
	}
}

void print_token_buffer(TokenBuffer buffer) {
	if (buffer.length == 0) {
		printf("[]\n");
		return;
	}

	printf("[\n");

	printf("  cap = %zu,\n  len = %zu,\n  next = %zu,\n  peeked = %zu\n  ----------\n", buffer.capacity, buffer.length, buffer.next_index, buffer.peeked_count);

	if (buffer.next_index == 0) print_token_buffer_range(buffer, 0, buffer.length);
	else {
		print_token_buffer_range(buffer, buffer.next_index, buffer.length);
		print_token_buffer_range(buffer, 0, buffer.next_index);
	}

	printf("]\n");
}
