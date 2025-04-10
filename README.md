# Sylvan

An x86-64 debugger

## Building from scratch

### Prerequisites

- Linux System (not sure if any linux specific things are used that aren't posix compliant)
- GCC compiler (GNU extensions are used)
- make

### Dependencies Installation Guide

This project uses [Zydis](https://github.com/zyantific/zydis) and [Zycore](https://github.com/zyantific/zycore-c) for disassembly and utility functionality. Below are the instructions to set up these dependencies on common Linux distributions.

---

#### Fedora

You can install all required dependencies directly from the official Fedora repositories:

```bash
sudo dnf install \
    zydis-devel \
    zycore-c-devel \
    elfutils-libelf-devel \
    libdwarf-devel 
```

### Ubuntu

#### Step 1: Install Required Tools and Libraries

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libelf-dev \
    libdwarf-dev
```

#### Step 2: Build and Install Zycore

```
git clone https://github.com/zyantific/zycore-c.git
cd zycore-c
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

#### Step 3: Build and Install Zydis

```
git clone https://github.com/zyantific/zydis.git
cd zydis
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

#### Step 4: Update the Linker Cache

```
sudo ldconfig
```

### Setup and Execution

#### Build

```make```

#### Run

```make run``` 
    
or

```./build/sylvan```

#### Clean

```make clean```

## Example Images

### Normal Run
![Normal Run](<images/Normal Run.png>)

### Help
![help](<images/help.png>)

### Breakpoints
![Breakpoints](<images/breakpoints.png>)

### Disassemble
![disassemble](<images/disassemble.png>)

### Memory Write
![memory_write](<images/memory_write.png>)

### Register Show
![registers](<images/registers.png>)

### Register Update
![register change](<images/register_set.png>)
![register change](<images/output change regsiter set.png>)

