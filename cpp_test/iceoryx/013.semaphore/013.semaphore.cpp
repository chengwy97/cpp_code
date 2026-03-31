#include <expected>
#include <iostream>
#include <iox/optional.hpp>
#include <iox/semaphore_interface.hpp>
#include <iox/spin_semaphore.hpp>
#include <string>

iox::optional<iox::detail::SemaphoreInterface<iox::concurrent::SpinSemaphore>> g_spin_semaphore;

int main() {
    return 0;
}
