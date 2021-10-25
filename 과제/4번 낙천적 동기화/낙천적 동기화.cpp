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

class OLIST {
    NODE head, tail;

public:
    OLIST()
    {
        head.value = 0x80000000;
        tail.value = 0x7FFFFFFF;

        head.next = &tail;
    }

    ~OLIST() {
        Init();
    }

    void Init() {
        while (head.next != &tail) {
            NODE* p = head.next;
            head.next = p->next;
            delete p;
        }
    }

    bool validate(NODE* pred, NODE* curr)
    {
        NODE* node = &head;
        while (node->value <= pred->value) {
            if (node == pred)
                return pred->next == curr;
            node = node->next;
        }
        return false;
    }

    bool Add(int x)
    {
        NODE* pred, * curr;
        while (true) {
            pred = &head;
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
                    NODE* new_node = new NODE(x);
                    new_node->next = curr;  // 순서 주의
                    pred->next = new_node;  // 순서 주의
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
        NODE* pred, * curr;
        while (true) {
            pred = &head;
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
        NODE* curr, * pred;
        while (true) {
            pred = &head;
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
                    return true;
                }
                else {
                    pred->nlock.unlock();
                    curr->nlock.unlock();
                    return false;
                }
            }
            else {
                pred->nlock.unlock();
                curr->nlock.unlock();
            }
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

OLIST olist;

void Benchmark_OList(int num_threads)
{
    for (int i = 0; i < NUM_TEST / num_threads; ++i) {
        int x = rand() % RANGE;
        switch (rand() % 3) {
        case 0:
            olist.Add(x);
            break;
        case 1:
            olist.Remove(x);
            break;
        case 2:
            olist.Contains(x);
            break;
        }
    }
}

int main()
{
    for (int i = 1; i <= 8; i *= 2) {
        vector<thread> workers;
        olist.Init();

        auto start_t = system_clock::now();
        for (int j = 0; j < i; ++j)
            workers.emplace_back(Benchmark_OList, i);
        for (auto& th : workers)
            th.join();
        auto end_t = system_clock::now();
        auto exec_t = end_t - start_t;

        //olist.Print20();

        cout << i << " threads";
        cout << " exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
    }
}