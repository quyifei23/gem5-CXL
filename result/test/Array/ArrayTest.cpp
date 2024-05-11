#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>

using namespace std;

// 生成随机数函数
int generateRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

int main() {
    const int ARRAY_SIZE = 1000000; // 数组大小
    const int NUM_OPERATIONS = 10000; // 操作数量

    // 初始化随机数种子
    srand(time(0));

    // 创建随机数组
    vector<int> randomArray(ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        randomArray[i] = generateRandomNumber(1, 1000); // 随机生成数组元素
    }

    // 记录操作开始时间
    clock_t startTime, endTime;

    // 随机访问操作
    startTime = clock();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int index = generateRandomNumber(0, ARRAY_SIZE - 1);
        int value = randomArray[index]; // 随机访问数组元素
    }
    endTime = clock();
    cout << "Random access time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    // 随机修改操作
    startTime = clock();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int index = generateRandomNumber(0, ARRAY_SIZE - 1);
        randomArray[index] = generateRandomNumber(1, 1000); // 随机修改数组元素
    }
    endTime = clock();
    cout << "Random modification time: " << double(endTime - startTime) / CLOCKS_PER_SEC << " seconds" << endl;

    return 0;
}
