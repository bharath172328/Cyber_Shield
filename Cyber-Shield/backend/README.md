# Cyber Shield — C++ backend

C++17 security engine + REST API. See the [root README](../README.md) for the overview and [docs/API.md](../docs/API.md) for endpoints.

```bash
cmake -S . -B build && cmake --build build -j
./build/cs_tests && ./build/cybershield
```
| Directory | Contents |
|---|---|
| `include/` `src/api` | `ApiServer`, `HttpUtil`, `AuthController`, `SecurityController`, `DashboardController` |
| `src/security` | `PasswordAnalyzer`, `URLAnalyzer`, `PortScanner`, `FileHasher`, `FileAnalyzer`, `FileIntegrityMonitor`, `ThreatDetector` |
| `src/authentication` | `AuthenticationManager` (PBKDF2, sessions, lockout) |
| `src/database` | `DatabaseManager` (SQLite, bound parameters) |
| `src/logging` | `SecurityLogger` (database + file) |
| `database/schema.sql` | Relational schema |
| `tests/test_main.cpp` | Dependency-free unit tests (`ctest` compatible) |

Adding a module: create the class in `src/security`, register it in `main.cpp`'s `ApiContext`, and add a route in a controller.
