//#include <iostream>
//#include <thread>
//#include <mutex>
//#include <vector>
//#include <chrono>
//
//using namespace std;
//using namespace chrono;
//
//const int NUM_TEST = 1000'0000;
//
//class NODE {
//public:
//    int value;
//    NODE* volatile next;
//
//    NODE() : next(nullptr) {}
//    NODE(int x) : value(x), next(nullptr) {}
//    ~NODE() {}
//};
//
//class nullmutex
//{
//public:
//    void lock() {}
//    void unlock() {}
//};
//
//class LFQUEUE {
//    NODE* volatile head;
//    NODE* volatile tail;
//
//public:
//    // volatile 위치가 다를 수 있음
//    bool CAS(NODE* volatile* addr, NODE* old_node, NODE* new_node)
//    {
//        int a_addr = reinterpret_cast<int>(addr);
//
//        return atomic_compare_exchange_strong(
//            reinterpret_cast<atomic_int*>(a_addr),
//            reinterpret_cast<int*>(&old_node),
//            reinterpret_cast<int>(new_node));
//    }
//
//    LFQUEUE()
//    {
//        head = tail = new NODE(0);
//    }
//
//    ~LFQUEUE() {
//        Init();
//        delete head;
//    }
//
//    void Init() {
//        // head와 tail이 만나면 보초노드 빼고 다 지운 것
//        while (head != tail) {
//            NODE* p = head;
//            head = head->next;
//            delete p;
//        }
//    }
//
//    void Enq(int x)
//    {
//        NODE* e = new NODE(x);
//        while (true) {
//            NODE* last = tail;
//            NODE* next = last->next;
//            if (last != tail) continue;
//            if (nullptr == next) {
//                if (CAS(&(last->next), nullptr, e)) {
//                    CAS(&tail, last, e);
//                    return;
//                }
//            }
//            else
//                CAS(&tail, last, next);
//        }
//    }
//
//    int Deq()
//    {
//        while (true) {
//            NODE* first = head;
//            NODE* last = tail;
//            NODE* next = first->next;
//            if (first != head) continue;
//            if (next == nullptr) return -1;
//            if (first == last) {
//                CAS(&tail, last, next);
//                continue;
//            }
//            int value = next->value;
//            if (false == CAS(&head, first, next)) continue;
//            delete first;
//            return value;
//        }
//
//    }
//
//    void Print20()
//    {
//        NODE* p = head->next;
//        for (int i = 0; i < 20; ++i) {
//            if (p == nullptr)
//                break;
//            cout << p->value << ",";
//            p = p->next;
//        }
//        cout << "  ";
//    }
//};
//
//LFQUEUE myqueue;
//
//void Benchmark_LFQueue(int num_threads)
//{
//    for (int i = 0; i < NUM_TEST / num_threads; ++i) {
//        if ((0 == rand() % 2) || (i < 32 / num_threads))
//            myqueue.Enq(i);
//        else
//            myqueue.Deq();
//    }
//}
//
//int main()
//{
//    for (int i = 1; i <= 8; i *= 2) {
//        vector<thread> workers;
//        myqueue.Init();
//
//        auto start_t = system_clock::now();
//        for (int j = 0; j < i; ++j)
//            workers.emplace_back(Benchmark_LFQueue, i);
//        for (auto& th : workers)
//            th.join();
//        auto end_t = system_clock::now();
//        auto exec_t = end_t - start_t;
//
//        //myqueue.Print20();
//
//        cout << i << " threads";
//        cout << " exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
//    }
//}