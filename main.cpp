// irrvp (irregular verbs practive) by maxim.topolsky@gmail.com

#include <signal.h> // TODO linux only

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <deque>
#include <iostream>
#include <fstream>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    /* Verbs info database types and (loading) functions */

    typedef struct
    {   std::string v1, trn;            // base form, translation
        std::set<std::string> v2, v3;   // V2/V3 forms
    } vinfo_t;

    typedef std::deque<vinfo_t> vdb_t;

    vdb_t   load_vdb(const char * filename);

    vinfo_t get_vinfo(const std::string &);

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

    /* Statistics hold structure, print functions */

    struct stat_t
    {   size_t      done;                   // number of passed tests
        size_t      vsuccess, vtotal;       // verbs (success, total)
        size_t      tsuccess, ttotal;       // translations (success, total)
        std::set<size_t> ft;                // failed tests (VBD index)

        stat_t()
        :   done(0)
        ,   vsuccess(0)
        ,   vtotal(0)
        ,   tsuccess(0)
        ,   ttotal(0)
        {}
    };

    std::deque<vinfo_t> VDB; // verbs database
    stat_t VTR; // verbs tests results

    void print_stat()
    {
        using std::cout;
        using std::endl;

        cout << "Number of tests = " << VTR.done;

        if (VTR.vtotal)
            cout << "; v-rate = " << 100. * VTR.vsuccess / VTR.vtotal << "%";

        if (VTR.ttotal)
            cout << "; t-rate = " << 100. * VTR.tsuccess / VTR.ttotal << "%";

        cout << endl;
   }

    void print_ft()
    {
        using std::cout;
        using std::endl;

        if (!VTR.ft.empty())
        {
            cout << "Number of failed tests: " << VTR.ft.size() << endl;

            for (auto & index: VTR.ft)
            {
                const vinfo_t & vinfo = VDB[index];
                cout << "  " << vinfo.v1;
                cout << " [";
                for (auto & word: vinfo.v2)
                    cout << " " << word;
                cout << " ] [";
                for (auto & word: vinfo.v3)
                    cout << " " << word;
                cout << " ] " << vinfo.trn << endl;
            }
        }
    }

    void sighandler(int signum)
    {
        std::cout << std::endl << std::endl;

        print_stat();
        print_ft();

        ::exit(signum);
    }

    /* Generate test queue (result contains shuffled numbers from 0 to VDB.size-1) */

    std::deque<size_t> gen_tq()
    {
        std::deque<size_t> tq(VDB.size());
        std::iota(tq.begin(), tq.end(), 0);
        std::shuffle(tq.begin(), tq.end(), std::mt19937(std::random_device{}()));
        return tq; // RVO
    }

} // namespace

int main(int argc, char ** argv)
{
    try
    {
        if (argc != 2)
            throw std::invalid_argument("Expected single argument (database)");

        ::signal(SIGINT, sighandler);

        VDB = load_vdb(argv[1]);

        // main loop

        std::random_device rdev;
        //std::mt19937 rengine(rdev());
        std::uniform_int_distribution<size_t> rdis(0, VDB.size()-1);

        // TODO save fauilts, show them at the end

        std::deque<size_t> tq = gen_tq();

        while (!tq.empty())
        {
            using std::cout;
            using std::cin;
            using std::endl;

            const size_t vindex = tq.front();
            tq.pop_front();

            const vinfo_t vinfo = VDB[vindex];

            cout << "Test " << (VTR.done + 1) << "/" << VDB.size() << endl;
            cout << endl;

            cout << "Suggest V2/V3 forms:" << endl;
            cout << "  V1: " << vinfo.v1 << endl;
            cout << "  V2: ";
            std::string v2;
            std::getline(cin, v2);

            cout << "  V3: ";
            std::string v3;
            std::getline(cin, v3);

            auto [ csuccess, ctotal ] = check_answer(vinfo, v2, v3);

            if (csuccess != ctotal)
            {
                VTR.ft.insert(vindex);

                // TODO divide failed from partial faile [ wrong / partial wrong ]
                cout << endl << "  You are wrong ("
                    << csuccess << "/" << ctotal
                    << "): v2 = [";
                for (auto & v2: vinfo.v2)
                    cout << " " << v2;
                cout << " ]; v3 = [";
                for (auto & v3: vinfo.v3)
                    cout << " " << v3;
                cout << " ]" << endl;

                std::getchar();
            }

            VTR.vtotal += ctotal;
            VTR.vsuccess += csuccess;

            // translation

            std::set<std::string> trn = { vinfo.trn };
            while (trn.size() != 5)
                trn.insert(VDB[rdis(rdev)].trn);

            cout << endl;

            cout << "Choose right translation:" << endl;
            size_t j = 1;
            for (auto & aux: trn)
                cout << "  " << j++ << ". " << aux << endl;

            std::optional<size_t> index;

            while (!index)
            {
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

            if (*it == vinfo.trn) {
                ++VTR.tsuccess;
            } else {
                VTR.ft.insert(vindex);

                cout << endl << "  You are wrong (" << vinfo.trn << ")" << endl;
                std::getchar();
            }

            ++VTR.ttotal;

            cout << endl;

            ++VTR.done;

            // update statistics

            if ((VTR.done % 10) == 0) {
                print_stat();
                cout << endl;
            }
        }

        print_stat();
        print_ft();
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
    vdb_t load_vdb(const char * filename)
    {
        if (!filename)
            throw std::invalid_argument("Invalid file name");

        std::fstream file(filename);

        if (!file.is_open())
            throw std::runtime_error("File access error");

        vdb_t vdb;
        size_t nline = 1;

        try
        {
            for (std::string line; std::getline(file, line); ++nline)
                vdb.push_back(get_vinfo(line));
        }
        catch (const std::exception & ex)
        {
            throw std::invalid_argument (
                    std::string("Database error (line ") +
                    std::to_string(nline) +
                    std::string("): ") +
                    std::string(ex.what())
                );
        }

        if (vdb.empty())
            throw std::runtime_error("Empty database");

        return vdb; // RVO
    }

    vinfo_t get_vinfo(const std::string & line)
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

        return vinfo;
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
