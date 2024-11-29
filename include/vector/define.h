#ifndef VECTOR_DEFINE_H
#define VECTOR_DEFINE_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VECTOR_DEFINE(...) VECTOR_DEFINE_(__VA_ARGS__, NULL, )
#define VECTOR_DEFINE_(name, drop, ...)                                                            \
	struct name name##_new(void) {                                                                 \
		return (struct name){                                                                      \
			.elements = NULL,                                                                      \
			.length = 0,                                                                           \
			.capacity = 0,                                                                         \
		};                                                                                         \
	}                                                                                              \
	void name##_drop(struct name *self) {                                                          \
		assert(self != NULL);                                                                      \
                                                                                                   \
		if ((drop) != NULL) {                                                                      \
			for (size_t i = 0; i < self->length; i++) {                                            \
				((void (*)(name##_type *))(drop))(&self->elements[i]);                             \
			}                                                                                      \
		}                                                                                          \
		free(self->elements);                                                                      \
	}                                                                                              \
	static bool name##_expand(struct name *self, size_t length) {                                  \
		assert(self != NULL);                                                                      \
                                                                                                   \
		if (length > SIZE_MAX / sizeof(*self->elements)) {                                         \
			return false;                                                                          \
		}                                                                                          \
                                                                                                   \
		size_t capacity = self->capacity;                                                          \
		do {                                                                                       \
			if (capacity == 0) {                                                                   \
				capacity = 1;                                                                      \
			} else if (capacity > SIZE_MAX / sizeof(*self->elements) / 2) {                        \
				capacity = SIZE_MAX / sizeof(*self->elements);                                     \
			} else {                                                                               \
				capacity *= 2;                                                                     \
			}                                                                                      \
		} while (capacity < length);                                                               \
                                                                                                   \
		name##_type *elements = realloc(self->elements, capacity * sizeof(*self->elements));       \
		if (elements == NULL) {                                                                    \
			return false;                                                                          \
		}                                                                                          \
                                                                                                   \
		self->elements = elements;                                                                 \
		self->capacity = capacity;                                                                 \
                                                                                                   \
		return true;                                                                               \
	}                                                                                              \
	bool name##_insert(                                                                            \
		struct name *self,                                                                         \
		size_t index,                                                                              \
		const name##_type *elements,                                                               \
		size_t length                                                                              \
	) {                                                                                            \
		assert(self != NULL && index <= self->length && (elements != NULL || length == 0));        \
                                                                                                   \
		if (length > SIZE_MAX - self->length) {                                                    \
			return false;                                                                          \
		}                                                                                          \
                                                                                                   \
		if (self->length + length > self->capacity &&                                              \
			!name##_expand(self, self->length + length)) {                                         \
			return false;                                                                          \
		}                                                                                          \
                                                                                                   \
		memmove(                                                                                   \
			&self->elements[index + length],                                                       \
			&self->elements[index],                                                                \
			(self->length - index) * sizeof(*self->elements)                                       \
		);                                                                                         \
		memmove(&self->elements[index], elements, length * sizeof(*self->elements));               \
                                                                                                   \
		self->length += length;                                                                    \
                                                                                                   \
		return true;                                                                               \
	}                                                                                              \
	void name##_remove(struct name *self, size_t index, size_t length) {                           \
		assert(self != NULL && index < self->length && length <= self->length - index);            \
                                                                                                   \
		if ((drop) != NULL) {                                                                      \
			for (size_t i = index; i < index + length; i++) {                                      \
				((void (*)(name##_type *))(drop))(&self->elements[i]);                             \
			}                                                                                      \
		}                                                                                          \
		memmove(                                                                                   \
			&self->elements[index],                                                                \
			&self->elements[index + length],                                                       \
			(self->length - index) * sizeof(*self->elements)                                       \
		);                                                                                         \
                                                                                                   \
		self->length -= length;                                                                    \
	}

#endif
