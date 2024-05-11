#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

// B+树节点结构
template <typename KeyType, typename ValueType>
struct Node {
    vector<KeyType> keys; // 键
    vector<Node*> children; // 子节点指针
    bool isLeaf; // 是否为叶子节点
    Node* next; // 指向下一个叶子节点的指针（用于叶子节点之间的连接）
    Node* parent; // 父节点指针
};

// B+树类
template <typename KeyType, typename ValueType>
class BPlusTree {
private:
    Node<KeyType, ValueType>* root; // 根节点指针
    int order; // B+树的阶数
    int leafSize; // 叶子节点的最大键数量

public:
    BPlusTree(int order, int leafSize) : order(order), leafSize(leafSize) {
        root = nullptr;
    }

    // 插入操作
    void insert(KeyType key, ValueType value) {
        if (!root) {
            root = new Node<KeyType, ValueType>;
            root->isLeaf = true;
            root->keys.push_back(key);
            root->children.push_back(nullptr);
            root->parent = nullptr;
            return;
        }

        // 在树中查找合适的叶子节点进行插入
        Node<KeyType, ValueType>* leaf = findLeaf(key);

        // 插入键值对
        insertIntoLeaf(leaf, key, value);

        // 分裂叶子节点
        if (leaf->keys.size() > leafSize) {
            splitLeaf(leaf);
        }
    }

    // 搜索操作
    ValueType search(KeyType key) {
        if (!root) {
            cout << "B+ tree is empty" << endl;
            return ValueType();
        }

        Node<KeyType, ValueType>* leaf = findLeaf(key);

        // 在叶子节点中查找键值
        auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it != leaf->keys.end() && *it == key) {
            cout << "Found key: " << key << ", value: " << leaf->children[it - leaf->keys.begin()] << endl;
        } else {
            cout << "Key " << key << " not found" << endl;
        }
    }

    // 删除操作
    void remove(KeyType key) {
        if (!root) {
            cout << "B+ tree is empty" << endl;
            return;
        }

        Node<KeyType, ValueType>* leaf = findLeaf(key);

        // 在叶子节点中查找并删除键值对
        auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it != leaf->keys.end() && *it == key) {
            leaf->keys.erase(it);
            leaf->children.erase(leaf->children.begin() + (it - leaf->keys.begin()));
            cout << "Key " << key << " removed successfully" << endl;
        } else {
            cout << "Key " << key << " not found" << endl;
        }
    }

private:
    // 在树中查找合适的叶子节点进行插入
    Node<KeyType, ValueType>* findLeaf(KeyType key) {
        Node<KeyType, ValueType>* node = root;
        while (!node->isLeaf) {
            int index = lower_bound(node->keys.begin(), node->keys.end(), key) - node->keys.begin();
            node = node->children[index];
        }
        return node;
    }

    // 插入键值对到叶子节点
    // 在叶子节点中插入键值对
    void insertIntoLeaf(Node<KeyType, ValueType>* leaf, KeyType key, ValueType value) {
        auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        leaf->keys.insert(it, key);
        leaf->children.insert(leaf->children.begin() + (it - leaf->keys.begin()), nullptr); // 插入空指针
        // 如果children存储的是指向叶子节点数据的指针，这里的插入需要修改为指向数据的指针
    }

    // 分裂叶子节点
    void splitLeaf(Node<KeyType, ValueType>* leaf) {
        Node<KeyType, ValueType>* newLeaf = new Node<KeyType, ValueType>;
        newLeaf->isLeaf = true;
        newLeaf->parent = leaf->parent;

        int splitIndex = leaf->keys.size() / 2;
        newLeaf->keys.assign(leaf->keys.begin() + splitIndex, leaf->keys.end());
        newLeaf->children.assign(leaf->children.begin() + splitIndex, leaf->children.end());
        leaf->keys.erase(leaf->keys.begin() + splitIndex, leaf->keys.end());
        leaf->children.erase(leaf->children.begin() + splitIndex, leaf->children.end());

        // 更新链表连接关系
        newLeaf->next = leaf->next;
        leaf->next = newLeaf;

        // 将中间键值插入到父节点中
        insertIntoParent(leaf, newLeaf->keys[0], newLeaf);
    }

    // 插入分裂后的叶子节点到父节点中
    void insertIntoParent(Node<KeyType, ValueType>* leftChild, KeyType key, Node<KeyType, ValueType>* rightChild) {
        Node<KeyType, ValueType>* parent = leftChild->parent;

        // 如果父节点为空，表示当前节点为根节点
        if (!parent) {
            parent = new Node<KeyType, ValueType>;
            parent->isLeaf = false;
            parent->keys.push_back(key);
            parent->children.push_back(leftChild);
            parent->children.push_back(rightChild);
            leftChild->parent = parent;
            rightChild->parent = parent;
            root = parent;
            return;
        }

        // 查找键值在父节点中的插入位置
        auto it = lower_bound(parent->keys.begin(), parent->keys.end(), key);
        parent->keys.insert(it, key);
        int index = it - parent->keys.begin();

        // 插入子节点指针
        parent->children.insert(parent->children.begin() + index + 1, rightChild);
        rightChild->parent = parent;

        // 分裂父节点
        if (parent->keys.size() > order) {
            splitNode(parent);
        }
    }

    // 分裂父节点
    void splitNode(Node<KeyType, ValueType>* node) {
        Node<KeyType, ValueType>* newNode = new Node<KeyType, ValueType>;
        newNode->isLeaf = false;
        newNode->parent = node->parent;

        int splitIndex = node->keys.size() / 2;
        KeyType middleKey = node->keys[splitIndex];
        newNode->keys.assign(node->keys.begin() + splitIndex + 1, node->keys.end());
        newNode->children.assign(node->children.begin() + splitIndex + 1, node->children.end());
        node->keys.erase(node->keys.begin() + splitIndex, node->keys.end());
        node->children.erase(node->children.begin() + splitIndex + 1, node->children.end());

        // 插入中间键值到父节点中
        insertIntoParent(node, middleKey, newNode);
    }
};
