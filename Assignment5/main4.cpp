#include <bits/stdc++.h>
#include "record_class4.h"

using namespace std;

#define buffer_size 250 //defines how many pages are available in the Main Memory 


Records buffers[buffer_size]; 
vector<string> run_files;

/*** You may need to modify the return type and arguments of the following functions based on your implementation. ***/

void PrintSorted(ofstream &sorted_out, const Records &record);

//Function for PASS 1
// Function for PASS 1: sort the buffers and store records into temporary run files.
void Sort_Buffer(){
    run_files.clear();

    fstream empin("Employee.csv", ios::in);
    if (!empin.is_open()) {
        return;
    }

    int loaded = 0;
    int run_id = 0;

    auto flush_run = [&](int count) {
        if (count <= 0) {
            return;
        }

        vector<Records> chunk(count);
        for (int i = 0; i < count; ++i) {
            chunk[i] = buffers[i];
        }

        sort(chunk.begin(), chunk.end(), [](const Records &a, const Records &b) {
            return a.emp_record.id < b.emp_record.id;
        });

        string run_name = "run_" + to_string(run_id++) + ".csv";
        ofstream run_out(run_name, ios::out | ios::trunc);
        for (int i = 0; i < count; ++i) {
            run_out << chunk[i].emp_record.id << ','
                    << chunk[i].emp_record.name << ','
                    << chunk[i].emp_record.bio << ','
                    << chunk[i].emp_record.manager_id << '\n';
        }
        run_out.close();
        run_files.push_back(run_name);
    };

    while (true) {
        Records rec = Grab_Emp_Record(empin);
        if (rec.no_values == -1) {
            break;
        }

        buffers[loaded++] = rec;
        if (loaded == buffer_size) {
            flush_run(loaded);
            loaded = 0;
        }
    }

    if (loaded > 0) {
        flush_run(loaded);
    }

    empin.close();

    return;
}

//Function for PASS 2
// Function for PASS 2: merge sorted temporary runs and store final result in EmpSorted.csv.
void Merge_Runs(){
    struct HeapNode {
        int64_t id;
        int run_index;
        Records record;
    };

    struct CompareNode {
        bool operator()(const HeapNode &a, const HeapNode &b) const {
            return a.id > b.id;
        }
    };

    ofstream sorted_out("EmpSorted.csv", ios::out | ios::trunc);
    if (!sorted_out.is_open()) {
        return;
    }

    vector<fstream> run_inputs(run_files.size());
    priority_queue<HeapNode, vector<HeapNode>, CompareNode> min_heap;

    for (size_t i = 0; i < run_files.size(); ++i) {
        run_inputs[i].open(run_files[i], ios::in);
        if (!run_inputs[i].is_open()) {
            continue;
        }

        Records rec = Grab_Emp_Record(run_inputs[i]);
        if (rec.no_values != -1) {
            min_heap.push({rec.emp_record.id, static_cast<int>(i), rec});
        }
    }

    while (!min_heap.empty()) {
        HeapNode smallest = min_heap.top();
        min_heap.pop();

        PrintSorted(sorted_out, smallest.record);

        Records next_rec = Grab_Emp_Record(run_inputs[smallest.run_index]);
        if (next_rec.no_values != -1) {
            min_heap.push({next_rec.emp_record.id, smallest.run_index, next_rec});
        }
    }

    for (size_t i = 0; i < run_inputs.size(); ++i) {
        if (run_inputs[i].is_open()) {
            run_inputs[i].close();
        }
    }
    sorted_out.close();

    return;
}

// Store sorted results from PASS 2 into EmpSorted.csv.
void PrintSorted(ofstream &sorted_out, const Records &record){
    sorted_out << record.emp_record.id << ','
               << record.emp_record.name << ','
               << record.emp_record.bio << ','
               << record.emp_record.manager_id << '\n';

    return;
}

int main() {

    fstream empin;     //Open file streams to read and write the Employee.csv

    empin.open("Employee.csv", ios::in);  //Opening out the Employee.csv that we want to sort

   
    fstream SortOut; //Open file streams to read and write the EmpSorted.csv
    SortOut.open("EmpSorted.csv", ios::out | ios::app);  //Creating the EmpSorted.csv file where we will store our sorted results


    //TO DO: PASS 1, Create sorted runs for Employee.csv using Sort_Buffer()


    //TO DO: PASS 2, Use Merge_Runs() to sort the runs and generate EmpSorted.csv


    //Please delete the temporary files (runs) after you've sorted the Employee.csv

    return 0;
}
