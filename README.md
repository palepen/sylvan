# Sylvan

An x86-64 debugger

## Building from scratch

### Prerequisites

- Linux System (not sure if any linux specific things are used that aren't posix compliant)
- GCC compiler (GNU extensions are used)
- make

### Setup and Execution

#### Build

```make```

#### Run

```make run``` 
    
or

```./sylvan```

#### Clean

```make clean```

## TODO

- ~~attach to a process~~
- ~~run a process~~
- ~~single step~~
- ~~regs~~
- breakpoints
- step
- software watchpoints
- hardware watchpoints
- get the errno of child if exec fails to set correct status and error (eg if a file isn't an executable, currently it says program has already exited when exec fails)
