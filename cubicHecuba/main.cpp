#include <iostream>
#include <string>
#include <map>
#include <random>
#include <algorithm>

#include <cassandra.h>
#include "gtest/gtest.h"
#include <CacheTable.h>
#include <StorageInterface.h>
#include <Writer.h>

#include <mpi.h>


std::string random_string() {
    std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    std::random_device rd;
    std::mt19937 generator(rd());

    std::shuffle(str.begin(), str.end(), generator);

    return str.substr(0, 32);    // assumes 32 < number of characters in str
}


std::string rand_string_len(int len) {
    std::string result = "";

    while (result.size() < len) {
        result += random_string();
    }

    return result.substr(0, len);
}

void run_query(std::string &query, StorageInterface *cc) {
    CassStatement *statement = cass_statement_new(query.c_str(), 0);
    cass_statement_set_consistency(statement, CASS_CONSISTENCY_QUORUM);

    CassFuture *result_future = cass_session_execute(const_cast<CassSession *>(cc->get_session()), statement);
    cass_statement_free(statement);

    CassError rc = cass_future_error_code(result_future);
    if (rc != CASS_OK) {
        printf("Query execution error: %s - %s\n", cass_error_desc(rc), query.c_str());


    }
    cass_future_free(result_future);
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int num_processes;
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::string usage = "program contact_names num_partitions_keys num_clustering_keys num_bytes";
    std::string defaults = "defaults are: 127.0.0.1 128 1024 128";

    if (argc == 1 || argc == 3 || argc == 5 || argc == 6) {
        std::cout << usage << std::endl << defaults << std::endl;
        if (argc != 1) return 0;
    }

    std::string nodeX = "127.0.0.1";

    if (argc > 1) nodeX = std::string(argv[1]);

    int clustering_keys = 1024;
    int partition_keys = 128;

    if (argc > 3) {
        partition_keys = std::atoi(argv[2]);
        clustering_keys = std::atoi(argv[3]);
    }

    int num_bytes = 128;

    if (argc > 4) {
        num_bytes = std::atoi(argv[4]);
    }

    int numCols = 1;

    if (argc > 5) {
        numCols = std::atoi(argv[5]);
    }

    char *keyspace = "testCubic";
    std::string tableName = "cubicTable";

    StorageInterface *cluster = new StorageInterface(9042, "");

    std::vector<std::map<std::string, std::string>> columns_names(numCols);
    columns_names = {{}};

    if (rank == 0) {

        std::string query0 = "DROP KEYSPACE IF EXISTS testCubic;";
        run_query(query0, cluster);

        std::string query1 = "CREATE KEYSPACE IF NOT EXISTS " + std::string(keyspace) +
                             " WITH replication = {'class': 'SimpleStrategy', 'replication_factor': 1};";


        run_query(query1, cluster);

        std::string col_table;
        for (int i = 0; i < numCols; ++i) {
            col_table = "value" + std::to_string(i);
            columns_names[0].insert(make_pair("name", col_table));
            col_table = col_table + " double, ";
        }

        std::string query2 = "CREATE TABLE IF NOT EXISTS " + std::string(keyspace) + "." + tableName +
                             "(key1 double, key2 double, " + col_table + "PRIMARY KEY(key1, key2));";


        run_query(query2, cluster);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    std::vector<std::map<std::string, std::string>> keys_names = {{{"name", "key1"}},
                                                                  {{"name", "key2"}}};
    TableMetadata *table_meta = new TableMetadata(tableName.c_str(), keyspace, keys_names, columns_names,
                                                  cluster->get_session());

    std::map<std::string, std::string> config = {};
    TupleRowFactory *KeyFactory = new TupleRowFactory(table_meta->get_keys());
    TupleRowFactory *ValueFactory = new TupleRowFactory(table_meta->get_values());
    Writer *W = new Writer(table_meta, (CassSession *) cluster->get_session(), config);


    //Use a non persistent dict
    //
    double *somek = nullptr;
    void *value = nullptr;
    std::cout << "HEY" << std::endl;
    for (int key = 0; key < partition_keys * clustering_keys; ++key) {
        somek = (double *) malloc(sizeof(double));
        *somek = key;
        value = malloc(num_bytes);
        memcpy(value, rand_string_len(num_bytes / 4).c_str(), num_bytes);
        std::cout << "TEXT IS: " << std::string((char *) value) << std::endl;
        W->write_to_cassandra(KeyFactory->make_tuple(somek), ValueFactory->make_tuple(value));
    }

    delete (W);
    delete (cluster);

    MPI_Finalize();

}

