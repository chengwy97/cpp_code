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


# spdlog
```bash
git clone https://github.com/gabime/spdlog.git
cd spdlog

mkdir build
cd build
cmake ..
sudo make install
```

```bash
sudo apt install libzmq3-dev libsqlite3-dev google-mock

sudo apt install libgtest-dev

cd /usr/src/googletest
# 创建一个构建目录
sudo mkdir build
cd build
# 配置并编译
sudo cmake ..
sudo make
# 安装到系统库目录
sudo make install


git clone https://github.com/BehaviorTree/BehaviorTree.CPP.git

mkdir build
cd build
cmake ..
sudo make install

```

