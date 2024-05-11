#include <iostream>
#include <ctime>
#include <cstdlib>
#include <map>
#include "BPlusTree.h" // 假设B+树实现在BPlusTree.h文件中

using namespace std;

// 生成随机数函数
int generateRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

int main() {
    const int MAX_KEY = 10000; // 最大键值范围
    const int NUM_OPERATIONS = 10000; // 操作数量

    // 初始化随机数种子
    srand(time(0));

    // 创建B+树对象
    BPlusTree<int, int> bPlusTree(3,10);

    // 记录操作开始时间
    clock_t startTime, endTime;

    // 插入操作
    startTime = clock();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int key = generateRandomNumber(1, MAX_KEY);
        bPlusTree.insert(key, key); // 插入键值对
    }
    endTime = clock();
    cout << "Insertion time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    // 搜索操作
    startTime = clock();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int key = generateRandomNumber(1, MAX_KEY);
        bPlusTree.search(key); // 搜索键值对
    }
    endTime = clock();
    cout << "Search time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    // 删除操作
    startTime = clock();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int key = generateRandomNumber(1, MAX_KEY);
        bPlusTree.remove(key); // 删除键值对
    }
    endTime = clock();
    cout << "Deletion time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    return 0;
}
