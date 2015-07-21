# Update the System
sudo yum -y update

# Install the Platform Development Tools
sudo yum -y groupinstall "Development Tools"

# Download wget
sudo yum install -y wget

# Upgrade autoconf
wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
tar xvf autoconf-2.69.tar.gz
cd autoconf-2.69
./configure --prefix=/usr
make
sudo make install
cd ..

# Upgrade automake
wget http://ftp.gnu.org/gnu/automake/automake-1.14.tar.gz
tar xvf automake-1.14.tar.gz
cd automake-1.14
./configure --prefix=/usr
make
sudo make install
cd ..

# Upgrade bison
wget http://ftp.gnu.org/gnu/bison/bison-2.5.1.tar.gz
tar xvf bison-2.5.1.tar.gz
cd bison-2.5.1
./configure --prefix=/usr
make
sudo make install
cd ..

# Install C++ Lib Dependencies
sudo yum -y install libevent-devel zlib-devel openssl-devel

# Upgrade Boost
wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz
tar xvf boost_1_55_0.tar.gz
cd boost_1_55_0
./bootstrap.sh
sudo ./b2 install

# Build and Install the Apache Thrift IDL Compiler
git clone https://git-wip-us.apache.org/repos/asf/thrift.git
cd thrift
./bootstrap.sh
./configure --with-lua=no
make
sudo make install
