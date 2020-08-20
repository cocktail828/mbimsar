#include "logger.h"

Severity Logger::m_log_level = Severity::INFO; /* only show log that with level 
                                    not higher than m_log_level */
bool Logger::m_sync_syslog = false;            /* sync log to syslog ? */
bool Logger::m_sync_stdout = true;             /* sync log to stdout */
bool Logger::m_sync_file = false;              /* sync log to file */
std::string Logger::m_logfile;
Severity Logger::m_severity = Severity::INFO;
std::string Logger::m_filename;
int Logger::m_line;
std::string Logger::m_funcname;