# RC

This library provides a simple replacement for when `std::shared_ptr`s are not
suitable. Rc stands for "reference counted" (very creative, I know), inspired by Rust's
`Arc`. The API is similar to that of `std::shared_ptr` and `std::weak_ptr`.

## Why

Rc has two main selling points.

One, both pointer types are *non* atomic (saving some of the associated overhead)--and thus *not* thread safe!

Second, both pointer types (`rc` and`weak_rc`) are the size of
the architecture's word size (i.e. equal to `sizeof(void*)`. Besides saving space, this allows rc pointers to be passed
to C style functions that take a `void*` as their generic parameter. To faciliatate
this, special conversion methods are provided to cast to `void*` and to
reconstruct rc pointers from `void*`.
