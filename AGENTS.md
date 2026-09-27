# C++ formatting preferences

- Write nested namespaces as separate blocks. Do not use `namespace a::b`.

```cpp
namespace a {
namespace b {

} // namespace b
} // namespace a
```

- Always use braces for conditional statement bodies, including single-line
  bodies and early returns. Keep the opening brace on the condition's line.

```cpp
if (condition) {
    return;
}
```

Apply these preferences to new and edited code. Do not reformat unrelated code.
