#include "timer/heaptimer.h"
#include <chrono>
#include <iostream>
#include <ostream>
#include <thread>

int main() {
    HeapTimer heapTimer;
    heapTimer.add([]() { std::cout << "4" << std::endl
                                   << std::flush; }, 3,
                  330);
    heapTimer.add([]() { std::cout << "3" << std::endl
                                   << std::flush; }, 2,
                  555);
    int id2 = heapTimer.add(
        []() { std::cout << "2" << std::endl
                         << std::flush; }, 0, 220);
    int id1 = heapTimer.add(
        []() { std::cout << "1" << std::endl
                         << std::flush; }, 0, 1);

    int id5 = heapTimer.add(
        []() { std::cout << "5" << std::endl
                         << std::flush; }, 0, 330);
    heapTimer.adjust(id5, 4, 444);

    int id6 =
        heapTimer.add([]() { std::cout << "6" << std::endl
                                       << std::flush; }, 0,
                      330); // shoud not output
    heapTimer.adjust(id2, 1, 0); 
    heapTimer.delTimerNode(id2);


    while (!heapTimer.empty()) {
        heapTimer.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //heapTimer.adjust(id1, 0, 0); // should fail
    return 0;
}