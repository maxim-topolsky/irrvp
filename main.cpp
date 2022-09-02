// irrvp (irregular verbs practive) by maxim.topolsky@gmail.com

#include <algorithm>
#include <cctype>
#include <deque>
#include <iostream>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    typedef struct
    {   std::string v1;
        std::set<std::string> v2, v3;
        std::string translate;
    } vinfo_t;

    vinfo_t get_vinfo(size_t fline, const std::string &);

    std::set<std::string> get_words_set(const std::string &, char delim);

} // namespace

int main(int argc, char ** argv)
{
    try
    {
        if (argc != 2)
            throw std::invalid_argument("Expected single argument (database)");

        std::fstream file(argv[1]);
        if (!file.is_open())
            throw std::runtime_error("File access error");

        std::deque<vinfo_t> varr;
        size_t fline = 1;

        for (std::string line; std::getline(file, line); ++fline)
            varr.push_back(get_vinfo(fline, line));

        if (varr.empty())
            throw std::runtime_error("Empty database");

        // main loop

        //std::cout << "ok" << std::endl;

        std::random_device rdev;
        std::uniform_int_distribution<size_t> rdis(0, varr.size()-1);

        while (true)
        {
            using std::cout;
            using std::cin;
            using std::endl;

            // TODO const vinfo_t = varr[index]

            const size_t index = rdis(rdev);
            cout << "v1: " << varr[index].v1 << endl;
            cout << "v2: ";

            std::string v2;
            std::getline(cin, v2);

            cout << "v3: ";
            std::string v3;
            std::getline(cin, v3);

            const std::set<std::string> v2s = get_words_set(v2, ' ');
            const std::set<std::string> v3s = get_words_set(v3, ' ');

            // TODO accept "1" and "2" solutions
            // calculate statistics

            if ((v2s == varr[index].v2) && (v3s == varr[index].v3))
                cout << "success" << endl;
            else
            {
                cout << "failed: v2 = [";
                for (auto & v2: varr[index].v2)
                    cout << " " << v2;
                cout << " ]; v3 = [";
                for (auto & v3: varr[index].v3)
                    cout << " " << v3;
                cout << " ]" << endl;
            }

            cout << endl;
        }
    }
    catch (const std::exception & ex)
    {
        std::cerr << ex.what() << std::endl;
        return -1;
    }

    return 0;
}

namespace
{
    vinfo_t get_vinfo(size_t fline, const std::string & line)
    {
        try
        {
            auto check_verb = [](char symbol) {
                return std::isalpha(symbol) && std::islower(symbol);
            };

            vinfo_t vinfo;

            std::stringstream ss(line);

            // v1

            ss >> vinfo.v1;

            if (vinfo.v1.empty())
                throw std::runtime_error("No V1 is available");

            if (!std::all_of(vinfo.v1.begin(), vinfo.v1.end(), check_verb))
                throw std::runtime_error("Invalid V1");

            // v2

            std::string vx;
            ss >> vx;

            if (vx.empty())
                throw std::runtime_error("No V2 is available");

            std::istringstream v2ss(vx);
            for (std::string v2; std::getline(v2ss, v2, '/'); )
            {
                if (v2 == "[1]")
                    vinfo.v2.insert(vinfo.v1);
                else
                {
                    if (!std::all_of(v2.begin(), v2.end(), check_verb))
                        throw std::runtime_error("Invalid V2");

                    vinfo.v2.insert(v2);
                }
            }

            // v3

            ss >> vx;

            if (vx.empty())
                throw std::runtime_error("No V3 is available");

            std::istringstream v3ss(vx);
            for (std::string v3; std::getline(v3ss, v3, '/'); )
            {
                if (v3 == "[1]")
                    vinfo.v3.insert(vinfo.v1);
                else
                if (v3 == "[2]")
                    for (auto & v2: vinfo.v2)
                        vinfo.v3.insert(v2);
                else
                {
                    if (!std::all_of(v3.begin(), v3.end(), check_verb))
                        throw std::runtime_error("Invalid V3");

                    vinfo.v3.insert(v3);
                }
            }

            /*
            using namespace std;
            cout << "v1 = " << vinfo.v1 << "; v2 = [";
            for (auto & v2: vinfo.v2)
                cout << " " << v2;
            cout << " ]; v3 = [";
            for (auto & v3: vinfo.v3)
                cout << " " << v3;
            cout << " ]" << endl;
            */

            return vinfo;
        }
        catch (const std::exception & ex)
        {
            throw std::invalid_argument (
                    std::string("Database error (line ") +
                    std::to_string(fline) +
                    std::string("): ") +
                    std::string(ex.what())
                );
        }
    }

    // TODO list of replaces? [1] [2] >>> sets, or word + set

    std::set<std::string> get_words_set(const std::string & line, char delim)
    {
        std::set<std::string> result;

        std::istringstream ss(line);
        for (std::string vv; std::getline(ss, vv, delim); )
            result.insert(vv);

        return result;
    }
}
