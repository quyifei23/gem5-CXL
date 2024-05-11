#pragma once
#include <queue>
#include "BPTNode.h"

int generateRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

template<class T>
class BPlusTree {
public:
	BPlusTree(int order = 3);//���캯��
	bool insert(T data);//���뺯��
	bool myDelete(T data);//ɾ������
	bool find(T data);//�������ݣ������Ƿ��ҵ�
	bool setOrder(int n);//�趨����
	bool isEmpty();//����B+���Ƿ�Ϊ��
	void printData();//��ӡ������������
	void printTree();//�����Ľṹ��ӡ������
private:
	BPTNode<T>* Root;
	int order;//�������������������,�ؼ������������1��Ĭ��Ϊ3
	int count;//����Ԫ����������ʼΪ0
	BPTNode<T>* getHead();//����Ҷ�ڵ�ĵ�һ���ڵ�
	BPTNode<T>* findLoc(T data);//���Һ�����������������λ�û�Ӧ�ò���λ��
};

template<class T>
inline BPlusTree<T>::BPlusTree(int order)//���ݽ�����ʼ��B+��
{
	this->order = order;
	count = 0;
	Root = new BPTNode<T>;
	Root->father = nullptr;
	Root->setorder(order);
}

template<class T>
bool BPlusTree<T>::insert(T data)//�ҵ�Ӧ�ò������ݵĽڵ㲢�������ݣ��������Ѿ����������ʧ��
{
	BPTNode<T>* tmp = findLoc(data);
	bool flag = tmp->insert(data,Root);
	if (flag) count++;
	// if (!flag)std::cout << "����ʧ�ܣ�����" << data << "�����ڣ�" << endl;
	return flag;
}

template<class T>
bool BPlusTree<T>::myDelete(T data)//�ҵ�Ӧ��ɾ�����ݵĽڵ㲢ɾ�����ݣ������ݲ�������ɾ��ʧ��
{
	BPTNode<T>* tmp = findLoc(data);
	bool flag = tmp->myDelete(data,Root);
	if (flag) count--;
	// if (!flag)std::cout << "ɾ��ʧ�ܣ��Ҳ�������" << data << "!" << endl;
	return flag;
}

template<class T>
inline bool BPlusTree<T>::find(T data)//�����в���ָ�����ݣ����ز��ҽ��
{
	BPTNode<T>* tmp = findLoc(data);
	return tmp->find(data);
}

template<class T>
bool BPlusTree<T>::setOrder(int n)//�����趨������ֻ������Ϊ����ʱ���Խ��д˲���
{
	if (!isEmpty())
		return false;
	order = n;
	return true;
}

template<class T>
bool BPlusTree<T>::isEmpty()//�������Ƿ�Ϊ��
{
	if (count)
		return false;
	return true;
}

template<class T>
inline void BPlusTree<T>::printData()//���������������
{
	if (Root->isEmpty()) {
		cout << "Tree is empty!\n";
		return;
	}
	BPTNode<T>* tmp = getHead();
	while (tmp && !tmp->isEmpty()) {
		int len = tmp->key.size();
		std::cout << "(";
		for (int i = 0; i < len; i++) {
			std::cout << tmp->key[i].data << " ";
			if (i == len - 1)
				tmp = tmp->next;	
		}
		std::cout << "\b)->";
	}
	std::cout << "\b\b";
	std::cout << "  " << endl;
}

template<class T>
inline void BPlusTree<T>::printTree()//����������Ľṹ
{
	if (Root->isEmpty()) {
		cout << "Tree is empty!\n";
		return;
	}
	queue<BPTNode<T>*> qu;
	qu.push(Root);
	while (!qu.empty()) {
		int len = qu.size();
		for (int i = 0; i < len; i++) {
			BPTNode<T>* tmp = qu.front();
			qu.pop();
			for (int j = 0; j < tmp->key.size(); j++) {
				if (tmp->key[j].left)
					qu.push(tmp->key[j].left);
				if (j == tmp->key.size() - 1 && tmp->key[j].right)
					qu.push(tmp->key[j].right);
			}
			std::cout << "(";
			for (int j = 0; j < tmp->key.size(); j++) {
				std::cout << tmp->key[j].data << " ";
			}
			std::cout << "\b)->";
		}
		std::cout << "\b\b  " << endl;
	}
}

template<class T>
inline BPTNode<T>* BPlusTree<T>::getHead()//�ҵ�������С���������ڽ��
{
	BPTNode<T>* tmp = Root;
	while (tmp->key.size() > 0 && tmp->key[0].left)
		tmp = tmp->key[0].left;
	return tmp;
}

template<class T>
BPTNode<T>* BPlusTree<T>::findLoc(T data)//�ҵ�ָ������Ӧ�ò���Ľ��λ��
{
	if (count == 0) return Root;
	BPTNode<T>* tmp = Root;
	if (Root->key[0].left == nullptr) return tmp;
	while (tmp->key[0].left != nullptr) {
		for (int i = 0; i < tmp->key.size(); i++) {
			if (data < tmp->key[i].data) {
				tmp = tmp->key[i].left;
				break;
			}
			if (i == tmp->key.size() - 1) {
				tmp = tmp->key[i].right;
				break;
			}
		}
	}
	return tmp;
}


