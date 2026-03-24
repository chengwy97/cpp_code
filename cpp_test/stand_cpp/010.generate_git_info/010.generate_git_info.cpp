#include <frozen/string.h>
#include <frozen/unordered_map.h>

#include <array>
#include <effolkronium/random.hpp>
#include <exec/static_thread_pool.hpp>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <stdexec/execution.hpp>

#include "version/version.hpp"

enum class Language : int {
    日本語  = 10,
    한국어  = 20,
    English = 30,
    Emoji   = 40,
};

class Test {
   private:
    class PrivateTag {};

   private:
    int         a_;
    double      b_;
    std::string c_;

   public:
    Test(PrivateTag, int a, double b, std::string c) : a_(a), b_(b), c_(std::move(c)) {}
    Test(PrivateTag, int a, double b) : a_(a), b_(b){};

   public:
    template <typename... Args>
    static std::unique_ptr<Test> create(Args&&... args) {
        return std::make_unique<Test>(PrivateTag(), std::forward<Args>(args)...);
    }

    void print() { std::cout << "Test: " << a_ << " " << b_ << " " << c_ << std::endl; }
};

int main() {
    std::cout << magic_enum::enum_name(Language::日本語) << std::endl;   // Japanese
    std::cout << magic_enum::enum_name(Language::한국어) << std::endl;   // Korean
    std::cout << magic_enum::enum_name(Language::English) << std::endl;  // English
    std::cout << magic_enum::enum_name(Language::Emoji) << std::endl;    // Emoji

    std::cout << std::boolalpha;
    std::cout << (magic_enum::enum_cast<Language>("日本語").value() == Language::日本語)
              << std::endl;  // true

    std::cout << "Git commit ID: " << GIT_COMMIT_ID << std::endl;
    std::cout << "Build time: " << BUILD_TIME << std::endl;
    std::cout << "Build type: " << BUILD_TYPE << std::endl;
    std::cout << "Build system: " << BUILD_SYSTEM << std::endl;
    std::cout << "Build system name: " << BUILD_SYSTEM_NAME << std::endl;
    std::cout << "Build system version: " << BUILD_SYSTEM_VERSION << std::endl;
    std::cout << "Build system processor: " << BUILD_SYSTEM_PROCESSOR << std::endl;
    std::cout << "Build machine: " << BUILD_MACHINE << std::endl;
    std::cout << "Build number: " << BUILD_NUMBER << std::endl;
    std::cout << "Build machine: " << BUILD_MACHINE << std::endl;
    std::cout << "Build number: " << BUILD_NUMBER << std::endl;
    std::cout << "Build machine: " << BUILD_MACHINE << std::endl;
    std::cout << "Build number: " << BUILD_NUMBER << std::endl;

    auto test = Test::create(1, 2.0, "test");
    test->print();

    std::cout << "Random number: " << effolkronium::random_static::get(0, 100) << std::endl;
    std::cout << "Random number: " << effolkronium::random_static::get(0, 100) << std::endl;
    std::cout << "Random number: " << effolkronium::random_static::get(0, 100) << std::endl;

    // Declare a pool of 3 worker threads:
    exec::static_thread_pool pool(3);

    // Get a handle to the thread pool:
    auto sched = pool.get_scheduler();

    // Describe some work:
    // Creates 3 sender pipelines that are executed concurrently by passing to `when_all`
    // Each sender is scheduled on `sched` using `on` and starts with `just(n)` that creates a
    // Sender that just forwards `n` to the next sender.
    // After `just(n)`, we chain `then(fun)` which invokes `fun` using the value provided from
    // `just()` Note: No work actually happens here. Everything is lazy and `work` is just an object
    // that statically represents the work to later be executed
    auto fun  = [](int i) { return i * i; };
    auto work = stdexec::when_all(stdexec::starts_on(sched, stdexec::just(0) | stdexec::then(fun)),
                                  stdexec::starts_on(sched, stdexec::just(1) | stdexec::then(fun)),
                                  stdexec::starts_on(sched, stdexec::just(2) | stdexec::then(fun)));

    // Launch the work and wait for the result
    auto [i, j, k] = stdexec::sync_wait(std::move(work)).value();

    // Print the results:
    std::cout << "i: " << i << ", j: " << j << ", k: " << k << std::endl;

    /* delete double free */
    // char* p = new char[10];
    // delete[] p;
    // delete[] p;

    /* use after free */
    // char* p = new char[10];
    // delete[] p;
    // std::cout << p[0] << std::endl;

    /* use uninitialized value */
    int* p = new int[10];
    std::cout << p[0] << std::endl;
    delete[] p;
    return 0;
}
