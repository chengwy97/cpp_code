好——下面一次性给你 **完整 30 天的学习版（每天 2–3 个特性）**，每一天包含：

1. **短而清晰的讲解（重点 & 易犯坑）**
2. **风格 B 的示例代码（完整可编译程序，约 20–40 行）**
3. **2–3 道练习题并给出提示（Hint）**

注意：部分示例使用 C++17/20/23 的特性（会在当天说明）。如果你的编译器还不支持某天的特性，可以把该天当作阅读/理解日，然后在支持的工具链（如最新的 GCC/Clang / MSVC）上运行。

---

# Day 1 — `auto` / `nullptr` / range-based for

**讲解**

* `auto`：让编译器推导类型，减少样板代码，但别滥用影响可读性。
* `nullptr`：类型安全的空指针，替代 `NULL`/`0`。
* range-for：用来遍历容器或数组，代码更简洁。

**示例**

```cpp
#include <iostream>
#include <vector>
#include <map>
int main() {
    auto n = 10;                // int
    auto pi = 3.1415;          // double
    std::string s = "hi";
    int* p = nullptr;          // 安全空指针

    std::vector<int> v{1,2,3,4};
    for (auto &x : v) {
        x *= 2;
    }
    for (auto x : v) std::cout << x << " ";
    std::cout << "\n";

    std::map<std::string,int> m{{"a",1},{"b",2}};
    for (auto it = m.begin(); it != m.end(); ++it)
        std::cout << it->first << ":" << it->second << " ";
    std::cout << "\n";
}
```

**练习**

1. 用 `auto` 声明 `std::map<std::string,int>::iterator`，打印所有键值。
   Hint：`auto it = m.begin();`
2. 写 range-for 遍历二维 `std::vector<std::vector<int>>` 并求和。
   Hint：嵌套两个 range-for。
3. 比较 `NULL` 与 `nullptr` 在函数重载中的行为差别。
   Hint：写两个重载 `f(int)` 和 `f(char*)`，传 `NULL` / `nullptr`。

---

# Day 2 — `decltype` / 统一初始化 `{}` / type traits（基础）

**讲解**

* `decltype(expr)` 把表达式的静态类型返回。
* `{}` 初始化可以禁止窄化（narrowing）并统一构造语法。
* `<type_traits>` 提供类型判断（`is_integral`、`is_same` 等），方便泛型编程。

**示例**

```cpp
#include <iostream>
#include <type_traits>
#include <vector>
struct S { int x; double y; };
int main() {
    int a = 1;
    decltype(a) b = 2; // b is int

    S s{10, 3.14};     // {} 初始化
    // auto c = {1,2}; // 小心：可能为 std::initializer_list<int>

    static_assert(std::is_integral<decltype(b)>::value, "b must be integral");

    std::vector<int> v{1,2,3};
    for (decltype(v.size()) i = 0; i < v.size(); ++i) std::cout << v[i] << " ";
    std::cout << "\n";
}
```

> **decltyp 规则总结**
`decltype(expr)` 返回 `expr` 的**静态类型**，但有细节（C++11 及以后）：
- `decltype(x)` —— 仅名字，结果是变量的类型（包括引用/const）
- `decltype((x))` —— 括号表示表达式，结果是**左值**表达式则为 T&，右值表达式则为 T
- `decltype(1 + 2.0)` —— 表达式类型自动推导（此例为 double）
- `decltype(auto)` 可以在返回值自动推断时结合上一条规则

举例：
```cpp
int a = 1;
int& b = a;
const int c = 2;
decltype(a) x1 = 0;        // int
decltype(b) x2 = x1;       // int&
decltype((b)) x3 = x1;     // int&  (因为(b)是左值表达式)
decltype(c) x4 = 3;        // const int
```

> **常用 type_traits 概览**
位于 `<type_traits>`，用于类型推断与元编程：
- `std::is_integral<T>`           是否为整型
- `std::is_floating_point<T>`     是否为浮点指针
- `std::is_same<T, U>`            类型是否相同
- `std::remove_reference<T>`      移除引用
- `std::remove_const<T>`          移除const
- `std::decay<T>`                 类似于函数参数推断后的类型
- `std::enable_if`, `std::conditional`, `std::void_t` 等，用于高级 SFINAE
- `std::is_const<T>`, `std::is_reference<T>`, `std::is_pointer<T>`, ...

范例：
```cpp
template<class T>
void foo(T val) {
    static_assert(std::is_integral<T>::value, "T 必须是整数");
}
static_assert(std::is_same<decltype(1+2.0), double>::value, "结果为 double");
```



**练习**

1. 用 `decltype` 推导 `x + y` 的类型（x: int, y: double）。
   Hint：`decltype(x + y)`。
2. 使用列表初始化创建 `struct` 对象并禁止窄化。
   Hint：尝试 `int i{3.14};` 会编译失败。
3. 写一个模板 `is_floating_point_or_integral<T>` 复合判断。
   Hint：`std::is_integral<T>::value || std::is_floating_point<T>::value`。

---

# Day 3 — 右值引用 / 移动语义 / `std::move`

**讲解**

* 右值引用 `T&&` 用于移动语义，减少拷贝开销。
* `std::move` 将左值转为能绑定到右值引用的类型（并不真的移动）。
* 实现移动构造/移动赋值时注意置空源对象以保证异常安全与一致性。

**示例**

```cpp
#include <iostream>
#include <vector>
struct Big {
    std::vector<int> data;
    Big() : data(1000000, 42) {}
    Big(const Big& other) : data(other.data) { std::cout << "copy\n"; }
    Big(Big&& other) noexcept : data(std::move(other.data)) { std::cout << "move\n"; }
};
int main() {
    Big a;
    Big b = std::move(a); // 使用移动构造
    std::cout << "done\n";
}
```

**练习**

1. 实现一个包含动态数组（`new[]`）的类，写移动构造与移动赋值。
   Hint：释放旧资源并将指针置为 `nullptr`。
2. 为什么 `std::move` 本身不做移动？
   Hint：`std::move` 只是类型转换，真正移动由移动构造或移动赋值完成。
3. 什么时候不应该使用 `std::move`？
   Hint：不要对将来还要使用的变量调用 `std::move`。

---

# Day 4 — 完美转发 / 可变参数模板 / `=default` / `=delete`

**讲解**

* 完美转发：`template<typename T> f(T&& t)` 配合 `std::forward<T>(t)` 保留值类别。
* 可变参数模板 `template<typename... Args>` 支持任意数量参数。
* `=default` 与 `=delete` 用于显式控制生成函数（构造/赋值/析构）。

**示例**

```cpp
#include <iostream>
#include <utility>

void sink(int&) { std::cout << "lvalue\n"; }
void sink(int&&) { std::cout << "rvalue\n"; }

template<typename T>
void forwarder(T&& t) {
    sink(std::forward<T>(t));
}

int main() {
    int x = 1;
    forwarder(x);            // lvalue
    forwarder(2);            // rvalue
}
```

**练习**

1. 写一个 `make_unique_like<T, Args...>`（可变模板）返回 `std::unique_ptr<T>`。
   Hint：`new T(std::forward<Args>(args)...)`。
2. 用 `=delete` 禁止某类型被拷贝。
   Hint：`MyType(const MyType&) = delete;`
3. 实现一个接受任意参数并打印类型数量的函数模板。
   Hint：`sizeof...(Args)`。

---

# Day 5 — `constexpr`（C++11）/ `enum class` / `static_assert`

**讲解**

* `constexpr` 让函数/变量在编译期求值（C++11 有限制，C++14+ 放宽）。
* `enum class` 为强类型枚举，避免命名污染与隐式转换。
* `static_assert` 编译期断言。

**示例**

```cpp
#include <iostream>
constexpr int fib(int n) {
    return n < 2 ? n : fib(n-1) + fib(n-2);
}
enum class Color { Red, Green, Blue };
static_assert(sizeof(int) >= 4, "int too small");

int main() {
    constexpr int f5 = fib(5); // 编译期计算
    std::cout << "fib(5)=" << f5 << "\n";
    Color c = Color::Green;
}
```

**练习**

1. 写 `constexpr` 函数计算阶乘并在 `static_assert` 中使用。
   Hint：`constexpr int fact(int n){ return n<=1?1:n*fact(n-1); }`
2. 比较 `enum` 和 `enum class` 的差异（隐式转换）。
   Hint：尝试把 `enum` 值赋给 `int`。
3. 使用 `static_assert` 检查模板参数是否满足条件。
   Hint：`static_assert(std::is_integral<T>::value, "T must be integral");`

---

# Day 6 — Lambda（基础）/ 捕获 / `std::function`

**讲解**

* Lambda 是匿名可调用对象，语法 `[captures](params){ body }`。
* 捕获可以按值 `[=]`、按引用 `[&]`、或指定捕获。
* `std::function` 可持有任意兼容可调用类型，但有性能开销。

**示例**

```cpp
#include <iostream>
#include <functional>
#include <vector>
int main() {
    int factor = 3;
    auto mul = [factor](int x){ return x * factor; }; // 值捕获
    std::vector<int> v{1,2,3};
    std::for_each(v.begin(), v.end(), [&](int &x){ x = mul(x); }); // 引用捕获外部 v
    std::function<int(int)> f = mul;
    for (auto x : v) std::cout << x << " ";
    std::cout << "\n";
}
```

**练习**

1. 写一个 lambda 捕获 `this` 并在成员函数中使用（类中）。
   Hint：`[this]{ ... }`
2. 将 lambda 赋给 `std::function` 并传递给函数作为回调。
   Hint：`void apply(std::function<void(int)> cb)`.
3. 解释捕获按值 vs 按引用在多线程中的风险。
   Hint：按引用捕获如果对象已销毁会悬挂。

---

# Day 7 — `unique_ptr` / `shared_ptr` / `weak_ptr`

**讲解**

* 智能指针管理资源：`unique_ptr` (独占)、`shared_ptr` (共享)、`weak_ptr` (弱引用避免循环引用)。
* 尽量使用 `unique_ptr`，只有必要时才用 `shared_ptr`。

**示例**

```cpp
#include <iostream>
#include <memory>
struct Node {
    int v;
    std::unique_ptr<Node> next;
    Node(int x):v(x){}
};
int main() {
    auto head = std::make_unique<Node>(1);
    head->next = std::make_unique<Node>(2);
    std::shared_ptr<int> sp = std::make_shared<int>(42);
    std::weak_ptr<int> wp = sp;
    if (auto s = wp.lock()) std::cout << *s << "\n";
    std::cout << head->v << " -> " << head->next->v << "\n";
}
```

**练习**

1. 用 `shared_ptr` 实现一个简单的图节点（可能有多入边）。
   Hint：小心循环依赖，考虑 `weak_ptr`。
2. 说明 `unique_ptr` 可以做哪些事情 `shared_ptr` 做不到（性能、语义）。
   Hint：没有引用计数开销。
3. 写一个函数返回 `unique_ptr<T>` 并演示移动返回。
   Hint：`return std::make_unique<T>(...)`。

---

# Day 8 — `std::thread` / `std::mutex` / `std::lock_guard`

**讲解**

* `std::thread` 创建线程，`std::mutex` 用于互斥，`std::lock_guard` RAII 风格管理锁，避免死锁/忘记 unlock。
* 注意共享数据的同步、竞态条件与死锁。

**示例**

```cpp
#include <iostream>
#include <thread>
#include <mutex>
int counter = 0;
std::mutex m;
void worker(int id) {
    for (int i=0;i<1000;i++) {
        std::lock_guard<std::mutex> lk(m);
        ++counter;
    }
}
int main() {
    std::thread t1(worker,1), t2(worker,2);
    t1.join(); t2.join();
    std::cout << "counter = " << counter << "\n";
}
```

**练习**

1. 改用 `std::atomic<int>` 替换 mutex，比较性能与安全性。
   Hint：`std::atomic<int> counter{0};`
2. 写一个条件变量（`std::condition_variable`）实现生产者-消费者的简单例子。
   Hint：使用 `wait` / `notify_one`。
3. 解释为什么持有锁时不要长时间阻塞或调用外部回调。
   Hint：会阻塞其他线程并可能导致死锁。

---

# Day 9 — `std::future`/`std::promise`/`std::async` & `std::chrono`

**讲解**

* `std::async` 简单获得异步结果（返回 `future`）。
* `promise` 用于手动在一个线程设置结果。
* `chrono` 用于精确时间度量与睡眠。

**示例**

```cpp
#include <iostream>
#include <future>
#include <chrono>

int slow_square(int x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return x*x;
}

int main() {
    auto fut = std::async(std::launch::async, slow_square, 5);
    std::cout << "Doing other work...\n";
    auto val = fut.get();
    std::cout << "Result: " << val << "\n";
}
```

**练习**

1. 用 `std::promise` 在一个线程中设置值，主线程等待 `future`。
   Hint：`std::promise<int> p; auto f = p.get_future();`
2. 用 `std::chrono` 计算一段代码运行耗时（毫秒）。
   Hint：`steady_clock::now()`。
3. `std::async` 有哪些启动策略？对性能有何影响？
   Hint：`std::launch::async` vs `std::launch::deferred`。

---

# Day 10 — C++14：泛型 lambda / `std::make_unique` / relaxed `constexpr`

**讲解**

* C++14 引入泛型 lambda（参数为 `auto`），更灵活。
* `std::make_unique` 填补 C++11 的空白。
* C++14 放宽 `constexpr` 的限制，允许更多语句。

**示例**

```cpp
#include <iostream>
#include <memory>
#include <vector>
int main() {
    auto add = [](auto a, auto b){ return a + b; }; // 泛型 lambda
    std::cout << add(1,2) << " " << add(1.5, 2.3) << "\n";
    auto p = std::make_unique<std::vector<int>>(std::initializer_list<int>{1,2,3});
    for (auto v : *p) std::cout << v << " ";
    std::cout << "\n";
}
```

**练习**

1. 写一个泛型 lambda 实现 `map`（给 vector 和函数）。
   Hint：返回一个新的 vector，逐项应用 lambda。
2. 用 `constexpr` 写一个能在编译期验证的函数（C++14）。
   Hint：可以用循环或局部变量（C++14 允许）。
3. 解释 `make_unique` 的优势。
   Hint：异常安全（避免裸 `new` 的临时泄露）。

---

# Day 11 — C++17：if/switch 初始化 / structured bindings

**讲解**

* `if (auto it = m.find(x); it != m.end())` 在 if 里创建短生命周期变量。
* 结构化绑定 `auto [k,v] = *it;` 直接解构 tuple/pair/struct。

**示例**

```cpp
#include <iostream>
#include <map>
int main() {
    std::map<std::string,int> m{{"a",1},{"b",2}};
    if (auto it = m.find("a"); it != m.end()) {
        auto [k,v] = *it;
        std::cout << k << "=" << v << "\n";
    }
    std::pair<int,std::string> p{7,"seven"};
    auto [num,str] = p;
    std::cout << num << " " << str << "\n";
}
```

**练习**

1. 在 `if` 初始化中使用 `std::find` 查找 vector 元素并打印索引。
   Hint：`if (auto it = std::find(v.begin(), v.end(), val); it != v.end())`
2. 用结构化绑定解构 `std::tuple<int,double,std::string>`。
   Hint：`std::tuple<int,double,std::string> t{...}; auto [a,b,c]=t;`
3. 结构化绑定并不会延长被绑定对象寿命——说明并举例。
   Hint：绑定临时会有不同语义，注意引用绑定。

---

# Day 12 — 折叠表达式 / inline 变量 / `constexpr` lambda

**讲解**

* 折叠表达式（fold expressions）简化变参模板操作（C++17）。
* `inline` 变量允许在头文件中定义全局变量而不冲突。
* `constexpr` lambda（C++17）可用于编译期计算。

**示例**

```cpp
#include <iostream>
template<typename... Args>
auto sum(Args... args) { return (args + ... + 0); } // 折叠表达式
inline int global_counter = 0; // C++17 inline 变量

int main() {
    std::cout << "sum = " << sum(1,2,3,4) << "\n";
    auto lam = [](int x) constexpr { return x+1; }; // 示例（部分编译器）
    std::cout << lam(5) << "\n";
}
```

**练习**

1. 用折叠表达式实现 `all_true(Args...)`。
   Hint：`return (args && ...);`
2. 把单例的静态变量改成 `inline` 变量，并讨论优缺点。
   Hint：头文件包含多次仍安全。
3. 用 `constexpr` lambda 实现编译期小工具（如果你的编译器支持）。
   Hint：GCC/Clang 新版支持。

---

# Day 13 — `std::optional` / `std::variant` / `std::any`（C++17）

**讲解**

* `optional<T>` 表示可能存在或不存在的值，替代裸指针或特殊值。
* `variant` 可在有限集合类型间切换，类似安全的 union。
* `any` 可存放任意类型（类型擦除），读取时需 `any_cast`。

**示例**

```cpp
#include <iostream>
#include <variant>
#include <optional>
#include <any>

int main() {
    std::optional<int> oi;
    oi = 5;
    if (oi) std::cout << *oi << "\n";

    std::variant<int,std::string> v = "hello";
    v = 42;
    std::visit([](auto&& x){ std::cout << x << "\n"; }, v);

    std::any a = 10;
    try { std::cout << std::any_cast<int>(a) << "\n"; } catch(...) {}
}
```

**练习**

1. 写一个函数返回 `optional<string>` 表示查找结果。
   Hint：若没找到返回 `std::nullopt`。
2. 用 `variant` 实现一个简单的 AST 节点（int 或 string 或 pair）。
   Hint：用 `std::visit` 处理。
3. 说明 `any` 与 `variant` 的不同场景。
   Hint：`variant` 在编译期限定可能的类型，`any` 不限定但需运行时转换。

---

# Day 14 — `std::string_view` / `std::filesystem`

**讲解**

* `string_view`：轻量、非拥有的字符串视图，避免不必要拷贝。小心生命周期。
* `filesystem`（C++17）：文件、路径、目录遍历与文件属性，跨平台封装。

**示例**

```cpp
#include <iostream>
#include <string_view>
#include <filesystem>

int main() {
    std::string s = "hello_world";
    std::string_view sv(s.c_str()+6, 5); // "world"
    std::cout << sv << "\n";

    for (auto &p : std::filesystem::directory_iterator(".")) {
        std::cout << p.path().string() << "\n";
    }
}
```

**练习**

1. 写函数接收 `string_view` 做字符串切片，不拷贝数据。
   Hint：返回 `std::string_view`。
2. 用 `filesystem` 递归列出当前目录下所有 `.cpp` 文件。
   Hint：`recursive_directory_iterator`。
3. 解释 `string_view` 的生命周期陷阱并举例。
   Hint：不要从临时 `std::string()` 获取 `string_view` 并长期使用。

---

# Day 15 — `std::pmr`（内存资源）与 `std::scoped_lock` 等并发工具

**讲解**

* `std::pmr`（polymorphic memory resource）：为高性能场景提供可插拔分配器，减少碎片与提升性能。
* `scoped_lock`（C++17）用于多 mutex 安全锁定以避免死锁。

**示例（示意）**

```cpp
#include <iostream>
#include <memory_resource>
#include <vector>

int main() {
    std::pmr::monotonic_buffer_resource pool(1024);
    std::pmr::vector<int> v(&pool);
    for (int i=0;i<10;i++) v.push_back(i);
    for (auto x : v) std::cout << x << " ";
    std::cout << "\n";
}
```

**练习**

1. 用 `monotonic_buffer_resource` 为一个 `pmr::string` 提供内存并比较性能。
   Hint：小对象大量分配场景更明显。
2. 使用 `std::scoped_lock` 同时锁两个 mutex。
   Hint：`std::scoped_lock lk(m1,m2);`
3. 讨论 `pmr` 在游戏/数值模拟的应用价值。
   Hint：大量重复分配/释放场景。

---

# Day 16 — C++20 Concepts（基础）

**讲解**

* Concepts 通过 `requires`、`concept` 指定模板参数约束，改善错误信息并提升类型安全。
* 可组合、可复用。

**示例（C++20）**

```cpp
#include <concepts>
#include <iostream>

template<typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

template<Addable T>
T add(T a, T b){ return a+b; }

int main() {
    std::cout << add(1,2) << "\n";
    // std::cout << add(std::string("a"), std::string("b")) << "\n"; // OK if convertible
}
```

**练习**

1. 写一个 `Sortable` concept 要求 `operator<` 可用。
   Hint：`requires(T a, T b) { { a < b } -> std::convertible_to<bool>; }`
2. 用 concept 限制模板仅接受整型或浮点型。
   Hint：`std::integral` / `std::floating_point`。
3. 比较 concepts 与 `static_assert` 的不同（可读性/错误信息）。
   Hint：试着传不合规类型并观察编译错误。

---

# Day 17 — Ranges / views（C++20）

**讲解**

* Ranges 提供惰性、链式的数据流处理（`views::filter`、`views::transform` 等）。
* 语法自然（管道 `|`），便于组合。

**示例（C++20）**

```cpp
#include <iostream>
#include <vector>
#include <ranges>

int main() {
    std::vector<int> v{1,2,3,4,5,6};
    auto even = v | std::views::filter([](int x){ return x%2==0; })
                  | std::views::transform([](int x){ return x*x; });
    for (int x : even) std::cout << x << " ";
    std::cout << "\n";
}
```

**练习**

1. 用 ranges 实现链式 `map` + `filter` + `take(3)`。
   Hint：`std::views::take(3)`。
2. ranges 与传统算法相比优势在哪里？举例。
   Hint：惰性求值、组合性。
3. 说明 ranges view 不持有数据，只持有引用/迭代器——讨论生命周期。
   Hint：源容器销毁后 view 无效。

---

# Day 18 — 三路比较 `<=>`（spaceship operator）

**讲解**

* `<=>` 可以自动生成全套比较操作（`<`, `>`, `==` 等），减少样板代码（C++20）。
* 支持默认比较语义：`auto operator<=>(const T&) const = default;`

**示例（C++20）**

```cpp
#include <compare>
#include <iostream>
struct Point {
    int x,y;
    auto operator<=>(const Point&) const = default;
};

int main() {
    Point a{1,2}, b{1,3};
    std::cout << std::boolalpha << (a < b) << "\n";
}
```

**练习**

1. 为自定义 `struct` 使用 `= default` 的 `<=>` 并测试 `==`/`<`。
   Hint：成员顺序影响比较优先级。
2. 什么时候你需要自定义 `<=>` 而不是 `= default`？
   Hint：需要不同字段权重或忽略字段。
3. 如何与浮点数比较配合使用 `<=>`？注意 NaN。
   Hint：`std::partial_ordering` 处理浮点三态比较。

---

# Day 19 — modules（C++20 基础）

**讲解**

* Modules 用于替代头文件，改善编译时间与封装，可导出符号。
* 需要支持模块的编译器（GCC/Clang/MSVC 新版支持）。

**示例（伪示意，需分文件）**

```cpp
// math.ixx (module interface)
export module math;
export int add(int a,int b){ return a+b; }

// main.cpp
import math;
#include <iostream>
int main(){ std::cout << add(2,3) << "\n"; }
```

**练习**

1. 在支持模块的编译器上尝试将一个工具函数移入 module。
   Hint：模块接口单独文件 `.ixx` 或 `cppm`。
2. 比较模块与传统头文件的优缺点。
   Hint：编译时间与符号暴露。
3. 讨论模块对大型工程的影响（依赖管理）。

---

# Day 20 — `consteval` / `constinit` / `constexpr` 扩展

**讲解**

* `consteval`：强制在编译期执行（若不能则为错误）。
* `constinit`：确保变量在静态初始化阶段被初始化（防止动态初始化顺序问题）。
* `constexpr` 越来越强大，可包含更多语句。

**示例（C++20）**

```cpp
#include <iostream>
consteval int square_consteval(int x) { return x*x; }
constinit int g = 5; // 确保静态初始化
int main() {
    constexpr int v = square_consteval(3);
    std::cout << v << "\n";
}
```

**练习**

1. 写 `consteval` 函数用于生成编译期常量（如表驱动数据）。
   Hint：调用时必须是编译期常量。
2. 解释 `constinit` 与 `constexpr` 的差别。
   Hint：`constinit` 不要求常量表达式，只要求静态初始化。
3. 写 `constexpr` 递归算法并在编译期断言。
   Hint：`static_assert` + `constexpr`。

---

# Day 21 — 协程（coroutines，C++20，基础）

**讲解**

* 协程通过 `co_await`/`co_yield`/`co_return` 实现轻量异步/生成器。
* 需要编写或使用协程 promise/awaiter 类型；标准库提供有限支持，需要第三方库（或自己实现简单生成器）。

**示例（生成器 简化版，需 C++20 协程支持）**

```cpp
// 简化示例 — 真实使用需较多 boilerplate 或使用库
#include <coroutine>
#include <iostream>
#include <optional>

template<typename T>
struct Generator {
    struct promise_type {
        T current;
        std::suspend_always yield_value(T v) {
            current = v;
            return {};
        }
        std::suspend_always initial_suspend(){ return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        Generator get_return_object() { return Generator{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    std::coroutine_handle<promise_type> h;
    Generator(std::coroutine_handle<promise_type> h):h(h){}
    ~Generator(){ if(h) h.destroy(); }
    std::optional<T> next() {
        if (!h || h.done()) return std::nullopt;
        h.resume();
        if (h.done()) return std::nullopt;
        return h.promise().current;
    }
};

Generator<int> gen() {
    for (int i=0;i<5;i++) co_yield i;
}

int main() {
    auto g = gen();
    while (auto v = g.next()) std::cout << *v << " ";
    std::cout << "\n";
}
```

**练习**

1. 实现一个简单的协程生成器（参见示例）并生成 Fibonacci。
   Hint：`co_yield` 每次产生一个值。
2. 讨论协程与线程的区别（开销/调度）。
   Hint：协程为用户态轻量调度，不占核线程资源。
3. 尝试使用已有库（cppcoro/folly/asio）学习更完整示例。
   Hint：可查阅库文档（若想我可以帮你找示例）。

---

# Day 22 — `std::jthread` / `std::stop_token` / `std::atomic_ref`

**讲解**

* `jthread` 自动在析构时请求停止并 join，简化线程管理（C++20）。
* `stop_token` 提供可取消的线程请求。
* `atomic_ref` 允许对非原子对象进行原子操作（包装引用）。

**示例（C++20）**

```cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <stop_token>

void worker(std::stop_token st) {
    while (!st.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << ".";
    }
    std::cout << "\nstopped\n";
}

int main() {
    std::jthread t(worker);
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    // jthread destructor will request stop and join
}
```

**练习**

1. 写用 `std::jthread` 的可取消循环并在主线程请求停止。
   Hint：`std::jthread t(func); t.request_stop();`
2. 说明 `atomic_ref` 与 `std::atomic<T>` 的不同。
   Hint：`atomic_ref` 作用于已有对象，不改变其内存布局。
3. 在设计可停止任务时考虑哪些清理工作？
   Hint：释放资源、数据一致性、通知其他组件。

---

# Day 23 — `std::expected`（C++23）与现代错误处理

**讲解**

* `std::expected<T,E>`（C++23）用于返回成功值或错误信息，比 `optional` 更好表达错误原因。
* 使用 `expected` 可减少异常与错误代码的混杂。

**示例（伪代码，取决于实现可用）**

```cpp
#include <iostream>
// 若编译器没有 std::expected，你可以用类似实现或第三方
#include <expected> // 假设可用

std::expected<int,std::string> parse_int(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (...) {
        return std::unexpected("parse error");
    }
}

int main() {
    auto r = parse_int("123");
    if (r) std::cout << *r << "\n"; else std::cout << r.error() << "\n";
}
```

**练习**

1. 用 `expected` 实现文件读取返回 `expected<std::string, ErrorCode>`。
   Hint：错误包含文件不存在或读取失败。
2. 比较 `expected`、`optional` 与 `exception` 的使用场景。
   Hint：`expected` 适合显式错误处理路径。
3. 若你的编译器没有 `std::expected`，实现一个小型替代（可选）。

---

# Day 24 — `std::mdspan`（C++23）与多维数组视图

**讲解**

* `mdspan` 提供对多维数组的轻量非拥有视图，方便数值计算与线性代数。
* 类似 `span` 的多维扩展。

**示例（示意）**

```cpp
// 伪示例，依赖实现
#include <iostream>
#include <mdspan> // 假设可用

int main() {
    double data[6] = {1,2,3,4,5,6};
    std::mdspan<double, std::extents<size_t,2,3>> m(data);
    std::cout << m(1,2) << "\n"; // 6
}
```

**练习**

1. 用 `mdspan` 视图实现矩阵转置（不复制内存，只改变访问模式）。
   Hint：理解 strides（步幅）。
2. 比较 `mdspan` 与裸指针/`vector` 的区别。
   Hint：`mdspan` 提供形状与索引语义。
3. 在没有 `mdspan` 的环境中如何模拟简单二维视图？
   Hint：用 `span` + stride 计算索引。

---

# Day 25 — Ranges 扩展（例如 `views::zip` / lazy algorithms）

**讲解**

* Ranges 的生态在发展，常见扩展包括 `zip`、`enumerate`、`chunk` 等（部分在标准库扩展或第三方库）。
* 可实现高可读的流水线数据处理。

**示例（使用 pseudo-zip or 手写 zip）**

```cpp
#include <iostream>
#include <vector>
#include <ranges>

int main() {
    std::vector<int> a{1,2,3}, b{4,5,6};
    for (size_t i=0;i<a.size() && i<b.size(); ++i)
        std::cout << a[i] + b[i] << " ";
    std::cout << "\n";
}
```

**练习**

1. 手写一个简单的 `zip` 函数返回 pair view（练习迭代器适配）。
   Hint：先用索引型方式实现。
2. 用 ranges 实现惰性 `sliding window`（窗口）算法（可用第三方）。
   Hint：理解 lazy evaluation。
3. 讨论标准库未来将如何扩展 ranges（可读可写的视图）。

---

# Day 26 — `std::print` / `std::format` 与格式化输出（C++20/23）

**讲解**

* `std::format`（C++20）提供 Python 风格的格式化字符串（如果实现了）。
* `std::print`（C++23）直接打印格式化结果。相比 `iostream` 更灵活、性能更好。

**示例（若支持）**

```cpp
#include <iostream>
#include <format>
int main() {
    std::string s = std::format("Hello {}, number = {:.2f}", "world", 3.14159);
    std::cout << s << "\n";
}
```

**练习**

1. 使用 `format` 生成对齐输出（宽度、填充）。
   Hint：`std::format("{:>10}", val)`.
2. 比较 `printf`、`iostream` 与 `format` 的优缺点。
   Hint：类型安全、国际化、性能。
3. 若你的库无 `std::format`，试用 `fmt` 第三方库（接口类似）。

---

# Day 27 — `constexpr` 扩展（C++20/23） & `if consteval`

**讲解**

* C++20/23 中 `constexpr` 很强，更多标准库函数可在编译期执行。
* `if consteval` 允许在编译期/运行期分支不同实现。

**示例（C++23 风格）**

```cpp
#include <iostream>
consteval int square_ct(int x) { return x*x; }
constexpr int maybe_square(int x) {
    if consteval {
        return square_ct(x);
    } else {
        return x*x;
    }
}
int main() {
    constexpr int v = maybe_square(4);
    std::cout << v << "\n";
}
```

**练习**

1. 写函数在编译期产生查找表（`consteval`）并在运行期使用。
   Hint：生成素数表或三角函数表。
2. 用 `if consteval` 提供编译期和运行期不同实现（如调试/优化）。
   Hint：编译期可返回预计算值。
3. 讨论什么时候应该优先使用编译期计算（成本/可维护性）。

---

# Day 28 — Networking（Networking TS / 第三方，如 ASIO）

**讲解**

* 标准仍在发展，但 Asio / Boost.Asio 为主流解决方案，支持同步/异步 TCP/UDP。
* 学习网络要关注：阻塞 vs 非阻塞、事件驱动、线程模型。

**示例（同步 TCP 客户端，伪示意，需 Boost.Asio）**

```cpp
// 需链接 boost_system / boost_asio，示意代码
#include <iostream>
#include <boost/asio.hpp>
int main() {
    boost::asio::io_context ctx;
    boost::asio::ip::tcp::resolver resolver(ctx);
    auto eps = resolver.resolve("example.com", "80");
    boost::asio::ip::tcp::socket sock(ctx);
    boost::asio::connect(sock, eps);
    std::string req = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
    boost::asio::write(sock, boost::asio::buffer(req));
    boost::asio::streambuf resp;
    boost::asio::read_until(sock, resp, "\r\n");
    std::istream is(&resp);
    std::string line; std::getline(is, line);
    std::cout << line << "\n";
}
```

**练习**

1. 用 Boost.Asio 写一个简单的 TCP 客户端获取 HTTP 首行。
   Hint：同步版本先学清楚。
2. 改写为异步（回调或协程风格），观察代码结构变化。
   Hint：使用 `async_read_until`。
3. 讨论网络编程常见异常与恢复策略（重连/超时）。

---

# Day 29 — 现代 C++ 编码规范与设计原则

**讲解**

* RAII、避免裸 `new/delete`、使用 `noexcept` 标注移动构造/析构（提高优化）、明确所有权语义（unique/shared）。
* 小原则：尽量用值语义、短函数、清晰接口；对外暴露最少实现细节。

**示例（示意）**

```cpp
#include <iostream>
#include <memory>

struct Resource {
    Resource() { std::cout << "acquire\n"; }
    ~Resource(){ std::cout << "release\n"; }
    void use() { std::cout << "using\n"; }
};

int main() {
    auto r = std::make_unique<Resource>();
    r->use();
}
```

**练习**

1. 将一个使用裸 `new`/`delete` 的老代码重构为 RAII 风格。
   Hint：`unique_ptr`、智能指针或封装类。
2. 讨论何时使用异常 vs `expected`/错误返回值。
   Hint：以接口清晰、可维护性为衡量。
3. 写一份简短团队编码规范草案（10 条）。

---

# Day 30 — 综合项目：现代 C++ 小项目

**目标**

* 将学到的若干特性整合成小项目（建议：文件处理 + 数据流 + 并发/协程 + 错误处理 + 格式化输出）。

**项目要点（示例）**

* 读取目录中所有 CSV 文件（`filesystem`）
* 用 `ranges` 过滤/映射并聚合数据
* 使用 `std::thread`/`jthread` 或协程并发处理多个文件
* 用 `expected` 统一错误返回
* 输出结果用 `format`/`print` 格式化

**示例主干（伪代码/结构）**

```cpp
// 伪代码概要，组合多个特性
// main:
// - list files via filesystem
// - spawn jthread pool to process each file
// - each worker: parse CSV (string_view, optional), aggregate
// - collect results, output using format
```

**练习（分解任务）**

1. 实现文件列表并并发读取文件内容（先同步再并发）。
   Hint：先实现 parser，后并发化。
2. 实现 CSV 解析器使用 `string_view`（零拷贝）并返回 `expected<Row,Error>`。
   Hint：处理字段与换行边界。
3. 用 `ranges` 对所有行做过滤（比如某列值 > 阈值）并汇总数值。
   Hint：用 `views::filter` 与 `views::transform`。

---

## 收尾建议

* 如果你希望，我可以把 **每一天的完整版（含完整、可拷贝的 20–40 行示例代码）逐日单独发一条**（例如把上面的每天示例扩充为完整、可直接复制运行的文件、并把练习题答案/解析也给出）。
* 或者我可以把 **所有 30 天生成一个 PDF**（需要我创建文件并提供下载）。我也可以把项目变成一个小 GitHub 风格的代码骨架（多个文件、CMakeLists）。

你现在想要哪种后续内容？

* A) **每天单条完整展开（Day1 完整-> Day30 完整）**（我会一次性把 30 天都展开）
* B) **先给 Day1–5 完整展开，确认风格后继续**
* C) **把这 30 天打包成 PDF/工程骨架**

直接回复：**A / B / C**（或告诉我你要的下一步），我马上继续。
