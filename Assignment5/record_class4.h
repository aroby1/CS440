/* This is a skeleton code for two-pass multi-way sorting, you can make modifications as long as you meet 
   all question requirements*/  
/* This record_class.h contains the class Records, which can be used to store tuples form Employee.csv */
#include <bits/stdc++.h>

using namespace std;

class Records{
    public:
    
    struct EmpRecord {
        int64_t id;
        string name;
        string bio;
        int64_t manager_id;
    }emp_record;

    /*** You can add more variables below if needed. ***/
    int no_values = 0; //You can use this to check if there are any more tuples

};

// Grab a single block from the Employee.csv file and put it inside the EmpRecord structure of the Records Class
Records Grab_Emp_Record(fstream &empin) {
    string line, word;
    Records emp;
    if (getline(empin, line, '\n')) {  // grab entire line
        stringstream s(line); // turn line into a stream

        getline(s, word,',');
        emp.emp_record.id = stoll(word);
        getline(s, word, ',');
        emp.emp_record.name = word;
        getline(s, word, ',');
        emp.emp_record.bio = word;
        getline(s, word, ',');
        emp.emp_record.manager_id = stoll(word);

        return emp;
    } else {
        emp.no_values = -1;
        return emp;
    }
}
