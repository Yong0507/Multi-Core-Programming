#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

const int NUM_TEST = 4000'000;
const int RANGE = 1000;

class NODE {
public:
	int value;
	NODE* next;
	mutex nlock;

	NODE() : next(nullptr) {}
	NODE(int x) : value(x), next(nullptr) {}
	~NODE() {}
};

class nullmutex
{
public:
	void lock() {}
	void unlock() {}
};

class FLIST {
    NODE head, tail;


public:
    FLIST()
    {
        head.value = 0x80000000;
        tail.value = 0x7FFFFFFF;

        head.next = &tail;
    }

    ~FLIST() {
        Init();
    }

    void Init() {
        while (head.next != &tail) {
            NODE* p = head.next;
            head.next = p->next;
            delete p;
        }
    }

    bool Add(int x)
    {

        NODE* pred, * curr;
        head.nlock.lock();
        pred = &head;
        curr = pred->next;
        curr->nlock.lock();
        while (curr->value < x) {
            pred->nlock.unlock();
            pred = curr;
            curr = curr->next;
            curr->nlock.lock();
        }
        if (curr->value == x) {
            curr->nlock.unlock();
            pred->nlock.unlock();
            return false;
        }
        else {
            NODE* new_node = new NODE(x);
            pred->next = new_node;
            new_node->next = curr;
            curr->nlock.unlock();
            pred->nlock.unlock();
            return true;
        }
    }

    bool Remove(int x)
    {
        NODE* pred, * curr;
        head.nlock.lock();
        pred = &head;
        curr = pred->next;
        curr->nlock.lock();
        while (curr->value < x) {
            pred->nlock.unlock();
            pred = curr;
            curr = curr->next;
            curr->nlock.lock();

        }
        if (curr->value != x) {
            curr->nlock.unlock();
            pred->nlock.unlock();
            return false;
        }
        else {
            pred->next = curr->next;
            curr->nlock.unlock();
            pred->nlock.unlock();
            delete curr;
            return true;
        }
    }

    bool Contains(int x)
    {
        NODE* curr, * pred;
        head.nlock.lock();
        pred = &head;
        curr = pred->next;
        curr->nlock.lock();
        while (curr->value < x) {
            pred->nlock.unlock();
            pred = curr;
            curr = curr->next;
            curr->nlock.lock();
        }
        if (curr->value != x) {
            curr->nlock.unlock();
            pred->nlock.unlock();
            return false;
        }
        else {
            curr->nlock.unlock();
            pred->nlock.unlock();
            return true;
        }
    }

    void Print20() {
        NODE* p = head.next;
        for (int i = 0; i < 20; ++i)
        {
            if (p == &tail)
                break;
            cout << p->value << ",";
            p = p->next;

        }
        cout << "  ";
    }
};


FLIST flist;

void Benchmark_FList(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		int x = rand() % RANGE;
		switch (rand() % 3) {
		case 0: 
			flist.Add(x);
			break;
		case 1:
			flist.Remove(x);
			break;
		case 2:
			flist.Contains(x);
			break;
		}
	}
}

int main()
{
	for (int i = 1; i <= 8; i *= 2) {
		vector<thread> workers;
		flist.Init();
		
		auto start_t = system_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark_FList, i);
		for (auto&th : workers)
			th.join();
		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;

		//flist.Print20();

		cout << i << " threads";
		cout << " exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
	}
}