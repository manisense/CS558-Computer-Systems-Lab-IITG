#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <queue>
#include <climits>
#include <cstring>
#include <cctype>

using namespace std;

class WorkerThread {
private:
    int id;
    int serviceId;
    int priority;
    int resources;
    bool available;

public:
    WorkerThread(int id, int serviceId, int priority, int resources) 
        : id(id), serviceId(serviceId), priority(priority), resources(resources), available(true) {}

    int getId() const { return id; }
    int getPriority() const { return priority; }
    int getResources() const { return resources; }
    bool isAvailable() const { return available; }
    void setAvailable(bool status) { available = status; }
};

class Request {
private:
    int id;
    string transactionType;
    int resourcesRequired;
    int arrivalTime;
    int startTime;
    int completionTime;
    bool processed;
    bool assigned;
    int assignedThreadId;

public:
    Request(int id, string type, int resources, int arrival) 
        : id(id), transactionType(type), resourcesRequired(resources), arrivalTime(arrival),
          startTime(0), completionTime(0), processed(false), assigned(false), assignedThreadId(-1) {}

    int getId() const { return id; }
    string getTransactionType() const { return transactionType; }
    void setTransactionType(const string& type) { transactionType = type; }
    int getResourcesRequired() const { return resourcesRequired; }
    void setResourcesRequired(int resources) { resourcesRequired = resources; }
    int getArrivalTime() const { return arrivalTime; }
    int getStartTime() const { return startTime; }
    void setStartTime(int time) { startTime = time; }
    int getCompletionTime() const { return completionTime; }
    void setCompletionTime(int time) { completionTime = time; }
    bool isProcessed() const { return processed; }
    void setProcessed(bool status) { processed = status; }
    bool isAssigned() const { return assigned; }
    void setAssigned(bool status) { assigned = status; }
    int getAssignedThreadId() const { return assignedThreadId; }
    void setAssignedThreadId(int threadId) { assignedThreadId = threadId; }
};

class Service {
private:
    int id;
    string serviceType;
    vector<WorkerThread> threads;

public:
    Service(int id, string type) : id(id), serviceType(type) {}

    int getId() const { return id; }
    string getServiceType() const { return serviceType; }
    
    void addThread(int threadId, int priority, int resources) {
        threads.push_back(WorkerThread(threadId, id, priority, resources));
    }
    
    int getThreadCount() const { return threads.size(); }
    
    WorkerThread& getThread(int index) { return threads[index]; }
    
    int getTotalResources() const {
        int total = 0;
        for (size_t i = 0; i < threads.size(); i++) {
            total += threads[i].getResources();
        }
        return total;
    }
};

// Comparator functions
struct CompareByArrival {
    bool operator()(const Request& a, const Request& b) const {
        return a.getArrivalTime() < b.getArrivalTime();
    }
};

struct CompareByCompletionTime {
    bool operator()(const Request& a, const Request& b) const {
        return a.getCompletionTime() < b.getCompletionTime();
    }
};

class SchedulingSystem {
private:
    vector<Service> services;
    vector<Request> requests;
    int currentTime;
    int processedRequests;
    int rejectedRequests;
    int waitingRequests;
    int blockedRequests;

public:
    SchedulingSystem() : currentTime(0), processedRequests(0), rejectedRequests(0), waitingRequests(0), blockedRequests(0) {}
    
    void initializeSystem() {
        int numServices;
        cout << "Enter the number of services: ";
        cin >> numServices;

        cout << endl;
        
        for (int i = 0; i < numServices; i++) {
            string serviceType;
            cout << "Enter the service type for service " << i << ": ";
            cin >> serviceType;

            cout << endl;
            
            Service service(i, serviceType);
            
            int numThreads;
            cout << "Enter the number of worker threads for service " << i << ": ";
            cin >> numThreads;

            cout << endl;
            
            for (int j = 0; j < numThreads; j++) {
                int priority, resources;
                cout << "Enter priority level and resources for thread " << j << " of service " << i << ": ";
                cin >> priority >> resources;
                cout << endl;
                
                service.addThread(j, priority, resources);
            }
            
            services.push_back(service);
        }
    }
    
    void loadRequests() {
        int totalRequests;
        cout << endl;
        cout << "Enter the number of requests: ";
        cin >> totalRequests;
        cout << endl;
        
        for (int i = 0; i < totalRequests; i++) {
            string transactionType;
            int resourcesRequired;
            int arrivalTime = currentTime + (rand() % 10); // Random arrival time
            
            cout << "Enter transaction type and resources required for request " << i << ": ";
            cin >> transactionType >> resourcesRequired;
            cout << endl;
            requests.push_back(Request(i, transactionType, resourcesRequired, arrivalTime));
        }
    }
    
    int findSuitableService(Request& request) {
        for (size_t i = 0; i < services.size(); i++) {
            string serviceType = services[i].getServiceType();
            string requestType = request.getTransactionType();
            
            // Case-insensitive comparison using standard library
            string lowerServiceType = serviceType;
            string lowerRequestType = requestType;
            transform(lowerServiceType.begin(), lowerServiceType.end(), lowerServiceType.begin(), ::tolower);
            transform(lowerRequestType.begin(), lowerRequestType.end(), lowerRequestType.begin(), ::tolower);
            
            // Check if service type is a prefix of the request type
            if (lowerRequestType.find(lowerServiceType) == 0) {
                request.setTransactionType(serviceType); // Correct the type
                return i;
            }
        }
        return -1;
    }
    
    int findSuitableThread(int serviceId, int resourcesRequired) {
        int highestPriority = -1;
        int bestThread = -1;
        int minResourcesDiff = INT_MAX;
        
        Service& service = services[serviceId];
        for (int i = 0; i < service.getThreadCount(); i++) {
            WorkerThread& thread = service.getThread(i);
            
            if (thread.isAvailable() && thread.getResources() >= resourcesRequired) {
                int resourcesDiff = thread.getResources() - resourcesRequired;
                
                if (thread.getPriority() > highestPriority || 
                    (thread.getPriority() == highestPriority && resourcesDiff < minResourcesDiff)) {
                    highestPriority = thread.getPriority();
                    minResourcesDiff = resourcesDiff;
                    bestThread = i;
                }
            }
        }
        
        return bestThread;
    }
    
    void processRequest(Request& request, int serviceId, int threadId) {
        if (request.isAssigned()) {
            return;
        }
        
        WorkerThread& thread = services[serviceId].getThread(threadId);
        
        if (request.getResourcesRequired() > thread.getResources()) {
            request.setProcessed(true);
            request.setAssigned(true);
            rejectedRequests++;
            processedRequests++;
            cout << endl;
            cout << "Request " << request.getId() << " rejected (Insufficient Resources) at time " 
                      << currentTime << endl;

            return;
        }
        
        thread.setAvailable(false);
        request.setAssigned(true);
        request.setStartTime(currentTime);
        request.setAssignedThreadId(thread.getId());
        
        int processingTime = (request.getResourcesRequired() * 2) / thread.getResources() + 1;
        request.setCompletionTime(currentTime + processingTime);
        
        cout << "Request " << request.getId() << " will complete at time " 
                  << request.getCompletionTime() << " (Processing time: " << processingTime << ")" << endl;

        cout << endl;
                  
        cout << "Request " << request.getId() << " assigned to thread " << threadId 
                  << " of service " << serviceId << " (Priority: " << thread.getPriority() << ")" << endl;
    }
    
    void updateThreadAvailability() {
        for (size_t i = 0; i < requests.size(); i++) {
            Request& request = requests[i];
            
            if (!request.isProcessed() && request.getCompletionTime() > 0 && 
                request.getCompletionTime() <= currentTime) {
                
                int serviceId = -1;
                for (size_t j = 0; j < services.size(); j++) {
                    if (services[j].getServiceType() == request.getTransactionType()) {
                        serviceId = j;
                        break;
                    }
                }
                
                if (serviceId != -1) {
                    services[serviceId].getThread(request.getAssignedThreadId()).setAvailable(true);
                    request.setProcessed(true);
                    processedRequests++;
                    
                    cout << "Request " << request.getId() << " completed at time " << currentTime << endl;

                    cout << endl;
                }
            }
        }
    }
    
    bool isHighTraffic() {
        int pendingRequests = 0;
        for (size_t i = 0; i < requests.size(); i++) {
            const Request& request = requests[i];
            if (!request.isProcessed() && request.getArrivalTime() <= currentTime) {
                pendingRequests++;
            }
        }
        
        return (pendingRequests > (int)requests.size() / 2);
    }
    
    void scheduleRequests() {
        bool highTrafficMode = false;
        int maxRetries = 3;
        int consecutiveBlocked = 0;
        
        while (processedRequests < (int)requests.size()) {
            updateThreadAvailability();
            highTrafficMode = isHighTraffic();
            
            // Sort requests by arrival time
            sort(requests.begin(), requests.end(), CompareByArrival());
            
            bool anyRequestProcessed = false;
            
            for (size_t i = 0; i < requests.size(); i++) {
                Request& request = requests[i];
                
                if (!request.isProcessed() && !request.isAssigned() && 
                    request.getArrivalTime() <= currentTime) {
                    
                    int serviceId = findSuitableService(request);
                    
                    if (serviceId != -1) {
                        int threadId = findSuitableThread(serviceId, request.getResourcesRequired());
                        
                        if (threadId != -1) {
                            processRequest(request, serviceId, threadId);
                            anyRequestProcessed = true;
                            consecutiveBlocked = 0;
                        } else if (highTrafficMode) {
                            consecutiveBlocked++;
                            waitingRequests++;
                            blockedRequests++;
                            
                            if (consecutiveBlocked >= maxRetries) {
                                request.setProcessed(true);
                                rejectedRequests++;
                                processedRequests++;
                                cout << "Request " << request.getId() << " rejected (High Traffic) at time " 
                                          << currentTime << endl;
                                          cout << endl;
                                anyRequestProcessed = true;
                                consecutiveBlocked = 0;
                            } else {
                                cout << "Request " << request.getId() << " blocked (High Traffic) at time " 
                                          << currentTime << endl;
                                          cout << endl;
                            }
                        }
                    } else {
                        request.setProcessed(true);
                        rejectedRequests++;
                        processedRequests++;
                        cout << "Request " << request.getId() << " rejected (No Service) at time " 
                                  << currentTime << endl;
                                  cout << endl;
                        anyRequestProcessed = true;
                    }
                }
            }
            
            if (!anyRequestProcessed && currentTime > (int)requests.size() * 10) {
                cout << "\nDeadlock detected! Forcing completion of remaining requests..." << endl;
                cout << endl;
                
                for (size_t i = 0; i < requests.size(); i++) {
                    Request& request = requests[i];
                    if (!request.isProcessed()) {
                        request.setProcessed(true);
                        rejectedRequests++;
                        processedRequests++;
                        cout << "Request " << request.getId() << " forcefully terminated (Deadlock)" << endl;
                        cout << endl;
                    }
                }
                break;
            }
            
            cout << "Time: " << currentTime << ", Processed: " <<   processedRequests 
                      << ", Total: " << requests.size() << endl;
            cout << endl;
            currentTime++;
        }
    }
    
    void calculateStatistics() {
        int totalWaitingTime = 0;
        int totalTurnaroundTime = 0;
        int successfullyProcessed = 0;
        
        cout << "\n==== PROCESSING STATISTICS ====\n";
        cout << endl;
        cout << "Order of processed requests:\n";
        cout << endl;
        // Sort requests by completion time
        vector<Request> sortedRequests = requests;
        sort(sortedRequests.begin(), sortedRequests.end(), CompareByCompletionTime());
        
        for (size_t i = 0; i < sortedRequests.size(); i++) {
            const Request& request = sortedRequests[i];
            if (request.isProcessed() && request.getCompletionTime() > 0) {
                cout << "Request " << request.getId() << " (Type: " << request.getTransactionType() 
                          << ", Resources: " << request.getResourcesRequired() << ")" << endl;
                cout << endl;
                int waitingTime = request.getStartTime() - request.getArrivalTime();
                int turnaroundTime = request.getCompletionTime() - request.getArrivalTime();
                
                totalWaitingTime += waitingTime;
                totalTurnaroundTime += turnaroundTime;
                successfullyProcessed++;
                
                cout << "  Waiting Time: " << waitingTime << ", Turnaround Time: " << turnaroundTime << endl;
                cout << endl;
            }
        }
        
        float avgWaitingTime = successfullyProcessed > 0 ? 
            (float)totalWaitingTime / successfullyProcessed : 0;
        float avgTurnaroundTime = successfullyProcessed > 0 ? 
            (float)totalTurnaroundTime / successfullyProcessed : 0;
        
        cout << "\nSuccessfully processed requests: " << successfullyProcessed << endl;
        cout << endl;
        cout << "Average waiting time: " << avgWaitingTime << " time units" << endl;
        cout << endl;
        cout << "Average turnaround time: " << avgTurnaroundTime << " time units" << endl;
        cout << endl;
        cout << "Number of requests rejected: " << rejectedRequests << endl;
        cout << endl;
        cout << "Number of requests blocked: " << blockedRequests << endl;
        cout << endl;
        cout << "Number of requests waiting: " << waitingRequests << endl;
        cout << endl;
        cout << "\n==== RESOURCE UTILIZATION ====\n";
        cout << endl;
        for (size_t i = 0; i < services.size(); i++) {
            Service& service = services[i];
            int totalResources = service.getTotalResources();
            int usedResourcesTime = 0;
            int processedCount = 0;
            
            for (size_t j = 0; j < requests.size(); j++) {
                const Request& request = requests[j];
                if (request.isProcessed() && request.getCompletionTime() > 0 && 
                    request.getTransactionType() == service.getServiceType()) {
                    processedCount++;
                    int processingTime = request.getCompletionTime() - request.getStartTime();
                    usedResourcesTime += processingTime * request.getResourcesRequired();
                }
            }
            
            float utilization = 0.0;
            if (currentTime > 0 && totalResources > 0) {
                utilization = ((float)usedResourcesTime / (totalResources * currentTime)) * 100;
            }
            
            cout << "Service " << i << " (" << service.getServiceType() << "):" << endl;
            cout << "  Total Resources: " << totalResources << endl;
            cout << "  Average Resource Utilization: " << utilization << "%" << endl;
            cout << "  Successfully Processed Requests: " << processedCount << endl;
            cout << "  Resource Time Used: " << usedResourcesTime << endl;
            cout << "  Total Available Time: " << totalResources * currentTime << endl;
            cout << endl;
        }
    }
};

int main() {
    srand((unsigned int)time(NULL));
    
    SchedulingSystem system;
    system.initializeSystem();
    system.loadRequests();
    system.scheduleRequests();
    system.calculateStatistics();
    
    return 0;
}
