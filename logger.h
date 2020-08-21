#ifndef __LOGGER
#define __LOGGER

#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <ctime>

#include <syslog.h>

typedef enum
{
    EMERG = LOG_EMERG,     /* system is unusable */
    ALERT = LOG_ALERT,     /* action must be taken immediately */
    CRIT = LOG_CRIT,       /* critical conditions */
    ERROR = LOG_ERR,       /* error conditions */
    WARNING = LOG_WARNING, /* warning conditions */
    NOTICE = LOG_NOTICE,   /* normal but significant condition */
    INFO = LOG_INFO,       /* informational */
    DEBUG = LOG_DEBUG      /* debug-level messages */
} Severity;

static std::string severity_str[] = {
    [EMERG] = "EMERG",     /* system is unusable */
    [ALERT] = "ALERT",     /* action must be taken immediately */
    [CRIT] = "CRIT",       /* critical conditions */
    [ERROR] = "ERROR",     /* error conditions */
    [WARNING] = "WARNING", /* warning conditions */
    [NOTICE] = "NOTICE",   /* normal but significant condition */
    [INFO] = "INFO",       /* informational */
    [DEBUG] = "DEBUG",     /* debug-level messages */
};

class Logger
{
private:
    Logger() {}

    ~Logger() {}

    /* forbide copy */
    Logger &operator=(const Logger &) = delete;
    Logger(const Logger &) = delete;

private:
    static Severity m_log_level;  /* only show log that with level 
                                    not higher than m_log_level */
    static bool m_sync_syslog;    /* sync log to syslog ? */
    static bool m_sync_stdout;    /* sync log to stdout */
    static bool m_sync_file;      /* sync log to file */
    static std::string m_logfile; /* file to save log */
    static Severity m_severity;
    static std::string m_filename;
    static int m_line;
    static std::string m_funcname;
    std::stringstream ss;
    std::mutex m_lk;

public:
    static Logger &Instance(Severity s = Severity::INFO,
                            const char *f = "", int l = 0,
                            const char *fc = "")
    {
        static Logger instance_;

        instance_.m_severity = s;
        instance_.m_filename = f;
        instance_.m_line = l;
        instance_.m_funcname = fc;
        return instance_;
    }

    static void Init(bool sync_flag, const char *logfile)
    {
        m_sync_file = sync_flag;
        m_logfile = logfile;
    }

    static void Init(bool sync_stdout, bool sync_file, bool sync_syslog)
    {
        m_sync_file = sync_file;
        m_sync_stdout = sync_stdout;
        m_sync_syslog = sync_syslog;
    }

    static void Init(const char *label = "LOGGER",
                     Severity s = Severity::INFO,
                     bool sync_syslog = false)
    {
        m_log_level = s;
        if (sync_syslog)
        {
            m_sync_syslog = true;
            openlog(label, LOG_CONS | LOG_NDELAY, LOG_USER | LOG_LOCAL1);
        }
    }

    Logger &operator<<(const char *t)
    {
        std::lock_guard<std::mutex> _lk(m_lk);
        ss << t;
        return *this;
    }

    template <typename T>
    Logger &operator<<(const T &t)
    {
        std::lock_guard<std::mutex> _lk(m_lk);
        ss << t;
        return *this;
    }

    Logger &operator<<(std::ostream &(*os)(std::ostream &))
    {
        dump_line();
        return *this;
    }

    void dump_line()
    {
        std::lock_guard<std::mutex> _lk(m_lk);
        time_t rawtime;
        char now[80];

        time(&rawtime);
        struct tm *tt = localtime(&rawtime);

        strftime(now, 80, "%Y-%m-%d %H:%M:%S", tt);

        std::string obuf = "";
        if (!m_filename.empty())
        {
            obuf += "[" + m_filename + ":" +
                    std::to_string(m_line) + " " +
                    m_funcname + "]";
        }

        obuf += "[" + std::string(now) + "][" +
                severity_str[static_cast<int>(m_severity)] + "] ";

        obuf += ss.str();

        if (m_log_level >= m_severity)
        {
            /* syslog */
            if (m_sync_syslog)
                syslog(LOG_USER | m_severity, "%s", obuf.c_str());

            /* stdout/stderr */
            if (m_sync_stdout)
                std::cerr << obuf << std::endl;

            if (m_sync_file)
            {
                std::ofstream fout(m_logfile, std::ios::app);
                fout.write(obuf.c_str(), obuf.length());
                fout.close();
            }
        }

        ss.str("");
        m_funcname = "";
        m_filename = "";
        m_line = 0;
    }

    template <typename T>
    void operator()(T t)
    {
        ss << t;
        dump_line();
    }

    template <typename T, typename... Args>
    void operator()(const T &t, const Args &... args)
    {
        ss << t;
        (*this)(args...);
    }
};

#define ENDL std::endl

#ifndef NDEBUG
#define LOG(v) Logger::Instance(v, __FILE__, __LINE__, __func__)
#define LOGD Logger::Instance(Severity::DEBUG, __FILE__, __LINE__, __func__)
#define LOGI Logger::Instance(Severity::INFO, __FILE__, __LINE__, __func__)
#define LOGN Logger::Instance(Severity::NOTICE, __FILE__, __LINE__, __func__)
#define LOGW Logger::Instance(Severity::WARNING, __FILE__, __LINE__, __func__)
#define LOGE Logger::Instance(Severity::ERROR, __FILE__, __LINE__, __func__)

#define LOGFD(a, v...) Logger::Instance(Severity::DEBUG, __FILE__, __LINE__, __func__)(a, ##v)
#define LOGFI(a, v...) Logger::Instance(Severity::INFO, __FILE__, __LINE__, __func__)(a, ##v)
#define LOGFN(a, v...) Logger::Instance(Severity::NOTICE, __FILE__, __LINE__, __func__)(a, ##v)
#define LOGFW(a, v...) Logger::Instance(Severity::WARNING, __FILE__, __LINE__, __func__)(a, ##v)
#define LOGFE(a, v...) Logger::Instance(Severity::ERROR, __FILE__, __LINE__, __func__)(a, ##v)

#else
#define LOG(v) Logger::Instance(v)
#define LOGD Logger::Instance(Severity::DEBUG)
#define LOGI Logger::Instance(Severity::INFO)
#define LOGN Logger::Instance(Severity::NOTICE)
#define LOGW Logger::Instance(Severity::WARNING)
#define LOGE Logger::Instance(Severity::ERROR)

#define LOGFD(a, v...) Logger::Instance(Severity::DEBUG)(a, ##v)
#define LOGFI(a, v...) Logger::Instance(Severity::INFO)(a, ##v)
#define LOGFN(a, v...) Logger::Instance(Severity::NOTICE)(a, ##v)
#define LOGFW(a, v...) Logger::Instance(Severity::WARNING)(a, ##v)
#define LOGFE(a, v...) Logger::Instance(Severity::ERROR)(a, ##v)
#endif // NDEBUG

#endif //__LOGGER
