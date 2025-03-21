#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>

using namespace std;

enum Direction { NORTH, SOUTH };

const int k = 5; 
bool fair_mode = true; 

mutex bridge_mutex;
condition_variable north_wait, south_wait;
int vehicles_on_bridge = 0;
Direction current_dir = NORTH;
int consecutive_crossings = 0;
int waiting_north = 0, waiting_south = 0; 

void enter_bridge(Direction dir) {
    unique_lock<mutex> lock(bridge_mutex);
    
    if (dir == NORTH) waiting_north++;
    else waiting_south++;
    
    if (fair_mode) { 
        while (( current_dir != dir && vehicles_on_bridge > 0) || (consecutive_crossings >= k)) {
            if (consecutive_crossings >= k && vehicles_on_bridge == 0) {
                current_dir = (current_dir == NORTH) ? SOUTH : NORTH; 
                consecutive_crossings = 0;
                if (current_dir == NORTH)
                    north_wait.notify_all();
                else
                    south_wait.notify_all();
            }
            if (dir == NORTH)
                north_wait.wait(lock);
            else
                south_wait.wait(lock);
        }
    } else { 
        while (vehicles_on_bridge > 0 && current_dir != dir) {
            if (dir == NORTH)
                north_wait.wait(lock);
            else
                south_wait.wait(lock);
        }
        current_dir = dir;
    }
    
    if (dir == NORTH) waiting_north--;
    else waiting_south--;
    
    vehicles_on_bridge++;
    consecutive_crossings++;
    cout << "Vehicle crossing bridge (Dir: " << (dir == NORTH ? "NORTH" : "SOUTH") << ")" << endl;
}

void exit_bridge(Direction dir) {
    unique_lock<mutex> lock(bridge_mutex);
    vehicles_on_bridge--;

    
    if (vehicles_on_bridge == 0) {
        if (fair_mode) {
            if (consecutive_crossings >= k || (current_dir == NORTH && waiting_south > 0) || (current_dir == SOUTH && waiting_north > 0)) {
                current_dir = (current_dir == NORTH) ? SOUTH : NORTH;
                consecutive_crossings = 0;
            }
        } else { 
            current_dir = (current_dir == NORTH) ? SOUTH : NORTH;
        }
        
        if (current_dir == NORTH)
            north_wait.notify_all();
        else
            south_wait.notify_all();
    }
}

void vehicle_thread(Direction dir) {
    enter_bridge(dir);
    this_thread::sleep_for(chrono::milliseconds(500)); 
    exit_bridge(dir);
}

int main() {
    cout << "Select mode:\n1. Strict Directional Priority (Case 1)\n2. Fair Direction Switching (Case 2)\nEnter choice: ";
    int choice;
    cin >> choice;
    fair_mode = (choice == 2);

    vector<thread> vehicles;
    
    for (int i = 0; i < 18; i++) {
        vehicles.emplace_back(vehicle_thread, (i % 3 == 0) ? NORTH : SOUTH);
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    
    for (auto &t : vehicles) {
        t.join();
    }
    
    return 0;
}
