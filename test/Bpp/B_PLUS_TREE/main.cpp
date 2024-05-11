#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <map>
#include "BPT.h"

int main() {
    const int MAX_KEY = 1000; // 最大键值范围
    const int NUM_OPERATIONS = 1000; // 操作数量

    // 初始化随机数种子
    srand(time(0));

    // 创建B+树对象
    BPlusTree<int> bPlusTree(10);

    // 记录操作开始时间
    clock_t startTime, endTime;

    // 插入操作
    startTime = clock();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int key = generateRandomNumber(1, MAX_KEY);
        bPlusTree.insert(key); // 插入键值对
    }
    endTime = clock();
    cout << "Insertion time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    // 搜索操作
    startTime = clock();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int key = generateRandomNumber(1, MAX_KEY);
        bPlusTree.find(key); // 搜索键值对
    }
    endTime = clock();
    cout << "Search time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    // // 删除操作
    // startTime = clock();
    // for (int i = 0; i < NUM_OPERATIONS; ++i) {
    //     int key = generateRandomNumber(1, MAX_KEY);
    //     bPlusTree.myDelete(key); // 删除键值对
    // }
    // endTime = clock();
    // cout << "Deletion time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    return 0;
}