#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>

using namespace std;

const int N = 5; 
const int TOTAL_PASSENGERS = 20; 
int waiting_passengers = 0; 
int boarded_passengers = 0; 
bool bus_departed = false; 

mutex mtx;
condition_variable cv_gate_open;
condition_variable cv_bus_depart;

void bus() {
    while (true) {
        unique_lock<mutex> lock(mtx);

        cv_bus_depart.wait(lock, [] { return bus_departed; });

        cout << "Bus is departing with " << N << " passengers.\n";
        this_thread::sleep_for(chrono::seconds(2)); 
        
        boarded_passengers = 0;
        bus_departed = false;

        cout << "Bus departed. Gate reopens for new passengers.\n";
        cv_gate_open.notify_all(); 
    }
}

void passenger(int id) {
    unique_lock<mutex> lock(mtx);
    
    waiting_passengers++;
    cout << "Passenger " << id << " is waiting at the gate.\n";
    
    cv_gate_open.wait(lock, [] { return boarded_passengers < N; });

    cout << "Passenger " << id << " enters the bus.\n";
    waiting_passengers--; 
    boarded_passengers++;
    
    if (boarded_passengers == N) {
       
        bus_departed = true;
        cv_bus_depart.notify_one();
    }
    
    lock.unlock();
}


int main() {
    thread busThread(bus);
    vector<thread> passengers;

    for (int i = 1; i <= TOTAL_PASSENGERS; ++i) {
        passengers.emplace_back(passenger, i);
        this_thread::sleep_for(chrono::milliseconds(500)); 
    }

    for (auto& p : passengers) {
        p.join();
    }
    busThread.detach();

    return 0;
}
