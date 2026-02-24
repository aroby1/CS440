#include <string>
#include <iostream>
#include <stdexcept>
#include "classes.h"

using namespace std;

int main(int argc, char* argv[]) {
    HashIndex hashIndex("EmployeeIndex.dat");
    hashIndex.createFromFile("Employee.csv");

    for (int i = 1; i < argc; i++) {
        int64_t id = stoll(argv[i]);
        hashIndex.findAndPrintEmployee(id);
    }

    return 0;
}