# TAO

## Table of Contents

- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Compilation](#compilation)
- [Test](#Test)
- [Usage](#usage)
- [Contact](#contact)

## Introduction

This a a a real-time nentwork traffic analysis task orchestration framework.

## Prerequisites

Before you begin, ensure you have met the following requirements:

- Operating System: Linux
- Software: DPDK

## Compilation

To compile the project, navigate to the `node` directory and run the following commands:

On Node

```bash
cd node
sudo make
```

On Client

```bash
sh test.sh test1 TEST6
```

## Test

```bash
cd node/task_examples
sh make_task_custom.sh
```

## Usage

On Node:

```bash
cd node/build
./node
```

On Client

```bash
./test1
```

## Contact

If you have any questions, feel free to reach out:

Huaijie Jiang  
<230219078@seu.edu.cn>
