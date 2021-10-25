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
    volatile bool removed;

    NODE() : next(nullptr), removed(false) {}
    NODE(int x) : value(x), next(nullptr), removed(false) {}
    ~NODE() {}
};

class SPNODE {
public:
    int value;
    shared_ptr<SPNODE> next;
    mutex nlock;
    volatile bool removed;

    SPNODE() : next(nullptr), removed(false) {}
    SPNODE(int x) : value(x), next(nullptr), removed(false) {}
    ~SPNODE() {}
};


class nullmutex
{
public:
    void lock() {}
    void unlock() {}
};

class SPZLIST {
    shared_ptr <SPNODE> head, tail;

public:
    SPZLIST()
    {

        head = make_shared<SPNODE>(0x80000000);
        tail = make_shared<SPNODE>(0x7FFFFFFF);
        head->next = tail; // tail 객체의 ref가 2가 된다.
    }

    ~SPZLIST() {
        Init();
    }

    void Init() {
        head->next = tail;  // 중간에 있던 노드들을 알아서 다 날려준다.
    }

    bool validate(const shared_ptr<SPNODE>& pred, const shared_ptr<SPNODE>& curr)
    {
        return (false == pred->removed)
            && (false == curr->removed)
            && (pred->next == curr);
    }

    bool Add(int x)
    {
        shared_ptr<SPNODE> pred, curr;
        while (true) {
            pred = head;
            curr = pred->next;
            while (curr->value < x) {
                pred = curr;
                curr = curr->next;
            }

            pred->nlock.lock();
            curr->nlock.lock();

            if (true == validate(pred, curr)) {
                if (curr->value == x) {
                    pred->nlock.unlock();
                    curr->nlock.unlock();
                    return false;
                }
                else {
                    shared_ptr<SPNODE> new_node = make_shared<SPNODE>(x);
                    new_node->next = curr;   // 순서 주의
                    pred->next = new_node;   // 순서 주의
                    pred->nlock.unlock();
                    curr->nlock.unlock();
                    return true;
                }
            }
            else {
                pred->nlock.unlock();
                curr->nlock.unlock();
            }
        }
    }

    bool Remove(int x)
    {
        shared_ptr<SPNODE> pred, curr;
        while (1) {
            pred = head;
            curr = pred->next;
            while (curr->value < x) {
                pred = curr;
                curr = curr->next;
            }

            pred->nlock.lock();
            curr->nlock.lock();

            if (true == validate(pred, curr)) {
                if (curr->value != x) {
                    pred->nlock.unlock();
                    curr->nlock.unlock();
                    return false;
                }
                else {
                    curr->removed = true;
                    atomic_thread_fence(memory_order_seq_cst);
                    pred->next = curr->next;
                    pred->nlock.unlock();
                    curr->nlock.unlock();
                    return true;
                }
            }
            else {
                pred->nlock.unlock();
                curr->nlock.unlock();
            }
        }
    }

    bool Contains(int x)
    {
        shared_ptr<SPNODE> curr;

        curr = head;
        while (curr->value < x) {
            curr = curr->next;
        }
        return (curr->value == x) && (false == curr->removed);
    }

    void Print20() {
        shared_ptr<SPNODE> p = head->next;
        for (int i = 0; i < 20; ++i)
        {
            if (p == tail)
                break;
            cout << p->value << ",";
            p = p->next;

        }
        cout << "  ";
    }
};

SPZLIST spzlist;

void Benchmark_SPZList(int num_threads)
{
    for (int i = 0; i < NUM_TEST / num_threads; ++i) {
        int x = rand() % RANGE;
        switch (rand() % 3) {
        case 0:
            spzlist.Add(x);
            break;
        case 1:
            spzlist.Remove(x);
            break;
        case 2:
            spzlist.Contains(x);
            break;
        }
    }
}

int main()
{
    for (int i = 1; i <= 8; i *= 2) {
        vector<thread> workers;
        spzlist.Init();

        auto start_t = system_clock::now();
        for (int j = 0; j < i; ++j)
            workers.emplace_back(Benchmark_SPZList, i);
        for (auto& th : workers)
            th.join();
        auto end_t = system_clock::now();
        auto exec_t = end_t - start_t;

        //spzlist.Print20();

        cout << i << " threads";
        cout << " exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
    }
}