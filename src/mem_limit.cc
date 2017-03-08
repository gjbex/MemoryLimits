#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <string.h>
#include <unistd.h>
#include <mpi.h>
#include <omp.h>

size_t convert_size(char *size_spec);
long convert_time(char *time_spec);
char* allocate_memory(size_t size);
void fill_memory(char *buffer, size_t size);

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
    if (rank == root) {
        char opt {'\0'};
        while ((opt = getopt(argc, argv, "f:t:m:i:s:l:vh")) != -1) {
            switch (opt) {
                case 'f':
                    conf_file_name = optarg;
                    name_length = strlen(conf_file_name);
                    break;
                case 't':
                    nr_threads = atoi(optarg);
                    break;
                case 'm':
                    max_size = convert_size(optarg);
                    break;
                case 'i':
                    increment = convert_size(optarg);
                    break;
                case 's':
                    sleeptime = convert_time(optarg);
                    break;
                case 'l':
                    lifetime = convert_time(optarg);
                case 'v':
                    is_verbose = 1;
                    break;
                case 'h':
                    // TODO
                    break;
                default:
                    // TODO
                    break;
            }
        }
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
        for (size_t mem = increments[thread_nr];
                mem <= max_sizes[thread_nr]; mem += increments[thread_nr]) {
            std::stringstream msg;
            msg << "rank " << rank << "#" << thread_nr << ": "
                << "allcocating " << mem << " bytes" << std::endl;
            std::cout << msg.str();
            char *buffer = allocate_memory(mem);
            msg.str("");
            msg << "rank " << rank << "#" << thread_nr << ": "
                << "filling " << mem << " bytes" << std::endl;
            std::cout << msg.str();
            fill_memory(buffer, mem);
            std::chrono::microseconds period(sleeptimes[thread_nr]);
            std::this_thread::sleep_for(period);
            delete[] buffer;
        }
    }
    std::chrono::microseconds period(lifetime);
    std::this_thread::sleep_for(period);
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == root) {
        std::stringstream msg;
        msg << "succesfully done" << std::endl;
        std::cout << msg.str();
    }
    MPI_Finalize();
    return 0;
}

size_t convert_size(char *size_spec) {
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

long convert_time(char *time_spec) {
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
    char* buffer = new char[size];
    if (buffer == nullptr)
        throw std::runtime_error("can not allocate mmemory");
    return buffer;
}

void fill_memory(char *buffer, size_t size) {
    char fill = 'A';
    for (size_t i = 0; i < size; i++) {
        buffer[i] = fill;
        fill = fill == 'Z' ? 'A' : fill + 1;
    }
}
