// Project Identifier: C0F4DFE8B340D81183C208F70F9D2D797908754D

#include <vector>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cassert>
#include <string>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <map>

#include <getopt.h>
#include "TableEntry.h"

using namespace std;
using std::cout;

enum class State : uint8_t { Hash, BST, None };

template<typename valueType>
class Compare {
public:
    Compare(string& operation_In, TableEntry& value_In, size_t colInd_In)
        : tEntry(value_In), operation(operation_In), colInd(colInd_In) {}

    bool operator()(const vector<TableEntry> t) const {
        // call the compareHelper function to compare the tableEntry type
        if (operation == "=") {
            return t[colInd] == tEntry;
        }
        else if (operation == "<") {
            return t[colInd] < tEntry;
        }
        else {
            return t[colInd] > tEntry;
        }
    }
private:
    TableEntry tEntry;
    string operation;
    size_t colInd;
};

class TableObject {
    friend class Database;

private:
    vector < vector<TableEntry> > table;
    unordered_map<string, size_t> colNameInd;

    unordered_map<TableEntry, vector<size_t>> hashInd;
    map<TableEntry, vector<size_t>> bstInd;

    unordered_map<TableEntry, vector<size_t>> tempHashInd;

    vector<string> colNames;
    vector<string> colTypes;
    string name;
    string indexGenCol;
    State state = State::None;

public:
    TableObject() {}

    TableObject(string& nameIn, size_t numColIn)
        : name(nameIn)
    {
        colNames.reserve(numColIn);
        colTypes.reserve(numColIn);
        table.reserve(numColIn);
    }

    void addColType(string& type) {
        colTypes.push_back(type);
    }

    void addColName(string& name, size_t i) {
        colNameInd.insert({ name, i });
        colNames.push_back(name);
    }

    TableEntry tableEntryMaker(string& type, string& value) {
        if (type == "int") {
            return TableEntry(stoi(value));
        }
        else if (type == "double") {
            return TableEntry(stod(value));
        }
        else if (type == "bool") {
            if (value == "true") {
                return TableEntry(true);
            }
            else {
                return TableEntry(false);
            }
        }
        else {
            return TableEntry(value);
        }
    }

    void printRows2(size_t numColPrint, bool quiet) {
        vector<string> colNamesPrint;
        string colName;
        string output;

        
        for (size_t i = 0; i < numColPrint; ++i) {
            cin >> colName;
            if (colNameInd.find(colName) != colNameInd.end()) {
                colNamesPrint.push_back(colName);
            }
            else {
                string junk;
                getline(cin, junk);
                cout << "Error during PRINT: "
                    << colName << " does not name a column in " << name << "\n";
                return;
            }

            if (!quiet) {
                output += (colName + " ");
            }
        }

        string printCOND; // to store WHERE or ALL
        cin >> printCOND;

        if (printCOND == "ALL") {
            printRowsAll(colNamesPrint, quiet, output);
        }
        
        else if (printCOND == "WHERE") {
            printRowsWhere1(colNamesPrint, quiet, output);
        }
    }

    void printRowsAll(vector<string>& colNamesPrint, bool quiet, string& output) {
        size_t printedRow = 0;

        if (!quiet) {
            cout << output << "\n";

            for (auto& t : table) {
                printedRow++;
                for (auto& item : colNamesPrint) {
                    cout << t[colNameInd[item]] << " ";

                }
                cout << "\n";

            }
        }
        else {
            printedRow = table.size();
        }
        cout << "Printed " << printedRow << " matching rows from " << name << "\n";
    }

    void printRowsWhere1(vector<string>& colNamesPrint, bool quiet, string& output) {
        string colName, operation, value;
        cin >> colName >> operation >> value;
        if (colNameInd.find(colName) == colNameInd.end()) {
            string junk;
            getline(cin, junk);
            cout << "Error during PRINT: "
                << colName << " does not name a column in " << name << "\n";
            return;
        }
        string type = colTypes[colNameInd[colName]];
        size_t printedRow = 0;

        if (!quiet) {
            cout << output << "\n";
        }

        if (indexGenCol == colName && state == State::Hash && operation == "=") {
            TableEntry t(tableEntryMaker(type, value));
            if (!quiet) {
                for (size_t i = 0; i < hashInd[t].size(); ++i) {
                    for (auto& j : colNamesPrint) {
                        cout << table[hashInd[t][i]][colNameInd[j]] << " ";
                    }
                    cout << "\n";
                    printedRow++;
                }
            }
            else {
                printedRow = hashInd[t].size();
            }
            cout << "Printed " << printedRow << " matching rows from " << name << "\n";
        }
        else if (indexGenCol == colName && state == State::BST) {
            TableEntry t(tableEntryMaker(type, value));
            if (operation == "<") {
                auto it = bstInd.begin();
                while (it != bstInd.end() && it->first < t) {
                    if (!quiet) {
                        for (size_t i = 0; i < it->second.size(); ++i) {
                            for (auto& j : colNamesPrint) {
                                cout << table[it->second[i]][colNameInd[j]] << " ";
                            }
                            cout << "\n";
                            printedRow++;
                        }
                    }
                    else {
                        printedRow += it->second.size();
                    }
                    ++it;
                }
            }
            else if (operation == ">") {
                auto it = bstInd.begin();
                for (; it != bstInd.end(); ++it) {
                    if (it->first > t) {
                        if (!quiet) {
                            for (size_t i = 0; i < it->second.size(); ++i) {
                                for (auto& j : colNamesPrint) {
                                    cout << table[it->second[i]][colNameInd[j]] << " ";
                                }
                                cout << "\n";
                                printedRow++;
                            }
                        }
                        else {
                            printedRow += it->second.size();
                        }
                    }
                }
            }
            else {
                if (!quiet) {
                    for (size_t i = 0; i < bstInd[t].size(); ++i) {
                        for (auto& j : colNamesPrint) {
                            cout << table[bstInd[t][i]][colNameInd[j]] << " ";
                        }
                        cout << "\n";
                        printedRow++;
                    }
                }
                else {
                    printedRow += bstInd[t].size();
                }
            }
            cout << "Printed " << printedRow << " matching rows from " << name << "\n";
        }
        else {
            TableEntry t(tableEntryMaker(type, value));
            Compare<string> compare(operation, t, colNameInd[colName]);
            printRowsWhere2(compare, colNamesPrint, quiet);
        }
    }

    template<typename valueType>
    void printRowsWhere2(Compare<valueType>& compare, vector<string>& colNamesPrint, bool quiet) {
        size_t printedRow = 0;

        for (auto &t : table) {
            if (compare(t)) {
                if (!quiet) {
                    for (auto &item : colNamesPrint) {
                        cout << t[colNameInd[item]] << " ";

                    }
                    cout << "\n";
                }
                printedRow++;
            }
        }
        cout << "Printed " << printedRow << " matching rows from " << name << "\n";
    }

    void insertRows2(size_t numRow) {
        table.reserve(table.size() + numRow);
        for (size_t i = 0; i < numRow; ++i) {
            vector<TableEntry> newVec;
            for (auto &j : colTypes) {
                //fill the vector from cin
                string input;
                cin >> input;
                if (j == "int") {
                    int modInput = stoi(input);
                    newVec.emplace_back(modInput); //emplacing because tableEntry isn't a type, but a variant that accepts many types
                }
                else if (j == "double") {
                    double modInput = stod(input);
                    newVec.emplace_back(modInput);
                }
                else if (j == "bool") {
                    bool modInput;
                    if (input == "true") {
                        modInput = true;
                    }
                    else {
                        modInput = false;
                    }
                    newVec.emplace_back(modInput);
                }
                else {
                    newVec.emplace_back(input);
                }
            }
            //push the vector to the table (table is a vector of vector)
            table.push_back(newVec);
        }

        if (state != State::None) {
            updateIndINS(numRow);
        }
    }

    void deleteRow2(string& colName, string& operation, string& value) {
        string junk;
        if (colNameInd.find(colName) == colNameInd.end()) {
            getline(cin, junk);
            cout << "Error during DELETE: "
                << colName << " does not name a column in " << name << "\n";
            return;
        }
        string type = colTypes[colNameInd[colName]];
        size_t deletedRows = table.size();
        TableEntry t(tableEntryMaker(type, value));
        Compare<string> compare(operation, t, colNameInd[colName]);
        table.erase(remove_if(table.begin(), table.end(), compare), table.end());
        deletedRows -= table.size();
        cout << "Deleted " << deletedRows << " rows from "
            << name << "\n";

        if (state != State::None) {
            updateIndDEL();
        }
    }

    void generateInd2(const string& colName, const string& hashType) {
        string junk;
        
        if (colNameInd.find(colName) == colNameInd.end()) {
            getline(cin, junk);
            cout << "Error during GENERATE: "
                << colName << " does not name a column in " << name << "\n";
            return;
        }
        indexGenCol = colName;
        if (hashType == "hash") {
            if (state == State::BST) {
                bstInd.clear();
            }
            else if (state == State::Hash) {
                hashInd.clear();
            }

            state = State::Hash;
            for (size_t i = 0; i < table.size(); ++i) {
                hashInd[table[i][colNameInd[colName]]].push_back(i);
            }
        }
        else {
            if (state == State::Hash) {
                hashInd.clear();
            }
            else if (state == State::BST) {
                bstInd.clear();
            }

            state = State::BST;
            for (size_t i = 0; i < table.size(); ++i) {
                bstInd[table[i][colNameInd[colName]]].push_back(i);
            }
        }

        cout << "Created " << hashType << " index for table " << name
            << " on column " << colName << "\n";
    }

    void updateIndDEL() {
        if (state == State::Hash) {
            hashInd.clear();
            for (size_t i = 0; i < table.size(); ++i) {
                hashInd[table[i][colNameInd[indexGenCol]]].push_back(i);
            }
        }
        else {
            bstInd.clear();
            for (size_t i = 0; i < table.size(); ++i) {
                bstInd[table[i][colNameInd[indexGenCol]]].push_back(i);
            }
        }
    }

    void updateIndINS(size_t numRow) {
        if (state == State::BST) {
            for (size_t i = table.size() - numRow; i < table.size(); ++i) {
                bstInd[table[i][colNameInd[indexGenCol]]].push_back(i);
            }
        }
        else {
            for (size_t i = table.size() - numRow; i < table.size(); ++i) {
                hashInd[table[i][colNameInd[indexGenCol]]].push_back(i);
            }
        }
    }

    void generateTempInd(string& colName) {
        for (size_t i = 0; i < table.size(); ++i) {
            tempHashInd[table[i][colNameInd[colName]]].push_back(i);
        }
    }

    void clearTempInd() {
        tempHashInd.clear();
    }

    void join2(TableObject& table2, string& colName1, bool quiet) {

        vector<pair<string, size_t>> colList;
        string printCol;
        string junk;
        size_t colSelect, totalCol;

        cin >> totalCol;
        colList.reserve(totalCol);

        size_t printedRow = 0;
        string output;

        for (size_t i = 0; i < totalCol; ++i) {
            cin >> printCol >> colSelect;
            colList.emplace_back(printCol, colSelect);

            if (colSelect == 1) {
                if (colNameInd.find(printCol) == colNameInd.end()) {
                    getline(cin, junk);
                    cout << "Error during JOIN: "
                        << printCol << " does not name a column in " << name << "\n";
                    return;
                }
            }
            else {
                if (table2.colNameInd.find(printCol) == table2.colNameInd.end()) {
                    getline(cin, junk);
                    cout << "Error during JOIN: "
                        << printCol << " does not name a column in " << table2.name << "\n";
                    return;
                }
            }

            if (!quiet) {
                output += (printCol + " ");
            }
            
        }

        if (!quiet) {
            cout << output << "\n";
        }

        for (auto &i : table) {
            TableEntry temp(i[colNameInd[colName1]]);
            if (table2.tempHashInd.find(temp) != table2.tempHashInd.end()) {
                if (!quiet) {
                    for (auto& k : table2.tempHashInd[temp]) {
                        for (auto& l : colList) {
                            if (l.second == 1) {
                                cout << i[colNameInd[l.first]] << " ";
                            }
                            else {
                                cout << table2.table[k][table2.colNameInd[l.first]] << " ";
                            }
                        }
                        cout << "\n";
                        printedRow++;
                    }
                }
                else {
                    printedRow += table2.tempHashInd[temp].size();
                }
            }
        }

        cout << "Printed " << printedRow << " rows from joining " << name << " to " << table2.name << "\n";
    }
};

ostream& operator<< (ostream& os, vector<string>& t) { // printing the strings in a vector with 1 space character in between.
    for (auto &i : t) {
        os << i << " ";
    }
    return os;
}


class Database {
private:
    unordered_map< string, TableObject > hashTable; // key is tableName
    bool quiet = false;
public:
    void quietMode() {
        quiet = true;
    }

    void createTable() {
        string tableName;
        size_t numCol;
        string output;

        cin >> tableName >> numCol;
        if (hashTable.find(tableName) == hashTable.end()) {
            TableObject table(tableName, numCol);
            string input;
            size_t i = 0;
            while (i < numCol) {
                cin >> input;
                table.addColType(input); // adding the column types
                ++i;
            }

            i = 0;
            while (i < numCol) {
                cin >> input;
                table.addColName(input, i); // adding the column names
                output += (input + " ");
                ++i;
            }

            hashTable.insert({ tableName, table });

            cout << "New table " << tableName << " with column(s) " << output << "created\n";
        }
        else {
            string junk;
            getline(cin, junk);
            cout << "Error during CREATE: Cannot create already existing table " << tableName << "\n";
            return;
        }
    }

    void removeTable() {
        string tableName;
        cin >> tableName;
        if (hashTable.find(tableName) != hashTable.end()) {
            hashTable.erase(tableName);
            cout << "Table " << tableName << " deleted\n";
        }
        else {
            string junk;
            getline(cin, junk);
            cout << "Error during REMOVE: "
                << tableName << " does not name a table in the database\n";
            return;
        }
    }

    void printRows1() {
        string junk, tableName;
        size_t numColPrint;
        cin >> junk >> tableName >> numColPrint;

        if (hashTable.find(tableName) != hashTable.end()) {
            hashTable[tableName].printRows2(numColPrint, quiet);
        }
        else {
            getline(cin, junk);
            cout << "Error during PRINT: "
                << tableName << " does not name a table in the database\n";
            return;
        }
    }

    void insertRows1() {
        string junk, tableName;
        size_t numRow;
        cin >> junk >> tableName >> numRow >> junk;

        if (hashTable.find(tableName) != hashTable.end()) {
            size_t startN = 0;
            if (hashTable[tableName].table.size() == 0) {
                startN = 0;
            }
            else {
                startN = hashTable[tableName].table.size();
            }
            hashTable[tableName].insertRows2(numRow);
            cout << "Added " << numRow << " rows to " << tableName << " from position " << startN << " to " << startN + numRow - 1 << "\n";
        }
        else {
            getline(cin, junk);
            cout << "Error during INSERT: "
                << tableName << " does not name a table in the database\n";
            return;
        }
    }

    void deleteRow1() {
        string junk, tableName, colName, operation, value;
        cin >> junk >> tableName >> junk >> colName >> operation >> value;

        if (hashTable.find(tableName) != hashTable.end()) {
            hashTable[tableName].deleteRow2(colName, operation, value);
        }
        else {
            getline(cin, junk);
            cout << "Error during DELETE: "
                << tableName << " does not name a table in the database\n";
            return;
        }
    }

    void generateInd1() {
        string junk, tableName, hashType, colName;
        cin >> junk >> tableName >> hashType >> junk >> junk >> colName;

        if (hashTable.find(tableName) != hashTable.end()) {
            hashTable[tableName].generateInd2(colName, hashType);
        }
        else {
            getline(cin, junk);
            cout << "Error during GENERATE: "
                << tableName << " does not name a table in the database\n";
            return;
        }
    }

    void join1() {
        string tableName1, tableName2, colName1, colName2, junk, operation;

        cin >> tableName1 >> junk >> tableName2 >> junk >> colName1 
            >> operation >> colName2 >> junk >> junk;

        if (hashTable.find(tableName1) == hashTable.end()) {
            getline(cin, junk);
            cout << "Error during JOIN: "
                << tableName1 << " does not name a table in the database\n";
            return;
        }

        if (hashTable.find(tableName2) == hashTable.end()) {
            getline(cin, junk);
            cout << "Error during JOIN: "
                << tableName2 << " does not name a table in the database\n";
            return;
        }

        if (hashTable[tableName1].colNameInd.find(colName1) == hashTable[tableName1].colNameInd.end()) {
            getline(cin, junk);
            cout << "Error during JOIN: "
                << colName1 << " does not name a column in " << tableName1 << "\n";
            return;
        }

        if (hashTable[tableName2].colNameInd.find(colName2) == hashTable[tableName2].colNameInd.end()) {
            getline(cin, junk);
            cout << "Error during JOIN: "
                << colName2 << " does not name a column in " << tableName2 << "\n";
            return;
        }

        hashTable[tableName2].generateTempInd(colName2);

        hashTable[tableName1].join2(hashTable[tableName2], colName1, quiet);

        hashTable[tableName2].clearTempInd();
    }

};

void getMode(int argc, char* argv[], Database& database) {
    // These are used with getopt_long()
    opterr = false; // Let us handle all error output for command line options
    int choice;
    int index = 0;
    struct option long_options[] = {
        // Fill in two lines, for the "mode" ('m') and
        // the "help" ('h') options.
        // ./project0 -m nosize
        // ./project0 --help
        { "quiet",  no_argument, nullptr, 'q'  },
        { "help",   no_argument, nullptr, 'h'  },
        { nullptr,  0,           nullptr, '\0' },
    };  // long_options[]

    // Fill in the double quotes, to match the mode and help options.
    while ((choice = getopt_long(argc, argv, "qh", long_options, &index)) != -1) {
        switch (choice) {

        case 'q':
            // quiet output
            database.quietMode();
            break;

        case 'h':
            // help output

            break;
        }  // switch ..choice
    }  // while
}  // getMode()

int main(int argc, char* argv[]) {
    ios_base::sync_with_stdio(false);
    cin >> boolalpha;
    cout << boolalpha;
    string cmd;
    Database database;
    getMode(argc, argv, database);
    do {
        if (cin.fail()) {
            cerr << "Error: Reading from cin has failed" << endl;
            exit(1);
        } // if
        cout << "% ";
        cin >> cmd;
        if (cmd[0] == '#') {
            string junk;
            getline(cin, junk);
        }
        else if (cmd == "CREATE") {
            database.createTable();
        }
        else if (cmd == "REMOVE") {
            database.removeTable();
        }
        else if (cmd == "PRINT") {
            database.printRows1();
        }
        else if (cmd == "INSERT") {
            database.insertRows1();
        }
        else if (cmd == "DELETE") {
            database.deleteRow1();
        }
        else if (cmd == "GENERATE") {
            database.generateInd1();
        }
        else if (cmd == "JOIN") {
            database.join1();
        }
        else if (cmd == "QUIT") {
            continue;
        }
        else {
            string junk;
            getline(cin, junk);
            cout << "Error: unrecognized command" << "\n";
        }
        //process cmd
    } while (cmd != "QUIT");

    cout << "Thanks for being silly!" << endl;
}