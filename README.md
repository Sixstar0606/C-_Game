# GrowXYZ
GrowXYZ Private Server -> [GrowXYZ Discord](https://discord.gg/5BRwQVwvRq).

> This is official GrowXYZ Source, this is designed to build&run on **Linux** server.

## Install Required Packages
```sh
sudo apt-get install -y cmake
sudo apt-get install -y git
sudo apt-get install -y python3
sudo apt-get install -y python3-pip
sudo apt-get install -y mysql-server
sudo apt-get install -y xvfb
```
**Install Conan Package Manager**
```py
pip install conan
```

## Configuration for Git (Private Repository)
```git
git config --global user.email "EMAIL"
git config --global user.name "NAME"
git config --global user.pass "PASS"
```

## SFML Dependencies for WorldRender
```sh
sudo apt-get install -y libx11-xcb-dev
sudo apt-get install -y libxcb-render0-dev
sudo apt-get install -y libxcb-render-util0-dev
sudo apt-get install -y libxcb-xkb-dev
sudo apt-get install -y libxcb-icccm4-dev
sudo apt-get install -y libxcb-image0-dev
sudo apt-get install -y libxcb-keysyms1-dev
sudo apt-get install -y libxcb-randr0-dev
sudo apt-get install -y libxcb-shape0-dev
sudo apt-get install -y libxcb-sync-dev
sudo apt-get install -y libxcb-xfixes0-dev
sudo apt-get install -y libxcb-xinerama0-dev
sudo apt-get install -y libxcb-dri3-dev
sudo apt-get install -y libxcb-util-dev
sudo apt-get install -y libxcb-util0-dev
```

## Setup MySQL Server
```sql
sudo systemctl start mysql.service
sudo mysql -u root -p
sudo mysql

mysql > ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'your_password';
mysql > exit
```

## Setup Display
```
Xvfb :0 -screen 0 3200x1920x24& 
export DISPLAY=:0
```

## Running The GrowXYZ Source
> Server's Configuration file is on **GrowXYZ/config.h**
```git
git clone RebillionXX/GrowXYZ.git
cd ./GrowXYZ
sudo bash ./build.sh
```
> It should be displayed as **Linking CXX executable server** on your terminal if its succeed

> After that you could run your server with following commands
```sh
cd ./build/src
xvfb-run ./server
```

Enjoy ;-) / **Rebillion#1234**
#   C - _ G a m e  
 