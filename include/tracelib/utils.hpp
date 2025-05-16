#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

/**
 * @brief Forms a human readable string out of size in bytes.
 * @param bytes size of the file
 * @return formated human readable string
 */
inline std::string formatFileSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && unitIndex < 8) {
        size /= 1024.0;
        unitIndex++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

/**
 * @brief Retrieves file size in bytes
 * @param file file path
 * @return size in bytes
 */
inline size_t getFileSize(const std::string& file) {
    std::filesystem::path filePath = file;
    if (std::filesystem::exists(filePath)) {
        return std::filesystem::file_size(filePath);
    }
    return -1;
}

struct PairHash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

class TimestampManager {
public:
    void start(const std::string& name) {
        const auto now = std::chrono::high_resolution_clock::now();
        if (timers.find(name) == timers.end()) {
            originalOrder.push_back(name);
            timers[name] = {now, std::chrono::nanoseconds(0)};
        } else {
            timers[name].start_time = now;
        }
    }

    void stop(const std::string& name) {
        const auto now = std::chrono::high_resolution_clock::now();
        if (timers.find(name) != timers.end()) {
            timers[name].total_duration += now - timers[name].start_time;
        } else {
            std::cerr << "[E]: Timer " << name << " was not started!" << std::endl;
        }
    }

    void report() const {
        for (const auto& name: originalOrder) {
            std::cout << "Timer: " << name << " - Duration: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(timers.find(name)->second.total_duration).count()
                      << " microsec" << std::endl;
        }
    }

private:
    struct TimerData {
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::nanoseconds total_duration{};
    };
    std::vector<std::string> originalOrder;
    std::unordered_map<std::string, TimerData> timers;
};


template<class TreeNode>
class Operation {
public:
    enum Type {
        REMOVE,
        INSERT,
        UPDATE,
        MATCH
    };

    Type type;
    TreeNode* arg1;
    TreeNode* arg2;

    explicit Operation(Type op, TreeNode* arg1 = nullptr, TreeNode* arg2 = nullptr): type(op), arg1(arg1), arg2(arg2) {}

    bool operator==(const Operation& other) const {
        return type == other.type && arg1 == other.arg1 && arg2 == other.arg2;
    }

    std::string toString() const {
        std::stringstream sstream;
        switch (this->type) {
            case Operation<TreeNode>::REMOVE: {
                sstream << "<Operation Remove: " << this->arg1->functionName << ">";
                break;
            }
            case Operation<TreeNode>::INSERT: {
                sstream << "<Operation Insert: " << this->arg2->functionName << ">";
                break;
            }
            case Operation<TreeNode>::UPDATE: {
                sstream << "<Operation Update: " << this->arg1->functionName << " to " << this->arg2->functionName << ">";
                break;
            }
            case Operation<TreeNode>::MATCH: {
                sstream << "<Operation Match: " << this->arg1->functionName << " to " << this->arg2->functionName << ">";
                break;
            }
        }
        return sstream.str();
    }
};

template<class TreeNode>
std::ostream& operator<<(std::ostream& os, const Operation<TreeNode>& op) {
    os << op.toString();
    return os;
}


#endif //UTILS_HPP

