#include <algorithm>
#include <iostream>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

size_t convert_size(char *size_spec);

int main(int argc, char *argv[]) {
    const int root {0};
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    char *conf_file_name {nullptr};
    unsigned sleeptime {0};
    size_t increment {0};
    size_t max_size {0};
    int is_verbose {0};
    int name_length {0};
    if (rank == root) {
        char opt {'\0'};
        while ((opt = getopt(argc, argv, "f:m:i:s:vh")) != -1) {
            switch (opt) {
                case 'f':
                    conf_file_name = optarg;
                    name_length = strlen(conf_file_name);
                    break;
                case 'm':
                    max_size = convert_size(optarg);
                    break;
                case 'i':
                    increment = convert_size(optarg);
                    break;
                case 's':
                    sleeptime = std::stoi(optarg);
                    break;
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
            msg << "rank: " << rank << ": "
                << "'" << conf_file_name << "'" << std::endl;
            std::cerr << msg.str();
        }
    } else {
        MPI_Bcast(&max_size, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&increment, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&sleeptime, 1, MPI_INT, root, MPI_COMM_WORLD);
        if (is_verbose) {
            std::stringstream msg;
            msg << "rank: " << rank << ": "
                << "max. size = " << max_size << ", "
                << "increment = " << increment << ", "
                << "sleep time = " << sleeptime << std::endl;
            std::cerr << msg.str();
        }
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
