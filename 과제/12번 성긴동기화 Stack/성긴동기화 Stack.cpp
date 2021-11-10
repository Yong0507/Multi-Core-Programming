#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

const int NUM_TEST = 1000'0000;
const int RANGE = 1000;

class nullmutex
{
public:
    void lock() {}
    void unlock() {}
};

class NODE;


class NODE {
public:
    int value;
    NODE* volatile next;

    NODE() {}
    NODE(int x) : value(x), next(nullptr) {}
    ~NODE() {}
};


class CSTACK {
    NODE* top;
    mutex top_lock;    

public:
    CSTACK() : top(nullptr) {}

    ~CSTACK() {
        Init();
    }

    void Init() {
        while (top != nullptr) {
            NODE* p = top;
            top = top->next;
            delete p;
        }
    }

    void Push(int x)
    {
        NODE* e = new NODE(x);        
        top_lock.lock();        
        e->next = top;
        top = e;        
        top_lock.unlock();
    }

    int Pop()
    {
        if (nullptr == top)
            return -2;

        top_lock.lock();        
        int temp = top->value;
        NODE* ptr = top;
        top = top->next;
        delete ptr;        
        top_lock.unlock();

        return temp;
    }



    void Print20() {
        NODE* p = top;
        for (int i = 0; i < 20; ++i)
        {
            if (p == nullptr)
                break;
            cout << p->value << ",";
            p = p->next;

        }
        cout << "  ";
    }
};

CSTACK mystack;

void Benchmark_STACK(int num_threads)
{
    for (int i = 0; i < NUM_TEST / num_threads; ++i) {
        if ((0 == rand() % 2) || (i < 1000))
            mystack.Push(i);
        else
            mystack.Pop();
    }
}

int main()
{
    for (int i = 1; i <= 8; i *= 2) {
        vector<thread> workers;
        mystack.Init();

        auto start_t = system_clock::now();
        for (int j = 0; j < i; ++j)
            workers.emplace_back(Benchmark_STACK, i);
        for (auto& th : workers)
            th.join();
        auto end_t = system_clock::now();
        auto exec_t = end_t - start_t;

        //mystack.Print20();

        cout << i << " threads";
        cout << " exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
    }
}