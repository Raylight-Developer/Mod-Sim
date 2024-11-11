#pragma once

#include "Include.hpp"

#include "Lace.hpp"

// FWD DECL OTHER

// FWD DECL THIS

// DECL
struct Session {
	static Session& getInstance();

	void flushLog();

	Session();
	~Session() = default;

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;

	Lace log = Lace();
	unordered_map<string, uint64> params = {};
	unordered_map<string, chrono::high_resolution_clock::time_point> start_times = {};
	unordered_map<string, dvec1> total_times = {};
	unordered_map<string, dvec1> delta_times = {};
};

#undef LOG
#undef FILE

#define ENDL << Lace_NL()

#define ANSI_RESET  << "\033[0m"

#define ANSI_BLACK  << "\033[90m"
#define ANSI_R      << "\033[91m"
#define ANSI_G      << "\033[92m"
#define ANSI_YELLOW << "\033[93m"
#define ANSI_B      << "\033[94m"
#define ANSI_PURPLE << "\033[95m"
#define ANSI_CYAN   << "\033[96m"
#define ANSI_WHITE  << "\033[97m"

#define NL  << Lace_NL()
#define SP  << Lace_S()
#define TAB << Lace_TAB()
#define PTR << "* "

#define LOG Session::getInstance().log
#define FLUSH Session::getInstance().flushLog()
#define TIMER(key) Session::getInstance().delta_times[key]
#define INIT_TIMER(key) Session::getInstance().start_times[key] = chrono::high_resolution_clock::now(); Session::getInstance().total_times[key] = 0.0; Session::getInstance().delta_times[key] = 0.0;
#define START_TIMER(key) Session::getInstance().start_times[key] = chrono::high_resolution_clock::now()
#define END_TIMER(key) Session::getInstance().delta_times[key] = chrono::duration<double>(chrono::high_resolution_clock::now() - Session::getInstance().start_times[key]).count()
#define ADD_TIMER(key) Session::getInstance().delta_times[key] += chrono::duration<double>(chrono::high_resolution_clock::now() - Session::getInstance().start_times[key]).count()
#define RESET_TIMER(key) Session::getInstance().start_times[key] = chrono::high_resolution_clock::now(); Session::getInstance().total_times[key] = 0.0; Session::getInstance().delta_times[key] = 0.0;
#define SESSION_SET(key, val, type) Session::getInstance().params[key] = bits<type, uint64>(val)
#define SESSION_GET(key, type) bits<uint64, type>(Session::getInstance().params[key])