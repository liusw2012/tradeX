# tradeX - A-Share Securities OMS

tradeX 是一个用 C++ 编写的 A 股证券订单管理系统（Order Management System，简称 OMS）。该系统为证券公司、基金公司和专业投资者提供完整的交易订单管理、风险控制和市场数据处理能力。

## 项目背景

随着中国资本市场的快速发展，高频交易、算法交易和量化投资的需求日益增长。tradeX 系统旨在提供一个高性能、可扩展的交易订单管理平台，支持：

- 多账户订单管理
- 实时风险监控
- 算法交易策略
- 策略交易执行
- 完整的日志审计

## 技术栈

| 组件 | 技术选择 |
|------|----------|
| 编程语言 | C++17 |
| 构建系统 | CMake 3.10+ |
| 数据库 | MySQL 5.7+ |
| 网络通信 | Socket (TCP/IP) |
| 平台支持 | Windows / Linux |

### 核心模块

- **Framework**: 日志、配置、线程池、状态监控
- **DAO**: 数据库连接池、基础数据访问
- **Order**: 订单管理、订单服务、订单状态机
- **Trade**: 交易执行、持仓跟踪、资金管理
- **Risk**: 风险规则（保证金、持仓限额、日内涨跌幅）
- **Task**: 任务队列、调度器
- **Auth**: 用户认证、会话管理
- **Gateway**: 交易所连接、订单路由
- **Market**: 行情数据、报价、订单簿
- **Algo**: 算法交易引擎（VWAP、TWAP、POV、Iceberg）
- **Strategy**: 策略引擎（MA、Grid 等）
- **Admin**: 命令行工具、健康检查、系统指标
- **Net**: 网络通信（服务端/客户端）

## 项目结构

```
tradeX/
├── CMakeLists.txt          # 构建配置
├── README.md               # 项目文档
├── CLAUDE.md               # Claude Code 开发指南
├── config/                 # 配置文件
│   └── tradex.json
├── framework/              # 框架层（公共组件）
│   ├── include/
│   │   └── tradex/framework/
│   │       ├── log.h           # 日志系统
│   │       ├── config.h        # 配置管理
│   │       ├── thread_pool.h   # 线程池
│   │       ├── status.h        # 状态与健康检查
│   │       ├── singleton.h     # 单例模板
│   │       └── version.h       # 版本信息
│   └── src/
├── dao/                    # 数据访问层
│   ├── include/
│   │   └── tradex/dao/
│   │       ├── connection_pool.h  # 连接池
│   │       └── base_dao.h        # DAO 基类
│   └── src/
├── order/                  # 订单管理
│   ├── include/
│   │   └── tradex/order/
│   │       ├── order.h         # 订单实体
│   │       └── order_service.h # 订单服务
│   └── src/
├── trade/                  # 交易执行
│   ├── include/
│   │   └── tradex/trade/
│   │       ├── trade.h         # 交易与持仓
│   │       └── trade_service.h
│   └── src/
├── risk/                   # 风险管理
│   ├── include/
│   │   └── tradex/risk/
│   │       └── risk.h          # 风险规则与管理器
│   └── src/
├── task/                   # 任务调度
│   ├── include/
│   │   └── tradex/task/
│   │       └── task.h          # 任务队列与调度器
│   └── src/
├── auth/                   # 认证授权
│   ├── include/
│   │   └── tradex/auth/
│   │       └── auth.h          # 用户与会话管理
│   └── src/
├── gateway/                # 交易所网关
│   ├── include/
│   │   └── tradex/gateway/
│   │       └── gateway.h       # 订单路由与网关
│   └── src/
├── market/                 # 行情数据
│   ├── include/
│   │   └── tradex/market/
│   │       └── market.h        # 行情数据服务
│   └── src/
├── algo/                   # 算法交易
│   ├── include/
│   │   └── tradex/algo/
│   │       └── algo.h          # 算法引擎与策略
│   └── src/
├── strategy/               # 策略交易
│   ├── include/
│   │   └── tradex/strategy/
│   │       └── strategy.h      # 策略引擎
│   └── src/
├── admin/                  # 管理工具
│   ├── include/
│   │   └── tradex/admin/
│   │       └── admin.h         # CLI 与健康检查
│   └── src/
├── net/                    # 网络通信
│   ├── include/
│   │   └── tradex/net/
│   │       └── net.h           # 网络服务端/客户端
│   └── src/
├── cli/                    # 命令行客户端
│   ├── cli_main.cpp
│   └── cli_commands.cpp
├── src/                    # 主程序入口
│   └── main.cpp
├── unit_test/              # 单元测试
└── logs/                   # 日志输出目录
```

## 模块说明

### 核心业务模块

1. **Order（订单模块）**
   - 订单实体定义（买卖方向、市价/限价、止损等）
   - 订单服务（提交、撤单、改单）
   - 订单状态机管理

2. **Trade（交易模块）**
   - 成交记录管理
   - 持仓跟踪
   - 资金账户管理

3. **Risk（风控模块）**
   - 保证金检查
   - 持仓限额检查
   - 日内涨跌幅限制
   - 订单频率限制

### 支持模块

4. **Gateway（网关模块）**
   - 交易所连接管理
   - 订单路由与分发

5. **Market（行情模块）**
   - 实时行情接收
   - 订单簿管理

6. **Algo（算法模块）**
   - VWAP 算法
   - TWAP 算法
   - POV 算法
   - Iceberg 算法

7. **Strategy（策略模块）**
   - 策略引擎
   - MA 策略
   - Grid 策略

8. **Admin（管理模块）**
   - CLI 管理工具
   - 健康检查
   - 系统监控

9. **Net（网络模块）**
   - TCP 服务端
   - TCP 客户端
   - 消息协议处理

10. **CLI（客户端）**
    - 命令行交易客户端
    - 连接服务器
    - 下单、撤单、查询

## 构建环境

### 环境要求

| 组件 | 最低版本 | 推荐版本 |
|------|----------|----------|
| 操作系统 | Windows 10 / Ubuntu 20.04 | Windows 11 / Ubuntu 22.04 |
| 编译器 | GCC 8.0 / MSVC 2019 | GCC 11+ / MSVC 2022 |
| CMake | 3.10 | 3.22+ |
| MySQL | 5.7 | 8.0 |
| 内存 | 4GB | 8GB+ |
| 磁盘 | 10GB | 50GB+ SSD |

### Windows 环境配置

1. 安装 MSYS2 或 MinGW-w64
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc
   pacman -S mingw-w64-ucrt-x86_64-cmake
   ```

2. 确保 PATH 包含编译器路径

### Linux 环境配置

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake mysql-server

# 或使用 GCC 11
sudo apt install gcc-11 g++-11 cmake
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
```

## 构建步骤

### 1. 创建构建目录

```bash
mkdir build
cd build
```

### 2. 配置项目

```bash
cmake -B build .
```

### 3. 编译项目

```bash
cmake --build .
```

### 4. 查看构建产物

```bash
ls bin/
```

应该能看到以下可执行文件：
- `tradeX` - 主程序
- `tradeX_cli` - 命令行客户端

## 运行步骤

### 启动 tradeX 服务端

```bash
./bin/tradeX
```

服务启动后会：
1. 初始化日志系统
2. 启动网络服务器（默认端口 9000）
3. 初始化各个服务模块
4. 提交一个测试订单

### 管理模式

以管理员模式启动，可以执行管理命令：

```bash
./bin/tradeX --admin
```

常用管理命令：
- `orders` - 查看所有订单
- `status` - 查看系统状态
- `metrics` - 查看系统指标
- `help` - 查看帮助

### 启动 CLI 客户端

```bash
# 交互模式
./bin/tradeX_cli

# 命令模式
./bin/tradeX_cli connect 127.0.0.1 9000 buy 600000 100 10.50
```

CLI 常用命令：
- `connect <host> <port>` - 连接服务器
- `buy <symbol> <qty> <price>` - 买入
- `sell <symbol> <qty> <price>` - 卖出
- `cancel <order_id>` - 撤单
- `orders` - 查询订单
- `positions` - 查询持仓
- `quit` - 退出

### 日志查看

运行时日志会输出到两个位置：
1. 控制台（stdout/stderr）- 实时显示服务端处理信息
2. 日志文件 `logs/tradex.log` - 持久化日志记录

服务端日志输出示例：
```
[SERVER] Client 1 request: SubmitOrder, body: {...}
[SERVER] Creating order: symbol=300033.SZ, qty=300, price=300.33, side=Buy
[SERVER] Order submitted successfully, ID: 1738223456001
[SERVER] Response sent to client 1
```

日志文件支持自动轮转，单文件最大 10MB，保留 5 个历史文件（.1 ~ .5）。

## 客户端与服务端交互

### 典型使用流程

1. **启动服务端**
   ```bash
   ./bin/tradeX
   ```

2. **启动客户端并连接**
   ```bash
   ./bin/tradeX_cli
   tradeX> connect 127.0.0.1 9000
   ```

3. **下单交易**
   ```
   tradeX> buy 300033.SZ 300 300.33
   ```

4. **查询订单**
   ```
   tradeX> orders
   ```

### 交互日志示例

**客户端执行：**
```
[tradeX] $ buy 300033.SZ 300 300.33
Order submitted: BUY 300033.SZ 300 @ 300.33 [RequestID: 1]
```

**服务端输出：**
```
[SERVER] Client 1 request: SubmitOrder, body: {"request_id":1,"symbol":"300033.SZ","qty":300,"price":300.33,"side":"Buy","type":"Limit"}
[SERVER] Received order request: {"request_id":1,"symbol":"300033.SZ","qty":300,"price":300.33,"side":"Buy","type":"Limit"}
[SERVER] Creating order: symbol=300033.SZ, qty=300, price=300.33, side=Buy
[SERVER] Order submitted successfully, ID: 1738223456001
[SERVER] Sending response: {"success":true,"message":"Order submitted successfully, ID: 1738223456001",...}
[SERVER] Response sent to client 1
```

**查询订单：**
```
tradeX> orders
Query sent for orders [RequestID: 2]
```

**服务端响应：**
```
[SERVER] Client 1 request: QueryOrders, body: {"request_id":2}
[SERVER] Query orders response: 1 orders found
```

### 服务端管理命令

在另一个终端连接服务端查询：
```bash
./bin/tradeX --admin
tradeX> orders
```

## 配置说明

配置文件位于 `config/tradex.json`：

```json
{
    "server": {
        "port": 9000
    },
    "database": {
        "host": "localhost",
        "port": 3306,
        "user": "tradex",
        "password": "",
        "database": "tradex"
    },
    "risk": {
        "max_position": 1000000,
        "max_order_qty": 100000,
        "max_order_amount": 1000000
    }
}
```

## 扩展开发

### 添加新模块

1. 在对应目录创建 `include/` 和 `src/` 子目录
2. 在模块目录添加 `CMakeLists.txt`
3. 在主 `CMakeLists.txt` 添加 `add_subdirectory()`

### 添加新风险规则

参考 `risk/` 模块，使用策略模式添加新规则：

```cpp
class MyRiskRule : public IRiskRule {
public:
    RiskResult check(const Order& order) override {
        // 实现风控逻辑
    }
};
```

## 常见问题

### 1. 编译报错找不到头文件

确保在项目根目录执行 cmake，并正确设置 include 路径。

### 2. 连接失败

检查服务端是否启动，端口是否正确（默认 9000），防火墙是否放行。

### 3. 日志文件无法写入

确保 `logs/` 目录存在且有写权限。

## 许可证

本项目仅供学习参考使用。

## 联系方式

如有问题，请提交 Issue 或 Pull Request。