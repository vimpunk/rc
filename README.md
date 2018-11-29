# RC

This library provides a simple replacement for when `std::shared_ptr`s are not
suitable. Its main selling point is that it's actually *not* atomic (saving some
of the associated overhead), and the two main types of the library (`rc` and
`weak_rc`) are the size of the architecture's word size, allowing to be passed
to C style functions take a `void*` as their generic parameter. To faciliatate
this, special converstion methods are provided to cast to `void*` and to
reconstruct the rc types from `void*`.
RC stands for "reference counted" (very creative, I know), inspired by Rust's
`Arc`. However, this library
