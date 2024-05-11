#pragma once
#include <vector>
#include <algorithm>
using namespace std;

template <class T>
class BPTNode
{
public:
	template<class T2>//�ؼ��ֽ����
	class keyNode
	{
	public:
		keyNode() {};
		keyNode(const T data) :data(data) {};//���캯��
		keyNode(const T data, BPTNode<T>* left, BPTNode<T>* right) :data(data), left(left), right(right) {};
		T data;//�ؼ�������
		BPTNode<T>* left = nullptr;//���ӣ�Ҷ�ڵ�����Ĭ��Ϊ��
		BPTNode<T>* right = nullptr;//�Һ��ӣ�Ҷ�ڵ��Һ���Ĭ��Ϊ��
	};
	vector<keyNode<T>> key = {};
	BPTNode<T>* father = nullptr;
	BPTNode<T>* next = nullptr;
	BPTNode();//Ĭ�Ϲ��캯����Ĭ�ϳ�ʼ��ΪֵΪ0��Ҷ���
	BPTNode(const T data, int order = 3);//���ݵ�һ���ݺͽ�����ʼ��B+�����
	BPTNode(vector<T>& key, int order = 3);//������������ͽ�����ʼ��B+�����
	bool find(const T keyValue);//�ڵ�ǰ�����Ҷ�Ӧ�����Ƿ����
	bool insert(const T keyValue, BPTNode<T>*& Root);//Ҷ���ר�ã������ֵ
	bool myDelete(const T keyValue, BPTNode<T>*& Root);//Ҷ���ר�ã�ɾ����ֵ
	bool isEmpty();//����Ƿ�Ϊ��
	bool setorder(int order);//�޸Ľ�����ֻ���޸ĺ��ֵ����С�ڽ����ſ����޸�
private:
	int order = 3;
	bool insert(keyNode<T>* newKey, BPTNode<T>*& Root);//�ڲ����ר�ã������ֵ
	bool myDelete(BPTNode<T>* now, BPTNode<T>*& Root);//�ڲ����ר�ã�ɾ����ֵ
	BPTNode<T>* getLeftBro(BPTNode<T>* now,BPTNode<T>* Root);//�ҵ���ǰ�������ֵܽ��
	BPTNode<T>* getHead(BPTNode<T>* Root);//�ҵ���ǰ�������������С���
};

template<class T>
inline BPTNode<T>::BPTNode() {

}

template<class T>
inline BPTNode<T>::BPTNode(const T data, int order)
{
	key = {};
	this->order = order;
	keyNode<T> tmp(data);
	key.push_back(tmp);
}

template<class T>
inline BPTNode<T>::BPTNode(vector<T>& key,int order)//�½��ڲ����
{
	this->order = order;
	sort(key.begin(), key.end());
	for (auto p : key) {
		keyNode<T> tmp(p);
		this->key.push_back(tmp);
	}
}

template<class T>
inline bool BPTNode<T>::find(const T keyValue)
{
	for (auto& p : key)
		if (p.data == keyValue)
			return true;
	return false;
}

template<class T>
inline bool BPTNode<T>::insert(const T keyValue, BPTNode<T>*& Root)//Ҷ�ڵ��������
{
	if (find(keyValue)) return false;//���������Ѿ����ڸ����ݣ�����ʧ��
	if (key.size() == 0)//���Ϊ��ֱ�Ӳ���
		key.push_back(keyNode<T>(keyValue));
	else if (keyValue > (*key.rbegin()).data)//�������ݱ������������ֵ������ֱ�Ӳ������λ��
		key.push_back(keyNode<T>(keyValue));
	else {//�ҵ�Ӧ�ò���λ�ò���������
		int len = key.size();
		int i = 0;
		for (i = 0; i < len; i++)
			if (keyValue < key[i].data) break;
		key.emplace(key.begin() + i, keyNode<T>(keyValue));
	}
	if (key.size() < order) {//�ռ����,������Է��϶���
		return true;
	}
	else {//Ҷ�ڵ�ռ䲻�����������ѳ�����Ҷ�ڵ㣬�����м�ļ�ֵ��λ���������
		int m = order / 2;
		keyNode<T>* newKey = new keyNode<T>(key[m]);//�����м�λ�õļ�ֵ
		BPTNode<T>* newBPTNode = new BPTNode<T>;//�����µ�Ҷ���
		newBPTNode->father = father;
		newBPTNode->setorder(order);
		newKey->left = this;
		newKey->right = newBPTNode;
		newBPTNode->next = this->next;
		this->next = newBPTNode;
		for (int i = m; i < key.size(); i++) {//��һ��ļ�ֵ���Ƶ��µ�Ҷ�ڵ���
			keyNode<T>* tmp = new keyNode<T>;
			tmp->data = key[i].data;
			tmp->left = key[i].left;
			tmp->right = key[i].right;
			newBPTNode->key.push_back(*tmp);
		}
		key.resize(m);//ɾ����ǰҶ�ڵ��벿�ּ�ֵ
		if (father) {//�����ڵ��������м�λ�õļ�ֵ���븸���
			father->insert(newKey, Root);
			return true;
		}
		else {//�����ڵ㲻��������һ���µ��ڲ��ڵ���Ϊ���ڵ�
			BPTNode<T>* tmp = new BPTNode<T>;//�����µ�Ҷ���
			tmp->setorder(order);
			this->father = tmp;
			newBPTNode->father = tmp;
			tmp->key.push_back(*newKey);
			Root = tmp;
			return true;
		}
	}
	return true;
}

template<class T>
inline bool BPTNode<T>::myDelete(const T keyValue, BPTNode<T>*& Root)//�����ɾ��һ����ֵ
{
	if (!find(keyValue))//�Ҳ���Ҫɾ���ļ�ֵ
		return false;
	for (int i = 0; i < key.size(); i++) {
		if (key[i].data == keyValue) {
			key.erase(key.begin() + i);
			break;
		}
	}
	int m = (order + 1) / 2 - 1;
	if (father == nullptr) return true;//����Ǹ���㣬ֱ��ɾ������
	if (key.size() >= m) {//ɾ���������㶨�壬ֱ��ɾ�����������ڵ��ж�Ӧ�ؼ��ּ���
		for (auto& p : father->key) {
			if (p.right == this) {
				p.data = key[0].data;
				break;
			}
		}
		return true;
	}
	else {//����ֵܽ���и�ԣ������ֵܽ���һ����ͬʱ�Ѹ��ڵ��ж�Ӧ�ؼ��ָ���Ϊ��Ĺؼ���
		BPTNode<T>* leftBro = nullptr;
		BPTNode<T>* rightBro = nullptr;
		int leftLoc = 0, rightLoc = 0;
		for (int i = 0; i < father->key.size(); i++) {
			if (father->key[i].left == this) {
				rightBro = father->key[i].right;
				rightLoc = i;
			}	
			if (father->key[i].right == this) {
				leftBro = father->key[i].left;
				leftLoc = i;
			}	
		}
		if (leftBro != nullptr && leftBro->key.size() > m) {//���ֵܽ�㹻��
			keyNode<T> newKey = leftBro->key[leftBro->key.size() - 1];
			key.emplace(key.begin(), newKey);
			leftBro->key.erase(leftBro->key.end() - 1);
			father->key[leftLoc].data = key[0].data;//���¸��ڵ��ж�Ӧ�ؼ���
			return true;
		}
		if (rightBro != nullptr && rightBro->key.size() > m) {//���ֵܽ�㹻��
			keyNode<T> newKey = rightBro->key[0];
			key.push_back(newKey);
			rightBro->key.erase(rightBro->key.begin());
			father->key[rightLoc].data = rightBro->key[0].data;//���¸��ڵ��ж�Ӧ�ؼ���
			return true;
		}
		
		if (leftBro != nullptr) {//��������ֵܶ������������ֵܴ��ڣ�������ֵܺϲ�����ɾ�����ڵ��ж�Ӧ�ؼ���
			for (int i = 0; i < leftBro->key.size(); i++) {//���ֵ����ݸ��Ƶ���ǰ���ǰ��
				key.emplace(key.begin() + i, leftBro->key[i]);
			}
			BPTNode<T>* cousin = getLeftBro(leftBro,Root);
			if (cousin != nullptr) cousin->next = this;
			if (leftLoc > 0)
				father->key[leftLoc - 1].right = this;
			father->key.erase(father->key.begin() + leftLoc);//ɾ�����ڵ��ж�Ӧ�ؼ���
			delete leftBro;
			return this->myDelete(father, Root);
		}
		else if (rightBro != nullptr) {//�����ֵܺϲ�����ɾ�����ڵ��ж�Ӧ�ؼ���
			for (int i = 0; i < rightBro->key.size(); i++) {//���ֵ����ݸ��Ƶ���ǰ���ĩβ
				key.push_back(rightBro->key[i]);
			}
			next = rightBro->next;
			if (rightLoc + 1 < father->key.size())
				father->key[rightLoc + 1].left = this;
			father->key.erase(father->key.begin() + rightLoc);//ɾ�����ڵ��ж�Ӧ�ؼ���
			delete rightBro;
			return this->myDelete(father, Root);
		}
	}
	return false;
}

template<class T>
inline bool BPTNode<T>::isEmpty()
{
	if (key.size() == 0) return true;
	return false;
}

template<class T>
inline bool BPTNode<T>::setorder(int order)
{
	if (key.size() >= order)
		return false;
	this->order = order;
	return true;
}

template<class T>
inline bool BPTNode<T>::insert(keyNode<T>* newKey, BPTNode<T>*& Root)//�ڲ�����������
{
	if (newKey->data > (*key.rbegin()).data)//�������ݱ������������ֵ������ֱ�Ӳ������λ��
		key.push_back(*newKey);
	else {//�ҵ�Ӧ�ò���λ�ò���������
		int len = key.size();
		int i = 0;
		for (i = 0; i < len; i++)
			if (newKey->data < key[i].data) break;
		if (i != 0)	key[i - 1].right = newKey->left;
		key[i].left = newKey->right;
		key.emplace(key.begin() + i, *newKey);
	}
	if (key.size() < order) {//�ռ����,������Է��϶���
		return true;
	}
	else {//�ڲ��ڵ�ռ䲻�����������ѳ������ڲ��ڵ㣬�����м�ļ�ֵ��λ���������
		int m = (order - 1) / 2;
		keyNode<T>* newKey = new keyNode<T>(key[m]);//�����м�λ�õļ�ֵ
		BPTNode<T>* newBPTNode = new BPTNode<T>;//�����µ��ڲ����
		newBPTNode->father = father;
		newBPTNode->setorder(order);
		newKey->left = this;
		newKey->right = newBPTNode;
		key[m].right->father = newBPTNode;
		for (int i = m + 1; i < key.size(); i++) {//��һ��ļ�ֵ���Ƶ��µ�Ҷ�ڵ���
			if (key[i].right) key[i].right->father = newBPTNode;
			keyNode<T>* tmp = new keyNode<T>;
			tmp->data = key[i].data;
			tmp->left = key[i].left;
			tmp->right = key[i].right;
			newBPTNode->key.push_back(*tmp);
		}
		key.resize(m);//ɾ����ǰҶ�ڵ��벿�ּ�ֵ
		if (father) {//�����ڵ��������м�λ�õļ�ֵ���븸���
			father->insert(newKey, Root);
			return true;
		}
		else {//�����ڵ㲻��������һ���µ��ڲ��ڵ���Ϊ���ڵ�
			BPTNode<T>* tmp = new BPTNode<T>;//�����µ��ڲ����
			tmp->setorder(order);
			this->father = tmp;
			newBPTNode->father = tmp;
			tmp->key.push_back(*newKey);
			Root = tmp;
			return true;
		}
	}
	return false;
}

template<class T>
inline bool BPTNode<T>::myDelete(BPTNode<T>* now, BPTNode<T>*& Root)
{
	int m = (order + 1) / 2 - 1;
	if (Root == now) {//��ǰ����Ǹ���㣬����ǰ��㲻Ϊ��ֱ�ӷ��أ�Ϊ���޸ĸ��ڵ�󷵻�
		if (now->key.size() == 0) {
			this->father = nullptr;
			Root = this;
			delete now;
			return true;
		}
		return true;
	}
	if (now->key.size() >= m) return true;//��ǰ��㲻�Ǹ���㣬ɾ������϶�����ֱ�ӷ���
	BPTNode<T>* dad = now->father;
	BPTNode<T>* leftBro = nullptr;
	BPTNode<T>* rightBro = nullptr;
	int leftLoc = 0, rightLoc = 0;
	for (int i = 0; i < dad->key.size(); i++) {
		if (dad->key[i].right == now) {
			leftLoc = i;
			leftBro = dad->key[i].left;
		}	
		if (dad->key[i].left == now) {
			rightLoc = i;
			rightBro = dad->key[i].right;
		}		
	}
	if (leftBro != nullptr && leftBro->key.size() > m) {//������ֵܽ�㹻��
		keyNode<T> newKey(dad->key[leftLoc].data);
		newKey.left = leftBro->key[leftBro->key.size() - 1].right;
		if (now->key.size() != 0)
			newKey.right = now->key[0].left;
		now->key.push_back(newKey);
		dad->key[leftLoc].data = leftBro->key[leftBro->key.size() - 1].data;
		leftBro->key.erase(leftBro->key.end() - 1);
		return true;
	}
	if (rightBro != nullptr && rightBro->key.size() > m) {//������ֵܽ�㹻��
		keyNode<T> newKey(dad->key[rightLoc].data);
		newKey.right = rightBro->key[0].left;
		if (now->key.size() != 0)
			newKey.left = now->key[now->key.size() - 1].right;
		now->key.push_back(newKey);
		dad->key[rightLoc].data = rightBro->key[0].data;
		rightBro->key.erase(rightBro->key.begin());
		return true;
	}
	if (leftBro != nullptr) {//��������ֵܶ������������ֵܴ���
		BPTNode<T>* dad = now->father;
		keyNode<T> newKey;
		newKey.data = dad->key[leftLoc].data;
		newKey.left = leftBro->key[leftBro->key.size() - 1].right;
		if (now->key.size() != 0)
			newKey.right = now->key[0].left;
		dad->key.erase(dad->key.begin() + leftLoc);//ɾ�����ڵ��ж�Ӧ�ؼ���
		now->key.emplace(now->key.begin(), newKey);//������ж�Ӧ�ؼ������Ƶ���ǰ���
		for (int i = 0; i < leftBro->key.size(); i++) {//���ֵ����ݸ��Ƶ���ǰ���ǰ��
			now->key.emplace(now->key.begin() + i, leftBro->key[i]);
			leftBro->key[i].left->father = now;
			leftBro->key[i].right->father = now;
		}
		delete leftBro;
		return now->myDelete(dad, Root);
	}
	if(rightBro!=nullptr){//��������ֵܶ������������ֵܴ���
		BPTNode<T>* dad = now->father;
		keyNode<T> newKey;
		newKey.data = dad->key[rightLoc].data;
		newKey.right = rightBro->key[0].left;
		if (now->key.size() != 0)
			newKey.left = now->key[now->key.size() - 1].right;
		dad->key.erase(dad->key.begin() + rightLoc);//ɾ�����ڵ��ж�Ӧ�ؼ���
		now->key.push_back(newKey);//������ж�Ӧ�ؼ������Ƶ���ǰ���
		for (int i = 0; i < rightBro->key.size(); i++) {//���ֵ����ݸ��Ƶ���ǰ������
			now->key.push_back(rightBro->key[i]);
			rightBro->key[i].left->father = now;
			rightBro->key[i].right->father = now;

		}
		delete rightBro;
		return now->myDelete(dad, Root);
	}
	return false;
}

template<class T>
inline BPTNode<T>* BPTNode<T>::getLeftBro(BPTNode<T>* now,BPTNode<T>* Root) {
	BPTNode<T>* tmp = getHead(Root);
	while (tmp->next) {
		if (tmp->next == now)
			return tmp;
		tmp = tmp->next;
	}
	return nullptr;
}

template<class T>
inline BPTNode<T>* BPTNode<T>::getHead(BPTNode<T>* Root) {
	BPTNode<T>* tmp = Root;
	while (tmp->key.size() > 0 && tmp->key[0].left)
		tmp = tmp->key[0].left;
	return tmp;
}

