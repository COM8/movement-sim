
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <bits/chrono.h>
#include <fcntl.h>
#include <unistd.h>

const off_t AMD_MSR_PWR_UNIT{0xC0010299};
const off_t AMD_MSR_CORE_ENERGY{0xC001029A};
const off_t AMD_MSR_PACKAGE_ENERGY{0xC001029B};

const uint64_t AMD_ENERGY_UNIT_MASK{0x1F00};

struct CPUInfo {
    int dieId{0};
    std::vector<int> coreIds{};

    explicit CPUInfo(int dieId) : dieId(dieId) {}
    CPUInfo() = default;
} __attribute__((aligned(32)));

struct CoreMsrInfo {
    int coreId{0};
    int fd{0};

    double packageWatts{0};
    double coreWatts{0};

    double coreEnergyVal{0};
    double packageEnergyVal{0};

    std::chrono::high_resolution_clock::time_point lastMeasurement{std::chrono::high_resolution_clock::time_point::min()};
} __attribute__((aligned(64)));

struct CPUMsrInfo {
    int dieId{0};
    std::map<int, CoreMsrInfo> coreMsr{};

    explicit CPUMsrInfo(int dieId) : dieId(dieId) {}
    CPUMsrInfo() = default;
} __attribute__((aligned(64)));

struct CoreUtilizationInfo {
    int coreId{0};
    double utilization{0};

    int lastSumWork{0};
    int lastSumAll{0};

    std::chrono::high_resolution_clock::time_point lastMeasurement{std::chrono::high_resolution_clock::time_point::min()};
} __attribute__((aligned(32)));

struct CPUUtilizationInfo {
    int dieId{0};
    std::map<int, CoreUtilizationInfo> coreUtilization{};

    explicit CPUUtilizationInfo(int dieId) : dieId(dieId) {}
    CPUUtilizationInfo() = default;
} __attribute__((aligned(64)));

int read_int(const std::filesystem::path& path) {
    std::ifstream inputFile(path, std::ios::in);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open and read int: " << path << "\n";
        return -1;
    }

    int number = -1;
    inputFile >> number;
    inputFile.close();
    return number;
}

std::vector<CPUInfo> get_cpu_info() {
    const std::filesystem::path CPU_BASE_PATH{"/sys/devices/system/cpu"};
    const std::regex CORE_DIR_NAME_REGEX{"^cpu([0-9]+)$"};

    std::vector<std::pair<int, std::filesystem::path>> corePaths{};
    std::ranges::for_each(std::filesystem::directory_iterator{CPU_BASE_PATH},
                          [&CORE_DIR_NAME_REGEX, &corePaths](const std::filesystem::path& entry) {
                              if (std::filesystem::is_directory(entry)) {
                                  std::cmatch m;
                                  if (std::regex_match(entry.filename().c_str(), m, CORE_DIR_NAME_REGEX)) {
                                      corePaths.emplace_back(std::make_pair(std::stoi(m[1].str()), entry));
                                  }
                              }
                          });

    std::vector<CPUInfo> cpuInfo;
    for (const std::pair<int, std::filesystem::path>& corePath : corePaths) {
        const std::filesystem::path dieIdPath = corePath.second / "topology" / "die_id";

        int dieId = read_int(dieIdPath);
        assert(dieId >= 0);

        bool found = false;
        for (CPUInfo& info : cpuInfo) {
            if (info.dieId == dieId) {
                info.coreIds.push_back(corePath.first);
                found = true;
                break;
            }
        }

        if (!found) {
            CPUInfo info(dieId);
            info.coreIds.push_back(corePath.first);
            cpuInfo.emplace_back(info);
        }
    }
    std::cout << "Found " << cpuInfo.size() << "CPUs\n";
    return cpuInfo;
}

int open_msr(const std::filesystem::path& path) {
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        if (errno == ENXIO) {
            std::cerr << "CPU does not exist: " << path << "\n";
        } else if (errno == EIO) {
            std::cerr << "CPU does not support MSR: " << path << "\n";
        } else {
            std::cerr << "Failed to open: " << path << "\n";
        }
    }
    assert(fd >= 0);
    return fd;
}

uint64_t read_msr(int msrFd, off_t offset) {
    uint64_t result{0};
    if (pread(msrFd, &result, sizeof(result), offset) != sizeof(result)) {
        std::cerr << "Failed to read from MSR file.\n";
    }
    return result;
}

void update_msr_info(std::map<int, CPUMsrInfo>& msrInfo) {
    for (std::pair<const int, CPUMsrInfo>& cpuMsrPair : msrInfo) {
        for (std::pair<const int, CoreMsrInfo>& coreMsrPair : cpuMsrPair.second.coreMsr) {
            uint64_t coreEnergyUnits = read_msr(coreMsrPair.second.fd, AMD_MSR_PWR_UNIT);
            uint64_t energyUnit = (coreEnergyUnits & AMD_ENERGY_UNIT_MASK) >> 8;

            double energyUnitVal = std::pow(0.5, static_cast<double>(energyUnit));

            // std::cout << "Time Unit: " << timeUnitVal << "\nEnergy Unit: " << energyUnitVal << "\nPower Unit: " << powerUnitVal << "\n";

            uint64_t coreEnergyOld = read_msr(coreMsrPair.second.fd, AMD_MSR_CORE_ENERGY);
            uint64_t packageEnergyOld = read_msr(coreMsrPair.second.fd, AMD_MSR_PACKAGE_ENERGY);

            double coreEnergyVal = static_cast<double>(coreEnergyOld) * energyUnitVal;
            double packageEnergyVal = static_cast<double>(packageEnergyOld) * energyUnitVal;

            if (coreMsrPair.second.lastMeasurement != std::chrono::high_resolution_clock::time_point::min()) {
                std::chrono::milliseconds milli = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - coreMsrPair.second.lastMeasurement);
                double modifier = static_cast<double>(milli.count()) / 10;
                coreMsrPair.second.coreWatts = (coreEnergyVal - coreMsrPair.second.coreEnergyVal) * modifier;
                coreMsrPair.second.packageWatts = (packageEnergyVal - coreMsrPair.second.packageEnergyVal) * modifier;

                // std::cout << "Core: " << coreMsrPair.second.coreWatts << " Package: " << coreMsrPair.second.packageWatts << "\n";
            }

            coreMsrPair.second.coreEnergyVal = coreEnergyVal;
            coreMsrPair.second.packageEnergyVal = packageEnergyVal;
            coreMsrPair.second.lastMeasurement = std::chrono::high_resolution_clock::now();
        }
    }
}

void prep_msr_info(const std::vector<CPUInfo>& cpuInfo, std::map<int, CPUMsrInfo>& msrInfo) {
    const std::filesystem::path CPU_MSR_BASE_PATH{"/dev/cpu"};

    for (const CPUInfo& info : cpuInfo) {
        CPUMsrInfo newMsrInfo(info.dieId);
        for (int coreId : info.coreIds) {
            const std::filesystem::path CPU_MSR_PATH = CPU_MSR_BASE_PATH / std::to_string(coreId) / "msr";
            assert(std::filesystem::exists(CPU_MSR_PATH));
            newMsrInfo.coreMsr[coreId] = CoreMsrInfo();
            newMsrInfo.coreMsr[coreId].fd = open_msr(CPU_MSR_PATH);
        }
        msrInfo[info.dieId] = newMsrInfo;
    }
}

std::string get_time_stamp() {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    std::chrono::days d = std::chrono::duration_cast<std::chrono::days>(tp.time_since_epoch());
    tp -= d;
    std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(tp.time_since_epoch());
    tp -= h;
    std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(tp.time_since_epoch());
    tp -= m;
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
    tp -= s;
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    tp -= ms;

    return std::to_string(h.count()) + ":" + std::to_string(m.count()) + ":" + std::to_string(s.count()) + "." + std::to_string(ms.count());
}

void export_data(const std::map<int, CPUMsrInfo>& msrInfo, const std::map<int, CPUUtilizationInfo>& utilizationInfo, const std::filesystem::path& outFilePath) {
    std::vector<double> packageAvg(msrInfo.size(), 0);
    std::vector<double> coreAvg(msrInfo.size(), 0);
    std::vector<double> sum(msrInfo.size(), 0);
    std::vector<double> utilization(msrInfo.size(), 0);

    for (const std::pair<const int, CPUMsrInfo>& cpuMsrPair : msrInfo) {
        for (const std::pair<const int, CoreMsrInfo>& coreMsrPair : cpuMsrPair.second.coreMsr) {
            packageAvg[cpuMsrPair.first] += coreMsrPair.second.packageWatts;
            coreAvg[cpuMsrPair.first] += coreMsrPair.second.coreWatts;
        }

        packageAvg[cpuMsrPair.first] /= static_cast<double>(cpuMsrPair.second.coreMsr.size());
        coreAvg[cpuMsrPair.first] /= static_cast<double>(cpuMsrPair.second.coreMsr.size());
        sum[cpuMsrPair.first] = packageAvg[cpuMsrPair.first] + coreAvg[cpuMsrPair.first];
    }

    for (const std::pair<const int, CPUUtilizationInfo>& cpuUtilizationPair : utilizationInfo) {
        for (const std::pair<const int, CoreUtilizationInfo>& coreUtilizationPair : cpuUtilizationPair.second.coreUtilization) {
            utilization[cpuUtilizationPair.first] += coreUtilizationPair.second.utilization;
        }
        utilization[cpuUtilizationPair.first] /= static_cast<double>(cpuUtilizationPair.second.coreUtilization.size());
        utilization[cpuUtilizationPair.first] = std::min(utilization[cpuUtilizationPair.first], 100.0);  // Cap at 100%
    }

    std::string result = get_time_stamp();
    for (size_t i = 0; i < packageAvg.size(); i++) {
        result += ";" + std::to_string(packageAvg[i]);
        result += ";" + std::to_string(coreAvg[i]);
        result += ";" + std::to_string(sum[i]);
        result += ";" + std::to_string(utilization[i] * 100);
    }
    std::cout << result + "\n";

    std::ofstream outputFile(outFilePath, std::ios::app);
    assert(outputFile.is_open());
    outputFile << result << "\n";
    outputFile.close();
}

void prep_utilization_info(const std::vector<CPUInfo>& cpuInfo, std::map<int, CPUUtilizationInfo>& utilizationInfo) {
    for (const CPUInfo& info : cpuInfo) {
        CPUUtilizationInfo newUtilizationInfo(info.dieId);
        for (int coreId : info.coreIds) {
            newUtilizationInfo.coreUtilization[coreId] = CoreUtilizationInfo();
        }
        utilizationInfo[info.dieId] = newUtilizationInfo;
    }
}

void update_utilization_info(std::map<int, CPUUtilizationInfo>& utilizationInfo) {
    const std::filesystem::path CPU_MSR_BASE_PATH{"/proc/stat"};
    const std::regex CORE_DIR_NAME_REGEX{"^cpu([0-9]+)$"};

    std::ifstream inputFile(CPU_MSR_BASE_PATH, std::ios::in);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open and read int: " << CPU_MSR_BASE_PATH << "\n";
        return;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);

        std::vector<std::string> words;
        std::string word;
        while (std::getline(iss, word, ' ')) {
            word.erase(std::remove_if(word.begin(), word.end(), ispunct), word.end());
            words.push_back(word);
        }

        if (words.size() == 11) {
            std::cmatch m;
            if (std::regex_match(words[0].c_str(), m, CORE_DIR_NAME_REGEX)) {
                int coreId = std::stoi(m[1].str());

                int sumWork = std::stoi(words[1]) + std::stoi(words[2]) + std::stoi(words[3]);
                int sumAll{0};
                for (size_t i = 1; i < words.size(); i++) {
                    sumAll += std::stoi(words[i]);
                }

                for (std::pair<const int, CPUUtilizationInfo>& cpuUtilizationPair : utilizationInfo) {
                    bool found = false;
                    for (std::pair<const int, CoreUtilizationInfo>& coreUtilizationPair : cpuUtilizationPair.second.coreUtilization) {
                        if (coreId == coreUtilizationPair.first) {
                            if (coreUtilizationPair.second.lastMeasurement != std::chrono::high_resolution_clock::time_point::min()) {
                                int divSumWork = sumWork - coreUtilizationPair.second.lastSumWork;
                                int divSumAll = sumAll - coreUtilizationPair.second.lastSumAll;

                                coreUtilizationPair.second.utilization = static_cast<double>(divSumWork) / static_cast<double>(divSumAll);
                                // std::cout << "Util: " << coreUtilizationPair.second.utilization * 100 << "%\n";
                            }

                            coreUtilizationPair.second.lastSumWork = sumWork;
                            coreUtilizationPair.second.lastSumAll = sumAll;
                            coreUtilizationPair.second.lastMeasurement = std::chrono::high_resolution_clock::now();
                            found = true;
                        }
                    }

                    if (found) {
                        break;
                    }
                }
            }
        }

        // process pair (a,b)
    }
    inputFile.close();
}

int main() {
    const std::filesystem::path OUT_FILE_PATH{"data.txt"};
    const std::vector<CPUInfo> cpuInfo = get_cpu_info();

    std::map<int, CPUUtilizationInfo> utilizationInfo;
    prep_utilization_info(cpuInfo, utilizationInfo);
    update_utilization_info(utilizationInfo);

    std::map<int, CPUMsrInfo> msrInfo;
    prep_msr_info(cpuInfo, msrInfo);
    update_msr_info(msrInfo);

    while (true) {
        // Measure the change after 100 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        update_msr_info(msrInfo);
        update_utilization_info(utilizationInfo);
        export_data(msrInfo, utilizationInfo, OUT_FILE_PATH);
    }

    return EXIT_SUCCESS;
}