# cpp_code
c++ 代码测试

boost 版本：1.89.0

``` bash
./bootstrap.sh --with-toolset=gcc --prefix=/usr/local/boost/1.89.0
sudo ./b2 install -j4
echo "/usr/local/boost/1.89.0/lib" | sudo tee /etc/ld.so.conf.d/boost.conf
sudo ldconfig
```


环境配置

``` bash
sudo apt install clang-format cmake-format clangd-18
pip install black autopep8 yapf
```

