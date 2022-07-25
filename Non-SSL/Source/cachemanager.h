////////////////////////////////////////////////////////
///
///     "cachemanager.h"
///
///     Class declaration for CacheManager
///     This class manages the locally stored cache
///     via a SQLite database
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <chrono>
#include <stdio.h>
#include "utilities.h"


using std::string; using std::vector;

class CacheManager {
public:
    CacheManager(size_t size = 110000);
    ~CacheManager();
    int check_cache(string query_file);

    vector<unsigned char> consume_binary();
    string get_file_type();
    string get_file_encoding();
    string get_transfer_encoding();

    void add_file_to_cache(string, vector<unsigned char>, string, string, string, size_t, size_t);
    void delete_file(string);

private:
    bool holding_data();
    string get_oldest_file();
    size_t get_current_size_of_cache();
    bool test_add(size_t);

    static boost::mutex write_mutex_;
    sqlite3 *db_connection_;
    sqlite3_stmt *curr_stmt;

    vector<unsigned char> binary_buff_;
    string text_buff_;
    const size_t MAX_SIZE_;
    string curr_file_type_, curr_encoding_type_, curr_transfer_encoding_type_;

};

#endif // CACHEMANAGER_H
