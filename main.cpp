// irrvp (irregular verbs practive) by maxim.topolsky@gmail.com

#include <algorithm>
#include <cctype>
#include <deque>
#include <iostream>
#include <fstream>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    typedef struct
    {   std::string v1;
        std::set<std::string> v2, v3;
        std::string trn;
    } vinfo_t;

    // TODO db_t load(..)
    
    vinfo_t get_vinfo(size_t fline, const std::string &);

    /* Checks the answer. Result consist of two numbers:
     * - first: number of matched answers
     * - second: number of reference answers */

    std::pair<size_t, size_t> check_answer
        (   const vinfo_t &
        ,   const std::string & v2ans
        ,   const std::string & v3and
        );

    /* Splits string to set of string using chosen delimiter */

    struct replace_info {
        std::string p1, s1, p2;
        std::set<std::string> s2;

        replace_info() {}

        replace_info
        (   const std::string & _p1
        ,   const std::string & _s1
        )
        :   p1(_p1)
        ,   s1(_s1)
        {}

        replace_info
        (   const std::string & _p1
        ,   const std::string & _s1
        ,   const std::string & _p2
        ,   const std::set<std::string> & _s2
        )
        :   p1(_p1)
        ,   s1(_s1)
        ,   p2(_p2)
        ,   s2(_s2)
        {}
    };

    std::set<std::string> split2set(const std::string &, char delim, const replace_info & = replace_info());

} // namespace

// TODO catch Ctrl+C, dump statistics

int main(int argc, char ** argv)
{
    try
    {
        if (argc != 2)
            throw std::invalid_argument("Expected single argument (database)");

        // TODO auto db = load(argv[1]);

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

        std::random_device rdev;
        std::mt19937 rengine(rdev());
        std::uniform_int_distribution<size_t> rdis(0, varr.size()-1);

        std::shuffle(varr.begin(), varr.end(), rengine);

        size_t vtotal = 0, vsuccess = 0, ttotal = 0, tsuccess = 0; // verbs/translation total/success

        // TODO save fauilts, show them at the end

        for (size_t i = 0, ilim = varr.size(); i < ilim; ++i)
        {
            using std::cout;
            using std::cin;
            using std::endl;

            //const vinfo_t vinfo = varr[rdis(rdev)];
            const vinfo_t vinfo = varr[i];

            cout << "Test " << (i + 1) << "/" << ilim << endl;
            cout << endl;
            cout << "v1: " << vinfo.v1 << endl;
            cout << "v2: ";

            std::string v2;
            std::getline(cin, v2);

            cout << "v3: ";
            std::string v3;
            std::getline(cin, v3);

            auto [ csuccess, ctotal ] = check_answer(vinfo, v2, v3);

            if (csuccess == ctotal)
                cout << "success" << endl;
            else
            {
                // TODO divide failed from partial faile [ wrong / partial wrong ]
                cout << "failed ("
                    << csuccess << "/" << ctotal
                    << "): v2 = [";
                for (auto & v2: vinfo.v2)
                    cout << " " << v2;
                cout << " ]; v3 = [";
                for (auto & v3: vinfo.v3)
                    cout << " " << v3;
                cout << " ]" << endl;
            }

            vtotal += ctotal;
            vsuccess += csuccess;

            // translation

            std::set<std::string> trn = { vinfo.trn };
            while (trn.size() != 5)
                trn.insert(varr[rdis(rdev)].trn);

            cout << endl;
            
            size_t j = 1;
            for (auto & aux: trn)
                cout << j++ << ". " << aux << endl;

            std::optional<size_t> index;

            while (!index)
            {
                cout << "Choose right translation: ";
                std::string aux;
                getline(cin, aux);
                if (std::all_of(aux.begin(), aux.end(), [](auto symbol) { return isdigit(symbol) != 0; })) {
                    index = atol(aux.c_str());
                    if (!*index || (*index > trn.size()))
                        index.reset();
                }
            }

            auto it = trn.begin();
            advance(it, *index - 1);

            if (*it != vinfo.trn)
                cout << "failed (" << vinfo.trn << ")" << endl;
            else
            {
                cout << "success" << endl;
                ++tsuccess;
            }

            ++ttotal;

            cout << endl;

            // update statistics

            if (((i + 1) % 10) == 0)
            {
                cout << "Number of tests = " << (i+1) << "; v-rate = "
                    << 100. * vsuccess / vtotal << "%; t-rate ="
                    << 100. * tsuccess / ttotal << "%"
                    << endl;
                cout << endl;
            }
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

            // translation

            std::string trn;
            while (!ss.eof())
            {
                ss >> trn;

                if (!vinfo.trn.empty())
                    vinfo.trn += ' ';

                vinfo.trn += trn;
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
            cout << "trn = " << vinfo.trn << endl;
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

    std::pair<size_t, size_t> check_answer
        (   const vinfo_t & vinfo
        ,   const std::string & v2ans
        ,   const std::string & v3ans
        )
    {
        std::pair<size_t, size_t> result(0, vinfo.v2.size() + vinfo.v3.size());

        const auto v2 = split2set(v2ans, ' ', replace_info("1", vinfo.v1));
        const auto v3 = split2set(v3ans, ' ', replace_info("1", vinfo.v1, "2", v2));

        if (v2.size() <= vinfo.v2.size())
            for (auto & answer: v2)
                if (vinfo.v2.count(answer))
                    ++result.first;

        if (v3.size() <= vinfo.v3.size())
            for (auto & answer: v3)
                if (vinfo.v3.count(answer))
                    ++result.first;

        return result;
    }

    std::set<std::string> split2set
        (   const std::string & str
        ,   char delim
        ,   const replace_info & rinfo
        )
    {
        std::set<std::string> result;

        std::istringstream ss(str);
        for (std::string word; std::getline(ss, word, delim); ) {
            if (word == rinfo.p1)
                result.insert(rinfo.s1);
            else
            if (word != rinfo.p2)
                result.insert(word);
            else {
                for (auto & ww: rinfo.s2)
                    result.insert(ww);
            }
        }

        return result;
    }
}
