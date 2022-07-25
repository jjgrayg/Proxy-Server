////////////////////////////////////////////////////////
///
///     "cachemanager.cpp"
///
///     Implementation of functions defined in
///     cachemanager.h
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#include "cachemanager.h"

// Initialize write mutex
boost::mutex CacheManager::write_mutex_ = boost::mutex();

// Constructor for CacheManager
// Creates the necessary .db sqlite file if it does not exist and populates
// the file with proper tables and then initializes the connection to the DB
CacheManager::CacheManager(size_t size) : MAX_SIZE_(size) {
    int result = sqlite3_open_v2("./cache_table.db", &db_connection_,
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (result != SQLITE_OK) {
        result = sqlite3_open_v2("./cache_table.db", &db_connection_,
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
        if (result == SQLITE_OK) {
            sqlite3_stmt* query_result;
            sqlite3_prepare_v2(db_connection_, "CREATE TABLE \"cache_track\" ("
                                               "\"file_name\"	TEXT NOT NULL,"
                                               "\"last_access\"	INTEGER NOT NULL,"
                                               "\"size\"        INTEGER NOT NULL,"
                                               "\"file_type\"   TEXT NOT NULL,"
                                               "\"content_encoding\"    TEXT,"
                                               "\"transfer_encoding\"    TEXT,"
                                               "\"max_age\"     INTEGER NOT NULL,"
                                               "\"age_from\"	INTEGER NOT NULL,"
                                               "PRIMARY KEY(\"file_name\")"
                                               ");", -1, &query_result, NULL);
            sqlite3_step(query_result);
            sqlite3_finalize(query_result);
        }
    }
}

// Closes connection to DB
CacheManager::~CacheManager() {
    sqlite3_close(db_connection_);
}

// Checks if the file is in the cache and updates last access time
int CacheManager::check_cache(string query_file) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_connection_, "SELECT * FROM cache_track WHERE file_name=?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, query_file.c_str(), strlen(query_file.c_str()), NULL);
    if(sqlite3_step(stmt) != SQLITE_DONE && stmt != nullptr) {
        string file_name(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        string file_path = "./cache/" + file_name;
        curr_file_type_ = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        curr_encoding_type_ = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        curr_transfer_encoding_type_ = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        size_t max_age = sqlite3_column_int(stmt, 6);
        size_t age_from = sqlite3_column_int(stmt, 7);

        // Update last access time
        sqlite3_stmt* update;
        sqlite3_prepare_v2(db_connection_, "UPDATE cache_track SET last_access=? WHERE file_name=?", -1, &update, NULL);
        sqlite3_bind_int(update, 1, time_since_epoch());
        sqlite3_bind_text(update, 2, file_name.c_str(), strlen(file_name.c_str()), NULL);
        sqlite3_step(update);

        std::ifstream bin_file;
        bin_file.open(file_path, std::ios::binary | std::ios::in);
        if (bin_file)
            binary_buff_ = vector<unsigned char>((std::istreambuf_iterator<char>(bin_file)), (std::istreambuf_iterator<char>()));

        sqlite3_finalize(update);
        if (max_age < time_since_epoch() - age_from) return 0;
        return 1;
    }
    sqlite3_finalize(stmt);
    return 2;
}

// Returns a copy of the buffer and clears it
vector<unsigned char> CacheManager::consume_binary() {
    if (holding_data()) {
        vector<unsigned char> result = binary_buff_;
        binary_buff_.clear();
        return result;
    }
    return vector<unsigned char>();
}

// Returns member var indicating the MIME type of the file currently held
string CacheManager::get_file_type() {
    return curr_file_type_;
}

// Returns member var indicating content encoding of file currently held
string CacheManager::get_file_encoding() {
    return curr_encoding_type_;
}

// Returns member var indicating transfer encoding of file currently held
string CacheManager::get_transfer_encoding() {
    return curr_transfer_encoding_type_;
}

// Returns true of the buffer is not empty
bool CacheManager::holding_data() {
    return (binary_buff_.size() > 0 || text_buff_.size() > 0);
}

// Adds the file to the cache by first writing the file and, if successful, writes all necessary data
// to the DB
void CacheManager::add_file_to_cache(string file_name, vector<unsigned char> file, string file_type,
                                     string content_encoding, string transfer_encoding, size_t max_age,
                                     size_t age_offset) {
    if(MAX_SIZE_ < file.size()) {
        string oldest_file = get_oldest_file();
        while (!test_add(file.size()) && oldest_file != "" && get_current_size_of_cache() != 0) {
            delete_file(oldest_file);
            oldest_file = get_oldest_file();
        }
        return;
    }
    write_mutex_.lock();
    vector<string> split_file = split_string(file_name, '/');
    string dir_path = "./cache";
    boost::filesystem::path base(dir_path.c_str());
    if(!(boost::filesystem::exists(base)))
        if (boost::filesystem::create_directory(base)) {}
    string file_path = "./cache/" + file_name;
    for (uint16_t i = 0; i < split_file.size() - 1; ++i) {
        dir_path.append("/" + split_file[i]);
        boost::filesystem::path dir(dir_path.c_str());
        if(!(boost::filesystem::exists(dir)))
            if (boost::filesystem::create_directory(dir)) {}
    }
    string oldest_file = get_oldest_file();
    while (!test_add(file.size()) && oldest_file != "" && get_current_size_of_cache() != 0) {
        write_mutex_.unlock();
        delete_file(oldest_file);
        oldest_file = get_oldest_file();
        write_mutex_.lock();
    }

    std::ofstream out_file;
    out_file.open(file_path, std::ios::binary);
    if (!out_file) {
        write_mutex_.unlock();
        return;
    }
    char* binaryArr = new char[file.size()];
    std::copy(file.begin(), file.end(), binaryArr);

    out_file.write(binaryArr, file.size());
    out_file.close();

    double time = time_since_epoch();
    size_t size = file.size();
    sqlite3_stmt* query_result;
    sqlite3_prepare_v2(db_connection_, "INSERT INTO cache_track (file_name, last_access, size, file_type, "
                                       "content_encoding, transfer_encoding, max_age, age_from) VALUES(?, ?, ?, ?, ?, ?, ?, ?)", -1, &query_result, NULL);
    sqlite3_bind_text(query_result, 1, file_name.c_str(), file_name.size(), NULL);
    sqlite3_bind_int(query_result, 2, time);
    sqlite3_bind_int(query_result, 3, size);
    sqlite3_bind_text(query_result, 4, file_type.c_str(), file_type.size(), NULL);
    sqlite3_bind_text(query_result, 5, content_encoding.c_str(), content_encoding.size(), NULL);
    sqlite3_bind_text(query_result, 6, transfer_encoding.c_str(), transfer_encoding.size(), NULL);
    int result;
    if (max_age != 0) {
        sqlite3_bind_int(query_result, 7, max_age);
        result = sqlite3_bind_int(query_result, 8, time - age_offset);
    }
    else {
        sqlite3_bind_int(query_result, 7, 78000);
        result = sqlite3_bind_int(query_result, 8, time);
    }
    if (result == SQLITE_OK)
        sqlite3_step(query_result);
    sqlite3_finalize(query_result);
    write_mutex_.unlock();
}

// Returns the current oldest file held in the DB
string CacheManager::get_oldest_file() {
    sqlite3_stmt* query_result;
    string result_str;
    sqlite3_prepare_v2(db_connection_, "SELECT * FROM cache_track ORDER BY last_access ASC, size DESC", -1, &query_result, NULL);
    if (sqlite3_step(query_result) != SQLITE_DONE) {
        result_str = string(reinterpret_cast<const char*>(sqlite3_column_text(query_result, 0)));
    }
    return result_str;
}

// Deletes a file by first deleting the actual file, then removing it from the DB
void CacheManager::delete_file(string file) {
    write_mutex_.lock();
    int status;
    string file_path = "./cache/" + file;
    status = remove(file_path.c_str());
    if(status == 0 || status == -1) {
        sqlite3_stmt* query_result;
        sqlite3_prepare_v2(db_connection_, "DELETE FROM cache_track WHERE file_name=?;", -1, &query_result, NULL);
        sqlite3_bind_text(query_result, 1, file.c_str(), strlen(file.c_str()), NULL);
        sqlite3_step(query_result);
        sqlite3_finalize(query_result);
    }
    write_mutex_.unlock();
}

// Gets current total size of all files held in cache
size_t CacheManager::get_current_size_of_cache() {
    sqlite3_stmt* query_result;
    sqlite3_prepare_v2(db_connection_, "SELECT SUM(size) FROM cache_track", -1, &query_result, NULL);
    sqlite3_step(query_result);
    int total_size = sqlite3_column_int(query_result, 0);
    sqlite3_finalize(query_result);
    return total_size;
}

// Tests if adding a new file of size 'file_size' will push cache size over alloted size
bool CacheManager::test_add(size_t file_size) {
    int cache_size = MAX_SIZE_;
    if (cache_size < static_cast<int>(get_current_size_of_cache() + file_size)) return false;
    else return true;
}
