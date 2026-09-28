#pragma once
#include <string>
class QTextEdit;

// Compatibility state for existing goal/graph screens; network operations live in WtApi.
class functb {
public:
    static std::string identifier;
    static std::string points;
    static std::string wins;
    static std::string seed;
    static std::string hour_missing;
    static void setLogBox(QTextEdit *box);
};
