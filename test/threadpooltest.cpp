#include "pool/threadpool.h"
#include <iostream>
#include <chrono>
using namespace std;

int main() {
    ThreadPool pool(4);
    pool.start();
    // add task, per task sleeps second = 1, 2, 3, 4
    for (int i = 1; i <= 4; ++i) {
        pool.addTask([i] {
            this_thread::sleep_for(chrono::seconds(i));
            cout << "task " << i << " finished" << endl;
        });
    }

    return 0;
}