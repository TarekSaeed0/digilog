#ifndef VECTOR_DECLARE_H
#define VECTOR_DECLARE_H

#include <stdbool.h>
#include <stddef.h>

#define VECTOR_DECLARE(name, type)                                                                 \
	typedef type name##_type;                                                                      \
	struct name {                                                                                  \
		name##_type *elements;                                                                     \
		size_t length;                                                                             \
		size_t capacity;                                                                           \
	};                                                                                             \
	struct name name##_new(void);                                                                  \
	void name##_drop(struct name *self);                                                           \
	bool name##_insert(                                                                            \
		struct name *self,                                                                         \
		size_t index,                                                                              \
		const name##_type *elements,                                                               \
		size_t length                                                                              \
	);                                                                                             \
	void name##_remove(struct name *self, size_t index, size_t length);

#endif
