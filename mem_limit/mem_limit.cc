#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <mpi.h>
#include <omp.h>

// exit codes for application
const int EXIT_OPT_ERROR {1};
const int EXIT_CONFIG_ERROR {2};
const int EXIT_MEM_ERROR {3};

size_t convert_size(const char *size_spec);
long convert_time(const char *time_spec);
char* allocate_memory(size_t size);
void fill_memory(char *buffer, size_t size);
void parse_config(const std::string& file_name, int target_line_nr,
                  int& nr_threads, size_t** max_sizes, size_t** increments,
                  long** sleeptimes);
void print_help();

int main(int argc, char *argv[]) {
    const int root {0};
    MPI_Init(&argc, &argv);
    int rank {0};
    int size {1};
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    char *conf_file_name {nullptr};
    int nr_threads {1};
    size_t max_size {0};
    size_t *max_sizes {nullptr};
    size_t increment {0};
    size_t *increments {nullptr};
    long sleeptime {0};
    long *sleeptimes {nullptr};
    long lifetime {0};
    int is_verbose {0};
    int name_length {0};
    int is_done = 0;
    if (rank == root) {
        bool opt_sufficient {false};
        char opt {'\0'};
        while ((opt = getopt(argc, argv, "f:t:m:i:s:l:vh")) != -1) {
            try {
                switch (opt) {
                    case 'f':
                        conf_file_name = optarg;
                        name_length = strlen(conf_file_name);
                        opt_sufficient = true;
                        break;
                    case 't':
                        nr_threads = atoi(optarg);
                        break;
                    case 'm':
                        max_size = convert_size(optarg);
                        opt_sufficient = true;
                        break;
                    case 'i':
                        increment = convert_size(optarg);
                        break;
                    case 's':
                        sleeptime = convert_time(optarg);
                        break;
                    case 'l':
                        lifetime = convert_time(optarg);
                        break;
                    case 'v':
                        is_verbose = 1;
                        break;
                    case 'h':
                        print_help();
                        opt_sufficient = 1;
                        is_done = 1;
                        break;
                    default:
                        std::stringstream msg;
                        msg << "# error: unknown option '-" << opt << "'"
                            << std::endl;
                        std::cerr << msg.str();
                        // TODO: call print_help
                        MPI_Abort(MPI_COMM_WORLD, EXIT_OPT_ERROR);
                }
            } catch (const std::invalid_argument& e) {
                    std::stringstream msg;
                    msg << "# error: invalid option value, " << e.what()
                        << std::endl;
                    std::cerr << msg.str();
                    print_help();
                    is_done = 1;
            }
        }
        if (!opt_sufficient) {
            std::stringstream msg;
            msg << "# error: expecting at least -f or -m option"
                << std::endl;
            std::cerr << msg.str();
            print_help();
            is_done = 1;
        }
    }
    MPI_Bcast(&is_done, 1, MPI_INT, root, MPI_COMM_WORLD);
    if (is_done) {
        MPI_Finalize();
        return 0;
    }
    MPI_Bcast(&is_verbose, 1, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Bcast(&name_length, 1, MPI_INT, root, MPI_COMM_WORLD);
    if (name_length > 0) {
        if (rank != root) {
            conf_file_name = new char[name_length + 1];
        }
        MPI_Bcast(conf_file_name, name_length + 1, MPI_CHAR,
                  root, MPI_COMM_WORLD);
        if (is_verbose) {
            std::stringstream msg;
            msg << "rank " << rank << ": "
                << "'" << conf_file_name << "'" << std::endl;
            std::cerr << msg.str();
        }
        try {
            parse_config(conf_file_name, rank, nr_threads, &max_sizes,
                         &increments, &sleeptimes);
        } catch (const std::exception& e) {
            std::stringstream msg;
            msg << "# error: invalid configuration file, " << e.what()
                << std::endl;
            std::cerr << msg.str();
            MPI_Abort(MPI_COMM_WORLD, EXIT_CONFIG_ERROR);
        }
    } else {
        MPI_Bcast(&nr_threads, 1, MPI_INT, root, MPI_COMM_WORLD);
        MPI_Bcast(&max_size, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&increment, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&sleeptime, 1, MPI_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&lifetime, 1, MPI_LONG, root, MPI_COMM_WORLD);
        if (is_verbose) {
            std::stringstream msg;
            msg << "rank " << rank << ": "
                << "threads = " << nr_threads << ", "
                << "max. size = " << max_size << ", "
                << "increment = " << increment << ", "
                << "sleep time = " << sleeptime << std::endl;
            std::cerr << msg.str();
        }
        max_sizes = new size_t[nr_threads];
        increments = new size_t[nr_threads];
        sleeptimes = new long[nr_threads];
        for (int i = 0; i < nr_threads; i++) {
            max_sizes[i] = max_size;
            increments[i] = increment;
            sleeptimes[i] = sleeptime;
        }
    }
    int max_threads =  omp_get_max_threads();
    if (max_threads < nr_threads) {
        std::stringstream msg;
        msg << "# warming rank " << rank << ": "
            << "nr. threads " << nr_threads
            << " exceeds max. threads " << max_threads
            << std::endl;
        std::cout << msg.str();
    }
    omp_set_num_threads(nr_threads);
#pragma omp parallel
    {
        int thread_nr = omp_get_thread_num();
        size_t increment = increments[thread_nr] > 0 ?
            increments[thread_nr] : max_sizes[thread_nr];
        for (size_t mem = increment; mem <= max_sizes[thread_nr];
                mem += increment) {
            int cpu_nr = sched_getcpu();
            std::stringstream msg;
            msg << "rank " << rank << "#" << thread_nr
                << " on " << cpu_nr << ": "
                << "allocating " << mem << " bytes" << std::endl;
            std::cout << msg.str();
            try {
                char *buffer = allocate_memory(mem);
                msg.str("");
                msg << "rank " << rank << "#" << thread_nr << ": "
                    << "filling " << mem << " bytes" << std::endl;
                std::cout << msg.str();
                fill_memory(buffer, mem);
                std::chrono::microseconds period(sleeptimes[thread_nr]);
                std::this_thread::sleep_for(period);
                delete[] buffer;
            } catch (const std::runtime_error& e) {
                std::stringstream msg;
                msg << "# error: allocation of " << mem << " bytes failed"
                    << std::endl;
                std::cerr << msg.str();
                MPI_Abort(MPI_COMM_WORLD, EXIT_MEM_ERROR);
            }
        }
    }
    std::chrono::microseconds period(lifetime);
    std::this_thread::sleep_for(period);
    delete[] max_sizes;
    delete[] increments;
    delete[] sleeptimes;
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == root) {
        std::stringstream msg;
        msg << "succesfully done" << std::endl;
        std::cout << msg.str();
    }
    MPI_Finalize();
    return 0;
}

size_t convert_size(const char *size_spec) {
    std::stringstream stream;
    stream.str(size_spec);
    size_t number {0};
    stream >> number;
    std::string unit;
    stream >> unit;
    if (unit != "") {
        std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
        if (unit == "kb") {
            number *= 1024;
        } else if (unit == "mb") {
            number *= 1024*1024;
        } else if (unit == "gb") {
            number *= 1024*1024*1024;
        } else if (unit != "b") {
            throw std::invalid_argument("unknown unit");
        }
    }
    return number;
}

long convert_time(const char *time_spec) {
    std::stringstream stream;
    stream.str(time_spec);
    long number {0};
    stream >> number;
    std::string unit;
    stream >> unit;
    if (unit != "") {
        std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
        if (unit == "s") {
            number *= 1000000;
        } else if (unit == "ms") {
            number *= 1000;
        } else if (unit == "m") {
            number *= 60*1000000;
        } else if (unit != "us") {
            throw std::invalid_argument("unknown unit");
        }
    }
    return number;
}

char* allocate_memory(size_t size) {
    char* buffer {NULL};
    if ((buffer = (char*) malloc(size*sizeof(char))) == NULL)
        throw std::runtime_error("can allocate memory");
    return buffer;
}

void fill_memory(char *buffer, size_t size) {
    char fill = 'A';
    for (size_t i = 0; i < size; i++) {
        buffer[i] = fill;
        fill = fill == 'Z' ? 'A' : fill + 1;
    }
}

std::vector<std::string> split(const std::string& str,
                               const std::string& delim) {
    std::vector<std::string> parts;
    size_t pos = 0, old_pos = 0;
    while ((pos = str.find(delim, old_pos)) != std::string::npos) {
        parts.push_back(str.substr(old_pos, pos - old_pos));
        old_pos = pos + delim.length();
    }
    parts.push_back(str.substr(old_pos));
    return parts;
}

void parse_config(const std::string& file_name, int target_line_nr,
                  int& nr_threads, size_t** max_sizes, size_t** increments,
                  long** sleeptimes) {
    std::regex comment_re {R"(^\s*#)"};
    std::regex empty_re {R"(^\s*$)"};
    std::ifstream config_file;
    config_file.open(file_name);
    int line_nr {0};
    std::string line;
    while (config_file) {
        std::string curr_line;
        std::getline(config_file, curr_line);
        std::smatch match;
        if (std::regex_search(curr_line, match, empty_re))
            continue;
        if (std::regex_search(curr_line, match, comment_re))
            continue;
        line = curr_line;
        if (line_nr++ == target_line_nr)
            break;
    }
    config_file.close();
    if (line.length() > 0) {
        std::vector<std::string> parts = split(line, ";");
        nr_threads = std::stoi(parts.at(0));
        *max_sizes = new size_t[nr_threads];
        *increments = new size_t[nr_threads];
        *sleeptimes = new long[nr_threads];
        parts = split(parts.at(1), ":");
        size_t i {0};
        for (i = 0; i < parts.size() && ((int) i) < nr_threads; i++) {
            std::vector<std::string> specs = split(parts.at(i), "+");
            (*max_sizes)[i] = convert_size(specs.at(0).c_str());
            (*increments)[i] = convert_size(specs.at(1).c_str());
            (*sleeptimes)[i] = convert_time(specs.at(2).c_str());
        }
        for (int j = i; j < nr_threads; j++) {
            (*max_sizes)[j] = (*max_sizes)[i - 1];
            (*increments)[j] = (*increments)[i - 1];
            (*sleeptimes)[j] = (*sleeptimes)[i - 1];
        }
    } else {
        // TODO: throw exception
    }
}

void print_help() {
    std::stringstream msg;
    msg << "Usage: mem_limit [-v] [-h] ( "
        << "-f <conf_file> | \\" << std::endl
        << "\t\t( -m <size> [-i <size>] [-s <time>] [-t <n>] )) "
        << "[-l <time>] "
        << std::endl;
    msg << "\t-f <conf_file>: configuration file to use" << std::endl;
    msg << "\t-m <size>: maximum memory size" << std::endl;
    msg << "\t-i <size>: memory increment per step" << std::endl;
    msg << "\t-s <time>: time to sleep between steps" << std::endl;
    msg << "\t-t <n>: number of threads per processs" << std::endl;
    msg << "\t-l <time>: time to stay alive after last step" << std::endl;
    msg << "\t-v: give verbose output" << std::endl;
    msg << "\t-h: show this help message" << std::endl;
    msg << "<size> takes units b, kb, mb, gb, default is bytes"
        << std::endl;
    msg << "<time> takes units m, s, ms, us, default is microseconds"
        << std::endl;
    std::cerr << msg.str();
}
