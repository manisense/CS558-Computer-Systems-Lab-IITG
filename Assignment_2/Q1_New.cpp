#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace std;

const int Max_Passengers = 15;
const int Total_Passengers = 49;
int waiting_passengers = 0;
int boarded_passenger = 0;
bool bus_departed = false;

condition_variable cv_gate_open;
condition_variable cv_bus_depart;
mutex mtx;

void bus(){
    unique_lock<mutex> lock(mtx);



}

void passenger(){

}


int main(){
    thread BusThread;
    vector<thread> PassengerThread;





    return 0;
}