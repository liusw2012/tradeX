# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

- **Project name**: tradeX
- **Type**: A-share Securities OMS (Order Management System)
- **Language**: C++17
- **Build system**: CMake
- **Database**: MySQL

## Build Commands

### Windows + MSYS2 (UCRT64)

```bash
# Configure with explicit compiler path (required for MSYS2)
mkdir -p build && cd build
cmake -DCMAKE_CXX_COMPILER=/d/install/mysys2/ucrt64/bin/g++.exe ..

# Build (using cmd to set TMP environment variable to avoid C:\Windows\ permission issue)
cmd //c "set TMP=D:\tmp && set TEMP=D:\tmp && cd /d D:\work\proj\tradeX\build && cmake --build ."

# Run
./bin/tradeX.exe
```

For a single build command:
```bash
rm -rf build && mkdir build && cd build && cmake -DCMAKE_CXX_COMPILER=/d/install/mysys2/ucrt64/bin/g++.exe .. && cmd //c "set TMP=D:\tmp && set TEMP=D:\tmp && cd /d D:\work\proj\tradeX\build && cmake --build ."
```

### Linux/macOS

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
./bin/tradeX
```

## Project Structure

```
tradeX/
├── CMakeLists.txt              # Build configuration
├── CLAUDE.md                   # Claude Code guidance
├── config/                     # Configuration files
│   └── tradex.json
├── framework/                  # Framework layer (公共组件)
│   ├── include/
│   │   └── tradex/framework/
│   │       ├── log.h           # Logging
│   │       ├── config.h        # Configuration
│   │       ├── thread_pool.h   # Thread pool
│   │       ├── status.h        # Status & health
│   │       ├── singleton.h     # Singleton template
│   │       └── version.h       # Version info
│   ├── src/
│   │   ├── log.cpp
│   │   ├── config.cpp
│   │   ├── thread_pool.cpp
│   │   └── status.cpp
│   └── CMakeLists.txt
├── dao/                        # Data Access Layer
│   ├── include/
│   │   └── tradex/dao/
│   │       ├── connection_pool.h
│   │       └── base_dao.h
│   ├── src/
│   │   └── connection_pool.cpp
│   └── CMakeLists.txt
├── order/                      # Order Management
│   ├── include/
│   │   └── tradex/order/
│   │       ├── order.h         # Order entity
│   │       └── order_service.h # Order service
│   ├── src/
│   │   ├── order.cpp
│   │   └── order_service.cpp
│   └── CMakeLists.txt
├── trade/                      # Trade Execution
│   ├── include/
│   │   └── tradex/trade/
│   │       ├── trade.h         # Trade & position
│   │       └── trade_service.h
│   ├── src/
│   │   └── trade.cpp
│   └── CMakeLists.txt
├── risk/                       # Risk Management
│   ├── include/
│   │   └── tradex/risk/
│   │       └── risk.h          # Risk rules & manager
│   ├── src/
│   │   └── risk.cpp
│   └── CMakeLists.txt
├── task/                       # Task Scheduling
│   ├── include/
│   │   └── tradex/task/
│   │       └── task.h          # Task queue & scheduler
│   ├── src/
│   │   └── task.cpp
│   └── CMakeLists.txt
├── auth/                       # Authentication
│   ├── include/
│   │   └── tradex/auth/
│   │       └── auth.h          # User & session
│   ├── src/
│   │   └── auth.cpp
│   └── CMakeLists.txt
├── gateway/                    # Exchange Gateway
│   ├── include/
│   │   └── tradex/gateway/
│   │       └── gateway.h       # Order router & gateway
│   ├── src/
│   │   └── gateway.cpp
│   └── CMakeLists.txt
├── market/                     # Market Data
│   ├── include/
│   │   └── tradex/market/
│   │       └── market.h        # Market data service
│   ├── src/
│   │   └── market.cpp
│   └── CMakeLists.txt
├── algo/                       # Algo Trading
│   ├── include/
│   │   └── tradex/algo/
│   │       └── algo.h          # Algo engine & strategies
│   ├── src/
│   │   └── algo.cpp
│   └── CMakeLists.txt
├── strategy/                   # Strategy Trading
│   ├── include/
│   │   └── tradex/strategy/
│   │       └── strategy.h      # Strategy engine
│   ├── src/
│   │   └── strategy.cpp
│   └── CMakeLists.txt
├── admin/                      # Admin Tools
│   ├── include/
│   │   └── tradex/admin/
│   │       └── admin.h         # CLI & health check
│   ├── src/
│   │   └── admin.cpp
│   └── CMakeLists.txt
├── thirdparty/                 # Third-party dependencies
├── unit_test/                  # Unit tests
└── src/
    └── main.cpp               # Main entry point
```

## Architecture

- **Framework Layer**: Logging, configuration, thread pool, status/health
- **DAO Layer**: Database connection pool, base DAO templates
- **Order Module**: Order entity, order service, order state machine
- **Trade Module**: Trade execution, position tracking, asset management
- **Risk Module**: Risk rules (margin, position, daily limits, price limits)
- **Task Module**: Task queue, scheduler for async operations
- **Auth Module**: User authentication, session management
- **Gateway Module**: Exchange connectivity, order routing
- **Market Module**: Market data, quotes, order books
- **Algo Module**: Algo trading engine (VWAP, TWAP, POV, Iceberg)
- **Strategy Module**: Strategy engine (MA, Grid, etc.)
- **Admin Module**: CLI tools, health checks, system metrics

## Key Concepts

- All components use `tradex` namespace
- Order side: Buy/Sell
- Order type: Market/Limit/Stop/StopLimit
- Order status: Pending/New/PartiallyFilled/Filled/Cancelled/Rejected
- Risk rules use Strategy pattern for extensibility

## Development Notes

- C++17 standard enforced via CMake
- Add new source files to respective module's CMakeLists.txt
- Headers in include/, implementations in src/
- Each module can be built and tested independently
- Follow existing code style and patterns

## Code Quality Standards

### Quality Assessment Dimensions

This project follows industry-standard code quality practices. Code must pass quality assessment before commit.

| Dimension | Weight | Pass Threshold |
|-----------|--------|-----------------|
| Security | 20% | >= 90% |
| Google C++ Compliance | 20% | >= 80% |
| Complexity | 20% | >= 80% |
| Readability | 15% | >= 80% |
| Performance | 15% | >= 80% |
| Test Coverage | 10% | >= 70% |

**Overall pass threshold: >= 85%**

### Issue Severity Levels

| Level | Description | Required Action |
|-------|-------------|-----------------|
| **Critical** | Memory leak, null pointer, data race | Must fix before commit |
| **Blocker** | Security vulnerability, resource leak | Must fix before commit |
| **Optimization** | Style inconsistency, readability | Suggested fix |

### Pass Conditions

Code review passes only if:
1. Overall pass rate >= 85%
2. Security pass rate >= 90%
3. **Zero Critical issues**
4. **Zero Blocker issues**

### Running Code Review

Use the code-review skill:
```
/code-review
```

Or manually run assessment:
```bash
# Check for common issues
grep -rn "std::atoi\|strcpy\|sprintf" src/
grep -rn "new \|malloc(" src/
```

### Known Issues (To Fix)

See `.claude/skills/code-review.yaml` for detailed assessment criteria.