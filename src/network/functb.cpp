#include "functb.h"
#include "appsettings.h"
#include "wtapi.h"
#include "wtdata.h"
#include <QPointer>
#include <QTextEdit>

std::string functb::identifier;
std::string functb::points="-1";
std::string functb::wins="-1";
std::string functb::seed="-1";
std::string functb::hour_missing="-1";
static QPointer<QTextEdit> s_logBox;
void functb::setLogBox(QTextEdit *box) {s_logBox=box;}
