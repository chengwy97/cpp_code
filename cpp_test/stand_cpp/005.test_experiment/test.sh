#!/bin/bash

for i in {1..1000}
do
    /app/cpp_code/build/cpp_test/stand_cpp/005.test_experiment/cpp_test_stand_cpp_005.test_experiment 2>&1 > /dev/null
    if [ $? -ne 0 ]; then
        echo "test failed"
        exit 1
    fi
    echo "test passed"
done

echo "all tests passed"
exit 0
