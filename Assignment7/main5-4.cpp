#include <bits/stdc++.h>
#include "record_class5.h"

using namespace std;

#define buffer_size 500

Records buffers[buffer_size];
vector<string> emp_runs, dept_runs;

// Compare functions 
bool compareEmp(const Records &a, const Records &b) {
    return a.emp_record.id < b.emp_record.id;
}

bool compareDept(const Records &a, const Records &b) {
    return a.dept_record.manager_id < b.dept_record.manager_id;
}

// Loads records into buffer, sorts by key, writes to a temp run file.
// Repeats until the entire file is processed into sorted runs.
void Sort_Buffer() {
    // Employee runs, sorted by id
    {
        fstream empin("Employee.csv", ios::in);
        int loaded = 0, run_id = 0;

        while (true) {
            Records rec = Grab_Emp_Record(empin);
            if (rec.no_values == -1) break;
            buffers[loaded++] = rec;
            if (loaded == buffer_size) {
                sort(buffers, buffers + loaded, compareEmp);
                string name = "emp_run_" + to_string(run_id++) + ".csv";
                ofstream out(name, ios::out | ios::trunc);
                for (int i = 0; i < loaded; i++)
                    out << buffers[i].emp_record.id << ','
                        << buffers[i].emp_record.name << ','
                        << buffers[i].emp_record.bio << ','
                        << buffers[i].emp_record.manager_id << '\n';
                emp_runs.push_back(name);
                loaded = 0;
            }
        }
        // Flush remaining records
        if (loaded > 0) {
            sort(buffers, buffers + loaded, compareEmp);
            string name = "emp_run_" + to_string(run_id++) + ".csv";
            ofstream out(name, ios::out | ios::trunc);
            for (int i = 0; i < loaded; i++)
                out << buffers[i].emp_record.id << ','
                    << buffers[i].emp_record.name << ','
                    << buffers[i].emp_record.bio << ','
                    << buffers[i].emp_record.manager_id << '\n';
            emp_runs.push_back(name);
        }
        empin.close();
    }

    // Dept runs, sorted by manager_id
    {
        fstream deptin("Dept.csv", ios::in);
        int loaded = 0, run_id = 0;

        while (true) {
            Records rec = Grab_Dept_Record(deptin);
            if (rec.no_values == -1) break;
            buffers[loaded++] = rec;
            if (loaded == buffer_size) {
                sort(buffers, buffers + loaded, compareDept);
                string name = "dept_run_" + to_string(run_id++) + ".csv";
                ofstream out(name, ios::out | ios::trunc);
                for (int i = 0; i < loaded; i++)
                    out << buffers[i].dept_record.did << ','
                        << buffers[i].dept_record.dname << ','
                        << buffers[i].dept_record.manager_id << '\n';
                dept_runs.push_back(name);
                loaded = 0;
            }
        }
        // Flush remaining records
        if (loaded > 0) {
            sort(buffers, buffers + loaded, compareDept);
            string name = "dept_run_" + to_string(run_id++) + ".csv";
            ofstream out(name, ios::out | ios::trunc);
            for (int i = 0; i < loaded; i++)
                out << buffers[i].dept_record.did << ','
                    << buffers[i].dept_record.dname << ','
                    << buffers[i].dept_record.manager_id << '\n';
            dept_runs.push_back(name);
        }
        deptin.close();
    }

    return;
}

// Writes one matched Employee + Dept pair to Join.csv.
void PrintJoin(ofstream &joinout, const Records &emp, const Records &dept) {
    joinout << emp.emp_record.id << ','
            << emp.emp_record.name << ','
            << emp.emp_record.bio << ','
            << emp.emp_record.manager_id << ','
            << dept.dept_record.did << ','
            << dept.dept_record.dname << ','
            << dept.dept_record.manager_id << '\n';
}

// Merges all emp and dept run files using a min-heap, one page per run in memory.
// Walks both merged streams and outputs matching pairs where Employee.id = Dept.manager_id.
void Merge_Join_Runs() {
    struct EmpNode {
        int64_t sort_value;
        int run_index;
        Records record;
        bool operator>(const EmpNode &compare_to) const { return sort_value > compare_to.sort_value; }
    };

    struct DeptNode {
        int64_t sort_value;
        int run_index;
        Records record;
        bool operator>(const DeptNode &compare_to) const { return sort_value > compare_to.sort_value; }
    };

    // One open file per run file
    vector<fstream> emp_inputs(emp_runs.size()), dept_inputs(dept_runs.size());
    // Min-heaps to track the smallest record across all runs
    priority_queue<EmpNode,  vector<EmpNode>,  greater<EmpNode>>  emp_heap;
    priority_queue<DeptNode, vector<DeptNode>, greater<DeptNode>> dept_heap;

    // Seed the heaps with the first record from each run
    for (size_t i = 0; i < emp_runs.size(); i++) {
        emp_inputs[i].open(emp_runs[i], ios::in);
        Records rec = Grab_Emp_Record(emp_inputs[i]);
        if (rec.no_values != -1) {
            EmpNode node;
            node.sort_value = rec.emp_record.id;
            node.run_index  = (int)i;
            node.record     = rec;
            emp_heap.push(node);
        }
    }

    for (size_t i = 0; i < dept_runs.size(); i++) {
        dept_inputs[i].open(dept_runs[i], ios::in);
        Records rec = Grab_Dept_Record(dept_inputs[i]);
        if (rec.no_values != -1) {
            DeptNode node;
            node.sort_value = rec.dept_record.manager_id;
            node.run_index  = (int)i;
            node.record     = rec;
            dept_heap.push(node);
        }
    }

    ofstream joinout("Join.csv", ios::out | ios::trunc);

    // Current records from each heap
    bool emp_valid  = !emp_heap.empty();
    bool dept_valid = !dept_heap.empty();

    EmpNode  cur_emp;
    DeptNode cur_dept;

    if (emp_valid)  { cur_emp  = emp_heap.top();  emp_heap.pop(); }
    if (dept_valid) { cur_dept = dept_heap.top(); dept_heap.pop(); }

    // Walk both sorted streams simultaneously
    while (emp_valid && dept_valid) {
        if (cur_emp.sort_value < cur_dept.sort_value) {
            // Advance emp stream
            if (!emp_heap.empty()) { cur_emp = emp_heap.top(); emp_heap.pop(); }
            else emp_valid = false;
        } else if (cur_emp.sort_value > cur_dept.sort_value) {
            // Advance dept stream
            if (!dept_heap.empty()) { cur_dept = dept_heap.top(); dept_heap.pop(); }
            else dept_valid = false;
        } else {
            // Match found, collect all emp and dept records with this key
            int64_t match_key = cur_emp.sort_value;
            vector<Records> emp_group, dept_group;

            // Gather all matching emp records (local variable, excluded from limit)
            while (emp_valid && cur_emp.sort_value == match_key) {
                emp_group.push_back(cur_emp.record);
                if (!emp_heap.empty()) { cur_emp = emp_heap.top(); emp_heap.pop(); }
                else emp_valid = false;
            }

            // Gather all matching dept records (local variable, excluded from limit)
            while (dept_valid && cur_dept.sort_value == match_key) {
                dept_group.push_back(cur_dept.record);
                if (!dept_heap.empty()) { cur_dept = dept_heap.top(); dept_heap.pop(); }
                else dept_valid = false;
            }

            // Output all combinations
            for (size_t e = 0; e < emp_group.size(); e++)
                for (size_t d = 0; d < dept_group.size(); d++)
                    PrintJoin(joinout, emp_group[e], dept_group[d]);
        }
    }

    joinout.close();

    for (size_t i = 0; i < emp_inputs.size(); i++) if (emp_inputs[i].is_open()) emp_inputs[i].close();
    for (size_t i = 0; i < dept_inputs.size(); i++) if (dept_inputs[i].is_open()) dept_inputs[i].close();

    return;
}

int main() {
    fstream empin;
    fstream deptin;

    empin.open("Employee.csv", ios::in);
    deptin.open("Dept.csv", ios::in);

    fstream joinout;
    joinout.open("Join.csv", ios::out | ios::app);

    // Create sorted runs for Dept and Employee
    Sort_Buffer();

    // Merge and join the runs, generate Join.csv
    Merge_Join_Runs();

    // Delete temp run files
    for (size_t i = 0; i < emp_runs.size();  i++) remove(emp_runs[i].c_str());
    for (size_t i = 0; i < dept_runs.size(); i++) remove(dept_runs[i].c_str());

    return 0;
}