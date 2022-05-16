////////////////////////////////////////////////////////
///
///     "utilites.cpp"
///
///     Implentation of functions defined in utilities.h
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#include "utilities.h"

// Splits a string on the delimiter provided into a vector of strings
vector<string> split_string(string str, char det) {
    string temp;
    bool working = true;
    vector<string> result;
    size_t begin = -1;
    size_t end = str.find(det, begin + 1);
    while (working)
    {
        if (end == (begin + 1)) end = end + 1;
        temp = str.substr(begin + 1, (end - 1) - begin);
        if (temp[0] == det) temp = '\0';
        if (temp[0] != '\0' && temp != " ") result.push_back(temp);
        begin = str.find(det, end - 1);
        if (begin == string::npos) break;
        end = str.find(det, begin + 1);
        if (end == string::npos)
        {
            end = str.size();
        }
    }

    return result;
}

// Splits a string on the delimiter provided into a vector of strings
vector<string> split_string(string str, char det1, char det2) {
    string temp;
    bool working = true;
    vector<string> result;
    size_t temp1, temp2;
    size_t begin = -1;
    temp1 = str.find(det1, begin + 1);
    temp2 = str.find(det2, begin + 1);
    size_t end = temp1 < temp2 && temp1 != string::npos ? temp1 : temp2;
    while (working)
    {
        if (end == (begin + 1)) end = end + 1;
        temp = str.substr(begin + 1, (end - 1) - begin);
        if (temp[0] == det1 || temp[0] == det2) temp = '\0';
        if (temp[0] != '\0' && temp != " ") result.push_back(temp);
        temp1 = str.find(det1, end - 1);
        temp2 = str.find(det2, end - 1);
        begin = temp1 < temp2 && temp1 != string::npos ? temp1 : temp2;
        if (begin == string::npos) break;
        temp1 = str.find(det1, begin + 1);
        temp2 = str.find(det2, begin + 1);
        end = temp1 < temp2 && temp1 != string::npos ? temp1 : temp2;
        if (end == string::npos) end = str.size();
    }

    return result;
}

// Replace all occurrences of a character in a string
std::string replace_all(string str, const string& from, const string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

// Get the current date and time as a string
std::string get_current_date_and_time(string sep) {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    string format = "%Y/%m/%d" + sep + "%X";
    strftime(buf, sizeof(buf), format.c_str(), &tstruct);
    string date(buf);
    string time_zone(*tzname);
    vector<string> time_zone_split = split_string(time_zone, ' ');
    string time_zone_abbr;
    for_each(time_zone_split.begin(), time_zone_split.end(), [&time_zone_abbr](string str) {
        time_zone_abbr += str[0];
    });
    date.append(' ' + time_zone_abbr);
    return date;
}

// Get the time in seconds since epoch (January 1, 1970 midnight UTC/GMT)
double time_since_epoch(std::chrono::high_resolution_clock::time_point* t) {
    using Clock = std::chrono::high_resolution_clock;
    return std::chrono::duration<double>((t != nullptr ? *t : Clock::now() ).time_since_epoch()).count();
}
