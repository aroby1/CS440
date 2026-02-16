#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cmath>
#include <cstdint>

using namespace std;

class Record {
public:
    int64_t id, manager_id; // Employee ID and their manager's ID
    string bio, name; // Fixed length string to store employee name and biography

    Record(vector<string> &fields) {
        id = stoll(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoll(fields[3]);
    }

    Record(int64_t id_, const string &name_, const string &bio_, int64_t manager_id_)
        : id(id_), manager_id(manager_id_), bio(bio_), name(name_) {}
	
	// Function to get the size of the record
    int get_size() {
        // sizeof(int) is for name/bio size() in serialize function
        return sizeof(id) + sizeof(manager_id) + sizeof(int) + name.size() + sizeof(int) + bio.size(); 
    }

    // Function to serialize the record for writing to file
    string serialize() const {
        ostringstream oss;
        oss.write(reinterpret_cast<const char *>(&id), sizeof(id));
        oss.write(reinterpret_cast<const char *>(&manager_id), sizeof(manager_id));
        int name_len = name.size();
        int bio_len = bio.size();
        oss.write(reinterpret_cast<const char *>(&name_len), sizeof(name_len));
        oss.write(name.c_str(), name.size());
        oss.write(reinterpret_cast<const char *>(&bio_len), sizeof(bio_len));
        oss.write(bio.c_str(), bio.size());
        return oss.str();
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }
};

class Page {
public:
    vector<Record> records; // Data_Area containing the records
    vector<pair<int, int>> slot_directory; // Slot directory containing offset and size of each record
    int cur_size = sizeof(int); // Current size of the page including the overflow page pointer. if you also write the length of slot directory change it accordingly.
    int overflowPointerIndex;  // Initially set to -1, indicating the page has no overflow page. 
							   // Update it to the position of the overflow page when one is created.

    // Constructor
    Page() : overflowPointerIndex(-1) {}

    // Function to insert a record into the page
    bool insert_record_into_page(Record r) {
        int record_size = r.get_size();
        int slot_size = sizeof(int) * 2;
        if (cur_size + record_size + slot_size > 4096) { // Check if page size limit exceeded, considering slot directory size
            return false; // Cannot insert the record into this page
        } else {
            int offset = 0;
            if (!slot_directory.empty()) {
                offset = slot_directory.back().first + slot_directory.back().second;
            }
            records.push_back(r);
            slot_directory.push_back({offset, record_size});
            cur_size += record_size + slot_size;
            return true;
        }
    }

    // Function to write the page to a binary output stream. You may use
    void write_into_data_file(ostream &out) const {
        char page_data[4096] = {0}; // Buffer to hold page data
        int offset = 0;

        // Write records into page_data buffer
        for (const auto &record: records) {
            string serialized = record.serialize();
            memcpy(page_data + offset, serialized.c_str(), serialized.size());
            offset += serialized.size();
        }

        int overflow_pos = 4096 - static_cast<int>(sizeof(int));
        memcpy(page_data + overflow_pos, &overflowPointerIndex, sizeof(int));

        int cursor = overflow_pos;
        for (size_t i = 0; i < slot_directory.size(); ++i) {
            cursor -= static_cast<int>(sizeof(int) * 2);
            int rec_offset = slot_directory[i].first;
            int rec_size = slot_directory[i].second;
            memcpy(page_data + cursor, &rec_offset, sizeof(int));
            memcpy(page_data + cursor + sizeof(int), &rec_size, sizeof(int));
        }

        // Write the page_data buffer to the output stream
        out.write(page_data, sizeof(page_data));
    }

    // Function to read a page from a binary input stream
    bool read_from_data_file(istream &in) {
        char page_data[4096] = {0}; // Buffer to hold page data
        in.read(page_data, 4096); // Read data from input stream

        streamsize bytes_read = in.gcount();
        if (bytes_read == 4096) {
            records.clear();
            slot_directory.clear();
            cur_size = sizeof(int);

            int overflow_pos = 4096 - static_cast<int>(sizeof(int));
            memcpy(&overflowPointerIndex, page_data + overflow_pos, sizeof(int));

            int cursor = overflow_pos - static_cast<int>(sizeof(int) * 2);
            while (cursor >= 0) {
                int rec_offset = 0;
                int rec_size = 0;
                memcpy(&rec_offset, page_data + cursor, sizeof(int));
                memcpy(&rec_size, page_data + cursor + sizeof(int), sizeof(int));

                if (rec_size <= 0 || rec_offset < 0 || rec_offset + rec_size > overflow_pos) {
                    break;
                }

                slot_directory.push_back({rec_offset, rec_size});
                cursor -= static_cast<int>(sizeof(int) * 2);
            }

            for (const auto &slot : slot_directory) {
                int rec_offset = slot.first;
                int rec_size = slot.second;
                int rec_end = rec_offset + rec_size;
                int p = rec_offset;

                int64_t id = 0;
                int64_t manager_id = 0;
                int name_len = 0;
                int bio_len = 0;

                if (p + static_cast<int>(sizeof(int64_t) * 2 + sizeof(int) * 2) > rec_end) {
                    continue;
                }

                memcpy(&id, page_data + p, sizeof(int64_t));
                p += sizeof(int64_t);
                memcpy(&manager_id, page_data + p, sizeof(int64_t));
                p += sizeof(int64_t);
                memcpy(&name_len, page_data + p, sizeof(int));
                p += sizeof(int);

                if (name_len < 0 || p + name_len > rec_end) {
                    continue;
                }
                string name(page_data + p, page_data + p + name_len);
                p += name_len;

                memcpy(&bio_len, page_data + p, sizeof(int));
                p += sizeof(int);

                if (bio_len < 0 || p + bio_len > rec_end) {
                    continue;
                }
                string bio(page_data + p, page_data + p + bio_len);

                records.emplace_back(id, name, bio, manager_id);
                cur_size += rec_size + static_cast<int>(sizeof(int) * 2);
            }

            return true;
        }

        if (bytes_read > 0) {
            cerr << "Incomplete read: Expected 4096 bytes, but only read " << bytes_read << " bytes." << endl;
        }

        return false;
    }
};

class HashIndex {
private:
    const size_t maxCacheSize = 1; // Maximum number of pages in the buffer
    const int Page_SIZE = 4096; // Size of each page in bytes
    vector<int> PageDirectory; // Map h(id) to a bucket location in EmployeeIndex(e.g., the jth bucket)
    // can scan to correct bucket using j*Page_SIZE as offset (using seek function)
    // can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int nextFreePage; // Next place to write a bucket
    string fileName;

    // Function to compute hash value for a given ID
    int compute_hash_value(int64_t id) {
        return static_cast<int>(id & 0xFF);
    }

    // Function to add a new record to an existing page in the index file
    void addRecordToIndex(int pageIndex, Page &page, Record &record) {
        // Open index file in binary mode for updating
        fstream indexFile(fileName, ios::binary | ios::in | ios::out);

        if (!indexFile) {
            ofstream createFile(fileName, ios::binary);
            createFile.close();
            indexFile.open(fileName, ios::binary | ios::in | ios::out);
            if (!indexFile) {
                cerr << "Error: Unable to open index file for adding record." << endl;
                return;
            }
        }
		
        // Load the starting page if it exists; otherwise treat it as an empty page.
        indexFile.seekg(pageIndex * Page_SIZE, ios::beg);
        if (!page.read_from_data_file(indexFile)) {
            page = Page();
        }

        while (!page.insert_record_into_page(record)) {
            if (page.overflowPointerIndex != -1) {
                // Follow overflow chain.
                pageIndex = page.overflowPointerIndex;
                indexFile.seekg(pageIndex * Page_SIZE, ios::beg);
                if (!page.read_from_data_file(indexFile)) {
                    page = Page();
                }
            } else {
                // Create a new overflow page and persist the updated pointer on current page.
                int previousPageIndex = pageIndex;
                page.overflowPointerIndex = nextFreePage;

                indexFile.seekp(previousPageIndex * Page_SIZE, ios::beg);
                page.write_into_data_file(indexFile);

                pageIndex = nextFreePage;
                nextFreePage++;
                page = Page();
            }
        }
        indexFile.seekp(pageIndex * Page_SIZE, ios::beg);
        page.write_into_data_file(indexFile);

        // Close the index file
        indexFile.close();
    }

    // Function to search for a record by ID in a given page of the index file
    void searchRecordByIdInPage(int pageIndex, int64_t id) {
        // Open index file in binary mode for reading
        ifstream indexFile(fileName, ios::binary | ios::in);

        // Seek to the appropriate position in the index file
        indexFile.seekg(pageIndex * Page_SIZE, ios::beg);

        // Read the page from the index file
        Page page;
        page.read_from_data_file(indexFile);

        // TODO:
        //  - Search for the record by ID in the page
        //  - Check for overflow pages and report if record with given ID is not found
    }

public:
    HashIndex(string indexFileName) : nextFreePage(256), fileName(indexFileName) {
    }

    // Function to create hash index from Employee CSV file
    void createFromFile(string csvFileName) {
        // Read CSV file and add records to index
        // Open the CSV file for reading
        ifstream csvFile(csvFileName);

        string line;
        // Read each line from the CSV file
        while (getline(csvFile, line)) {
            // Parse the line and create a Record object
            stringstream ss(line);
            string item;
            vector<string> fields;
            while (getline(ss, item, ',')) {
                fields.push_back(item);
            }
            Record record(fields);

            // TODO:
            //   - Compute hash value for the record's ID using compute_hash_value() function.
            //   - Get the page index from PageDirectory. If it's not in PageDirectory, define a new page using nextFreePage.
            //   - Insert the record into the appropriate page in the index file using addRecordToIndex() function.


        }

        // Close the CSV file
        csvFile.close();
    }

    // Function to search for a record by ID in the hash index
    void findAndPrintEmployee(int64_t id) {
        // Open index file in binary mode for reading
        ifstream indexFile(fileName, ios::binary | ios::in);

        // TODO:
        //  - Compute hash value for the given ID using compute_hash_value() function
        //  - Search for the record in the page corresponding to the hash value using searchRecordByIdInPage() function

        // Close the index file
        indexFile.close();
    }
};
