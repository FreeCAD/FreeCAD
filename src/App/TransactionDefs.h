
#ifndef APP_TRANSACTIONDEFS_H_
#define APP_TRANSACTIONDEFS_H_

#include <string>

namespace App {

class Document;

constexpr int NullTransaction = 0;

struct TransactionName {
    // Name of the transaction as it will appear in the GUI
    std::string name;
    
    // If true, the transaction is allowed to be renamed
    bool temporary { false };
};

struct TransactionDescription {
    // Document on which the transaction was first created
    // useful for mass transaction (e.g. delete from an assembly)
    // where other documents may use the same transaction id
    Document* initiator { nullptr };
    TransactionName name;
};

enum class TransactionCloseMode {
    Commit = 0,
    Abort = 1,
};
}

#endif