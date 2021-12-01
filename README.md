## Project 1: KVStore using Log-structured Merge Tree


The handout files include two main parts:

- The `KVStoreAPI` class in `kvstore_api.h` that specifies the interface of KVStore.
- Test files including correctness test (`correctness.cc`) and persistence test (`persistence.cc`).

Explanation of each handout file:

```text
.
├── Makefile  // Makefile if you use GNU Make
├── README.md // This readme file
├── correctness.cc // Correctness test, you should not modify this file
├── data      // Data directory used in our test
├── kvstore.cc     // your implementation
├── kvstore.h      // your implementation
├── kvstore_api.h  // KVStoreAPI, you should not modify this file
├── persistence.cc // Persistence test, you should not modify this file
├── utils.h         // Provides some cross-platform file/directory interface
├── MurmurHash3.h  // Provides murmur3 hash function
└── test.h         // Base class for testing, you should not modify this file
```


First have a look at the `kvstore_api.h` file to check functions you need to implement. Then modify the `kvstore.cc` and `kvstore.h` files and feel free to add new class files.

We will use all files with `.cc`, `.cpp`, `.cxx` suffixes to build correctness and persistence tests. Thus, you can use any IDE to finish this project as long as you ensure that all C++ source files are submitted.

For the test files, of course you could modify it to debug your programs. But remember to change it back when you are testing.

Good luck :)

# KVStore-using-Log-structured-Merge-Tree

# LSM Tree Report

# 1 背景介绍

## 1.1 LSM 树

LSM 树，即日志结构合并树 (Log-Structured Merge-Tree)。是一种可以高性能执行大量写操作的数据结构。支持增、删、读、改、顺序扫描操作。而且通过批量存储技术规避磁盘随机写入问题。LSM 树和 B+ 树相比，LSM 树牺牲了部分读性能，用来大幅提高写性能。

### 1.1.1 设计思想

LSM 树的设计思想非常朴素:将对数据的修改增量保持在内存中，达到指定的大小限制后将这些修改操作批量写入磁盘，不过读取的时候稍微麻烦，需要合并磁盘中历史数据和内存中最近修改操作，所以写入性能大大提升，读取时可能需要先看是否命中内存，否则需要访问较多的磁盘文件。

### 1.1.2 基本结构

LSM Tree 键值存储系统分为内存存储和硬盘存储两部分，采用不同的存储方式 (如图 1 所示)

![图 1: LSM-Tree 键值存储系统结构](README.assets/Untitled.png)

图 1: LSM-Tree 键值存储系统结构

内存存储 本次实验选择使用跳表，支持增删改查。值得注意的是:其中的插入操作需要覆盖原先值。删除操作需要查看内存和当前跳表，即使存在也需要插入一个特殊字符串” DELETED ”。硬盘存储 当内存中 MemTable 数据达到阈值 (即转换成 SSTable 后大小超过 2MB) 时，要将 MemTable 中的数据写入硬盘。

# 2 挑战

## 2.1 困难

### 2.1.1 对 merge 的理解

merge 操作情况很多。对于第一层和最后一层以及中间层需要不同的处
理。对其思路整理画出流程图如下:

<img src="README.assets/Untitled 1.png" alt="图 2: merge 相关流程" style="zoom:67%;" />

图 2: merge 相关流程

内存泄露 发现内存不断扩大，存在内存泄露。于是使用了 xcode 的内存泄露检测工具 Leak。找到相关函数，查找资料发现 vector 的 clear 不能完全清空内存，需要用 swap 函数清空。然而还是没有解决问题，最后发现在 skiplist 中使用了之前已经释放的指针。通过这个了解了管理内存的重要性以及学会了检测内存泄露。如图是 Leak 使用图。

<img src="README.assets/Untitled 2.png" alt="图3: Leak工具" style="zoom:33%;" />

图3: Leak工具

可以查看内存泄露的函数

<img src="README.assets/Untitled 3.png" alt="图 4: 内存泄露的函数" style="zoom:33%;" />

图 4: 内存泄露的函数

# 3 测试

## 3.1 性能测试

### 3.1.1 预期结果

1. 三种操作的吞吐量和时延应为倒数关系。删除操作的流程为先查找是否存在。之后再 PUT 一个删除的标志。可能会触发合并，如果每次插入的数据比较小，那么均摊到每次删除的操作的附加值较小，又因为DEL 插入的字符串很小，只有 9 个字节，很少会触发合并，因此字符串很长的 PUT 操作时延会显著高于 DEL。而字符串小的时候，DEl比 PUT 时延高。
2. 通过 BloomFliter 直接判断中是否有查询的 key，这个操作是 O(1) 的。
   可以避免无用的查找，因此会提高很多 GET 性能。
3. 如果缓存了索引，那每次读取数据的时候就不用先读索引，而是在内
   存中找。然后直接定位到相应的位置。磁盘的速度比内存慢很多个数
   量级，在内存中缓存 index 可以显著地加快查找的速度。

### 3.1.2 常规分析

<img src="README.assets/Untitled 4.png" alt="图 5: 各个操作的延迟，单位:ms" style="zoom:50%;" />

图 5: 各个操作的延迟，单位:ms

【参数】:每次的数据大小如表格所示，i 从 0-max, 每次插入长度为 i 的数据。

【讨论】:可以看见在数据规模较小的时候，没有进行合并操作，DEL 操作的时间比 PUT 高一些，当数据量足够大后，PUT 操作的平均时延比 DEL低了，这符合上述的讨论。

### 3.1.3 索引缓存与 Bloom Filter 的效果测试

需要对比下面三种情况 GET 操作的平均时延

1. 内存中没有缓存 SSTable 的任何信息，从磁盘中访问 SSTable 的索引，
   在找到 offset 之后读取数据

2. 内存中只缓存了 SSTable 的索引信息，通过二分查找从 SSTable 的索
   引中找到 offset，并在磁盘中读取对应的值

<img src="README.assets/Untitled 5.png" alt="Untitled" style="zoom:33%;" />

图 6: 没有缓存

<img src="README.assets/Untitled 6.png" alt="Untitled" style="zoom:33%;" />

图 7: 部分缓存

3. 内存中缓存 SSTable 的 Bloom Filter 和索引，先通过 Bloom Filter
   判断一个键值是否可能在一个 SSTable 中，如果存在再利用二分查找，
   否则直接查看下一个 SSTable 的索引。

<img src="README.assets/Untitled 7.png" alt="Untitled" style="zoom:33%;" />

图 8: 全部缓存
从图中可以看出，全部缓存的效率 > 部分缓存 > 无缓存。

<img src="README.assets/Untitled 8.png" alt="Untitled" style="zoom:50%;" />

图 9: 缓存效果测试

### 3.1.4 Compaction 的影响

不断插入数据的情况下，统计每秒钟处理的 PUT 请求个数(即吞吐
量)，并绘制其随时间变化的折线图，测试需要表现出 compaction 对吞吐
量的影响。插入 10240 个 key=1,value 等于 std::string(1024,’s’) 的数据，测
试的如图。从图中可以看出，合并的次数越多，层数约大，最终的时延迟约
多。出现了两个峰，应该是在 put 过程中生成了 sstable，合并过程中没有
put 操作。

<img src="README.assets/Untitled 9.png" alt="Untitled" style="zoom:33%;" />

图 10: GET 操作吞吐量随时间变化

# 4 结论

通过对比分析，可以看到缓存索引，bloom 过滤器对于速度提升有重要
作用。了解了 LSM 树这种可以高性能执行大量写操作的数据结构，同时对
于局部性有了一定的认识。将对数据的修改增量保持在内存中，达到指定的
大小限制后将这些修改操作批量写入磁盘。这种 cache 的思想也值得借鉴。
之后在代码时，需要注意如何提升性能。

### 4.1 两个测试的结果

<img src="README.assets/Untitled 10.png" alt="Untitled" style="zoom:33%;" />

图 11: CorrectnessTest

<img src="README.assets/Untitled 11.png" alt="Untitled" style="zoom:33%;" />

图 12: PersistenceTest
