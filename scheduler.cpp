#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>
#include <iomanip>

using namespace std;

struct Process {
    int id;
    int arrival_time;
    int burst_time;
    int priority;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    int remaining_time;
    bool started;
    
    Process(int i, int at, int bt, int p) : 
        id(i), arrival_time(at), burst_time(bt), priority(p),
        completion_time(0), turnaround_time(0), waiting_time(0), 
        response_time(0), remaining_time(bt), started(false) {}
};

struct GanttBlock {
    int process_id;
    int start_time;
    int end_time;
    
    GanttBlock(int pid, int start, int end) : 
        process_id(pid), start_time(start), end_time(end) {}
};

class ProcessScheduler {
private:
    vector<Process> processes;
    vector<GanttBlock> gantt_chart;
    
public:
    ProcessScheduler(vector<Process> procs) : processes(procs) {}
    
    void clearResults() {
        gantt_chart.clear();
        for (auto& p : processes) {
            p.completion_time = 0;
            p.turnaround_time = 0;
            p.waiting_time = 0;
            p.response_time = 0;
            p.remaining_time = p.burst_time;
            p.started = false;
        }
    }
    
    // First Come First Serve (FCFS)
    void fcfs() {
        clearResults();
        vector<Process> temp_processes = processes;
        
        // Sort by arrival time
        sort(temp_processes.begin(), temp_processes.end(), 
             [](const Process& a, const Process& b) {
                 return a.arrival_time < b.arrival_time;
             });
        
        int current_time = 0;
        
        for (auto& p : temp_processes) {
            if (current_time < p.arrival_time) {
                current_time = p.arrival_time;
            }
            
            int start_time = current_time;
            int end_time = current_time + p.burst_time;
            
            // Add to Gantt chart
            gantt_chart.push_back(GanttBlock(p.id, start_time, end_time));
            
            // Calculate metrics
            p.response_time = start_time - p.arrival_time;
            p.completion_time = end_time;
            p.turnaround_time = p.completion_time - p.arrival_time;
            p.waiting_time = p.turnaround_time - p.burst_time;
            
            current_time = end_time;
            
            // Update original process
            for (auto& orig : processes) {
                if (orig.id == p.id) {
                    orig = p;
                    break;
                }
            }
        }
    }
    
    // Shortest Job First (Non-preemptive)
    void sjf() {
        clearResults();
        vector<Process> remaining = processes;
        int current_time = 0;
        
        while (!remaining.empty()) {
            // Get available processes
            vector<Process*> available;
            for (auto& p : remaining) {
                if (p.arrival_time <= current_time) {
                    available.push_back(&p);
                }
            }
            
            if (available.empty()) {
                current_time = min_element(remaining.begin(), remaining.end(),
                    [](const Process& a, const Process& b) {
                        return a.arrival_time < b.arrival_time;
                    })->arrival_time;
                continue;
            }
            
            // Select shortest job
            Process* selected = *min_element(available.begin(), available.end(),
                [](const Process* a, const Process* b) {
                    return a->burst_time < b->burst_time;
                });
            
            int start_time = current_time;
            int end_time = current_time + selected->burst_time;
            
            // Add to Gantt chart
            gantt_chart.push_back(GanttBlock(selected->id, start_time, end_time));
            
            // Calculate metrics
            selected->response_time = start_time - selected->arrival_time;
            selected->completion_time = end_time;
            selected->turnaround_time = selected->completion_time - selected->arrival_time;
            selected->waiting_time = selected->turnaround_time - selected->burst_time;
            
            current_time = end_time;
            
            // Update original and remove from remaining
            for (auto& orig : processes) {
                if (orig.id == selected->id) {
                    orig = *selected;
                    break;
                }
            }
            
            remaining.erase(remove_if(remaining.begin(), remaining.end(),
                [selected](const Process& p) { return p.id == selected->id; }),
                remaining.end());
        }
    }
    
    // Shortest Remaining Time First (Preemptive SJF)
    void srtf() {
        clearResults();
        vector<Process> temp_processes = processes;
        int current_time = 0;
        int max_time = 0;
        
        // Calculate maximum possible time
        for (const auto& p : temp_processes) {
            max_time = max(max_time, p.arrival_time + p.burst_time);
        }
        max_time += 100; // Safety margin
        
        while (current_time < max_time) {
            // Get available processes
            vector<Process*> available;
            for (auto& p : temp_processes) {
                if (p.arrival_time <= current_time && p.remaining_time > 0) {
                    available.push_back(&p);
                }
            }
            
            if (available.empty()) {
                current_time++;
                continue;
            }
            
            // Select process with shortest remaining time
            Process* selected = *min_element(available.begin(), available.end(),
                [](const Process* a, const Process* b) {
                    return a->remaining_time < b->remaining_time;
                });
            
            // Set response time if first time
            if (!selected->started) {
                selected->response_time = current_time - selected->arrival_time;
                selected->started = true;
            }
            
            int start_time = current_time;
            selected->remaining_time--;
            current_time++;
            
            // Add or extend Gantt chart block
            if (!gantt_chart.empty() && gantt_chart.back().process_id == selected->id && 
                gantt_chart.back().end_time == start_time) {
                gantt_chart.back().end_time = current_time;
            } else {
                gantt_chart.push_back(GanttBlock(selected->id, start_time, current_time));
            }
            
            // If process completed
            if (selected->remaining_time == 0) {
                selected->completion_time = current_time;
                selected->turnaround_time = selected->completion_time - selected->arrival_time;
                selected->waiting_time = selected->turnaround_time - selected->burst_time;
                
                // Update original process
                for (auto& orig : processes) {
                    if (orig.id == selected->id) {
                        orig = *selected;
                        break;
                    }
                }
            }
            
            // Check if all processes are done
            bool all_done = true;
            for (const auto& p : temp_processes) {
                if (p.remaining_time > 0) {
                    all_done = false;
                    break;
                }
            }
            if (all_done) break;
        }
    }
    
    // Round Robin
    void roundRobin(int time_quantum) {
        clearResults();
        vector<Process> temp_processes = processes;
        queue<Process*> ready_queue;
        int current_time = 0;
        
        // Add initial processes
        for (auto& p : temp_processes) {
            if (p.arrival_time == 0) {
                ready_queue.push(&p);
            }
        }
        
        while (!ready_queue.empty() || 
               any_of(temp_processes.begin(), temp_processes.end(),
                      [](const Process& p) { return p.remaining_time > 0; })) {
            
            // Add newly arrived processes
            for (auto& p : temp_processes) {
                if (p.arrival_time == current_time && p.remaining_time == p.burst_time) {
                    bool already_in_queue = false;
                    queue<Process*> temp_queue = ready_queue;
                    while (!temp_queue.empty()) {
                        if (temp_queue.front()->id == p.id) {
                            already_in_queue = true;
                            break;
                        }
                        temp_queue.pop();
                    }
                    if (!already_in_queue && p.arrival_time != 0) {
                        ready_queue.push(&p);
                    }
                }
            }
            
            if (ready_queue.empty()) {
                current_time++;
                continue;
            }
            
            Process* current_process = ready_queue.front();
            ready_queue.pop();
            
            // Set response time if first time
            if (!current_process->started) {
                current_process->response_time = current_time - current_process->arrival_time;
                current_process->started = true;
            }
            
            int start_time = current_time;
            int execution_time = min(time_quantum, current_process->remaining_time);
            current_process->remaining_time -= execution_time;
            current_time += execution_time;
            
            // Add to Gantt chart
            gantt_chart.push_back(GanttBlock(current_process->id, start_time, current_time));
            
            // Add newly arrived processes during execution
            for (auto& p : temp_processes) {
                if (p.arrival_time > start_time && p.arrival_time <= current_time && 
                    p.remaining_time == p.burst_time) {
                    ready_queue.push(&p);
                }
            }
            
            if (current_process->remaining_time > 0) {
                ready_queue.push(current_process);
            } else {
                current_process->completion_time = current_time;
                current_process->turnaround_time = current_process->completion_time - current_process->arrival_time;
                current_process->waiting_time = current_process->turnaround_time - current_process->burst_time;
                
                // Update original process
                for (auto& orig : processes) {
                    if (orig.id == current_process->id) {
                        orig = *current_process;
                        break;
                    }
                }
            }
        }
    }
    
    // Priority Scheduling (Non-preemptive, lower number = higher priority)
    void priorityScheduling() {
        clearResults();
        vector<Process> remaining = processes;
        int current_time = 0;
        
        while (!remaining.empty()) {
            // Get available processes
            vector<Process*> available;
            for (auto& p : remaining) {
                if (p.arrival_time <= current_time) {
                    available.push_back(&p);
                }
            }
            
            if (available.empty()) {
                current_time = min_element(remaining.begin(), remaining.end(),
                    [](const Process& a, const Process& b) {
                        return a.arrival_time < b.arrival_time;
                    })->arrival_time;
                continue;
            }
            
            // Select highest priority (lowest number)
            Process* selected = *min_element(available.begin(), available.end(),
                [](const Process* a, const Process* b) {
                    if (a->priority == b->priority) {
                        return a->arrival_time < b->arrival_time; // FCFS for same priority
                    }
                    return a->priority < b->priority;
                });
            
            int start_time = current_time;
            int end_time = current_time + selected->burst_time;
            
            // Add to Gantt chart
            gantt_chart.push_back(GanttBlock(selected->id, start_time, end_time));
            
            // Calculate metrics
            selected->response_time = start_time - selected->arrival_time;
            selected->completion_time = end_time;
            selected->turnaround_time = selected->completion_time - selected->arrival_time;
            selected->waiting_time = selected->turnaround_time - selected->burst_time;
            
            current_time = end_time;
            
            // Update original and remove from remaining
            for (auto& orig : processes) {
                if (orig.id == selected->id) {
                    orig = *selected;
                    break;
                }
            }
            
            remaining.erase(remove_if(remaining.begin(), remaining.end(),
                [selected](const Process& p) { return p.id == selected->id; }),
                remaining.end());
        }
    }
    
    // Calculate and display metrics
    void displayMetrics(const string& algorithm_name) {
        cout << "\n=== " << algorithm_name << " Results ===\n";
        cout << "PID\tAT\tBT\tPri\tCT\tTAT\tWT\tRT\n";
        cout << "-----------------------------------------------\n";
        
        double total_tat = 0, total_wt = 0, total_rt = 0;
        int total_bt = 0;
        
        for (const auto& p : processes) {
            cout << "P" << p.id << "\t" << p.arrival_time << "\t" << p.burst_time 
                 << "\t" << p.priority << "\t" << p.completion_time << "\t" 
                 << p.turnaround_time << "\t" << p.waiting_time << "\t" 
                 << p.response_time << "\n";
            
            total_tat += p.turnaround_time;
            total_wt += p.waiting_time;
            total_rt += p.response_time;
            total_bt += p.burst_time;
        }
        
        int max_completion = max_element(processes.begin(), processes.end(),
            [](const Process& a, const Process& b) {
                return a.completion_time < b.completion_time;
            })->completion_time;
        
        cout << "\nAverage Turnaround Time: " << fixed << setprecision(2) 
             << total_tat / processes.size() << "\n";
        cout << "Average Waiting Time: " << total_wt / processes.size() << "\n";
        cout << "Average Response Time: " << total_rt / processes.size() << "\n";
        cout << "CPU Utilization: " << (double)total_bt / max_completion * 100 << "%\n";
        cout << "Throughput: " << (double)processes.size() / max_completion << " processes/unit\n";
    }
    
    // Display Gantt Chart
    void displayGanttChart() {
        cout << "\nGantt Chart:\n";
        cout << "|";
        for (const auto& block : gantt_chart) {
            cout << " P" << block.process_id << " |";
        }
        cout << "\n";
        
        cout << "0";
        for (const auto& block : gantt_chart) {
            cout << "    " << block.end_time;
        }
        cout << "\n";
    }
    
    // Get Gantt chart for web interface
    vector<GanttBlock> getGanttChart() const {
        return gantt_chart;
    }
    
    // Get processes with calculated metrics
    vector<Process> getProcesses() const {
        return processes;
    }
};

// Example usage and testing
int main() {
    // Example processes: {id, arrival_time, burst_time, priority}
    vector<Process> processes = {
        Process(1, 0, 5, 2),
        Process(2, 1, 3, 1),
        Process(3, 2, 8, 4),
        Process(4, 3, 6, 3)
    };
    
    cout << "Process Scheduling Algorithms Demonstration\n";
    cout << "==========================================\n";
    
    ProcessScheduler scheduler(processes);
    
    // Test FCFS
    scheduler.fcfs();
    scheduler.displayMetrics("First Come First Serve (FCFS)");
    scheduler.displayGanttChart();
    
    // Test SJF
    scheduler.sjf();
    scheduler.displayMetrics("Shortest Job First (SJF)");
    scheduler.displayGanttChart();
    
    // Test SRTF
    scheduler.srtf();
    scheduler.displayMetrics("Shortest Remaining Time First (SRTF)");
    scheduler.displayGanttChart();
    
    // Test Round Robin
    scheduler.roundRobin(2);
    scheduler.displayMetrics("Round Robin (Time Quantum = 2)");
    scheduler.displayGanttChart();
    
    // Test Priority
    scheduler.priorityScheduling();
    scheduler.displayMetrics("Priority Scheduling");
    scheduler.displayGanttChart();
    
    return 0;
}

/*
COMPILATION AND EXECUTION:
==========================

To compile and run this C++ program:

1. Save as 'scheduler.cpp'
2. Compile: g++ -o scheduler scheduler.cpp
3. Run: ./scheduler

Or with better optimization:
g++ -std=c++17 -O2 -o scheduler scheduler.cpp

OUTPUT EXPLANATION:
==================
- PID: Process ID
- AT: Arrival Time
- BT: Burst Time  
- Pri: Priority (lower number = higher priority)
- CT: Completion Time
- TAT: Turnaround Time
- WT: Waiting Time
- RT: Response Time

INTEGRATION WITH WEB:
====================
The C++ algorithms are designed to match the JavaScript versions exactly.
Both implementations use the same logic and produce identical results.
The web interface provides visualization while C++ provides the core algorithms.
*/
